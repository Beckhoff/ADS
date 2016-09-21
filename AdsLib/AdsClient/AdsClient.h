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

struct AdsLocalPort {
    static void PortClose(long* port)
    {
        AdsPortCloseEx(*port);
        delete port;
    }

    AdsLocalPort()
        : port(new long {AdsPortOpenEx()}, &PortClose)
    {
        if (!*port) {
            throw AdsException(ADSERR_CLIENT_PORTNOTOPEN);
        }
    }

    operator long() const
    {
        return *port;
    }

    const uint32_t Timeout() const
    {
        uint32_t timeout = 0;
        auto error = AdsSyncGetTimeoutEx(*port, &timeout);
        if (error) {throw AdsException(error); }
        return timeout;
    }
    void Timeout(const uint32_t timeout) const
    {
        auto error = AdsSyncSetTimeoutEx(*port, timeout);
        if (error) {throw AdsException(error); }
    }
private:
    std::unique_ptr<long, decltype(& PortClose)> port;
};

class AdsRoute {
    static void DelRoute(AmsNetId* netId)
    {
        AdsPortCloseEx(*netId);
        delete netId;
    }

    using RouteGuard = std::unique_ptr<AmsNetId, decltype(& DelRoute)>;

    static RouteGuard MakeRoute(const AmsNetId remoteNetId, const std::string& ip)
    {
        auto error = AdsAddRoute(remoteNetId, ip.c_str());
        if (error) {
            throw AdsException(error);
        }
        return RouteGuard {new AmsNetId {remoteNetId}, &DelRoute};
    }

    std::unique_ptr<AmsNetId, decltype(& DelRoute)> netId;

public:
    AdsRoute(const AmsNetId remoteNetId, const std::string& ip)
        : netId(MakeRoute(remoteNetId, ip))
    {}

    operator AmsNetId() const
    {
        return *netId;
    }
};

struct AdsDevice {
    AdsDevice(const AmsAddr amsAddr)
        : address(amsAddr)
    {}

    const AmsAddr GetLocalAddress() const
    {
        AmsAddr address;
        auto error = AdsGetLocalAddressEx(m_Port, &address);
        if (error) {throw AdsException(error); }
        return address;
    }

    // Device info
    const DeviceInfo Info() const
    {
        DeviceInfo deviceInfo;
        auto error = AdsSyncReadDeviceInfoReqEx(m_Port, &address, deviceInfo.name, &deviceInfo.version);
        if (error) {throw AdsException(error); }
        return deviceInfo;
    }

private:
    const AmsAddr address;
    AdsLocalPort m_Port;
};

#endif
