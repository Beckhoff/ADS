#include "ECatAccess.h"
#include "AdsLib.h"
#include "Log.h"
#include <iostream>
#include <vector>

namespace bhf
{
namespace ads
{
#define IOADS_IGR_IODEVICESTATE_BASE 0x5000
#define IOADS_IOF_READDEVIDS 0x1
#define IOADS_IOF_READDEVNAME 0x1
#define IOADS_IOF_READDEVCOUNT 0x2
#define IOADS_IOF_READDEVNETID 0x5
#define IOADS_IOF_READDEVTYPE 0x7

#define ECADS_IGRP_MASTER_FLBCMDS 0x0000002C

#define SOCCOM_REG_SOCCOM_TYPE 0
#define EC_CMD_TYPE_APRD 1
#define EC_HEAD_IDX_EXTERN_VALUE 0xff

#pragma pack(push, 1)
struct ETYPE_EC_HEADER {
	uint8_t cmd;
	uint8_t idx;
	uint16_t adp;
	uint16_t ado;
	uint16_t length;
	uint16_t irq;
};

struct ETYPE_EC_ULONG_CMD {
	ETYPE_EC_HEADER head;
	uint32_t data;
	uint16_t cnt;
};
#pragma pack(pop)

ECatAccess::ECatAccess(const std::string &gw, const AmsNetId netid,
		       const uint16_t port)
	: device(gw, netid, port ? port : uint16_t(AMSPORT_R0_IO))
{
}

long ECatAccess::ListECatMasters(std::ostream &os) const
{
	uint32_t numberOfDevices;
	uint32_t bytesRead;

	auto status = device.ReadReqEx2(IOADS_IGR_IODEVICESTATE_BASE,
					IOADS_IOF_READDEVCOUNT,
					sizeof(numberOfDevices),
					&numberOfDevices, &bytesRead);

	if (status != ADSERR_NOERR) {
		LOG_ERROR("Reading device count failed with 0x" << std::hex
								<< status);
		return status;
	}

	if (numberOfDevices == 0) {
		return status;
	}

	// the first element of the vector is set to devCount,
	// so the actual device Ids start at index 1
	std::vector<uint16_t> deviceIds(numberOfDevices + 1);

	status = device.ReadReqEx2(IOADS_IGR_IODEVICESTATE_BASE,
				   IOADS_IOF_READDEVIDS,
				   deviceIds.capacity() * sizeof(uint16_t),
				   deviceIds.data(), &bytesRead);

	if (status != ADSERR_NOERR) {
		LOG_ERROR("Reading device ids failed with 0x" << std::hex
							      << status);
		return status;
	}

	// Skip the device count, which is at the first index
	for (uint32_t i = 1; i <= numberOfDevices; i++) {
		uint16_t devType;
		status = device.ReadReqEx2(
			IOADS_IGR_IODEVICESTATE_BASE + deviceIds[i],
			IOADS_IOF_READDEVTYPE, sizeof(devType), &devType,
			&bytesRead);

		if (status != ADSERR_NOERR) {
			LOG_ERROR("Reading type for device["
				  << deviceIds[i] << "] failed with 0x"
				  << std::hex << status);
			return status;
		}

		char deviceName[0xff] = { 0 };
		status = device.ReadReqEx2(
			IOADS_IGR_IODEVICESTATE_BASE + deviceIds[i],
			IOADS_IOF_READDEVNAME, sizeof(deviceName) - 1,
			deviceName, &bytesRead);

		if (status != ADSERR_NOERR) {
			LOG_ERROR("Reading name for device["
				  << deviceIds[i] << "] failed with 0x"
				  << std::hex << status);
			return status;
		}

		AmsNetId netId = { 0 };
		status = device.ReadReqEx2(IOADS_IGR_IODEVICESTATE_BASE +
						   deviceIds[i],
					   IOADS_IOF_READDEVNETID,
					   sizeof(netId), &netId, &bytesRead);

		if (status != ADSERR_NOERR) {
			LOG_ERROR("Reading AmsNetId for device["
				  << deviceIds[i] << "] failed with 0x"
				  << std::hex << status);
			return status;
		}

		const auto slaveCount = CountECatSlaves(netId);
		os << deviceIds[i] << " | " << devType << " | " << deviceName
		   << " | " << netId << " | " << slaveCount << '\n';
	}
	return status;
}

uint32_t ECatAccess::CountECatSlaves(const AmsNetId &ecatMaster) const
{
	uint32_t bytesRead;

	ETYPE_EC_ULONG_CMD cmd = {};
	cmd.head.cmd = EC_CMD_TYPE_APRD;
	cmd.head.idx = EC_HEAD_IDX_EXTERN_VALUE;
	cmd.head.adp = 0;
	cmd.head.ado = SOCCOM_REG_SOCCOM_TYPE;
	cmd.head.length = sizeof(uint32_t);
	cmd.head.irq = 0;

	// We have to talk to a different AdsDevice, the EtherCAT master. Now, it
	// is handy that ADS doesn't really implement "connections" so we can just
	// use the AMS port of our R0_IO object, but adust the target AmsPort.
	const AmsAddr addr{ ecatMaster, 0xffff };
	const auto status = AdsSyncReadWriteReqEx2(
		device.GetLocalPort(), &addr, ECADS_IGRP_MASTER_FLBCMDS, 0,
		sizeof(cmd), &cmd, sizeof(cmd), &cmd, &bytesRead);

	// When no coupler is connected, the request returns a device timeout
	if (status == ADSERR_DEVICE_TIMEOUT) {
		return 0;
	}
	if (status == ADSERR_NOERR) {
		return cmd.head.adp;
	}

	LOG_ERROR("Reading slave count for ["
		  << ecatMaster << "] failed with 0x" << std::hex << status);
	throw AdsException(status);
}
}
}
