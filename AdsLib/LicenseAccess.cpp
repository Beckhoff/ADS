// SPDX-License-Identifier: MIT
/**
   Copyright (c) 2021 - 2022 Beckhoff Automation GmbH & Co. KG
 */

#include "LicenseAccess.h"
#include "Log.h"
#include "wrap_endian.h"
#include <iostream>
#include <iomanip>
#include <vector>

namespace bhf
{
namespace ads
{
LicenseAccess::LicenseAccess(const std::string& gw, const AmsNetId netid, const uint16_t port)
    : device(gw, netid, port ? port : 30)
{}

int LicenseAccess::ShowPlatformId(std::ostream& os) const
{
    uint16_t platformId;
    uint32_t bytesRead = 0;
    const auto status = device.ReadReqEx2(0x01010004,
                                          0x2,
                                          sizeof(platformId),
                                          &platformId,
                                          &bytesRead);
    if (ADSERR_NOERR != status) {
        LOG_ERROR(__FUNCTION__ << "(): failed with: 0x" << std::hex << status << '\n');
        return 1;
    }
    os << bhf::ads::letoh(platformId) << '\n';
    return !os.good();
}

int LicenseAccess::ShowSystemId(std::ostream& os) const
{
    std::vector<uint8_t> readBuffer(16);
    uint32_t bytesRead = 0;
    const auto status = device.ReadReqEx2(0x01010004,
                                          0x1,
                                          readBuffer.size(),
                                          readBuffer.data(),
                                          &bytesRead);
    if (ADSERR_NOERR != status) {
        LOG_ERROR(__FUNCTION__ << "(): failed with: 0x" << std::hex << status << '\n');
        return 1;
    }
    char buf[38];
    snprintf(buf, sizeof(buf), "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X\n"
             , readBuffer[3], readBuffer[2], readBuffer[1], readBuffer[0]
             , readBuffer[5], readBuffer[4]
             , readBuffer[7], readBuffer[6]
             , readBuffer[8], readBuffer[9]
             , readBuffer[10], readBuffer[11], readBuffer[12], readBuffer[13], readBuffer[14], readBuffer[15]
             );
    os << buf;
    return !os.good();
}

int LicenseAccess::ShowVolumeNo(std::ostream& os) const
{
    uint32_t volumeNo;
    uint32_t bytesRead = 0;
    const auto status = device.ReadReqEx2(0x01010004,
                                          0x5,
                                          sizeof(volumeNo),
                                          &volumeNo,
                                          &bytesRead);
    if (ADSERR_NOERR != status) {
        LOG_ERROR(__FUNCTION__ << "(): failed with: 0x" << std::hex << status << '\n');
        return 1;
    }
    os << bhf::ads::letoh(volumeNo) << '\n';
    return !os.good();
}
}
}
