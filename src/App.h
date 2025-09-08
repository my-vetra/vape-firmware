
#pragma once

/**
 * @file App.h
 * @brief Main application logic and lifecycle management for the device.
 *
 * The App class manages setup, main loop, and event handling for wakeup and puff events.
 *
 * @author Vetra Firmware Team
 * @copyright Copyright (c) 2025 Vetra
 */

// --- Project Includes ---
#include "BLEManager.h"
#include "StateMachine.h"
#include "Device.h"

// -----------------------------------------------------------------------------
// Application Constants
// -----------------------------------------------------------------------------

/// @brief Delay in ms for wakeup debounce or sleep transitions
#define WAKE_DELAY_MS 100

// -----------------------------------------------------------------------------
// App Class
// -----------------------------------------------------------------------------

/**
 * @class App
 * @brief Main application class managing lifecycle, setup, and event handling.
 *
 * Methods:
 *   - setup(): Initialize hardware and services
 *   - loop(): Main application loop
 *   - handleWakeup(): Handle wakeup events
 *   - handlePuffCountRising(): Handle puff rising edge events
 *   - handlePuffCountFalling(): Handle puff falling edge events
 */
class App {
public:
  /**
   * @brief Initialize hardware and services.
   */
  void setup();

  /**
   * @brief Main application loop.
   */
  void loop();

  /**
   * @brief Handle wakeup events.
   */
  void handleWakeup();

  /**
   * @brief Handle puff rising edge events.
   */
  void handlePuffCountRising();

  /**
   * @brief Handle puff falling edge events.
   */
  void handlePuffCountFalling();

private:
  /**
   * @brief Update device state based on current conditions.
   */
  void updateDeviceState();

  BLEManager* bleManager = nullptr;         ///< BLE manager instance
  StateMachine* puffCounterSm = nullptr;    ///< State machine instance
};
