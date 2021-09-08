// SPDX-License-Identifier: MIT
/**
   Copyright (c) 2021 Beckhoff Automation GmbH & Co. KG
 */

#pragma once

#include "AdsDevice.h"

namespace bhf
{
namespace ads
{
struct LicenseAccess {
    LicenseAccess(const std::string& gw, AmsNetId netid, uint16_t port);
    bool ShowPlatformId(std::ostream& os) const;
    bool ShowSystemId(std::ostream& os) const;
    bool ShowVolumeNo(std::ostream& os) const;
private:
    AdsDevice device;
};
}
}
