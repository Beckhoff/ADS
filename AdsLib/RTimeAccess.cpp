// SPDX-License-Identifier: MIT
/**
   Copyright (c) 2021 - 2022 Beckhoff Automation GmbH & Co. KG
 */

#include "RTimeAccess.h"
#include "AdsLib.h"
#include "Log.h"
#include <iostream>
#include <limits>

static std::ostream &operator<<(std::ostream &os,
				const bhf::ads::RTimeCpuLatency &info)
{
	return os << std::dec << info.current << " " << info.maximum << " "
		  << info.limit;
}

std::ostream &operator<<(std::ostream &os,
			 const bhf::ads::RTimeCpuSettings &info)
{
	return os << std::dec << "nWinCPUs: " << info.nWinCPUs << '\n'
		  << "nNonWinCPUs: " << info.nNonWinCPUs << '\n'
		  << "dwAffinityMask: 0x" << std::hex << info.affinityMask
		  << '\n'
		  << "nRtCpus: " << std::dec << info.nRtCpus << '\n'
		  << "nCpuType: 0x" << std::hex << info.nCpuType << '\n'
		  << "nCpuFamily: 0x" << info.nCpuFamily << '\n'
		  << "nCpuFreq: " << std::dec << info.nCpuFreq << '\n';
}

namespace bhf
{
namespace ads
{
RTimeAccess::RTimeAccess(const std::string &gw, const AmsNetId netid,
			 const uint16_t port)
	: device(gw, netid, port ? port : 200)
{
}

RTimeCpuSettings RTimeAccess::ReadCpuSettings() const
{
	struct RTimeCpuSettings settings;
	uint32_t bytesRead;

	const auto status =
		device.ReadReqEx2(ADSSRVID_READDEVICEINFO, RTIME_CPU_SETTINGS,
				  sizeof(settings), &settings, &bytesRead);
	if (ADSERR_NOERR != status) {
		LOG_ERROR(__FUNCTION__ << "(): failed with: 0x" << std::hex
				       << status << '\n');
		throw AdsException(status);
	}
	return settings;
}

long RTimeAccess::SetSharedCores(const uint32_t sharedCores) const
{
	const auto oldSettings = ReadCpuSettings();

	if (sharedCores == std::numeric_limits<uint32_t>::max()) {
		// 0xffffffff means RESET -> no isolated cores, all shared.
		if (0 == oldSettings.nNonWinCPUs) {
			std::cout
				<< "Requested shared core configuration already active, no change applied.\n";
			return ADSERR_DEVICE_EXISTS;
		}
	} else {
		// All other values mean limit the number of shared cores -> use remaining cores as isolated.
		if (sharedCores == oldSettings.nWinCPUs) {
			std::cout
				<< "Requested shared core configuration already active, no change applied.\n";
			return ADSERR_DEVICE_EXISTS;
		}
	}
	// We have to apply changes to the current core configuration. For this we
	// have to talk to a different AdsDevice, the system service. Now, it is handy
	// that ADS doesn't really implement "connections" so we can just use the AMS
	// port of our RTimeAccess object, but adust the target AmsPort.
	const AmsAddr systemService{ device.m_Addr.netId, 10000 };
	return AdsSyncWriteReqEx(device.GetLocalPort(), &systemService,
				 SYSTEMSERVICE_SETNUMPROC, 0,
				 sizeof(sharedCores), &sharedCores);
}

long RTimeAccess::ShowLatency(const uint32_t indexOffset,
			      const uint32_t cpuId) const
{
	struct RTimeCpuLatency info;
	uint32_t bytesRead;

	const auto status = device.ReadWriteReqEx2(ADSSRVID_READDEVICEINFO,
						   indexOffset, sizeof(info),
						   &info, sizeof(cpuId), &cpuId,
						   &bytesRead);

	if (ADSERR_NOERR != status) {
		LOG_ERROR(__FUNCTION__ << "(" << std::dec << cpuId
				       << "): failed with: 0x" << std::hex
				       << status << '\n');
		return status;
	}
	std::cout << std::dec << cpuId << ": " << info << '\n';
	return 0;
}

long RTimeAccess::ShowLatency(const uint32_t indexOffset) const
{
	const auto info = ReadCpuSettings();

	std::cout << "RTimeCpuSettings:\n" << info;
	for (uint8_t cpuId = 0; cpuId < 8 * sizeof(info.affinityMask);
	     ++cpuId) {
		static_assert((8 * sizeof(info.affinityMask) <
			       std::numeric_limits<decltype(cpuId)>::max()),
			      "cpuId is to small for affinityMask");
		if (info.affinityMask & (1ULL << cpuId)) {
			ShowLatency(indexOffset, cpuId);
		}
	}
	return 0;
}
}
}
