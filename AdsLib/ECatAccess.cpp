#include "ECatAccess.h"
#include "Log.h"
#include <iostream>
#include <vector>

namespace bhf
{
namespace ads
{
#define IOADS_IGR_IODEVICESTATE_BASE    0x5000
#define IOADS_IOF_READDEVIDS            0x1
#define IOADS_IOF_READDEVNAME           0x1
#define IOADS_IOF_READDEVCOUNT          0x2
#define IOADS_IOF_READDEVNETID          0x5
#define IOADS_IOF_READDEVTYPE           0x7

ECatAccess::ECatAccess(const std::string& gw, const AmsNetId netid, const uint16_t port)
    : device(gw, netid, port ? port : uint16_t(AMSPORT_R0_IO))
{}

long ECatAccess::ListECatMasters(std::ostream& os) const
{
    uint32_t numberOfDevices;
    uint32_t bytesRead;

    auto status = device.ReadReqEx2(
        IOADS_IGR_IODEVICESTATE_BASE,
        IOADS_IOF_READDEVCOUNT,
        sizeof(numberOfDevices), &numberOfDevices,
        &bytesRead);

    if (status != ADSERR_NOERR) {
        LOG_ERROR("Reading device count failed with 0x" << std::hex << status);
        return status;
    }

    if (numberOfDevices == 0) {
        return status;
    }

    // the first element of the vector is set to devCount,
    // so the actual device Ids start at index 1
    std::vector<uint16_t> deviceIds(numberOfDevices + 1);

    status = device.ReadReqEx2(
        IOADS_IGR_IODEVICESTATE_BASE,
        IOADS_IOF_READDEVIDS,
        deviceIds.capacity() * sizeof(uint16_t),
        deviceIds.data(), &bytesRead);

    if (status != ADSERR_NOERR) {
        LOG_ERROR("Reading device ids failed with 0x" << std::hex << status);
        return status;
    }

    // Skip the device count, which is at the first index
    for (uint32_t i = 1; i <= numberOfDevices; i++) {
        uint16_t devType;
        status = device.ReadReqEx2(
            IOADS_IGR_IODEVICESTATE_BASE + deviceIds[i],
            IOADS_IOF_READDEVTYPE,
            sizeof(devType),
            &devType,
            &bytesRead);

        if (status != ADSERR_NOERR) {
            LOG_ERROR("Reading type for device[" << deviceIds[i] << "] failed with 0x" << std::hex << status);
            return status;
        }

        char deviceName[0xff] = {0};
        status = device.ReadReqEx2(
            IOADS_IGR_IODEVICESTATE_BASE + deviceIds[i],
            IOADS_IOF_READDEVNAME,
            sizeof(deviceName) - 1,
            deviceName,
            &bytesRead);

        if (status != ADSERR_NOERR) {
            LOG_ERROR("Reading name for device[" << deviceIds[i] << "] failed with 0x" << std::hex << status);
            return status;
        }

        AmsNetId netId = {0};
        status = device.ReadReqEx2(
            IOADS_IGR_IODEVICESTATE_BASE + deviceIds[i],
            IOADS_IOF_READDEVNETID,
            sizeof(netId),
            &netId,
            &bytesRead);

        if (status != ADSERR_NOERR) {
            LOG_ERROR("Reading AmsNetId for device[" << deviceIds[i] << "] failed with 0x" << std::hex << status);
            return status;
        }

        os << deviceIds[i] << " | " << devType << " | " << deviceName << " | " << netId << '\n';
    }
    return status;
}
}
}
