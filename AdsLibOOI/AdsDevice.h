#pragma once

#include "AdsRoute.h"
#include <string>

struct AdsDeviceState {
    ADSSTATE AdsState;
    ADSSTATE DeviceState;
};

struct AdsDevice {
    AdsDevice(const AdsRoute& route);

    const std::string GetName() const;
    const AdsVersion GetVersion() const;
    void SetState(const ADSSTATE AdsState, const ADSSTATE DeviceState);
    const AdsDeviceState GetState();

private:
    AdsRoute m_Route;
    DeviceInfo m_DeviceInfo;
    AdsDeviceState m_State;
};
