// SPDX-License-Identifier: MIT
/**
    Copyright (C) Beckhoff Automation GmbH & Co. KG
    Author: Jan Ole Hüser <j.hueser@beckhoff.com>
 */
#pragma once

#include "AdsLib.h"
#include <string>

namespace bhf
{
namespace adstest
{
int testDeleteRegistryKey(const std::string &ipV4, AmsNetId netId,
			  uint16_t port);
}
}
