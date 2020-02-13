// SPDX-License-Identifier: MIT
/**
   Copyright (c) 2020 Beckhoff Automation GmbH & Co. KG
 */

#pragma once

#include "AdsDevice.h"

struct AdsFile {
    AdsFile(const AdsDevice& route, const std::string& filename, uint32_t flags);
    void Read(const size_t size, void* data, uint32_t& bytesRead) const;
    void Write(const size_t size, const void* data) const;

    static void Delete(const AdsDevice& route, const std::string& filename, uint32_t flags);
private:
    const AdsDevice& m_Route;
    const AdsHandle m_Handle;
};
