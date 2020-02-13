// SPDX-License-Identifier: MIT
/**
   Copyright (c) 2016 - 2020 Beckhoff Automation GmbH & Co. KG
 */

#pragma once
#include "AdsException.h"
#include "AdsDef.h"
#include <cstdint>
#include <functional>
#include <memory>

/**
 * @brief Maximum size for device name.
 */
static const size_t DEVICE_NAME_LENGTH = 16;

/**
 * @brief Device information containing device name and version.
 */
struct DeviceInfo {
    /** Device name */
    char name[DEVICE_NAME_LENGTH];

    /** Device version as defined above */
    AdsVersion version;
};

struct AdsDeviceState {
    ADSSTATE ads;
    ADSSTATE device;
};

template<class T>
struct ResourceDeleter {
    ResourceDeleter(const std::function<long(T)> func)
        : FreeResource(func)
    {}

    void operator()(T* resource) noexcept
    {
        FreeResource(*resource);
        delete resource;
    }
private:
    const std::function<long(T)> FreeResource;
};
template<typename T>
using AdsResource = std::unique_ptr<T, ResourceDeleter<T> >;

using AdsHandle = AdsResource<uint32_t>;

struct AdsDevice {
    AdsDevice(const std::string& ipV4, AmsNetId netId, uint16_t port);

    DeviceInfo GetDeviceInfo() const;

    /** Get handle to access AdsVariable by indexGroup/Offset */
    AdsHandle GetHandle(uint32_t offset) const;

    /** Get handle for access by symbol name */
    AdsHandle GetHandle(const std::string& symbolName) const;

    /** Get notification handle */
    AdsHandle GetHandle(uint32_t                     indexGroup,
                        uint32_t                     indexOffset,
                        const AdsNotificationAttrib& notificationAttributes,
                        PAdsNotificationFuncEx       callback,
                        uint32_t                     hUser) const;

    /** Get handle to access files */
    AdsHandle OpenFile(const std::string& filename, uint32_t flags) const;

    long GetLocalPort() const;

    AdsDeviceState GetState() const;
    void SetState(const ADSSTATE AdsState, const ADSSTATE DeviceState) const;

    uint32_t GetTimeout() const;
    void SetTimeout(const uint32_t timeout) const;

    long ReadReqEx2(uint32_t group, uint32_t offset, uint32_t length, void* buffer, uint32_t* bytesRead) const;
    long ReadWriteReqEx2(uint32_t    indexGroup,
                         uint32_t    indexOffset,
                         uint32_t    readLength,
                         void*       readData,
                         uint32_t    writeLength,
                         const void* writeData,
                         uint32_t*   bytesRead) const;
    long WriteReqEx(uint32_t group, uint32_t offset, uint32_t length, const void* buffer) const;

    AdsResource<const AmsNetId> m_NetId;
    const AmsAddr m_Addr;
private:
    AdsResource<const long> m_LocalPort;
    long CloseFile(uint32_t handle) const;
    long DeleteNotificationHandle(uint32_t handle) const;
    long DeleteSymbolHandle(uint32_t handle) const;
};
