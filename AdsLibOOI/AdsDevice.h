#pragma once

#include "AdsRoute.h"

struct AdsDevice {
    AdsDevice(const AdsRoute& route);

    const AdsRoute& m_Route;
    const DeviceInfo m_Info;

private:
    static DeviceInfo __ReadDeviceInfo(const AdsRoute& route);
};
