// SPDX-License-Identifier: MIT
/**
    Copyright (c) 2015 -2022 Beckhoff Automation GmbH & Co. KG
    Author: Patrick Bruenn <p.bruenn@beckhoff.com>
 */

#include "Log.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iostream>

#ifdef _WIN32
#define TIME_T_TO_STRING(DATE_TIME, TIME_T) do { \
        struct tm temp; \
        localtime_s(&temp, TIME_T); \
        std::strftime(DATE_TIME, sizeof(DATE_TIME), "%Y-%m-%dT%H:%M:%S ", &temp); \
} while (0);
#elif defined(__CYGWIN__)
#define TIME_T_TO_STRING(DATE_TIME, TIME_T) std::strftime(DATE_TIME, sizeof(DATE_TIME), "%FT%T ", localtime(TIME_T));
#else
#define TIME_T_TO_STRING(DATE_TIME, TIME_T) std::strftime(DATE_TIME, sizeof(DATE_TIME), "%FT%T%z ", localtime(TIME_T));
#endif

size_t Logger::logLevel = 1;

static const char* CATEGORY[] = {
    "Verbose: ",
    "Info: ",
    "Warning: ",
    "Error: "
};

void Logger::Log(const size_t level, const std::string& msg)
{
    if (level >= logLevel) {
        std::time_t tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        const auto category = CATEGORY[std::min(level, sizeof(CATEGORY) / sizeof(CATEGORY[0]))];
        char dateTime[28];

        //TODO use std::put_time() when available
        TIME_T_TO_STRING(dateTime, &tt);
        std::cerr << dateTime << category << msg << std::endl;
    }
}
