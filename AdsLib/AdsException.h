// SPDX-License-Identifier: MIT
/**
   Copyright (c) 2020 - 2022 Beckhoff Automation GmbH & Co. KG
 */

#pragma once

#include <stdexcept>
#include <string>

struct AdsException : std::runtime_error {
	explicit AdsException(const long adsErrorCode)
		: std::runtime_error("Ads operation failed with error code " +
		                     std::to_string(adsErrorCode) + "."),
		  errorCode(adsErrorCode)
	{
	}

	const long errorCode;
};
