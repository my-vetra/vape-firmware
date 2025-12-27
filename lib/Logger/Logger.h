
#pragma once

/**
 * @file Logger.h
 * @brief Logging utilities for info, warning, and error messages.
 *
 * Provides static methods for formatted and unformatted logging, routed to the log buffer.
 *
 * @author Vetra Firmware Team
 * @copyright Copyright (c) 2025 Vetra
 */

// --- Standard Library Includes ---
#include <Arduino.h>

// -----------------------------------------------------------------------------
// Logger Class
// -----------------------------------------------------------------------------

/**
 * @class Logger
 * @brief Static logging utility for info, warning, and error messages.
 *
 * All methods are thread-safe and log to the internal log buffer.
 */
class Logger {
public:
     // Compile-time log level control
     // 0 = ERROR, 1 = WARNING, 2 = INFO (default)
     #ifndef LOG_LEVEL
     #define LOG_LEVEL 2
     #endif

    /**
     * @brief Log an informational message.
     * @param msg Null-terminated string.
     */
    static void info(const char* msg);

    /**
     * @brief Log a formatted informational message.
     * @param fmt printf-style format string.
     * @param ... Arguments for format string.
     */
    static void infof(const char* fmt, ...);

    /**
     * @brief Log a warning message.
     * @param msg Null-terminated string.
     */
    static void warning(const char* msg);

    /**
     * @brief Log a formatted warning message.
     * @param fmt printf-style format string.
     * @param ... Arguments for format string.
     */
    static void warningf(const char* fmt, ...);

    /**
     * @brief Log an error message.
     * @param msg Null-terminated string.
     */
    static void error(const char* msg);

    /**
     * @brief Log a formatted error message.
     * @param fmt printf-style format string.
     * @param ... Arguments for format string.
     */
    static void errorf(const char* fmt, ...);
};
