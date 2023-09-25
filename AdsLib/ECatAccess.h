#pragma once

#include "AdsDevice.h"

namespace bhf
{
namespace ads
{
struct ECatAccess {
    ECatAccess(const std::string& gw, AmsNetId netId, const uint16_t port);
    long ListECatMasters(std::ostream& os) const;

private:
    AdsDevice device;
};
}
}
