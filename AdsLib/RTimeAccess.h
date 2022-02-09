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
#define RTIME_CPU_SETTINGS 0xd
#define RTIME_READ_LATENCY 0x2
#define RTIME_RESET_LATENCY 0xb

struct RTimeCpuSettings {
    uint32_t nWinCPUs;
    uint32_t nNonWinCPUs;
    uint64_t affinityMask;
    uint32_t nRtCpus;
    uint32_t nCpuType;
    uint32_t nCpuFamily;
    uint32_t nCpuFreq;
};

struct RTimeCpuLatency {
    uint32_t current;
    uint32_t maximum;
    uint32_t limit;
};

struct RTimeAccess {
    RTimeAccess(const std::string& gw, AmsNetId netid, uint16_t port);
    long ShowLatency(uint32_t indexOffset) const;
    long ShowLatency(uint32_t indexOffset, uint32_t cpuId) const;
private:
    AdsDevice device;
};
}
}

std::ostream& operator<<(std::ostream& os, const bhf::ads::RTimeCpuSettings& info);
