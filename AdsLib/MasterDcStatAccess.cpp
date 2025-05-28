#include "MasterDcStatAccess.h"
#include "AdsLib.h"
#include "Log.h"
#include <array>
#include <iostream>
#include <string>
#include <map>

namespace bhf
{
namespace ads
{
#define ECADS_IGRP_MASTER_DC_STAT 0x2f

MasterDcStatAccess::MasterDcStatAccess(const std::string &gw, AmsNetId netId,
				       const uint16_t port)
	: device(gw, netId, port ? port : uint16_t(0xffff))
{
}

long MasterDcStatAccess::Activate() const
{
	return RunCommand(ECADS_IOFFS_MASTER_DC_STAT::ACTIVATE);
}

long MasterDcStatAccess::Deactivate() const
{
	return RunCommand(ECADS_IOFFS_MASTER_DC_STAT::DEACTIVATE);
}

long MasterDcStatAccess::Clear() const
{
	return RunCommand(ECADS_IOFFS_MASTER_DC_STAT::CLEAR);
}

long MasterDcStatAccess::Print(std::ostream &os) const
{
	uint32_t bytesRead;

	struct {
		uint32_t count;
		uint32_t pos;
		uint32_t neg;
		uint32_t maxDiff;
		uint32_t posArr[10];
		uint32_t negArr[10];
	} stat;

	const auto status = device.ReadReqEx2(ECADS_IGRP_MASTER_DC_STAT, 0,
					      sizeof(stat), &stat, &bytesRead);

	if (status == ADSERR_DEVICE_INVALIDSTATE) {
		LOG_ERROR(
			"Reading DC diagnosis failed with 0x"
			<< std::hex << status
			<< ". Try activating it first ('adstool dc-diag activate').");
		return status;
	} else if (status != ADSERR_NOERR) {
		LOG_ERROR("Reading DC diagnosis failed with 0x" << std::hex
								<< status);
		return status;
	} else if (bytesRead != sizeof(stat)) {
		LOG_ERROR(__FUNCTION__
			  << "Corrupt ECAT_DC_TIMING_STAT length.\n");
		throw AdsException(ADSERR_DEVICE_INVALIDDATA);
	}

	const std::array<const std::string,
			 sizeof(stat.posArr) / sizeof(stat.posArr[0])>
		rowNames{ {
			"1", "2", "5", "10", "20", "50", "100", "200", "500",
			"\u221E" // infinity symbol
		} };

	os << "Deviation <" << " | " << "Count (neg)" << " | " << "Count (pos)"
	   << '\n';
	for (size_t i = 0; i < rowNames.size(); i++) {
		os << rowNames[i] << " | " << stat.negArr[i] << " | "
		   << stat.posArr[i] << '\n';
	}
	os << "Sum" << " | " << stat.neg << " | " << stat.pos << '\n';

	return status;
}

long MasterDcStatAccess::RunCommand(ECADS_IOFFS_MASTER_DC_STAT command) const
{
	const std::map<ECADS_IOFFS_MASTER_DC_STAT, std::string> dcDiagNames = {
		{ ECADS_IOFFS_MASTER_DC_STAT::ACTIVATE, "Activating" },
		{ ECADS_IOFFS_MASTER_DC_STAT::CLEAR, "Clearing" },
		{ ECADS_IOFFS_MASTER_DC_STAT::DEACTIVATE, "Deactivating" },
	};

	const auto status = device.WriteReqEx(ECADS_IGRP_MASTER_DC_STAT,
					      command, 0, nullptr);

	if (status != ADSERR_NOERR) {
		LOG_ERROR(dcDiagNames.at(command)
			  << " DC diagnosis failed with 0x" << std::hex
			  << status);
		return status;
	}

	return status;
}
}
}
