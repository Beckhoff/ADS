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
#include "..\AdsLib.h"
#include "AdsException.h"
#include "AdsReadResponse.h"
#include "AdsReadArrayResponse.h"

class AdsClient {
public:
    AdsClient()
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
    const DeviceInfo ReadDeviceInfo(const AmsAddr& address) const
    {
        DeviceInfo deviceInfo;
        auto error = AdsSyncReadDeviceInfoReqEx(m_Port, &address, deviceInfo.name, &deviceInfo.version);
        if (error) {throw AdsException(error); }
        return deviceInfo;
    }

    // Read variables by symbolic name
    template<typename T>
    const AdsReadResponse<T> Read(
        const AmsAddr&     address,
        const std::string& symbolName) const
    {
        auto handle = GetHandle(address, symbolName.c_str());
        AdsReadResponse<T> response;
        uint32_t bytesRead = 0;
        auto error = AdsSyncReadReqEx2(
            m_Port,
            &address,
            ADSIGRP_SYM_VALBYHND,
            handle,
            sizeof(T),
            response.GetPointer(),
            &bytesRead);
        ReleaseHandle(address, handle);
        if (error) {throw AdsException(error); }
        response.SetBytesRead(bytesRead);
        return response;
    }

    // Read variables by index group and index offset
    template<typename T>
    const AdsReadResponse<T> Read(
        const AmsAddr& address,
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
        const AmsAddr&     address,
        const std::string& symbolName) const
    {
        auto handle = GetHandle(address, symbolName.c_str());
        AdsReadArrayResponse<T, count> response;
        uint32_t bytesRead = 0;

        auto error = AdsSyncReadReqEx2(
            m_Port,
            &address,
            ADSIGRP_SYM_VALBYHND,
            handle,
            sizeof(T) * count,
            response.GetPointer(),
            &bytesRead);
        ReleaseHandle(address, handle);
        if (error) {throw AdsException(error); }
        response.SetBytesRead(bytesRead);
        return response;
    }

    // Read arrays by index group and index offset
    template<typename T, uint32_t count = 1>
    const AdsReadArrayResponse<T, count> ReadArray(
        const AmsAddr& address,
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
        const AmsAddr&     address,
        const std::string& symbolName,
        const T&           value)
    {
        auto handle = GetHandle(address, symbolName.c_str());
        auto error = AdsSyncWriteReqEx(
            m_Port,
            &address,
            ADSIGRP_SYM_VALBYHND,
            handle,
            sizeof(T) * count,
            &value);
        ReleaseHandle(address, handle);
        if (error) {throw AdsException(error); }
    }

    // Write variable by index group and index offset
    template<typename T, uint32_t count = 1> void Write(
        const AmsAddr& address,
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
        const AmsAddr&     address,
        const std::string& symbolName,
        const T*           pValue)
    {
        auto handle = GetHandle(address, symbolName.c_str());
        auto error = AdsSyncWriteReqEx(
            m_Port,
            &address,
            ADSIGRP_SYM_VALBYHND,
            handle,
            sizeof(T) * count,
            pValue);
        ReleaseHandle(address, handle);
        if (error) {throw AdsException(error); }
    }

    // Write array by index group and index offset
    template<typename T, uint32_t count = 1> void WriteArray(
        const AmsAddr& address,
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
    uint32_t m_Port;

    // Get the handle for a given symbol name
    const uint32_t GetHandle(
        const AmsAddr&    address,
        const char* const symbolName) const
    {
        uint32_t handle = 0;
        uint32_t bytesRead = 0;
        uint32_t error = AdsSyncReadWriteReqEx2(
            m_Port,
            &address,
            ADSIGRP_SYM_HNDBYNAME, 0,
            sizeof(handle), &handle,
            strlen(symbolName),
            (void*)symbolName,
            &bytesRead
            );

        if (error || (sizeof(handle) != bytesRead)) {
            throw AdsException(error);
        }
        return handle;
    }

    // Release a given handle
    void ReleaseHandle(
        const AmsAddr& address,
        const uint32_t     handle) const
    {
        uint32_t error = AdsSyncWriteReqEx(
            m_Port,
            &address,
            ADSIGRP_SYM_RELEASEHND, 0,
            sizeof(handle), &handle
            );

        if (error) {
            throw AdsException(error);
        }
    }
};

#endif
