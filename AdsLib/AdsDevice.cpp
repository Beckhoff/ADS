// SPDX-License-Identifier: MIT
/**
   Copyright (c) 2016 - 2022 Beckhoff Automation GmbH & Co. KG
 */

#include "AdsDevice.h"
#include "AdsException.h"
#include "AdsLib.h"
#include <limits>
#include "Log.h"

std::mutex AdsDevice::m_CallbackMutex;
uint32_t AdsDevice::m_NextCallbackHandle = 0x80000000;
std::unordered_map<uint32_t, PAdsNotificationFuncExFunc> AdsDevice::m_FuncCallbacks;

static AmsNetId* AddRoute(AmsNetId ams, const char* ip)
{
    const auto error = bhf::ads::AddLocalRoute(ams, ip);
    if (error) {
        throw AdsException(error);
    }
    return new AmsNetId {ams};
}

AdsDevice::AdsDevice(const std::string& ipV4, AmsNetId netId, uint16_t port)
    : m_NetId(AddRoute(netId, ipV4.c_str()), {[](AmsNetId ams){bhf::ads::DelLocalRoute(ams); return 0; }}),
    m_Addr({netId, port}),
    m_LocalPort(new long { AdsPortOpenEx() }, {AdsPortCloseEx})
{}

long AdsDevice::DeleteNotificationHandle(uint32_t handle, uint32_t hUser) const
{
    AdsDevice::DeleteFuncCallback(hUser);
    if (handle) {
        return AdsSyncDelDeviceNotificationReqEx(GetLocalPort(), &m_Addr, handle);
    }
    return 0;
}

long AdsDevice::CloseFile(uint32_t handle) const
{
    return ReadWriteReqEx2(SYSTEMSERVICE_FCLOSE, handle, 0, nullptr, 0, nullptr, nullptr);
}

long AdsDevice::DeleteSymbolHandle(uint32_t handle) const
{
    return WriteReqEx(ADSIGRP_SYM_RELEASEHND, 0, sizeof(handle), &handle);
}


DeviceInfo AdsDevice::GetDeviceInfo() const
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

AdsHandle AdsDevice::GetHandle(const uint32_t offset) const
{
    return {new uint32_t {offset}, {[](uint32_t){ return 0; }}};
}

AdsHandle AdsDevice::GetHandle(const std::string& symbolName) const
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

    handle = bhf::ads::letoh(handle);
    return {new uint32_t {handle}, {std::bind(&AdsDevice::DeleteSymbolHandle, this, std::placeholders::_1)}};
}

AdsHandle AdsDevice::GetHandle(const uint32_t               indexGroup,
                               const uint32_t               indexOffset,
                               const AdsNotificationAttrib& notificationAttributes,
                               PAdsNotificationFuncExFunc   callback) const
{
    uint32_t handle = 0;
    uint32_t hUser = AdsDevice::AddFuncCallback(callback);
    auto error = AdsSyncAddDeviceNotificationReqEx(
        *m_LocalPort, &m_Addr,
        indexGroup, indexOffset,
        &notificationAttributes,
        &AdsDevice::CallFuncCallback,
        hUser,
        &handle);
    if (error || !handle) {
        AdsDevice::DeleteFuncCallback(hUser);
        throw AdsException(error);
    }
    handle = bhf::ads::letoh(handle);
    return {new uint32_t {handle}, {std::bind(&AdsDevice::DeleteNotificationHandle, this, std::placeholders::_1, hUser)}};
}

long AdsDevice::GetLocalPort() const
{
    return *m_LocalPort;
}

AdsDeviceState AdsDevice::GetState() const
{
    uint16_t state[2];
    auto error = AdsSyncReadStateReqEx(GetLocalPort(),
                                       &m_Addr,
                                       &state[0],
                                       &state[1]);

    if (error) {
        throw AdsException(error);
    }

    for (auto i: state) {
        if (i >= ADSSTATE::ADSSTATE_MAXSTATES) {
            throw std::out_of_range("Unknown ADSSTATE(" + std::to_string(i) + ')');
        }
    }
    return {static_cast<ADSSTATE>(state[0]), static_cast<ADSSTATE>(state[1])};
}

void AdsDevice::SetState(const ADSSTATE AdsState, const ADSSTATE DeviceState) const
{
    auto error = AdsSyncWriteControlReqEx(GetLocalPort(),
                                          &m_Addr,
                                          AdsState,
                                          DeviceState,
                                          0, nullptr);

    if (error) {
        throw AdsException(error);
    }
}

void AdsDevice::SetTimeout(const uint32_t timeout) const
{
    const auto error = AdsSyncSetTimeoutEx(GetLocalPort(), timeout);
    if (error) {
        throw AdsException(error);
    }
}

uint32_t AdsDevice::GetTimeout() const
{
    uint32_t timeout = 0;
    const auto error = AdsSyncGetTimeoutEx(GetLocalPort(), &timeout);
    if (error) {
        throw AdsException(error);
    }
    return timeout;
}

AdsHandle AdsDevice::OpenFile(const std::string& filename, const uint32_t flags) const
{
    uint32_t bytesRead = 0;
    uint32_t handle;
    const auto error = ReadWriteReqEx2(SYSTEMSERVICE_FOPEN,
                                       flags,
                                       sizeof(handle),
                                       &handle,
                                       filename.length(),
                                       filename.c_str(),
                                       &bytesRead);

    if (error) {
        throw AdsException(error);
    }
    handle = bhf::ads::letoh(handle);
    return {new uint32_t {handle}, {std::bind(&AdsDevice::CloseFile, this, std::placeholders::_1)}};
}

long AdsDevice::ReadReqEx2(uint32_t group, uint32_t offset, size_t length, void* buffer, uint32_t* bytesRead) const
{
    if (length > std::numeric_limits<uint32_t>::max()) {
        return ADSERR_DEVICE_INVALIDSIZE;
    }
    return AdsSyncReadReqEx2(*m_LocalPort, &m_Addr, group, offset, static_cast<uint32_t>(length), buffer, bytesRead);
}

long AdsDevice::ReadWriteReqEx2(uint32_t    indexGroup,
                                uint32_t    indexOffset,
                                size_t      readLength,
                                void*       readData,
                                size_t      writeLength,
                                const void* writeData,
                                uint32_t*   bytesRead) const
{
    if (readLength > std::numeric_limits<uint32_t>::max()) {
        return ADSERR_DEVICE_INVALIDSIZE;
    }
    if (writeLength > std::numeric_limits<uint32_t>::max()) {
        return ADSERR_DEVICE_INVALIDSIZE;
    }
    return AdsSyncReadWriteReqEx2(GetLocalPort(),
                                  &m_Addr,
                                  indexGroup, indexOffset,
                                  static_cast<uint32_t>(readLength), readData,
                                  static_cast<uint32_t>(writeLength), writeData,
                                  bytesRead
                                  );
}

long AdsDevice::WriteReqEx(uint32_t group, uint32_t offset, size_t length, const void* buffer) const
{
    if (length > std::numeric_limits<uint32_t>::max()) {
        return ADSERR_DEVICE_INVALIDSIZE;
    }
    return AdsSyncWriteReqEx(GetLocalPort(), &m_Addr, group, offset, static_cast<uint32_t>(length), buffer);
}

uint32_t AdsDevice::AddFuncCallback(PAdsNotificationFuncExFunc callback)
{
    std::lock_guard<decltype(m_CallbackMutex)> lock(m_CallbackMutex);
    m_FuncCallbacks.insert(std::make_pair(m_NextCallbackHandle, callback));
    return m_NextCallbackHandle++;
}

void AdsDevice::DeleteFuncCallback(uint32_t handle)
{
    std::lock_guard<decltype(m_CallbackMutex)> lock(m_CallbackMutex);
    if(m_FuncCallbacks.erase(handle) == 0)
    {
        LOG_WARN("Handle " << handle << " has already been deleted.");
    }
}

void AdsDevice::CallFuncCallback(CONST AmsAddr* pAddr, const AdsNotificationHeader* pNotification, uint32_t hUser)
{
    std::lock_guard<decltype(m_CallbackMutex)> lock(m_CallbackMutex);
    auto it = m_FuncCallbacks.find(hUser);
    if(it == m_FuncCallbacks.end())
    {
        LOG_WARN("Callback for handle " << hUser << " not found.");
        return;
    }
    it->second(pAddr, pNotification);
}
