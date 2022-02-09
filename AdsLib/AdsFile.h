// SPDX-License-Identifier: MIT
/**
   Copyright (c) 2020 - 2022 Beckhoff Automation GmbH & Co. KG
 */

#pragma once

#include "AdsDevice.h"

namespace bhf
{
namespace ads
{
enum FOPEN : uint32_t {
    READ = 1 << 0,
    WRITE = 1 << 1,
    APPEND = 1 << 2,
    PLUS = 1 << 3,
    BINARY = 1 << 4,
    TEXT = 1 << 5,
    ENSURE_DIR = 1 << 6,
    ENABLE_DIR = 1 << 7,
    OVERWRITE = 1 << 8,
    OVERWRITE_RENAME = 1 << 9,
    SHIFT_OPENPATH = 16,
};
}
}

struct AdsFile {
    AdsFile(const AdsDevice& route, const std::string& filename, uint32_t flags);
    void Read(const size_t size, void* data, uint32_t& bytesRead) const;
    void Write(const size_t size, const void* data) const;

    static void Delete(const AdsDevice& route, const std::string& filename, uint32_t flags);
private:
    const AdsDevice& m_Route;
    const AdsHandle m_Handle;
};
