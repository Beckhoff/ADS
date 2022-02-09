// SPDX-License-Identifier: MIT
/**
    Copyright (c) 2015 -2022 Beckhoff Automation GmbH & Co. KG
    Author: Patrick Bruenn <p.bruenn@beckhoff.com>
 */

#pragma once

#include <sstream>

#define asHex(X) "0x" << std::hex << (int)(X)

#define LOG(LEVEL, ARGS) \
    do { \
        std::stringstream stream; \
        stream << ARGS; \
        Logger::Log(LEVEL, stream.str()); \
    } while (0)

#define LOG_VERBOSE(ARGS) LOG(0, ARGS)
#define LOG_INFO(ARGS) LOG(1, ARGS)
#define LOG_WARN(ARGS) LOG(2, ARGS)
#define LOG_ERROR(ARGS) LOG(3, ARGS)

struct Logger {
    static size_t logLevel;
    static void Log(size_t level, const std::string& msg);
};
