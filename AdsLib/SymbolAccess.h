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
    static std::pair<std::string, SymbolEntry> Parse(const uint8_t* data, size_t lengthLimit);
    AdsSymbolEntry header;
    std::string name;
    std::string typeName;
    std::string comment;
};

using SymbolEntryMap = std::map<std::string, SymbolEntry>;

struct SymbolAccess {
    SymbolAccess(const std::string& gw, AmsNetId netid, uint16_t port);
    SymbolEntryMap FetchSymbolEntries() const;
private:
    AdsDevice device;
};
}
}
