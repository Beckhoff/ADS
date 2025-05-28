// SPDX-License-Identifier: MIT
/**
    Copyright (C) 2021 - 2022 Beckhoff Automation GmbH & Co. KG
    Author: Patrick Bruenn <p.bruenn@beckhoff.com>
 */

#pragma once

#include "AdsDef.h"
#include <functional>
#include <sstream>
#include <map>

namespace bhf
{
struct ParameterOption {
	static constexpr bool asHex = true;
	const std::string key;
	std::string value;
	bool isFlag;
	bool wasSet;
	ParameterOption(const std::string k, bool f = false,
			const std::string v = "")
		: key(k)
		, value(v)
		, isFlag(f)
		, wasSet(false)
	{
	}
};

template <typename T> T StringTo(const std::string &v, T value = {})
{
	if (v.size()) {
		const auto asHex = (v.npos != v.rfind("0x", 0));
		std::stringstream converter;
		converter << (asHex ? std::hex : std::dec) << v;
		converter >> value;
	}
	return value;
}

template <> std::string StringTo<>(const std::string &v, std::string value);
template <> bool StringTo<bool>(const std::string &v, bool value);
template <>
uint8_t StringTo<uint8_t>(const std::string &v, uint8_t defaultValue);

struct ParameterList {
	using MapType = std::map<std::string, ParameterOption>;
	MapType map;
	ParameterList(std::initializer_list<ParameterOption> list)
	{
		for (auto p : list) {
			map.emplace(p.key, p);
		}
	}

	int Parse(int argc, const char *argv[]);

	template <typename T>
	T Get(const std::string &key, const T defaultValue = {}) const
	{
		auto it = map.find(key);
		if (it == map.end()) {
			throw std::runtime_error("invalid parameter " + key);
		}
		return StringTo<T>(it->second.value, defaultValue);
	}
};

struct Commandline {
	using UsageFunc = std::function<void(const std::string &)>;
	Commandline(UsageFunc usage, int argc, const char *argv[]);
	template <typename T> T Pop(const std::string &errorMessage = {})
	{
		if (argc) {
			--argc;
			return StringTo<T>(*(argv++));
		}
		if (!errorMessage.empty()) {
			usage(errorMessage);
		}
		return T{};
	}
	ParameterList &Parse(ParameterList &params);

    private:
	UsageFunc usage;
	int argc;
	const char **argv;
};
template <> const char *Commandline::Pop(const std::string &errorMessage);
}
