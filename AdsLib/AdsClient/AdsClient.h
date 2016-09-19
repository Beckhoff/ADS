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
#include "AdsVariable.h"
#include "AdsException.h"

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
