// SPDX-License-Identifier: MIT
/**
   Copyright (c) 2024 Beckhoff Automation GmbH & Co. KG
 */

#pragma once

#include "AdsDevice.h"
#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace bhf
{
namespace ads
{

/**
 * @brief Confirmation state of an alarm event.
 * https://infosys.beckhoff.com/english.php?content=../content/1033/tc3_eventlogger/6196012811.html
 */
enum class TcEventConfirmState : uint16_t {
    Confirmed = 0,
    NotRequired = 1,
    NotSupported = 2,
    Reset = 3,
    WaitForConfirm = 4,
};

/**
 * @brief Severity levels as defined by the TwinCAT EventLogger.
 * https://infosys.beckhoff.com/english.php?content=../content/1033/tc3_eventlogger/6196017547.html
 */
enum class TcEventSeverity : uint16_t {
    Verbose  = 0,
    Info     = 1,
    Warning  = 2,
    Error    = 3,
    Critical = 4,
};

/**
 * @brief Event type discriminator.
 * https://infosys.beckhoff.com/english.php?content=../content/1033/tc3_eventlogger/6196015627.html
 * A Message is a one-shot event; an Alarm has a distinct come/gone lifecycle.
 */
enum class EventTypeEnum : uint16_t {
    Alarm = 0,
    Message = 1,
};

/**
 * @brief Wire-format of a single event entry as delivered in an ADS notification
 *        from the TwinCAT EventLogger Publisher V2 (AMS port 132).
 *
 * The structure is immediately followed by @c nCbData bytes of event-specific
 * payload data (typically a UTF-16LE-encoded description string).
 *
 * @note The layout matches the TwinCAT TC_EVENT_ENTRY definition.  If Beckhoff
 *       changes the wire format in a future TwinCAT release the member names and
 *       offsets below must be updated accordingly.
 */
#pragma pack(push, 1)
struct TcEventEntry {
    /** Event Class GUID xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx.
     * Offset 0x00: 16 bytes
     */
    uint8_t abEventClass[16];
    
    /** Application-defined event identifier.
     * Offset 0x10: 4 bytes
    */
    uint32_t nEventId;

    /** Severity of this event (cast to TcEventSeverity for named access).
     * Offset 0x18: 2 bytes
    */
    uint16_t nSeverity;

    /** Distinguishes message events from alarm events (cast to EventTypeEnum).
     * Offset 0x2A: 2 bytes
     */
    uint16_t nEventType;

    /** Unique ID for each event.
     * Offset 0x2C: 4 bytes
     */
    uint32_t nUniqueId;
    
    /** Event timestamp expressed as a 64-bit count of 100-nanosecond intervals
     * since 1601-01-01 00:00:00.000 UTC (Windows FILETIME convention).
     * Time when the event was raised.
     * Offset 0x30: 8 bytes
     */
    uint64_t nTimeRaised;
    /** Time when the event was cleared.
     * Offset 0x38: 8 bytes
     */
    uint64_t nTimeCleared;
    /** Time when the event was confirmed.
     * Offset 0x50: 8 bytes
     */
    uint64_t nTimeConfirmed;

    /** Confirmation state of the alarm type event (cast to TcEventConfirmationState).
     * Offset 0x48: 2 bytes
    */
    uint16_t nConfirmationState;
    
    /** Identifies the source device / task that raised the event.
     * Offset 0x90: 4 bytes
    */
    uint32_t nSrcId;

    /** Source name string length in bytes (typically UTF-16LE).
     * Offset 0x94: 4 bytes
     * Ends with a null terminator.
    */
    uint32_t nCbSourceName;

    /**
     * Number of payload bytes that immediately follow this header.
     * May be zero when no text is attached.
     */
    uint32_t nCbData;

    /* Payload: nCbData bytes follow in memory (not a struct member). */
};
#pragma pack(pop)

/**
 * @brief Connects to the TwinCAT EventLogger Publisher V2 (AMS port 132).
 *
 * Use Subscribe()/Unsubscribe() for explicit lifecycle management.
 */
struct EventLoggerAccess {
    /// <summary>
    /// Callback invoked for every parsed EventLogger record.
    /// Parameters: parsed event data, decoded UTF-8 text, and SourceName.
    /// </summary>
    using EventCallback = std::function<void(const TcEventEntry&, const std::string&, const std::string&)>;

    /**
     * @param gw      IP address (or hostname) used to reach the target AMS
     *                router (the gateway).
     * @param netId   AmsNetId of the TwinCAT system that hosts the EventLogger.
     * @param port    AMS port of the EventLogger Publisher V2.
     *                Defaults to 132 when 0 is passed.
     */
    EventLoggerAccess(const std::string &gw, AmsNetId netId,
                      uint16_t port = 0);

    EventLoggerAccess(const std::string &gw, AmsNetId netId,
                      uint16_t port, uint32_t severityFilter);

    /// <summary>Destructor. Automatically unsubscribes if needed.</summary>
    ~EventLoggerAccess();

    EventLoggerAccess(const EventLoggerAccess&) = delete;
    EventLoggerAccess& operator=(const EventLoggerAccess&) = delete;

    /// <summary>Sets the callback invoked for each parsed event.</summary>
    void SetCallback(EventCallback callback);

    /// <summary>Subscribes to EventLogger notifications. Non-blocking.</summary>
    long Subscribe();

    /// <summary>Reads and dispatches buffered events that accumulated before subscription.</summary>
    void ReadBacklog();

    /// <summary>Unsubscribes from EventLogger notifications.</summary>
    void Unsubscribe();

  private:
    AdsDevice m_Device;
    EventCallback m_Callback;
    std::unique_ptr<AdsHandle> m_NotificationHandle;
    uint32_t m_SeverityFilter = 0;
    uint32_t m_CallbackId = 0;
    bool m_Subscribed = false;

    /** Maximum byte size of a single ADS notification payload. */
    static constexpr uint32_t NOTIFICATION_BUFFER_SIZE = 8192;

    // IndexGroup / IndexOffset for reading the EventLogger ring-buffer backlog.
    // This implementation directly uses ReadWrite(0xC8, 0x02) with a fixed
    // 16-byte request header (version/indexGroup/unknown/bufferSize).
    static constexpr uint32_t EVTLOG_INDEXGROUP  = 0x000000C8;
    static constexpr uint32_t EVTLOG_READ_EVENTS = 0x00000002;

    static std::mutex s_RegistryMutex;
    static std::unordered_map<uint32_t, EventLoggerAccess*> s_Registry;
    static std::atomic<uint32_t> s_NextId;

    /**
     * @brief Parses a flat buffer of back-to-back TcEventEntry records and
     *        dispatches each one to the callback.
     */
    void ParseEventBuffer(const uint8_t *raw, uint32_t remaining);

    /**
     * @brief Parses the backlog ReadWrite response container and dispatches
     *        each contained event to the callback.
     */
    void ParseBacklogBuffer(const uint8_t* raw, uint32_t totalBytes);

    /**
     * @brief Parses a single live notification event record.
     */
    void ParseSingleEventRecord(const uint8_t* raw, uint32_t size, uint64_t stampTimestamp);

    /**
     * @brief Parses one GUID-based offset event block.
     */
    void ParseOffsetEventRecord(const uint8_t* block, uint32_t blockSize, uint64_t stampTimestamp);

    /**
     * @brief Attempts to parse a TwinSAFE or device-specific event block.
     */
    bool TryParseTwinSafeEvent(const uint8_t* block, uint32_t blockSize);

    /**
     * @brief ADS notification callback invoked by the ADS router thread for
     *        each event received from the EventLogger publisher.
     */
    static void NotificationCallback(const AmsAddr           *pAddr,
                                     const AdsNotificationHeader *pNotification,
                                     uint32_t                     hUser);

    void HandleNotification(const AdsNotificationHeader* pNotification);
};

} // namespace ads
} // namespace bhf
