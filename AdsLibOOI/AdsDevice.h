#pragma once

#include "AdsRoute.h"

struct AdsDeviceState {
    ADSSTATE ads;
    ADSSTATE device;
};

struct AdsDevice {
    AdsDevice(const AdsRoute& route);

    void SetState(const ADSSTATE AdsState, const ADSSTATE DeviceState) const;
    AdsDeviceState GetState() const;

    const AdsRoute m_Route;
    const DeviceInfo m_Info;

private:
    static DeviceInfo __ReadDeviceInfo(const AdsRoute& route);
};
