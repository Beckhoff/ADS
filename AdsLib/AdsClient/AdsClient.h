/**
   Objectoriented-Interface for AdsLib.

   Example usage:
   try
   {
   AdsClient client;
   auto readUIntResponse = client.Read<uint32_t>(...);
   auto readUIntArrayResponse = client.Read<uint32_t, 10>(...);
   client.Write<uint32_t>(...);
   client.WriteArray<uint32_t, 10>(...);
   }
   catch (AdsException& ex){};
 */

#pragma once
#if (__cplusplus > 199711L || _MSC_VER >= 1700) // Check for c++11 support
#include <memory>
#include <cstring>
#include <utility>
#include "..\AdsLib.h"
#include "AdsException.h"
#include "AdsReadResponse.h"
#include "AdsReadArrayResponse.h"

class AdsClient {
public:
    AdsClient(const AmsAddr amsAddr)
        : address(amsAddr)
    {
        m_Port = AdsPortOpenEx();
        if (0 == m_Port) {
            throw AdsException(ADSERR_CLIENT_PORTNOTOPEN);
        }
    }

    ~AdsClient()
    {
        AdsPortCloseEx(m_Port);
    }

    // Routing
    static void AddRoute(const AmsNetId amsNetId, const std::string& ip)
    {
        auto error = AdsAddRoute(amsNetId, ip.c_str());
        if (error) {throw AdsException(error); }
    }

    static void DeleteRoute(const AmsNetId amsNetId)
    {
        AdsDelRoute(amsNetId);
    }

    const AmsAddr GetLocalAddress() const
    {
        AmsAddr address;
        auto error = AdsGetLocalAddressEx(m_Port, &address);
        if (error) {throw AdsException(error); }
        return address;
    }

    // Timeout
    const uint32_t GetTimeout() const
    {
        uint32_t timeout = 0;
        uint32_t error = AdsSyncGetTimeoutEx(m_Port, &timeout);
        if (error) {throw AdsException(error); }
        return timeout;
    }
    void SetTimeout(const uint32_t timeout) const
    {
        uint32_t error = AdsSyncSetTimeoutEx(m_Port, timeout);
        if (error) {throw AdsException(error); }
    }

    // Device info
    const DeviceInfo ReadDeviceInfo() const
    {
        DeviceInfo deviceInfo;
        auto error = AdsSyncReadDeviceInfoReqEx(m_Port, &address, deviceInfo.name, &deviceInfo.version);
        if (error) {throw AdsException(error); }
        return deviceInfo;
    }

    // Read variables by symbolic name
    template<typename T>
    const AdsReadResponse<T> Read(
        const std::string& symbolName) const
    {
        auto handle = GetHandle(symbolName);
        AdsReadResponse<T> response;
        uint32_t bytesRead = 0;
        auto error = AdsSyncReadReqEx2(
            m_Port,
            &address,
            ADSIGRP_SYM_VALBYHND,
            *handle,
            sizeof(T),
            response.GetPointer(),
            &bytesRead);
        if (error) {throw AdsException(error); }
        response.SetBytesRead(bytesRead);
        return response;
    }

    // Read variables by index group and index offset
    template<typename T>
    const AdsReadResponse<T> Read(
        const uint32_t indexGroup,
        const uint32_t indexOffset) const
    {
        AdsReadResponse<T> response;
        uint32_t bytesRead = 0;

        auto error = AdsSyncReadReqEx2(
            m_Port,
            &address,
            indexGroup,
            indexOffset,
            sizeof(T),
            response.GetPointer(),
            &bytesRead);
        if (error) {throw AdsException(error); }
        response.SetBytesRead(bytesRead);
        return response;
    }

    // Read arrays by symbolic name
    template<typename T, uint32_t count = 1>
    const AdsReadArrayResponse<T, count> ReadArray(
        const std::string& symbolName) const
    {
        auto handle = GetHandle(symbolName);
        AdsReadArrayResponse<T, count> response;
        uint32_t bytesRead = 0;

        auto error = AdsSyncReadReqEx2(
            m_Port,
            &address,
            ADSIGRP_SYM_VALBYHND,
            *handle,
            sizeof(T) * count,
            response.GetPointer(),
            &bytesRead);
        if (error) {throw AdsException(error); }
        response.SetBytesRead(bytesRead);
        return response;
    }

    // Read arrays by index group and index offset
    template<typename T, uint32_t count = 1>
    const AdsReadArrayResponse<T, count> ReadArray(
        const uint32_t indexGroup,
        const uint32_t indexOffset) const
    {
        AdsReadArrayResponse<T, count> response;
        uint32_t bytesRead = 0;

        auto error = AdsSyncReadReqEx2(
            m_Port,
            &address,
            indexGroup,
            indexOffset,
            sizeof(T) * count,
            response.GetPointer(),
            &bytesRead);
        if (error) {throw AdsException(error); }
        response.SetBytesRead(bytesRead);
        return response;
    }

    // Write variable by symbolic name
    template<typename T, uint32_t count = 1> void Write(
        const std::string& symbolName,
        const T&           value)
    {
        auto handle = GetHandle(symbolName);
        auto error = AdsSyncWriteReqEx(
            m_Port,
            &address,
            ADSIGRP_SYM_VALBYHND,
            *handle,
            sizeof(T) * count,
            &value);
        if (error) {throw AdsException(error); }
    }

    // Write variable by index group and index offset
    template<typename T, uint32_t count = 1> void Write(
        const uint32_t indexGroup,
        const uint32_t indexOffset,
        const T&       value)
    {
        auto error = AdsSyncWriteReqEx(
            m_Port,
            &address,
            indexGroup,
            indexOffset,
            sizeof(T) * count,
            &value
            );
        if (error) {throw AdsException(error); }
    }

    // Write array by symbolic name
    template<typename T, uint32_t count = 1> void WriteArray(
        const std::string& symbolName,
        const T*           pValue)
    {
        auto handle = GetHandle(symbolName);
        auto error = AdsSyncWriteReqEx(
            m_Port,
            &address,
            ADSIGRP_SYM_VALBYHND,
            *handle,
            sizeof(T) * count,
            pValue);
        if (error) {throw AdsException(error); }
    }

    // Write array by index group and index offset
    template<typename T, uint32_t count = 1> void WriteArray(
        const uint32_t indexGroup,
        const uint32_t indexOffset,
        const T*       pValue)
    {
        auto error = AdsSyncWriteReqEx(
            m_Port,
            &address,
            indexGroup,
            indexOffset,
            sizeof(T) * count,
            pValue
            );
        if (error) {throw AdsException(error); }
    }

private:
    const AmsAddr address;
    uint32_t m_Port;

    // Release a given handle
    void ReleaseHandle(uint32_t* handle) const
    {
        uint32_t error = AdsSyncWriteReqEx(
            m_Port,
            &address,
            ADSIGRP_SYM_RELEASEHND, 0,
            sizeof(*handle), handle
            );

        if (error) {
            throw AdsException(error);
        }
    }

    using AdsHandle =
              std::unique_ptr<uint32_t,
                              decltype(std::bind(& AdsClient::ReleaseHandle, std::declval<const AdsClient*>(),
                                                 std::placeholders::_1))>;

    // Get the handle for a given symbol name
    AdsHandle GetHandle(
        const std::string& symbolName) const
    {
        uint32_t handle = 0;
        uint32_t bytesRead = 0;
        uint32_t error = AdsSyncReadWriteReqEx2(
            m_Port,
            &address,
            ADSIGRP_SYM_HNDBYNAME, 0,
            sizeof(handle), &handle,
            symbolName.size(),
            symbolName.c_str(),
            &bytesRead
            );

        if (error || (sizeof(handle) != bytesRead)) {
            throw AdsException(error);
        }
        return AdsHandle {new uint32_t {handle}, std::bind(&AdsClient::ReleaseHandle, this, std::placeholders::_1)};
    }
};

#endif
