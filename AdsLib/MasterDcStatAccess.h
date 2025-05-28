#pragma once

#include "AdsDevice.h"

namespace bhf
{
namespace ads
{
enum ECADS_IOFFS_MASTER_DC_STAT : uint32_t {
	CLEAR = 0,
	ACTIVATE = 1,
	DEACTIVATE = 2
};

struct MasterDcStatAccess {
	MasterDcStatAccess(const std::string &gw, AmsNetId netId,
			   const uint16_t port);
	long Activate() const;
	long Deactivate() const;
	long Clear() const;
	long Print(std::ostream &os) const;

    private:
	AdsDevice device;
	long RunCommand(ECADS_IOFFS_MASTER_DC_STAT command) const;
};
}
}
