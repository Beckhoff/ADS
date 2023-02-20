// SPDX-License-Identifier: MIT
/**
   Copyright (c) 2022 Beckhoff Automation GmbH & Co. KG
 */

#pragma once
#include "AdsDevice.h"
#include <string>
#include <vector>

namespace bhf
{
namespace ads
{
// Registry Hive System Service Index Groups
enum nRegHive : uint32_t {
    REG_NOT_FOUND = 0,
    REG_HKEYLOCALMACHINE = 200,
    REG_HKEYCURRENTUSER = 201,
    REG_HKEYCLASSESROOT = 202,
};

struct RegistryEntry {
    std::vector<uint8_t> buffer;
    nRegHive hive;
    size_t keyLen;
    uint32_t type;
    size_t dataLen;

    // Create a registry key from a string
    static RegistryEntry Create(const std::string& line);
    // Create a registry key or value from a byte buffer (network)
    static RegistryEntry Create(const std::vector<uint8_t>&& buffer, nRegHive hive, uint32_t regFlag);
    std::ostream& Write(std::ostream& os) const;
    size_t Append(const void* data, size_t length);
    template<typename T>
    void PushData(T data)
    {
        dataLen += Append(&data, sizeof(data));
    }
    void ParseStringValue(const char*& it, size_t& lineNumber);
};

struct RegistryAccess {
    RegistryAccess(const std::string& ipV4, AmsNetId netId, uint16_t port);
    int Export(const std::string& key, std::ostream& os) const;
    int Import(std::istream& is) const;
    static int Verify(std::istream& is, std::ostream& os);

private:
    std::vector<RegistryEntry> Enumerate(const RegistryEntry& key, const uint32_t regFlag,
                                         const size_t bufferSize) const;
    AdsDevice device;
};
}
}
