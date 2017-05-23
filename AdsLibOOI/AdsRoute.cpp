#include "AdsRoute.h"
#include "AdsException.h"
#include "AdsLib/AdsLib.h"

static AmsNetId* AddRoute(AmsNetId ams, const char* ip)
{
    const auto error = AdsAddRoute(ams, ip);
    if (error) {
        throw AdsException(error);
    }
    return new AmsNetId {ams};
}

AdsRoute::AdsRoute(const std::string& ipV4, AmsNetId netId, uint16_t port)
    : m_NetId(AddRoute(netId, ipV4.c_str()), {[](AmsNetId ams){AdsDelRoute(ams); return 0; }}),
    m_Addr({netId, port}),
    m_LocalPort(new long { AdsPortOpenEx() }, {AdsPortCloseEx})
{}

long AdsRoute::DeleteNotificationHandle(uint32_t handle) const
{
    if (handle) {
        return AdsSyncDelDeviceNotificationReqEx(GetLocalPort(), &m_Addr, handle);
    }
    return 0;
}

long AdsRoute::DeleteSymbolHandle(uint32_t handle) const
{
    return WriteReqEx(ADSIGRP_SYM_RELEASEHND, 0, sizeof(handle), &handle);
}

DeviceInfo AdsRoute::GetDeviceInfo() const
{
    DeviceInfo info;
    auto error = AdsSyncReadDeviceInfoReqEx(GetLocalPort(),
                                            &m_Addr,
                                            &info.name[0],
                                            &info.version);

    if (error) {
        throw AdsException(error);
    }
    return info;
}

AdsHandle AdsRoute::GetHandle(const uint32_t offset) const
{
    return {new uint32_t {offset}, {[](uint32_t){ return 0; }}};
}

AdsHandle AdsRoute::GetHandle(const std::string& symbolName) const
{
    uint32_t handle = 0;
    uint32_t bytesRead = 0;
    uint32_t error = ReadWriteReqEx2(
        ADSIGRP_SYM_HNDBYNAME, 0,
        sizeof(handle), &handle,
        symbolName.size(),
        symbolName.c_str(),
        &bytesRead
        );

    if (error || (sizeof(handle) != bytesRead)) {
        throw AdsException(error);
    }

    return {new uint32_t {handle}, {std::bind(&AdsRoute::DeleteSymbolHandle, this, std::placeholders::_1)}};
}

AdsHandle AdsRoute::GetHandle(uint32_t                     indexGroup,
                              uint32_t                     indexOffset,
                              const AdsNotificationAttrib& notificationAttributes,
                              PAdsNotificationFuncEx       callback) const
{
    uint32_t handle = 0;
    auto error = AdsSyncAddDeviceNotificationReqEx(
        *m_LocalPort, &m_Addr,
        indexGroup, indexOffset,
        &notificationAttributes,
        callback,
        indexOffset,
        &handle);
    if (error || !handle) {
        throw AdsException(error);
    }
    return {new uint32_t {handle}, {std::bind(&AdsRoute::DeleteNotificationHandle, this, std::placeholders::_1)}};
}

long AdsRoute::GetLocalPort() const
{
    return *m_LocalPort;
}

AdsDeviceState AdsRoute::GetState() const
{
    AdsDeviceState state;
    static_assert(sizeof(state.ads) == sizeof(uint16_t), "size missmatch");
    static_assert(sizeof(state.device) == sizeof(uint16_t), "size missmatch");
    auto error = AdsSyncReadStateReqEx(GetLocalPort(),
                                       &m_Addr,
                                       (uint16_t*)&state.ads,
                                       (uint16_t*)&state.device);

    if (error) {
        throw AdsException(error);
    }

    return state;
}

void AdsRoute::SetState(const ADSSTATE AdsState, const ADSSTATE DeviceState) const
{
    auto error = AdsSyncWriteControlReqEx(GetLocalPort(),
                                          &m_Addr,
                                          AdsState,
                                          DeviceState,
                                          0, nullptr); // No additional data

    if (error) {
        throw AdsException(error);
    }
}

void AdsRoute::SetTimeout(const uint32_t timeout) const
{
    const auto error = AdsSyncSetTimeoutEx(GetLocalPort(), timeout);
    if (error) {
        throw AdsException(error);
    }
}

uint32_t AdsRoute::GetTimeout() const
{
    uint32_t timeout = 0;
    const auto error = AdsSyncGetTimeoutEx(GetLocalPort(), &timeout);
    if (error) {
        throw AdsException(error);
    }
    return timeout;
}

long AdsRoute::ReadReqEx2(uint32_t group, uint32_t offset, uint32_t length, void* buffer, uint32_t* bytesRead) const
{
    return AdsSyncReadReqEx2(*m_LocalPort, &m_Addr, group, offset, length, buffer, bytesRead);
}

long AdsRoute::ReadWriteReqEx2(uint32_t    indexGroup,
                               uint32_t    indexOffset,
                               uint32_t    readLength,
                               void*       readData,
                               uint32_t    writeLength,
                               const void* writeData,
                               uint32_t*   bytesRead) const
{
    return AdsSyncReadWriteReqEx2(GetLocalPort(),
                                  &m_Addr,
                                  indexGroup, indexOffset,
                                  readLength, readData,
                                  writeLength, writeData,
                                  bytesRead
                                  );
}

long AdsRoute::WriteReqEx(uint32_t group, uint32_t offset, uint32_t length, const void* buffer) const
{
    return AdsSyncWriteReqEx(GetLocalPort(), &m_Addr, group, offset, length, buffer);
}