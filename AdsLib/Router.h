// SPDX-License-Identifier: MIT
/**
   Copyright (c) 2015 - 2022 Beckhoff Automation GmbH & Co. KG
 */

#pragma once

#include "AdsDef.h"

struct Router {
    static const size_t NUM_PORTS_MAX = 128;
    static const uint16_t PORT_BASE = 30000;
    static_assert(NUM_PORTS_MAX + PORT_BASE <= UINT16_MAX, "Port limit is out of range");
    virtual ~Router() {}

    virtual long GetLocalAddress(uint16_t port, AmsAddr* pAddr) = 0;
};
