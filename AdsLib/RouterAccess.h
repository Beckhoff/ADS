// SPDX-License-Identifier: MIT
/**
   Copyright (c) 2021 - 2022 Beckhoff Automation GmbH & Co. KG
 */

#pragma once

#include "AdsDevice.h"

namespace bhf
{
namespace ads
{
struct RouterAccess {
    RouterAccess(const std::string& gw, AmsNetId netid, uint16_t port);
    bool PciScan(uint64_t pci_id, std::ostream& os) const;
private:
    AdsDevice device;
};
}
}
