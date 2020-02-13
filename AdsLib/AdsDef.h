// SPDX-License-Identifier: MIT
/**
   Copyright (c) 2015 - 2020 Beckhoff Automation GmbH & Co. KG
 */
#pragma once

#if defined(USE_TWINCAT_ROUTER)
#include "TwinCAT/AdsDef.h"
#else
#include "standalone/AdsDef.h"
#endif

#include <iosfwd>
bool operator<(const AmsNetId& lhs, const AmsNetId& rhs);
bool operator<(const AmsAddr& lhs, const AmsAddr& rhs);
std::ostream& operator<<(std::ostream& os, const AmsNetId& netId);
AmsNetId make_AmsNetId(const std::string& addr);
