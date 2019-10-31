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

#ifndef LOG_H
#define LOG_H

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

#endif // LOG_H
