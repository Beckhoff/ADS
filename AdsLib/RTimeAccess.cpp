// SPDX-License-Identifier: MIT
/**
   Copyright (c) 2021 - 2022 Beckhoff Automation GmbH & Co. KG
 */

#include "RTimeAccess.h"
#include "Log.h"
#include <iostream>
#include <limits>

static std::ostream& operator<<(std::ostream& os, const bhf::ads::RTimeCpuLatency& info)
{
    return os << std::dec << info.current << " " << info.maximum << " " << info.limit;
}

std::ostream& operator<<(std::ostream& os, const bhf::ads::RTimeCpuSettings& info)
{
    return os << std::hex <<
           "nWinCPUs: " << info.nWinCPUs << '\n' <<
           "nNonWinCPUs: " << info.nNonWinCPUs << '\n' <<
           "dwAffinityMask: " << info.affinityMask << '\n' <<
           "nRtCpus: " << info.nRtCpus << '\n' <<
           "nCpuType: " << info.nCpuType << '\n' <<
           "nCpuFamily: " << info.nCpuFamily << '\n' <<
           "nCpuFreq: " << std::dec << info.nCpuFreq << '\n'
    ;
}

namespace bhf
{
namespace ads
{
RTimeAccess::RTimeAccess(const std::string& gw, const AmsNetId netid, const uint16_t port)
    : device(gw, netid, port ? port : 200)
{}

long RTimeAccess::ShowLatency(const uint32_t indexOffset, const uint32_t cpuId) const
{
    struct RTimeCpuLatency info;
    uint32_t bytesRead;

    const auto status = device.ReadWriteReqEx2(
        ADSSRVID_READDEVICEINFO,
        indexOffset,
        sizeof(info), &info,
        sizeof(cpuId), &cpuId,
        &bytesRead
        );

    if (ADSERR_NOERR != status) {
        LOG_ERROR(__FUNCTION__ << "(" << std::dec << cpuId << "): failed with: 0x" << std::hex << status << '\n');
        return status;
    }
    std::cout << std::dec << cpuId << ": " << info << '\n';
    return 0;
}

long RTimeAccess::ShowLatency(const uint32_t indexOffset) const
{
    struct RTimeCpuSettings info;
    uint32_t bytesRead;

    const auto status = device.ReadReqEx2(ADSSRVID_READDEVICEINFO, RTIME_CPU_SETTINGS, sizeof(info), &info, &bytesRead);
    if (ADSERR_NOERR != status) {
        LOG_ERROR(__FUNCTION__ << "(): failed with: 0x" << std::hex << status << '\n');
        return status;
    }
    std::cout << "RTimeCpuSettings:\n" << info;
    for (uint8_t cpuId = 0; cpuId < 8 * sizeof(info.affinityMask); ++cpuId) {
        static_assert((8 * sizeof(info.affinityMask) < std::numeric_limits<decltype(cpuId)>::max()),
                      "cpuId is to small for affinityMask");
        if (info.affinityMask & (1ULL << cpuId)) {
            ShowLatency(indexOffset, cpuId);
        }
    }
    return 0;
}
}
}
