// SPDX-License-Identifier: MIT
/**
    Copyright (C) 2023 Beckhoff Automation GmbH & Co. KG
 */

#pragma once

#include <string>

namespace bhf
{
template <class T> static T try_stoi(const char *str, const T defaultValue = 0)
{
	try {
		if (str && *str) {
			return static_cast<T>(std::stoi(++str));
		}
	} catch (...) {
	}
	return defaultValue;
}
}
