#include "AdsDevice.h"
#include "AdsException.h"
#include "AdsLib/AdsLib.h"

AdsDevice::AdsDevice(const AdsRoute& route)
    : m_Route(route)
{
    auto amsAddr = m_Route->GetSymbolsAmsAddr();
    auto error = AdsSyncReadDeviceInfoReqEx(m_Route->GetLocalPort(),
                                            &amsAddr,
                                            &m_DeviceInfo.name[0],
                                            &m_DeviceInfo.version);

    if (error) {
        throw AdsException(error);
    }
}

const std::string AdsDevice::GetName() const
{
    return m_DeviceInfo.name;
}

const AdsVersion AdsDevice::GetVersion() const
{
    return m_DeviceInfo.version;
}

const AdsDeviceState AdsDevice::GetState()
{
    auto amsAddr = m_Route->GetSymbolsAmsAddr();
    auto error = AdsSyncReadStateReqEx(m_Route->GetLocalPort(),
                                       &amsAddr,
                                       (uint16_t*)&m_State.AdsState,
                                       (uint16_t*)&m_State.DeviceState);

    if (error) {
        throw AdsException(error);
    }

    return m_State;
}

void AdsDevice::SetState(const ADSSTATE AdsState, const ADSSTATE DeviceState)
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

    m_State.AdsState = AdsState;
    m_State.DeviceState = DeviceState;
}
