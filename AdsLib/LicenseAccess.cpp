// SPDX-License-Identifier: MIT
/**
   Copyright (c) Beckhoff Automation GmbH & Co. KG
 */

#include "LicenseAccess.h"
#include "Log.h"
#include "wrap_endian.h"
#include <iostream>
#include <iomanip>
#include <map>
#include <vector>

namespace bhf
{
namespace ads
{
struct TcLicenseOnlineInfo {
	uint8_t licenseId[16];
	int64_t tExpireTime;
	uint32_t nCount;
	uint32_t nUsed;
	uint32_t nResult;
	uint32_t nVolumeNo;
	uint64_t ignored;

	std::ostream &ShowInstances(std::ostream &os) const
	{
		if (nCount) {
			return os << std::dec << nCount;
		}
		return os << "CPU";
	}

	int ShowName(std::ostream &os, const AdsDevice &device) const
	{
		static const uint32_t LICENSE_NAME = 0x0101000C;
		uint32_t bytesRead = 0;
		auto readBuffer = std::vector<char>(64);
		const auto status = device.ReadWriteReqEx2(
			LICENSE_NAME, 0x0, readBuffer.capacity(),
			readBuffer.data(), sizeof(licenseId), &licenseId,
			&bytesRead);
		if (ADSERR_NOERR != status) {
			LOG_ERROR(__FUNCTION__ << "(): failed with: 0x"
					       << std::hex << status << '\n');
			return 1;
		}

		if (1 < bytesRead) {
			// Ensure we have a NULL terminated string
			readBuffer.back() = '\0';
			os << readBuffer.data();
		} else {
			os << "N/A";
		}
		return !os.good();
	}

	int ShowOrderNo(std::ostream &os, const AdsDevice &device) const
	{
		uint32_t bytesRead = 0;
		auto readBuffer = std::vector<char>(16);
		const auto status = device.ReadWriteReqEx2(
			0x0101000D, 0x0, readBuffer.capacity(),
			readBuffer.data(), sizeof(licenseId), &licenseId,
			&bytesRead);
		if (ADSERR_NOERR != status) {
			LOG_ERROR(__FUNCTION__ << "(): failed with: 0x"
					       << std::hex << status << '\n');
			return 1;
		}

		if (1 < bytesRead) {
			// Ensure the order number is NULL terminated.
			readBuffer.back() = '\0';
			os << readBuffer.data();
		} else {
			os << "N/A";
		}
		return !os.good();
	}

	std::ostream &ShowStatus(std::ostream &os) const
	{
		static const std::map<uint32_t, std::string> states = {
			{ 0, "Valid" },
			{ 515, "Valid (Pending)" },
			{ 596, "Valid (Trial)" },
			{ 597, "Valid (OEM)" },
		};
		const auto it = states.find(nResult);
		if (states.end() != it) {
			return os << it->second;
		}
		return os << "0x" << std::hex << nResult;
	}
};

LicenseAccess::LicenseAccess(const std::string &gw, const AmsNetId netid,
			     const uint16_t port)
	: device(gw, netid, port ? port : 30)
{
}

int LicenseAccess::ShowOnlineInfo(std::ostream &os) const
{
	uint32_t licenseCount = 0;
	uint32_t bytesRead = 0;
	const auto countStatus = device.ReadReqEx2(0x01010006, 0x0,
						   sizeof(licenseCount),
						   &licenseCount, &bytesRead);
	if (ADSERR_NOERR != countStatus) {
		LOG_ERROR(__FUNCTION__
			  << "(): read license count failed with: 0x"
			  << std::hex << countStatus << '\n');
		return 1;
	}

	licenseCount = bhf::ads::letoh(licenseCount);
	if (0 == licenseCount) {
		LOG_WARN(__FUNCTION__ << "(): no license available\n");
		return 0;
	}

	auto readBuffer = std::vector<TcLicenseOnlineInfo>(licenseCount);
	const auto bufferSize = sizeof(readBuffer[0]) * readBuffer.capacity();
	const auto status = device.ReadReqEx2(0x01010006, 0x0, bufferSize,
					      readBuffer.data(), &bytesRead);
	if (ADSERR_NOERR != status) {
		LOG_ERROR(__FUNCTION__ << "(): failed with: 0x" << std::hex
				       << status << '\n');
		return 1;
	}

	if (bufferSize != bytesRead) {
		LOG_ERROR(__FUNCTION__
			  << "(): read unexpected number of bytes: 0x"
			  << std::hex << bytesRead << ", expected: 0x"
			  << readBuffer.capacity() << '\n');
		return 1;
	}

	os << "OrderNo.;Instances;Status;License\n";
	for (const auto &next : readBuffer) {
		if (next.ShowOrderNo(os, device)) {
			LOG_ERROR(__FUNCTION__
				  << "(): reading order number failed with: 0x"
				  << std::hex << status << '\n');
			return 1;
		}

		os << ';';
		next.ShowInstances(os) << ';';
		next.ShowStatus(os) << ';';
		if (next.ShowName(os, device)) {
			LOG_ERROR(__FUNCTION__
				  << "(): reading license name failed with: 0x"
				  << std::hex << status << '\n');
			return 1;
		}
		os << '\n';
	}
	return !os.good();
}

int LicenseAccess::ShowPlatformId(std::ostream &os) const
{
	uint16_t platformId;
	uint32_t bytesRead = 0;
	const auto status = device.ReadReqEx2(
		0x01010004, 0x2, sizeof(platformId), &platformId, &bytesRead);
	if (ADSERR_NOERR != status) {
		LOG_ERROR(__FUNCTION__ << "(): failed with: 0x" << std::hex
				       << status << '\n');
		return 1;
	}
	os << bhf::ads::letoh(platformId) << '\n';
	return !os.good();
}

int LicenseAccess::ShowSystemId(std::ostream &os) const
{
	std::vector<uint8_t> readBuffer(16);
	uint32_t bytesRead = 0;
	const auto status = device.ReadReqEx2(0x01010004, 0x1,
					      readBuffer.size(),
					      readBuffer.data(), &bytesRead);
	if (ADSERR_NOERR != status) {
		LOG_ERROR(__FUNCTION__ << "(): failed with: 0x" << std::hex
				       << status << '\n');
		return 1;
	}
	char buf[38];
	snprintf(
		buf, sizeof(buf),
		"%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X\n",
		readBuffer[3], readBuffer[2], readBuffer[1], readBuffer[0],
		readBuffer[5], readBuffer[4], readBuffer[7], readBuffer[6],
		readBuffer[8], readBuffer[9], readBuffer[10], readBuffer[11],
		readBuffer[12], readBuffer[13], readBuffer[14], readBuffer[15]);
	os << buf;
	return !os.good();
}

int LicenseAccess::ShowVolumeNo(std::ostream &os) const
{
	uint32_t volumeNo;
	uint32_t bytesRead = 0;
	const auto status = device.ReadReqEx2(0x01010004, 0x5, sizeof(volumeNo),
					      &volumeNo, &bytesRead);
	if (ADSERR_NOERR != status) {
		LOG_ERROR(__FUNCTION__ << "(): failed with: 0x" << std::hex
				       << status << '\n');
		return 1;
	}
	os << bhf::ads::letoh(volumeNo) << '\n';
	return !os.good();
}
}
}
