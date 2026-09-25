// SPDX-License-Identifier: MIT
/**
   Copyright (c) 2024 Beckhoff Automation GmbH & Co. KG
 */

#include "EventLoggerAccess.h"
#include "AdsException.h"
#include "AdsLib.h"
#include "Log.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <memory>
#include <sstream>
#include <vector>

// ---------------------------------------------------------------------------
// Helper: convert a Windows FILETIME (100-ns ticks since 1601-01-01) to a
// human-readable UTC date/time string "YYYY-MM-DD HH:MM:SS.mmm UTC".
// ---------------------------------------------------------------------------
static std::string FormatFiletime(uint64_t filetime)
{
    // Offset between Windows epoch (1601-01-01) and Unix epoch (1970-01-01)
    // in 100-ns intervals.
    static constexpr uint64_t EPOCH_DIFF_100NS = 116444736000000000ULL;

    if (filetime < EPOCH_DIFF_100NS) {
        return "<invalid timestamp>";
    }

    const uint64_t unixNs100 = filetime - EPOCH_DIFF_100NS;
    const time_t   seconds   = static_cast<time_t>(unixNs100 / 10000000ULL);
    const uint32_t millis    = static_cast<uint32_t>((unixNs100 / 10000ULL) % 1000ULL);

    struct tm utcTm {};
#if defined(_WIN32)
    gmtime_s(&utcTm, &seconds);
#else
    gmtime_r(&seconds, &utcTm);
#endif

    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &utcTm);

    std::ostringstream oss;
    oss << buf << '.' << std::setfill('0') << std::setw(3) << millis << " UTC";
    return oss.str();
}

// ---------------------------------------------------------------------------
// Helper: map TcEventSeverity to a short string label.
// ---------------------------------------------------------------------------
static const char *SeverityToString(uint16_t severity)
{
    using S = bhf::ads::TcEventSeverity;
    switch (static_cast<S>(severity)) {
        case S::Verbose:  return "VERBOSE";
        case S::Info:     return "INFO";
        case S::Warning:  return "WARNING";
        case S::Error:    return "ERROR";
        case S::Critical: return "CRITICAL";
        default:          return "UNKNOWN";
    }
}

// ---------------------------------------------------------------------------
// Helper: map EventTypeEnum to a short string label.
// ---------------------------------------------------------------------------
static const char *EventTypeToString(uint16_t eventType)
{
    using T = bhf::ads::EventTypeEnum;
    switch (static_cast<T>(eventType)) {
        case T::Message: return "Message";
        case T::Alarm:   return "Alarm";
        default:         return "Unknown";
    }
}

// ---------------------------------------------------------------------------
// AMS port of the TwinCAT EventLogger Publisher V2.
// ---------------------------------------------------------------------------
static constexpr uint16_t AMSPORT_EVENTLOGGER_PUBLISHER_V2 = 132;

// ---------------------------------------------------------------------------
// Index group / offset used to subscribe to the EventLogger Publisher V2.
//
// IndexGroup 0x309 is the EventLogger Publisher V2 notification subscription
// group.  IndexOffset encodes the minimum severity level that the publisher
// will forward:
//   0x0000 = Verbose and above (all events)
//   0x0001 = Info and above
//   0x0002 = Warning and above
//   0x0004 = Error and above
//   0x0008 = Critical only
// ---------------------------------------------------------------------------
static constexpr uint32_t EVENTLOGGER_SUBSCRIBE_INDEXGROUP  = 0x00000309;

// Shared offsets/markers for GUID-based EventLogger record parsing.
static constexpr uint32_t OFF_EVENT_CLASS = 0x00;
static constexpr uint32_t OFF_EVENT_ID = 0x10;
static constexpr uint32_t OFF_EVENT_FLAGS = 0x14;
static constexpr uint32_t OFF_SEVERITY = 0x18;
static constexpr uint32_t OFF_EVENT_TYPE = 0x2A;
static constexpr uint32_t OFF_UNIQUE_ID = 0x2C;
static constexpr uint32_t OFF_TIME_RAISED = 0x30;
static constexpr uint32_t OFF_TIME_CLEARED = 0x38;
static constexpr uint32_t OFF_CONFIRM_STATE = 0x48;
static constexpr uint32_t OFF_TIME_CONFIRMED = 0x50;
static constexpr uint32_t OFF_PAYLOAD_OFFSET = 0x6C;
static constexpr uint32_t OFF_PAYLOAD_LENGTH = 0x70;
static constexpr uint32_t OFF_SOURCE_ID = 0x90;
static constexpr uint32_t OFF_SOURCE_NAME = 0x94;
// Control character that indicates the payload is a TwinCAT or custom event.
static constexpr uint16_t MARKER_DIRECT = 0x000E;
static constexpr uint16_t MARKER_GAP48 = 0x0014;
static constexpr uint32_t MIN_EVENT_SIZE = 0x98;
static constexpr uint32_t LIVE_EVENT_OFFSET = 12;
static constexpr uint32_t BACKLOG_EVENT_OFFSET = 28;
// The gap in the TwinCAT messages that separates the 48-byte header from the payload.
static constexpr uint32_t TWINCAT_TEXT_GAP48_BYTES = 48;
// Not sure this is the right threshold, but it seems to work for TwinSAFE events.
// It is the only way to distinguish TwinSAFE events from other device-specific events.
static constexpr uint32_t TWINSAFE_ID_THRESHOLD = 1000000;

static constexpr uint64_t FILETIME_YEAR2000 = 0x01BF53EB211D0000ULL;

std::mutex bhf::ads::EventLoggerAccess::s_RegistryMutex;
std::unordered_map<uint32_t, bhf::ads::EventLoggerAccess*> bhf::ads::EventLoggerAccess::s_Registry;
std::atomic<uint32_t> bhf::ads::EventLoggerAccess::s_NextId{1};

static std::string FormatEventText(const std::string& tmpl,
                                   const uint8_t*     params,
                                   size_t             paramSize)
{
    if (params == nullptr || paramSize == 0 || tmpl.find('%') == std::string::npos) {
        return tmpl;
    }

    std::string result;
    const uint8_t* p = params;
    const uint8_t* end = params + paramSize;
    result.reserve(tmpl.size() + 64);

    for (size_t i = 0; i < tmpl.size(); ++i) {
        if (tmpl[i] != '%' || i + 1 >= tmpl.size()) {
            result += tmpl[i];
            continue;
        }

        ++i;

        while (i < tmpl.size() && (tmpl[i] == '+' || tmpl[i] == '-' || tmpl[i] == '0' ||
                                   tmpl[i] == '#' || tmpl[i] == ' ')) {
            ++i;
        }

        while (i < tmpl.size() && std::isdigit(static_cast<unsigned char>(tmpl[i]))) {
            ++i;
        }

        if (i < tmpl.size() && tmpl[i] == '.') {
            ++i;
            while (i < tmpl.size() && std::isdigit(static_cast<unsigned char>(tmpl[i]))) {
                ++i;
            }
        }

        if (i >= tmpl.size()) {
            result += '%';
            break;
        }

        bool hasLong = false;
        if (tmpl[i] == 'l' || tmpl[i] == 'L') {
            hasLong = true;
            ++i;
        } else if (tmpl[i] == 'h' || tmpl[i] == 'H') {
            ++i;
        }

        if (i >= tmpl.size()) {
            result += '%';
            break;
        }

        const char spec = tmpl[i];
        if (spec == 's') {
            while (p < end && *p != 0) {
                result += static_cast<char>(*p++);
            }
            if (p < end) {
                ++p;
            }
        } else if (spec == 'd' || spec == 'i') {
            if (p + 4 <= end) {
                int32_t val = 0;
                memcpy(&val, p, 4);
                p += 4;
                result += std::to_string(val);
            }
        } else if (spec == 'u') {
            if (p + 4 <= end) {
                uint32_t val = 0;
                memcpy(&val, p, 4);
                p += 4;
                result += std::to_string(val);
            }
        } else if (spec == 'x' || spec == 'X') {
            if (p + 4 <= end) {
                uint32_t val = 0;
                memcpy(&val, p, 4);
                p += 4;
                char buf[16] = {};
                std::snprintf(buf, sizeof(buf), (spec == 'X') ? "%X" : "%x", val);
                result += buf;
            }
        } else if (spec == 'f' || spec == 'F' || spec == 'g' || spec == 'G' || spec == 'e' || spec == 'E') {
            if (p + 8 <= end) {
                double val = 0.0;
                memcpy(&val, p, 8);
                p += 8;
                char buf[32] = {};
                std::snprintf(buf, sizeof(buf), "%.6g", val);
                result += buf;
            }
        } else {
            result += '%';
            if (hasLong) {
                result += 'l';
            }
            result += spec;
        }
    }

    return result;
}

static std::string ReadAsciiCString(const uint8_t* start, const uint8_t* end)
{
    if (start == nullptr || end == nullptr || start >= end) {
        return std::string();
    }

    const uint8_t* p = start;
    while (p < end && *p != 0) {
        ++p;
    }

    std::string result;
    result.reserve(static_cast<size_t>(p - start));
    for (const uint8_t* c = start; c < p; ++c) {
        const uint8_t ch = *c;
        result += static_cast<char>((ch >= 0x20 && ch < 0x7F) ? ch : '?');
    }
    return result;
}

static std::string ExtractBestPrintableString(const uint8_t* start,
                                              const uint8_t* end,
                                              const uint8_t** outAfterNull)
{
    if (outAfterNull) {
        *outAfterNull = nullptr;
    }

    if (start == nullptr || end == nullptr || start >= end) {
        return std::string();
    }

    std::string best;
    const uint8_t* bestAfterNull = nullptr;
    bool bestHasFormat = false;

    for (const uint8_t* p = start; p < end; ) {
        while (p < end && (*p < 0x20 || *p >= 0x7F)) {
            ++p;
        }
        if (p >= end) {
            break;
        }

        const uint8_t* strStart = p;
        while (p < end && *p >= 0x20 && *p < 0x7F) {
            ++p;
        }

        const size_t len = static_cast<size_t>(p - strStart);
        if (len >= 4) {
            std::string candidate(reinterpret_cast<const char*>(strStart), len);
            const bool hasFormat = candidate.find('%') != std::string::npos;

            if ((hasFormat && !bestHasFormat) ||
                (hasFormat == bestHasFormat && len > best.size())) {
                best = std::move(candidate);
                bestHasFormat = hasFormat;
                bestAfterNull = (p < end && *p == 0) ? (p + 1) : p;
            }
        }

        if (p < end && *p == 0) {
            ++p;
        }
    }

    if (outAfterNull) {
        *outAfterNull = bestAfterNull;
    }

    return best;
}

static bool LooksLikeOffsetEventRecord(const uint8_t* block, uint32_t blockSize)
{
    if (block == nullptr || blockSize < MIN_EVENT_SIZE) {
        return false;
    }

    uint16_t severity = 0;
    memcpy(&severity, block + OFF_SEVERITY, sizeof(severity));
    if (severity > 16) {
        return false;
    }

    const uint8_t* end = block + blockSize;
    const uint8_t* p = block + OFF_SOURCE_NAME;
    if (p >= end) {
        return false;
    }

    size_t sourceLen = 0;
    while (p + sourceLen < end && p[sourceLen] != 0) {
        const uint8_t ch = p[sourceLen];
        if (ch < 0x20 || ch >= 0x7F) {
            return false;
        }
        ++sourceLen;
        if (sourceLen > 64) {
            return false;
        }
    }

    if (sourceLen == 0 || p + sourceLen >= end) {
        return false;
    }

    const uint8_t* afterSource = p + sourceLen + 1;
    if (afterSource + sizeof(uint16_t) > end) {
        return false;
    }

    const uint8_t* searchEnd = afterSource + 64;
    if (searchEnd > end) {
        searchEnd = end;
    }

    for (const uint8_t* q = afterSource; q + sizeof(uint16_t) <= searchEnd; ++q) {
        uint16_t marker = 0;
        memcpy(&marker, q, sizeof(marker));
        if (marker == MARKER_DIRECT || marker == MARKER_GAP48) {
            return true;
        }
    }

    return false;
}

static const uint8_t* FindOffsetEventRecordStart(const uint8_t* raw, uint32_t size)
{
    if (raw == nullptr || size < MIN_EVENT_SIZE) {
        return nullptr;
    }

    const uint8_t* end = raw + size;
    if (LooksLikeOffsetEventRecord(raw, size)) {
        return raw;
    }
    if (size > BACKLOG_EVENT_OFFSET && LooksLikeOffsetEventRecord(raw + BACKLOG_EVENT_OFFSET, size - BACKLOG_EVENT_OFFSET)) {
        return raw + BACKLOG_EVENT_OFFSET;
    }

    for (const uint8_t* p = raw; p + MIN_EVENT_SIZE <= end; ++p) {
        if (LooksLikeOffsetEventRecord(p, static_cast<uint32_t>(end - p))) {
            return p;
        }
    }

    return nullptr;
}

static bool TryGetOffsetEventBlockLength(const uint8_t* block,
                                         uint32_t       availableBytes,
                                         uint32_t&      outBlockLength)
{
    if (block == nullptr || availableBytes < 0x74) {
        return false;
    }

    uint32_t payloadOffset = 0;
    uint32_t payloadLength = 0;
    memcpy(&payloadOffset, block + OFF_PAYLOAD_OFFSET, sizeof(payloadOffset));
    memcpy(&payloadLength, block + OFF_PAYLOAD_LENGTH, sizeof(payloadLength));

    const uint64_t total = static_cast<uint64_t>(payloadOffset) +
                           static_cast<uint64_t>(payloadLength);

    if (payloadOffset < 0x90 || payloadLength == 0 || total > availableBytes || total > 0x01000000ULL) {
        return false;
    }

    outBlockLength = static_cast<uint32_t>(total);
    return outBlockLength >= MIN_EVENT_SIZE;
}

static const uint8_t* FindNextGuidMarker(const uint8_t* start,
                                         const uint8_t* end,
                                         const uint8_t* guid16)
{
    if (start == nullptr || end == nullptr || guid16 == nullptr || start >= end) {
        return nullptr;
    }

    for (const uint8_t* p = start; p + 16 <= end; ++p) {
        if (memcmp(p, guid16, 16) == 0) {
            return p;
        }
    }
    return nullptr;
}

namespace bhf
{
namespace ads
{

// ---------------------------------------------------------------------------
// EventLoggerAccess – constructor
// ---------------------------------------------------------------------------
EventLoggerAccess::EventLoggerAccess(const std::string &gw,
                                     const AmsNetId     netId,
                                     const uint16_t     port)
    : m_Device(gw, netId,
               port ? port : AMSPORT_EVENTLOGGER_PUBLISHER_V2)
    , m_SeverityFilter(0)
{
}

EventLoggerAccess::EventLoggerAccess(const std::string &gw,
                                     const AmsNetId     netId,
                                     const uint16_t     port,
                                     const uint32_t     severityFilter)
    : m_Device(gw, netId,
               port ? port : AMSPORT_EVENTLOGGER_PUBLISHER_V2)
    , m_SeverityFilter(severityFilter)
{
}

// ---------------------------------------------------------------------------
// EventLoggerAccess – destructor
// ---------------------------------------------------------------------------
EventLoggerAccess::~EventLoggerAccess()
{
    if (m_Subscribed) {
        Unsubscribe();
    }
}

// ---------------------------------------------------------------------------
// EventLoggerAccess::SetCallback
// ---------------------------------------------------------------------------
void EventLoggerAccess::SetCallback(EventCallback callback)
{
    m_Callback = std::move(callback);
}

// ---------------------------------------------------------------------------
// EventLoggerAccess::Subscribe
// ---------------------------------------------------------------------------
long EventLoggerAccess::Subscribe()
{
    if (m_NotificationHandle)
    {
        return ADSERR_NOERR;
    }
    
    m_CallbackId = s_NextId.fetch_add(1);
    {
        std::lock_guard<std::mutex> lock(s_RegistryMutex);
        s_Registry[m_CallbackId] = this;
    }
    
    const AdsNotificationAttrib attrib = {
        NOTIFICATION_BUFFER_SIZE,
        ADSTRANS_SERVERCYCLE,
        0,
        { 0 }
    };
    
    try {
        m_NotificationHandle = std::make_unique<AdsHandle>(
            m_Device.GetHandle(
                EVENTLOGGER_SUBSCRIBE_INDEXGROUP,
                m_SeverityFilter,
                attrib,
                reinterpret_cast<PAdsNotificationFuncEx>(&NotificationCallback),
                m_CallbackId));

        LOG_INFO("EventLoggerAccess::Subscribe(): subscribed to EventLogger on AMS port "
             << std::dec << m_Device.m_Addr.port);
        
        m_Subscribed = true;
        return ADSERR_NOERR;
    }
    catch (const AdsException& ex) {
        {
            std::lock_guard<std::mutex> lock(s_RegistryMutex);
            s_Registry.erase(m_CallbackId);
        }
        m_CallbackId = 0;
        LOG_ERROR("EventLoggerAccess::Subscribe() failed with ADS error "
                  << ex.errorCode << " (" << ex.what() << ")\n");
        return ex.errorCode;
    }
}

// ---------------------------------------------------------------------------
// EventLoggerAccess::Unsubscribe
// ---------------------------------------------------------------------------
void EventLoggerAccess::Unsubscribe()
{
    if (m_NotificationHandle == nullptr && m_CallbackId == 0) {
        return;
    }

    m_NotificationHandle.reset();

    if (m_CallbackId != 0) {
        std::lock_guard<std::mutex> lock(s_RegistryMutex);
        s_Registry.erase(m_CallbackId);
        m_CallbackId = 0;
    }

    m_Subscribed = false;
}

// ---------------------------------------------------------------------------
// EventLoggerAccess::NotificationCallback  (static)
// ---------------------------------------------------------------------------
void EventLoggerAccess::NotificationCallback(
    const AmsAddr               * /*pAddr*/,
    const AdsNotificationHeader *pNotification,
    uint32_t                      hUser)
{
    if (!pNotification) {
        return;
    }

    EventLoggerAccess* self = nullptr;
    {
        std::lock_guard<std::mutex> lock(s_RegistryMutex);
        auto it = s_Registry.find(hUser);
        if (it != s_Registry.end()) {
            self = it->second;
        }
    }

    if (self == nullptr) {
        return;
    }

    self->HandleNotification(pNotification);
}

void EventLoggerAccess::HandleNotification(const AdsNotificationHeader* pNotification)
{
    if (!m_Callback || !pNotification) {
        return;
    }

    const uint8_t* raw = reinterpret_cast<const uint8_t*>(pNotification + 1);
    const uint32_t size = pNotification->cbSampleSize;

    if (size <= 16) {
        return;
    }

    ParseSingleEventRecord(raw, size, pNotification->nTimeStamp);
}

// ---------------------------------------------------------------------------
// EventLoggerAccess::ReadBacklog
// ---------------------------------------------------------------------------
void EventLoggerAccess::ReadBacklog()
{
    // The ReadWrite(0xC8, 0x02) expects a fixed 16-byte header structure
    // that specifies the read operation parameters. Do NOT use a cursor from 0xC8/0x01.
    // Format (little-endian):
    //   - version (0x01)
    //   - IndexGroup (0xC8)
    //   - unknown field (0x04)
    //   - buffer size hint (1000)
    struct BacklogHeader {
        uint32_t version;
        uint32_t indexGroup;
        uint32_t unknown;
        uint32_t bufferSize;
    } header;
    
    header.version = 0x01;
    header.indexGroup = EVTLOG_INDEXGROUP;  // 0xC8
    header.unknown = 0x04;
    header.bufferSize = 1000;

    // Exchange the header for the buffered event records.
    // 1 MB is enough for any realistic backlog; the server will clamp the
    // response to what is actually available.
    std::vector<uint8_t> eventBuf(0x100000);
    uint32_t eventBytes = 0;
    const long readErr = m_Device.ReadWriteReqEx2(
        EVTLOG_INDEXGROUP, EVTLOG_READ_EVENTS,
        eventBuf.size(), eventBuf.data(),
        sizeof(header), reinterpret_cast<uint8_t*>(&header),
        &eventBytes);

    if (readErr) {
        LOG_WARN("EventLoggerAccess::ReadBacklog() – ReadWrite(0xC8, 0x02) failed with ADS error "
                 << readErr << '\n');
        return;
    }

    if (eventBytes == 0) {
        LOG_INFO("EventLoggerAccess::ReadBacklog(): backlog empty");
        return;
    }

    LOG_INFO("EventLoggerAccess::ReadBacklog(): parsing " << std::dec << eventBytes << " backlog bytes");
    ParseBacklogBuffer(eventBuf.data(), eventBytes);
}

bool EventLoggerAccess::TryParseTwinSafeEvent(const uint8_t* block, uint32_t blockSize)
{
    if (!m_Callback || block == nullptr || blockSize < MIN_EVENT_SIZE) {
        return false;
    }

    uint32_t sourceId = 0;
    memcpy(&sourceId, block + OFF_SOURCE_ID, sizeof(sourceId));
    if (sourceId < TWINSAFE_ID_THRESHOLD) {
        return false;
    }

    uint32_t eventId = 0;
    uint32_t uniqueId = 0;
    uint16_t eventType = static_cast<uint16_t>(EventTypeEnum::Message);
    uint16_t severity = static_cast<uint16_t>(TcEventSeverity::Warning);
    uint16_t confirmState = static_cast<uint16_t>(TcEventConfirmState::NotSupported);
    uint64_t timeRaised = 0;
    uint64_t timeCleared = 0;
    uint64_t timeConfirmed = 0;
    uint32_t payloadLength = 0;

    if (blockSize > OFF_EVENT_ID + sizeof(eventId)) {
        memcpy(&eventId, block + OFF_EVENT_ID, sizeof(eventId));
    }
    if (blockSize > OFF_UNIQUE_ID + sizeof(uniqueId)) {
        memcpy(&uniqueId, block + OFF_UNIQUE_ID, sizeof(uniqueId));
    }
    if (blockSize > OFF_EVENT_TYPE + sizeof(eventType)) {
        memcpy(&eventType, block + OFF_EVENT_TYPE, sizeof(eventType));
    }
    if (blockSize > OFF_SEVERITY + sizeof(severity)) {
        memcpy(&severity, block + OFF_SEVERITY, sizeof(severity));
    }
    if (blockSize > OFF_CONFIRM_STATE + sizeof(confirmState)) {
        memcpy(&confirmState, block + OFF_CONFIRM_STATE, sizeof(confirmState));
    }
    if (blockSize > OFF_TIME_RAISED + sizeof(timeRaised)) {
        memcpy(&timeRaised, block + OFF_TIME_RAISED, sizeof(timeRaised));
    }
    if (blockSize > OFF_TIME_CLEARED + sizeof(timeCleared)) {
        memcpy(&timeCleared, block + OFF_TIME_CLEARED, sizeof(timeCleared));
    }
    if (blockSize > OFF_TIME_CONFIRMED + sizeof(timeConfirmed)) {
        memcpy(&timeConfirmed, block + OFF_TIME_CONFIRMED, sizeof(timeConfirmed));
    }
    if (blockSize > OFF_PAYLOAD_LENGTH + sizeof(payloadLength)) {
        memcpy(&payloadLength, block + OFF_PAYLOAD_LENGTH, sizeof(payloadLength));
    }

    const uint8_t* end = block + blockSize;
    std::string sourceName = ReadAsciiCString(block + OFF_SOURCE_NAME, end);
    if (sourceName.empty()) {
        sourceName = "TwinSAFE Device";
    }

    char errorMsg[256] = {};
    std::snprintf(errorMsg, sizeof(errorMsg), "'TwinSAFE error code 0x%X %s'", eventId, sourceName.c_str());
    std::string customText(errorMsg);

    TcEventEntry entry = {};
    if (blockSize >= OFF_EVENT_CLASS + sizeof(entry.abEventClass)) {
        memcpy(entry.abEventClass, block + OFF_EVENT_CLASS, sizeof(entry.abEventClass));
    }
    entry.nEventId = eventId;
    entry.nSeverity = severity;
    entry.nEventType = eventType;
    entry.nTimeRaised = timeRaised;
    entry.nTimeCleared = timeCleared;
    entry.nTimeConfirmed = timeConfirmed;
    entry.nConfirmationState = confirmState;
    entry.nSrcId = sourceId;
    entry.nCbSourceName = static_cast<uint32_t>(sourceName.size() + 1);
    entry.nCbData = payloadLength;
    entry.nUniqueId = uniqueId;

    m_Callback(entry, customText, sourceName);
    return true;
}

void EventLoggerAccess::ParseOffsetEventRecord(const uint8_t* block,
                                               uint32_t       blockSize,
                                               uint64_t       stampTimestamp)
{
    if (!m_Callback || block == nullptr || blockSize < MIN_EVENT_SIZE) {
        return;
    }

    if (blockSize <= OFF_SOURCE_NAME) {
        return;
    }

    uint16_t severity = 0;
    uint16_t eventType = static_cast<uint16_t>(EventTypeEnum::Message);
    uint32_t eventId = 0;
    uint32_t eventFlags = 0;
    uint32_t uniqueId = 0;
    uint32_t sourceId = 0;
    uint16_t confirmState = 0;
    uint32_t payloadOffset = 0;
    uint32_t payloadLength = 0;
    uint64_t timeRaised = 0;
    uint64_t timeCleared = 0;
    uint64_t timeConfirmed = 0;

    memcpy(&eventId, block + OFF_EVENT_ID, sizeof(eventId));
    memcpy(&eventFlags, block + OFF_EVENT_FLAGS, sizeof(eventFlags));
    memcpy(&severity, block + OFF_SEVERITY, sizeof(severity));
    memcpy(&eventType, block + OFF_EVENT_TYPE, sizeof(eventType));
    memcpy(&uniqueId, block + OFF_UNIQUE_ID, sizeof(uniqueId));
    memcpy(&timeRaised, block + OFF_TIME_RAISED, sizeof(timeRaised));
    memcpy(&timeCleared, block + OFF_TIME_CLEARED, sizeof(timeCleared));
    memcpy(&confirmState, block + OFF_CONFIRM_STATE, sizeof(confirmState));
    memcpy(&timeConfirmed, block + OFF_TIME_CONFIRMED, sizeof(timeConfirmed));
    if (blockSize > OFF_PAYLOAD_OFFSET + sizeof(payloadOffset)) {
        memcpy(&payloadOffset, block + OFF_PAYLOAD_OFFSET, sizeof(payloadOffset));
    }
    if (blockSize > OFF_PAYLOAD_LENGTH + sizeof(payloadLength)) {
        memcpy(&payloadLength, block + OFF_PAYLOAD_LENGTH, sizeof(payloadLength));
    }
    memcpy(&sourceId, block + OFF_SOURCE_ID, sizeof(sourceId));

    uint64_t eventTime = timeRaised;
    if (eventTime < FILETIME_YEAR2000) {
        eventTime = stampTimestamp;
    }

    const uint8_t* blockEnd = block + blockSize;
    const uint8_t* sourceNameStart = block + OFF_SOURCE_NAME;
    std::string sourceName = ReadAsciiCString(sourceNameStart, blockEnd);

    const uint8_t* afterSourceName = sourceNameStart;
    while (afterSourceName < blockEnd && *afterSourceName != 0) {
        ++afterSourceName;
    }
    if (afterSourceName < blockEnd) {
        ++afterSourceName;
    }

    const uint8_t* customTextStart = nullptr;
    const uint8_t* markerPos = nullptr;
    if (afterSourceName + sizeof(uint16_t) <= blockEnd) {
        const uint8_t* markerSearchEnd = afterSourceName + 64;
        if (markerSearchEnd > blockEnd) {
            markerSearchEnd = blockEnd;
        }

        uint16_t controlWord = 0;
        for (const uint8_t* p = afterSourceName; p + sizeof(uint16_t) <= markerSearchEnd; ++p) {
            memcpy(&controlWord, p, sizeof(controlWord));
            if (controlWord == MARKER_DIRECT || controlWord == MARKER_GAP48) {
                markerPos = p;
                break;
            }
        }

        if (markerPos != nullptr) {
            memcpy(&controlWord, markerPos, sizeof(controlWord));
            if (controlWord == MARKER_DIRECT) {
                customTextStart = markerPos + sizeof(uint16_t);
            } else if (controlWord == MARKER_GAP48) {
                const uint8_t* candidate = markerPos + sizeof(uint16_t) + TWINCAT_TEXT_GAP48_BYTES;
                while (candidate < blockEnd && *candidate == 0) {
                    ++candidate;
                }
                if (candidate < blockEnd) {
                    customTextStart = candidate;
                }
            }
        }
    }

    std::string text;
    const uint8_t* paramStart = nullptr;
    uint16_t controlChar = 0;
    if (customTextStart != nullptr && customTextStart < blockEnd) {
        text = ReadAsciiCString(customTextStart, blockEnd);
        if (!text.empty()) {
            paramStart = customTextStart + text.size();
            if (paramStart < blockEnd && *paramStart == 0) {
                ++paramStart;
            }
        }

        if (markerPos != nullptr) {
            memcpy(&controlChar, markerPos, sizeof(controlChar));
            if (controlChar == MARKER_GAP48) {
                const uint8_t* scannedParamStart = nullptr;
                const std::string scanned = ExtractBestPrintableString(customTextStart, blockEnd, &scannedParamStart);
                if (!scanned.empty()) {
                    text = scanned;
                    paramStart = scannedParamStart;
                }
            }
        }
    }

    if (text.empty() && afterSourceName < blockEnd) {
        for (const uint8_t* p = afterSourceName; p < blockEnd; ) {
            while (p < blockEnd && (*p < 0x20 || *p >= 0x7F)) {
                ++p;
            }
            if (p >= blockEnd) {
                break;
            }

            const uint8_t* runStart = p;
            while (p < blockEnd && *p >= 0x20 && *p < 0x7F) {
                ++p;
            }

            const size_t len = static_cast<size_t>(p - runStart);
            if (len > text.size()) {
                text.assign(reinterpret_cast<const char*>(runStart), len);
            }

            if (p < blockEnd && *p == 0) {
                ++p;
            }
        }
    }

    if (text.find('%') != std::string::npos && paramStart != nullptr && paramStart < blockEnd) {
        text = FormatEventText(text, paramStart, static_cast<size_t>(blockEnd - paramStart));
    }

    uint16_t resolvedEventType = eventType;
    if (resolvedEventType > static_cast<uint16_t>(EventTypeEnum::Message)) {
        resolvedEventType = static_cast<uint16_t>((eventFlags >> 16) & 0xFFFF);
    }

    uint32_t resolvedPayloadLength = payloadLength;
    if (resolvedPayloadLength == 0 && payloadOffset > 0 && payloadOffset < blockSize) {
        resolvedPayloadLength = static_cast<uint32_t>(blockSize - payloadOffset);
    }
    if (resolvedPayloadLength == 0 && paramStart != nullptr && paramStart <= blockEnd) {
        resolvedPayloadLength = static_cast<uint32_t>(blockEnd - paramStart);
    }
    if (resolvedPayloadLength == 0 && customTextStart != nullptr && customTextStart <= blockEnd) {
        resolvedPayloadLength = static_cast<uint32_t>(blockEnd - customTextStart);
    }

    TcEventEntry entry{};
    memcpy(entry.abEventClass, block + OFF_EVENT_CLASS, sizeof(entry.abEventClass));
    entry.nSeverity = severity;
    entry.nEventType = resolvedEventType;
    entry.nSrcId = sourceId;
    entry.nEventId = eventId;
    entry.nUniqueId = uniqueId;
    entry.nTimeRaised = eventTime;
    entry.nTimeCleared = timeCleared;
    entry.nTimeConfirmed = timeConfirmed;
    entry.nConfirmationState = confirmState;
    entry.nCbData = resolvedPayloadLength;
    entry.nCbSourceName = static_cast<uint32_t>(sourceName.size() + 1);

    m_Callback(entry, text, sourceName);
}

void EventLoggerAccess::ParseSingleEventRecord(const uint8_t* raw,
                                               uint32_t       size,
                                               uint64_t       stampTimestamp)
{
    if (!m_Callback || raw == nullptr || size < 16) {
        return;
    }

    const uint8_t* bestOffset = nullptr;
    if (size >= MIN_EVENT_SIZE && LooksLikeOffsetEventRecord(raw, size)) {
        bestOffset = raw;
    } else if (size > LIVE_EVENT_OFFSET && size - LIVE_EVENT_OFFSET >= MIN_EVENT_SIZE && 
        LooksLikeOffsetEventRecord(raw + LIVE_EVENT_OFFSET, size - LIVE_EVENT_OFFSET)) {
        bestOffset = raw + LIVE_EVENT_OFFSET;
    } else if (size > BACKLOG_EVENT_OFFSET && size - BACKLOG_EVENT_OFFSET >= MIN_EVENT_SIZE && 
        LooksLikeOffsetEventRecord(raw + BACKLOG_EVENT_OFFSET, size - BACKLOG_EVENT_OFFSET)) {
        bestOffset = raw + BACKLOG_EVENT_OFFSET;
    } else {
        bestOffset = FindOffsetEventRecordStart(raw, size);
    }

    if (bestOffset != nullptr) {
        ParseOffsetEventRecord(bestOffset, static_cast<uint32_t>((raw + size) - bestOffset), stampTimestamp);
    }
}

void EventLoggerAccess::ParseBacklogBuffer(const uint8_t* raw, uint32_t totalBytes)
{
    if (!m_Callback || raw == nullptr || totalBytes < 52) {
        return;
    }

    uint32_t nEvents = 0;
    memcpy(&nEvents, raw + 16, sizeof(nEvents));
    if (nEvents == 0 || nEvents > 10000) {
        LOG_WARN("EventLoggerAccess::ParseBacklogBuffer(): invalid nEvents=" << nEvents);
        return;
    }

    const uint8_t* end = raw + totalBytes;
    const uint8_t* blockGuid = raw + BACKLOG_EVENT_OFFSET;
    const uint8_t* blockStart = raw + BACKLOG_EVENT_OFFSET;

    uint32_t parsedBlocks = 0;
    while (blockStart + 16 <= end) {
        const uint32_t available = static_cast<uint32_t>(end - blockStart);
        uint32_t blockSize = 0;

        if (!TryGetOffsetEventBlockLength(blockStart, available, blockSize)) {
            const uint8_t* nextGuid = FindNextGuidMarker(blockStart + 16, end, blockGuid);
            blockSize = (nextGuid != nullptr)
                ? static_cast<uint32_t>(nextGuid - blockStart)
                : available;
        }

        if (blockSize < 16) {
            break;
        }

        if (LooksLikeOffsetEventRecord(blockStart, blockSize)) {
            ParseOffsetEventRecord(blockStart, blockSize, 0);
        } else if (!TryParseTwinSafeEvent(blockStart, blockSize)) {
            LOG_WARN("EventLoggerAccess::ParseBacklogBuffer(): skipped non-matching block index="
                     << parsedBlocks << " size=" << blockSize);
        }

        ++parsedBlocks;
        if (parsedBlocks > 100000) {
            LOG_WARN("EventLoggerAccess::ParseBacklogBuffer(): safety break");
            break;
        }

        if (blockStart + blockSize >= end) {
            break;
        }
        blockStart += blockSize;
    }
}

// ---------------------------------------------------------------------------
// EventLoggerAccess::ParseEventBuffer
// ---------------------------------------------------------------------------
void EventLoggerAccess::ParseEventBuffer(const uint8_t *raw, uint32_t remaining)
{
    if (!m_Callback || raw == nullptr) {
        return;
    }

    while (remaining >= sizeof(TcEventEntry)) {
        const TcEventEntry* entry = reinterpret_cast<const TcEventEntry*>(raw);
        const uint32_t totalSize = static_cast<uint32_t>(sizeof(TcEventEntry)) + entry->nCbData;
        if (totalSize > remaining) {
            LOG_WARN("EventLoggerAccess::ParseEventBuffer(): truncated event entry");
            break;
        }

        std::string text;
        if (entry->nCbData >= sizeof(uint16_t)) {
            const uint16_t* wchars = reinterpret_cast<const uint16_t*>(raw + sizeof(TcEventEntry));
            const size_t numWchars = entry->nCbData / sizeof(uint16_t);
            text.reserve(numWchars);
            for (size_t i = 0; i < numWchars; ++i) {
                const uint16_t wc = wchars[i];
                if (wc == 0) {
                    break;
                }
                text += static_cast<char>((wc >= 0x20 && wc < 0x7F) ? wc : '?');
            }
        }

        m_Callback(*entry, text, std::string());
        raw += totalSize;
        remaining -= totalSize;
    }
}

} // namespace ads
} // namespace bhf
