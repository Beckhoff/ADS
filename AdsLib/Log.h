#ifndef LOG_H
#define LOG_H

#include <iosfwd>
#include <sstream>

#define asHex(X) "0x" << std::hex << (int)(X)

#define LOG(LEVEL, ARGS) \
    do { \
        if (LEVEL >= Logger::level) { \
            std::stringstream stream; \
            stream << ARGS; \
            Logger::Log(LEVEL, stream.str()); \
        } \
    } while (0)

#define LOG_VERBOSE(ARGS) LOG(0, ARGS)
#define LOG_INFO(ARGS) LOG(1, ARGS)
#define LOG_WARN(ARGS) LOG(2, ARGS)
#define LOG_ERROR(ARGS) LOG(3, ARGS)

struct Logger {
    static void Log(size_t level, const std::string& msg);
    static size_t level;
private:
    static std::ostream& output;
};

#endif // LOG_H
