// SPDX-License-Identifier: MIT
/**
   Copyright (c) 2020 Beckhoff Automation GmbH & Co. KG
 */
#pragma once

#include "standalone/AdsDef.h"

#include <iosfwd>
bool operator<(const AmsNetId& lhs, const AmsNetId& rhs);
bool operator<(const AmsAddr& lhs, const AmsAddr& rhs);
std::ostream& operator<<(std::ostream& os, const AmsNetId& netId);
AmsNetId make_AmsNetId(const std::string& addr);
