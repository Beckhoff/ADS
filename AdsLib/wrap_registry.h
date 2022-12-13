// SPDX-License-Identifier: MIT
/**
   Copyright (c) 2022 Beckhoff Automation GmbH & Co. KG
 */

#pragma once

#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
#include <windows.h>
#else
// Registry Value Types
constexpr uint32_t REG_NONE = 0;
constexpr uint32_t REG_SZ = 1;
constexpr uint32_t REG_EXPAND_SZ = 2;
constexpr uint32_t REG_BINARY = 3;
constexpr uint32_t REG_DWORD = 4;
constexpr uint32_t REG_DWORD_LITTLE_ENDIAN = 4;
constexpr uint32_t REG_DWORD_BIG_ENDIAN = 5;
constexpr uint32_t REG_LINK = 6;
constexpr uint32_t REG_MULTI_SZ = 7;
constexpr uint32_t REG_RESOURCE_LIST = 8;
constexpr uint32_t REG_FULL_RESOURCE_DESCRIPTOR = 9;
constexpr uint32_t REG_RESOURCE_REQUIREMENTS_LIST = 10;
constexpr uint32_t REG_QWORD = 11;
constexpr uint32_t REG_QWORD_LITTLE_ENDIAN = 11;
#endif
