#include "AdsDevice.h"
#include "AdsException.h"
#include "AdsLib/AdsLib.h"

DeviceInfo AdsDevice::__ReadDeviceInfo(const AdsRoute& route)
{
    DeviceInfo info;
    auto amsAddr = route->GetSymbolsAmsAddr();
    auto error = AdsSyncReadDeviceInfoReqEx(route->GetLocalPort(),
                                            &amsAddr,
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

AdsDeviceState AdsDevice::GetState() const
{
    AdsDeviceState state;
    auto amsAddr = m_Route->GetSymbolsAmsAddr();
    static_assert(sizeof(state.ads) == sizeof(uint16_t), "size missmatch");
    static_assert(sizeof(state.device) == sizeof(uint16_t), "size missmatch");
    auto error = AdsSyncReadStateReqEx(m_Route->GetLocalPort(),
                                       &amsAddr,
                                       (uint16_t*)&state.ads,
                                       (uint16_t*)&state.device);

    if (error) {
        throw AdsException(error);
    }

    return state;
}

void AdsDevice::SetState(const ADSSTATE AdsState, const ADSSTATE DeviceState) const
{
    auto amsAddr = m_Route->GetSymbolsAmsAddr();
    auto error = AdsSyncWriteControlReqEx(m_Route->GetLocalPort(),
                                          &amsAddr,
                                          AdsState,
                                          DeviceState,
                                          0, nullptr); // No additional data

    if (error) {
        throw AdsException(error);
    }
}
