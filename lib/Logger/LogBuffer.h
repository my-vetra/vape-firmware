
#pragma once

/**
 * @file LogBuffer.h
 * @brief Circular buffer for log messages (FIFO, thread-safe).
 *
 * Provides a bounded FIFO for log lines, with truncation and capacity management.
 *
 * @author Vetra Firmware Team
 * @copyright Copyright (c) 2025 Vetra
 */

// --- Standard Library Includes ---
#include <string>
#include <deque>

// -----------------------------------------------------------------------------
// LogBuffer Class
// -----------------------------------------------------------------------------

/**
 * @class LogBuffer
 * @brief Singleton circular buffer for log messages.
 *
 * Oldest entries are dropped when full. Lines are truncated to a safe BLE-friendly size.
 */
class LogBuffer {
public:
    /**
     * @brief Get singleton instance of LogBuffer.
     */
    static LogBuffer& instance();

    /**
     * @brief Push a line into the buffer (truncated if needed).
     * @param line Log line to push.
     */
    void push(const std::string& line);

    /**
     * @brief Pop a line if available.
     * @param out Output string for the popped line.
     * @return True if a line was returned.
     */
    bool pop(std::string& out);

    /**
     * @brief Number of queued lines.
     */
    size_t size() const;

    /**
     * @brief Maximum lines retained in the buffer.
     */
    size_t capacity() const { return kCapacity; }

private:
    LogBuffer() = default;
    static constexpr size_t kCapacity = 100;      ///< Max lines in buffer
    static constexpr size_t kMaxLineLen = 512;    ///< Max line length (truncated)
    std::deque<std::string> q_;
};
