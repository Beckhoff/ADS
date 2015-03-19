#ifndef LOG_H
#define LOG_H

#include <cstdint>
#include <iostream>
#include <sstream>
#include <vector>

#define asHex(X) "0x" << std::hex << (int)(X)

#define LOG_LEVEL 0

#define LOG(LEVEL, ARGS) \
    do { \
        if (LEVEL >= LOG_LEVEL) { \
            std::stringstream stream; \
            stream << ARGS; \
            Logger::Log(LEVEL, stream.str()); \
        } \
    } while(0)

#define LOG_INFO(ARGS) LOG(0, ARGS)
#define LOG_WARN(ARGS) LOG(1, ARGS)
#define LOG_ERROR(ARGS) LOG(2, ARGS)

struct Logger {
    static void Log(size_t level, const std::string& msg);
};

#endif // LOG_H
