#include "ECatAccess.h"
#include "AdsLib.h"
#include "Log.h"
#include <iostream>
#include <limits>
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
#define SOCCOM_REG_STATION_ADDRESS 0x0010
#define SOCCOM_REG_AL_STATUS 0x0130
#define EC_CMD_TYPE_APRD 1
#define EC_HEAD_IDX_EXTERN_VALUE 0xff

namespace {
bool IsRelativeNetId(const AmsNetId &netId)
{
	return netId.b[0] == 0 && netId.b[1] == 0 && netId.b[2] == 0
	    && netId.b[3] == 0;
}

AmsNetId ResolveNetId(const AmsNetId &rawNetId,
			    const AmsNetId &remoteTarget)
{
	if (!IsRelativeNetId(rawNetId)) {
		return rawNetId;
	}

	AmsNetId resolved = rawNetId;
	resolved.b[0] = remoteTarget.b[0];
	resolved.b[1] = remoteTarget.b[1];
	resolved.b[2] = remoteTarget.b[2];
	resolved.b[3] = remoteTarget.b[3];
	return resolved;
}
}

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

struct ETYPE_EC_USHORT_CMD {
	ETYPE_EC_HEADER head;
	uint16_t data;
	uint16_t cnt;
};
#pragma pack(pop)

ECatAccess::ECatAccess(const std::string &gw, const AmsNetId netid,
		       const uint16_t port)
	: device(gw, netid, port ? port : uint16_t(AMSPORT_R0_IO))
	, gateway(gw)
{
}

long ECatAccess::ListECatMasters(std::ostream &os) const
{
	uint32_t numberOfDevices = 0;
	uint32_t bytesRead = 0;

	auto status = device.ReadReqEx2(IOADS_IGR_IODEVICESTATE_BASE,
					IOADS_IOF_READDEVCOUNT,
					sizeof(numberOfDevices),
					&numberOfDevices, &bytesRead);

	if (status != ADSERR_NOERR) {
		LOG_ERROR("Reading device count failed with 0x" << std::hex
								<< status);
		return status;
	}

	if (bytesRead != sizeof(numberOfDevices)) {
		LOG_ERROR("Corrupt device count length: " << std::dec
							  << bytesRead);
		return ADSERR_DEVICE_INVALIDDATA;
	}

	numberOfDevices = letoh(numberOfDevices);
	if (numberOfDevices == 0) {
		return status;
	}

	// A device id is 16 bit, so a bigger count is nonsense and would make
	// the "+ 1" below wrap to an empty vector we then index into.
	if (numberOfDevices > std::numeric_limits<uint16_t>::max()) {
		LOG_ERROR("Device count out of range: " << std::dec
							<< numberOfDevices);
		return ADSERR_DEVICE_INVALIDDATA;
	}

	// the first element of the vector is set to devCount,
	// so the actual device Ids start at index 1
	std::vector<uint16_t> deviceIds(numberOfDevices + 1);

	status = device.ReadReqEx2(IOADS_IGR_IODEVICESTATE_BASE,
				   IOADS_IOF_READDEVIDS,
				   deviceIds.size() * sizeof(uint16_t),
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

		const auto masterNetId =
			ResolveNetId(netId, device.m_Addr.netId);
		const auto slaveCount = CountECatSlaves(masterNetId);
		os << deviceIds[i] << " | " << devType << " | " << deviceName
		   << " | " << masterNetId << " | " << slaveCount << '\n';
	}
	return status;
}

std::vector<AmsNetId> ECatAccess::GetECatMasterNetIds() const
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
		throw AdsException(status);
	}

	std::vector<AmsNetId> masters;
	if (numberOfDevices == 0) {
		return masters;
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
		throw AdsException(status);
	}

	// Skip the device count, which is at the first index
	for (uint32_t i = 1; i <= numberOfDevices; i++) {
		AmsNetId netId = { 0 };
		status = device.ReadReqEx2(IOADS_IGR_IODEVICESTATE_BASE +
						   deviceIds[i],
					   IOADS_IOF_READDEVNETID,
					   sizeof(netId), &netId, &bytesRead);

		if (status != ADSERR_NOERR) {
			LOG_ERROR("Reading AmsNetId for device["
				  << deviceIds[i] << "] failed with 0x"
				  << std::hex << status);
			throw AdsException(status);
		}

		masters.push_back(ResolveNetId(netId, device.m_Addr.netId));
	}
	return masters;
}

std::map<AmsNetId, std::vector<ECatSlaveStatus> >
ECatAccess::GetECatSlaveStatus() const
{
	std::map<AmsNetId, std::vector<ECatSlaveStatus> > states;
	for (const auto &master : GetECatMasterNetIds()) {
		const auto slaveCount = CountECatSlaves(master);
		if (slaveCount == 0) {
			states[master] = {};
			continue;
		}
		auto &slaveStates = states[master];
		slaveStates.reserve(slaveCount);
		for (uint16_t slave = 0; slave < slaveCount; ++slave) {
			slaveStates.push_back({
				ReadECatSlaveStationAddress(master, slave),
				ReadECatSlaveAlStatus(master, slave)
			});
		}
	}
	return states;
}

uint16_t ECatAccess::ReadECatSlaveStationAddress(
	const AmsNetId &ecatMaster, const uint16_t slaveIndex) const
{
	uint32_t bytesRead;

	const auto routeStatus = AddLocalRoute(ecatMaster, gateway.c_str());
	if (routeStatus != 0) {
		LOG_ERROR("Adding route for ECat master ["
			  << ecatMaster << "] via gateway [" << gateway
			  << "] failed with 0x" << std::hex << routeStatus);
		throw AdsException(routeStatus);
	}

	ETYPE_EC_USHORT_CMD cmd = {};
	cmd.head.cmd = EC_CMD_TYPE_APRD;
	cmd.head.idx = EC_HEAD_IDX_EXTERN_VALUE;
	cmd.head.adp = static_cast<uint16_t>(0u - slaveIndex);
	cmd.head.ado = SOCCOM_REG_STATION_ADDRESS;
	cmd.head.length = sizeof(uint16_t);
	cmd.head.irq = 0;

	const AmsAddr addr{ ecatMaster, 0xffff };
	const auto status = AdsSyncReadWriteReqEx2(
		device.GetLocalPort(), &addr, ECADS_IGRP_MASTER_FLBCMDS, 0,
		sizeof(cmd), &cmd, sizeof(cmd), &cmd, &bytesRead);

	DelLocalRoute(ecatMaster);

	if (status != ADSERR_NOERR) {
		LOG_ERROR("Reading station address for slave["
			  << slaveIndex << "] of master [" << ecatMaster
			  << "] failed with 0x" << std::hex << status);
		throw AdsException(status);
	}

	return cmd.data;
}

uint16_t ECatAccess::ReadECatSlaveAlStatus(const AmsNetId &ecatMaster,
					   const uint16_t slaveIndex) const
{
	uint32_t bytesRead;

	const auto routeStatus = AddLocalRoute(ecatMaster, gateway.c_str());
	if (routeStatus != 0) {
		LOG_ERROR("Adding route for ECat master ["
			  << ecatMaster << "] via gateway [" << gateway
			  << "] failed with 0x" << std::hex << routeStatus);
		throw AdsException(routeStatus);
	}

	ETYPE_EC_USHORT_CMD cmd = {};
	cmd.head.cmd = EC_CMD_TYPE_APRD;
	cmd.head.idx = EC_HEAD_IDX_EXTERN_VALUE;
	cmd.head.adp = static_cast<uint16_t>(0u - slaveIndex);
	cmd.head.ado = SOCCOM_REG_AL_STATUS;
	cmd.head.length = sizeof(uint16_t);
	cmd.head.irq = 0;

	const AmsAddr addr{ ecatMaster, 0xffff };
	const auto status = AdsSyncReadWriteReqEx2(
		device.GetLocalPort(), &addr, ECADS_IGRP_MASTER_FLBCMDS, 0,
		sizeof(cmd), &cmd, sizeof(cmd), &cmd, &bytesRead);

	DelLocalRoute(ecatMaster);

	if (status != ADSERR_NOERR) {
		LOG_ERROR("Reading ADS state for slave["
			  << slaveIndex << "] of master [" << ecatMaster
			  << "] failed with 0x" << std::hex << status);
		throw AdsException(status);
	}

	return cmd.data;
}

uint32_t ECatAccess::CountECatSlaves(const AmsNetId &ecatMaster) const
{
	uint32_t bytesRead;

	// ECat masters can use a different AMS NetId suffix than the runtime
	// device. Add a temporary alias route for that NetId to the same gateway.
	const auto routeStatus = AddLocalRoute(ecatMaster, gateway.c_str());
	if (routeStatus != 0) {
		LOG_ERROR("Adding route for ECat master ["
			  << ecatMaster << "] via gateway [" << gateway
			  << "] failed with 0x" << std::hex << routeStatus);
		throw AdsException(routeStatus);
	}

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

	DelLocalRoute(ecatMaster);

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
