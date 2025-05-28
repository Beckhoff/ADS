// SPDX-License-Identifier: MIT
/**
    Copyright (C) 2021 - 2022 Beckhoff Automation GmbH & Co. KG
    Author: Patrick Bruenn <p.bruenn@beckhoff.com>
 */

#include "ParameterList.h"
#include "Log.h"

namespace bhf
{
#define ERR_INVALID_PARAMETER (-2)

template <>
std::string StringTo<std::string>(const std::string &v, const std::string)
{
	return v;
}

template <> bool StringTo<bool>(const std::string &v, const bool)
{
	return !v.compare("true");
}

template <>
uint8_t StringTo<uint8_t>(const std::string &v, uint8_t defaultValue)
{
	return static_cast<uint8_t>(StringTo<uint32_t>(v, defaultValue));
}

int ParameterList::Parse(int argc, const char *argv[])
{
	int found = 0;
	while ((found < argc) && ('-' == argv[found][0])) {
		auto key_value = std::string(argv[found]);
		const auto keylen = key_value.find_first_of("=");
		const auto key = key_value.substr(0, keylen);
		auto it = map.find(key);
		if (it == map.end()) {
			throw std::runtime_error("Unknown option '" + key +
						 "'");
		}
		auto &o = it->second;
		if (o.wasSet) {
			LOG_ERROR("Parameter '" << o.key << "' set twice");
			throw ERR_INVALID_PARAMETER;
		}
		++found;
		o.wasSet = true;
		if (o.isFlag) {
			o.value = "true";
			continue;
		}
		if (key_value.npos == keylen) {
			LOG_ERROR("Parameter '" << o.key << "' needs a value");
			throw ERR_INVALID_PARAMETER;
		}
		o.value = key_value.substr(keylen + 1);
	}
	return found;
}

Commandline::Commandline(UsageFunc _usage, int _argc, const char *_argv[])
	: usage(_usage)
	, argc(_argc)
	, argv(_argv)
{
}

ParameterList &Commandline::Parse(ParameterList &params)
{
	const auto found = params.Parse(argc, argv);
	argc -= found;
	argv += found;
	return params;
}

template <> const char *Commandline::Pop(const std::string &errorMessage)
{
	if (argc) {
		--argc;
		return *(argv++);
	}
	if (!errorMessage.empty()) {
		usage(errorMessage);
	}
	return nullptr;
}
}
