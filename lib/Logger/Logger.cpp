
/**
 * @file Logger.cpp
 * @brief Implementation of Logger static logging utilities.
 *
 * Routes all log messages to the LogBuffer singleton.
 */

#include "Logger.h"
#include "LogBuffer.h"
#include <stdarg.h>

// -----------------------------------------------------------------------------
// Internal Helper
// -----------------------------------------------------------------------------

/**
 * @brief Format and push a log message with a given level.
 * @param level Log level string (e.g., "INFO").
 * @param fmt printf-style format string.
 * @param args va_list of arguments.
 */
static void vlogf_and_push(const char* level, const char* fmt, va_list args) {
    char buf[128];
    vsnprintf(buf, sizeof(buf), fmt, args);
    std::string line = std::string(level) + ": " + buf;
    LogBuffer::instance().push(line);
}

// -----------------------------------------------------------------------------
// Logger Method Implementations
// -----------------------------------------------------------------------------

void Logger::info(const char* msg) {
#if LOG_LEVEL >= 2
    std::string line = std::string("INFO: ") + msg;
    LogBuffer::instance().push(line);
#endif
}

void Logger::infof(const char* fmt, ...) {
#if LOG_LEVEL >= 2
    va_list args; va_start(args, fmt);
    vlogf_and_push("INFO", fmt, args);
    va_end(args);
#endif
}

void Logger::warning(const char* msg) {
#if LOG_LEVEL >= 1
    std::string line = std::string("WARNING: ") + msg;
    LogBuffer::instance().push(line);
#endif
}

void Logger::warningf(const char* fmt, ...) {
#if LOG_LEVEL >= 1
    va_list args; va_start(args, fmt);
    vlogf_and_push("WARNING", fmt, args);
    va_end(args);
#endif
}

void Logger::error(const char* msg) {
    std::string line = std::string("ERROR: ") + msg;
    LogBuffer::instance().push(line);
}

void Logger::errorf(const char* fmt, ...) {
    va_list args; va_start(args, fmt);
    vlogf_and_push("ERROR", fmt, args);
    va_end(args);
}