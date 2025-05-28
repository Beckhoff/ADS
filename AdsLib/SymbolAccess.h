// SPDX-License-Identifier: MIT
/**
   Copyright (c) 2022 - 2023 Beckhoff Automation GmbH & Co. KG
   Author: Patrick Bruenn <p.bruenn@beckhoff.com>
 */

#pragma once

#include "AdsDevice.h"
#include <map>

namespace bhf
{
namespace ads
{
struct SymbolEntry {
	static std::pair<std::string, SymbolEntry> Parse(const uint8_t *data,
							 size_t lengthLimit);
	AdsSymbolEntry header;
	std::string name;
	std::string typeName;
	std::string comment;
	void WriteAsJSON(std::ostream &os) const;
};

using SymbolEntryMap = std::map<std::string, SymbolEntry>;

struct SymbolAccess {
	SymbolAccess(const std::string &gw, AmsNetId netid, uint16_t port);
	SymbolEntryMap FetchSymbolEntries() const;
	int Read(const std::string &name, std::ostream &os) const;
	int Write(const std::string &name, const std::string &value) const;
	int ShowSymbols(std::ostream &os) const;

    private:
	AdsDevice device;
	template <typename T>
	int Write(const SymbolEntry &symbol, const std::string &value) const;
};
}
}
