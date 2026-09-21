#pragma once

#include "AdsDevice.h"
#include <map>
#include <string>
#include <vector>

namespace bhf
{
namespace ads
{
struct ECatSlaveStatus {
	uint16_t stationAddress;
	uint16_t alStatus;
};

struct ECatAccess {
	ECatAccess(const std::string &gw, AmsNetId netId, const uint16_t port);
	long ListECatMasters(std::ostream &os) const;
	std::map<AmsNetId, std::vector<ECatSlaveStatus> >
	GetECatSlaveStatus() const;

	private:
	AdsDevice device;
	const std::string gateway;
	std::vector<AmsNetId> GetECatMasterNetIds() const;
	uint16_t ReadECatSlaveStationAddress(const AmsNetId &ecatMaster,
					    uint16_t slaveIndex) const;
	uint16_t ReadECatSlaveAlStatus(const AmsNetId &ecatMaster,
				      uint16_t slaveIndex) const;
	uint32_t CountECatSlaves(const AmsNetId &ecatMaster) const;
};
}
}
