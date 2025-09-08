
#pragma once

/**
 * @file Timer.h
 * @brief Timer utility functions for timekeeping and formatting.
 *
 * Provides helpers for NTP/system time, epoch formatting, and wall time access.
 *
 * @author Vetra Firmware Team
 * @copyright Copyright (c) 2025 Vetra
 */

// --- Standard Library Includes ---
#include <Arduino.h>
#include <sys/time.h>

// -----------------------------------------------------------------------------
// Timer/NTP Helper API
// -----------------------------------------------------------------------------

/**
 * @brief Update system time to the given epoch seconds.
 * @param newEpochSeconds New epoch time (seconds).
 * @return True on success.
 */
bool updateSystemTime(uint32_t newEpochSeconds);

/**
 * @brief Format an epoch seconds value into a human-readable timestamp (local time).
 * @param epochSec Epoch seconds to format.
 * @param out Output buffer for formatted string.
 * @param outSize Size of output buffer.
 * @return True on success. Example format: "YYYY-MM-DD HH:MM:SS".
 */
bool epochToTimestamp(uint32_t epochSec, char* out, size_t outSize);

/**
 * @brief Get wall/RTC milliseconds since epoch (may reset if time sync changes).
 * @return Milliseconds since Unix epoch.
 */
inline uint64_t epochMillis() { struct timeval tv; gettimeofday(&tv,nullptr); return ((uint64_t)tv.tv_sec * 1000) + (tv.tv_usec / 1000); }

/**
 * @brief Get wall/RTC seconds since epoch (may reset if time sync changes).
 * @return Seconds since Unix epoch.
 */
inline uint32_t epochSeconds() { struct timeval tv; gettimeofday(&tv,nullptr); return (uint32_t)tv.tv_sec; }