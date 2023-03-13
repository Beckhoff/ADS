// SPDX-License-Identifier: MIT
/**
    Copyright (C) Beckhoff Automation GmbH & Co. KG
    Author: Jan Ole Hüser <j.hueser@beckhoff.com>
 */

#include "RegistryAccessTest.h"

#include "Log.h"
#include "RegistryAccess.h"
#include "wrap_registry.h"

#include <cstring>
#include <sstream>

namespace bhf
{
namespace adstest
{
int testDeleteRegistryKey(const std::string &ipV4, AmsNetId netId,
			  uint16_t port)
{
	bhf::ads::RegistryAccess regAccess(ipV4, netId, port);

	constexpr auto SYSTEMSERVICE_REGISTRY_INVALIDKEY = 1;

	auto createKey = [&](const std::string &key) {
		std::string regFile = "Windows Registry Editor Version 5.00\n"
				      "\n"
				      "[" +
				      key +
				      "]\n"
				      "\"SomeValueName\"=\"SomeData\"\n";

		std::istringstream iss(regFile);

		return regAccess.Import(iss);
	};

	auto sendEntry = [&](const ads::RegistryEntry &entry) {
		return regAccess.device.WriteReqEx(entry.hive, 0,
						   entry.buffer.size(),
						   entry.buffer.data());
	};

	const char *const keyPathForDeletion =
		"-HKEY_LOCAL_MACHINE\\Software\\Test\\KeyA";
	const char *const keyPathForCreation =
		"HKEY_LOCAL_MACHINE\\Software\\Test\\KeyA";
	const char *const keyPathForCreationOfSubKey =
		"HKEY_LOCAL_MACHINE\\Software\\Test\\KeyA\\SubKeyA";

	auto entryForDeletion = ads::RegistryEntry::Create(keyPathForDeletion);
	auto entryForEnumeration =
		ads::RegistryEntry::Create(keyPathForCreation);

	// create key
	long status = 0;
	int error_counter = 0;
	if (0 != (status = createKey(keyPathForCreation))) {
		LOG_ERROR("Failed to create Key. Error code: " << status
							       << "\n");
		++error_counter;
	} else {
		LOG_INFO("Key created successfully.\n");
	}

	// delete key the first time
	if (ADSERR_NOERR != (status = sendEntry(entryForDeletion))) {
		LOG_ERROR("Failed to delete key. Error code: " << status
							       << "\n");
		++error_counter;
	} else {
		LOG_INFO("Key deleted successfully.\n");
	}

	// delete key a second time, and expect error, because the key is already gone

	if (SYSTEMSERVICE_REGISTRY_INVALIDKEY !=
	    (status = sendEntry(entryForDeletion))) {
		LOG_ERROR(
			"Deleting key a second time yielded an unexpeted error code: "
			<< status << ". Expected: "
			<< SYSTEMSERVICE_REGISTRY_INVALIDKEY << "\n");
		++error_counter;
	} else {
		LOG_INFO("Deleting key a second time failed as expected.\n");
	}

	// create key again
	if (0 != (status = createKey(keyPathForCreation))) {
		LOG_ERROR("Failed to create Key. Error code: " << status
							       << "\n");
		++error_counter;
	} else {
		LOG_INFO("Key created successfully.\n");
	}

	// send invalid delete request with more data after the null terminator
	{
		auto entryForDeletionWithExtraData = entryForDeletion;
		entryForDeletionWithExtraData.buffer.push_back('a');
		entryForDeletionWithExtraData.buffer.push_back('b');
		entryForDeletionWithExtraData.buffer.push_back('c');

		if (ADSERR_DEVICE_INVALIDDATA !=
		    (status = sendEntry(entryForDeletionWithExtraData))) {
			LOG_ERROR(
				"Sending invalid delete request with more data after null terminator yielded an unexpeted error code: "
				<< status << ". Expected: "
				<< ADSERR_DEVICE_INVALIDDATA << "\n");
			++error_counter;
		} else {
			LOG_INFO(
				"Sending invalid delete request with more data after null terminator failed as expected.\n");
		}
	}

	// send invalid delete request with missing null terminator
	{
		auto entryForDeletionWithoutNull = entryForDeletion;
		entryForDeletionWithoutNull.buffer.pop_back();

		if (ADSERR_DEVICE_INVALIDDATA !=
		    (status = sendEntry(entryForDeletionWithoutNull))) {
			LOG_ERROR(
				"Sending invalid delete request with missing null terminator yielded an unexpeted error code: "
				<< status << ". Expected: "
				<< ADSERR_DEVICE_INVALIDDATA << "\n");
			++error_counter;
		} else {
			LOG_INFO(
				"Sending invalid delete request with missing null terminator failed as expected.\n");
		}
	}

	// Check if subkeys get deleted
	{
		if (0 != (status = createKey(keyPathForCreationOfSubKey))) {
			LOG_ERROR("Failed to create sub-key. Error code: "
				  << status << "\n");
			++error_counter;
		} else {
			LOG_INFO("Sub-key created successfully.\n");
		}

		// Check that the number of subkeys is 1
		{
			auto subkeys = regAccess.Enumerate(
				entryForEnumeration, ads::REGFLAG_ENUMKEYS,
				0x400);
			if (size_t(1u) != subkeys.size()) {
				LOG_ERROR(
					"Expected the number of subkeys to be one, but it is "
					<< subkeys.size() << "\n");
				++error_counter;
			} else {
				LOG_INFO(
					"The number of sub keys is one, as exepected.\n");
			}
		}

		// delete the key and it's subkey
		if (ADSERR_NOERR != (status = sendEntry(entryForDeletion))) {
			LOG_ERROR("Failed to delete key and it's sub-key.\n");
			++error_counter;
		} else {
			LOG_INFO(
				"Successfully deleted key and it's sub-key.\n");
		}

		// delete key a second time, and expect error, because the key is already gone
		if (SYSTEMSERVICE_REGISTRY_INVALIDKEY !=
		    (status = sendEntry(entryForDeletion))) {
			LOG_ERROR(
				"Request to delete key (and sub-key) a second time yielded an unexpeted error code: "
				<< status << ". Expected: "
				<< SYSTEMSERVICE_REGISTRY_INVALIDKEY << "\n");
			++error_counter;
		} else {
			LOG_INFO(
				"Deleting key and sub-key a second time failed as expected.\n");
		}
	}
	return error_counter;
}
}
}
