/**
   Copyright (c) 2015 Beckhoff Automation GmbH & Co. KG

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
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

static const char* CATEGORY[] = {
    "Verbose: ",
    "Info: ",
    "Warning: ",
    "Error: "
};

size_t Logger::logLevel = 1;

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
