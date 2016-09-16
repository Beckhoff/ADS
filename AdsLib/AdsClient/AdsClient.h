/**
   Objectoriented-Interface for AdsLib.

   Example usage:
   try
   {
     AdsClient client;
     auto adsVar = client.GetAdsVariable<uint32_t>(...);
     adsVar = 1; // Writes a variable
     uint32_t currentValue = adsVar; // Reads the value of the variable
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

class AdsHandle {
    static void ReleaseHandleDummy(const AmsAddr address, long port, uint32_t* handle) {}
    static void ReleaseHandle(const AmsAddr address, long port, uint32_t* handle)
    {
        uint32_t error = AdsSyncWriteReqEx(
            port,
            &address,
            ADSIGRP_SYM_RELEASEHND, 0,
            sizeof(*handle), handle
            );

        if (error) {
            throw AdsException(error);
        }
    }

    using AdsHandleGuard =
              std::unique_ptr<uint32_t,
                              decltype(std::bind(& ReleaseHandle, std::declval<const AmsAddr>(),
                                                 std::declval<long>(), std::placeholders::_1))>;

    static AdsHandleGuard GetHandle(const AmsAddr address, long port,
                                    const std::string& symbolName)
    {
        uint32_t handle = 0;
        uint32_t bytesRead = 0;
        uint32_t error = AdsSyncReadWriteReqEx2(
            port,
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
        return AdsHandleGuard {new uint32_t {handle}, std::bind(&ReleaseHandle, address, port, std::placeholders::_1)};
    }

    AdsHandleGuard m_Handle;
public:
    AdsHandle(uint32_t offset)
        : m_Handle{new uint32_t {offset}, std::bind(&ReleaseHandleDummy, AmsAddr {}, 0L, std::placeholders::_1)}
    {}

    AdsHandle(const AmsAddr address, long port,
              const std::string& symbolName)
        : m_Handle{GetHandle(address, port, symbolName)}
    {}

    operator uint32_t() const
    {
        return *m_Handle;
    }
};

template<typename T>
struct AdsVariable {
    AdsVariable(const AmsAddr address, const std::string& symbolName, const long localPort)
        : m_RemoteAddr(address),
        m_LocalPort(localPort),
        m_IndexGroup(ADSIGRP_SYM_VALBYHND),
        m_Handle(address, localPort, symbolName)
    {}

    AdsVariable(const AmsAddr address, const uint32_t group, const uint32_t offset, const long localPort)
        : m_RemoteAddr(address),
        m_LocalPort(localPort),
        m_IndexGroup(group),
        m_Handle(offset)
    {}

    void Read(const size_t size, void* data) const
    {
        uint32_t bytesRead = 0;
        auto error = AdsSyncReadReqEx2(m_LocalPort,
                                       &m_RemoteAddr,
                                       m_IndexGroup,
                                       m_Handle,
                                       size,
                                       data,
                                       &bytesRead);

        if (error || (size != bytesRead)) {
            throw AdsException(error);
        }
    }

    void Write(const size_t size, const void* data) const
    {
        auto error = AdsSyncWriteReqEx(m_LocalPort,
                                       &m_RemoteAddr,
                                       m_IndexGroup,
                                       m_Handle,
                                       size,
                                       data);

        if (error) {
            throw AdsException(error);
        }
    }

    operator T() const
    {
        T buffer;
        Read(sizeof(buffer), &buffer);
        return buffer;
    }

    void operator=(const T& value) const
    {
        Write(sizeof(T), &value);
    }

    template<typename U, size_t N>
    operator std::array<U, N>() const
    {
        std::array<U, N> buffer;
        Read(sizeof(U) * N, buffer.data());
        return buffer;
    }

    template<typename U, size_t N>
    void operator=(const std::array<U, N>& value) const
    {
        Write(sizeof(U) * N, value.data());
    }
private:
    const AmsAddr m_RemoteAddr;
    const long m_LocalPort;
    const uint32_t m_IndexGroup;
    AdsHandle m_Handle;
};

struct AdsClient {
    AdsClient(const AmsAddr amsAddr, const std::string& ip)
        : address(amsAddr)
    {
        auto error = AdsAddRoute(amsAddr.netId, ip.c_str());
        if (error) {
            throw AdsException(error);
        }

        m_Port = AdsPortOpenEx();
        if (0 == m_Port) {
            throw AdsException(ADSERR_CLIENT_PORTNOTOPEN);
        }
    }
    AdsClient(const AdsClient&) = delete;
    AdsClient(AdsClient&&) = delete;
    AdsClient& operator=(const AdsClient&)& = delete;
    AdsClient& operator=(AdsClient&&)& = delete;

    ~AdsClient()
    {
        AdsPortCloseEx(m_Port);
        AdsDelRoute(address.netId);
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

    template<typename T>
    AdsVariable<T> GetAdsVariable(const std::string& symbolName)
    {
        return AdsVariable<T>(address, symbolName, m_Port);
    }

    template<typename T>
    AdsVariable<T> GetAdsVariable(const uint32_t group, const uint32_t offset)
    {
        return AdsVariable<T>(address, group, offset, m_Port);
    }

    // Device info
    const DeviceInfo ReadDeviceInfo() const
    {
        DeviceInfo deviceInfo;
        auto error = AdsSyncReadDeviceInfoReqEx(m_Port, &address, deviceInfo.name, &deviceInfo.version);
        if (error) {throw AdsException(error); }
        return deviceInfo;
    }

private:
    const AmsAddr address;
    uint32_t m_Port;
};

#endif
