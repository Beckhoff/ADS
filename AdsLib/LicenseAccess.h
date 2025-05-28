// SPDX-License-Identifier: MIT
/**
   Copyright (c) Beckhoff Automation GmbH & Co. KG
 */

#pragma once

#include "AdsDevice.h"

namespace bhf
{
namespace ads
{
struct LicenseAccess {
	LicenseAccess(const std::string &gw, AmsNetId netid, uint16_t port);
	int ShowOnlineInfo(std::ostream &os) const;
	int ShowPlatformId(std::ostream &os) const;
	int ShowSystemId(std::ostream &os) const;
	int ShowVolumeNo(std::ostream &os) const;

    private:
	AdsDevice device;
};
}
}
