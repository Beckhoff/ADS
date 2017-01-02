#include "AdsDevice.h"
#include "AdsException.h"
#include "AdsLib/AdsLib.h"

DeviceInfo AdsDevice::__ReadDeviceInfo(const AdsRoute& route)
{
    DeviceInfo info;
    auto error = AdsSyncReadDeviceInfoReqEx(route.GetLocalPort(),
                                            &route.m_Addr,
                                            &info.name[0],
                                            &info.version);

    if (error) {
        throw AdsException(error);
    }
    return info;
}

AdsDevice::AdsDevice(const AdsRoute& route)
    : m_Route(route),
    m_Info(__ReadDeviceInfo(route))
{}
