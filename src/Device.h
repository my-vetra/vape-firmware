
#pragma once

/**
 * @file Device.h
 * @brief Hardware abstraction for device-specific features (pins, coil, state).
 *
 * Provides static methods for pin setup, coil control, and device state management.
 *
 * @author Vetra Firmware Team
 * @copyright Copyright (c) 2025 Vetra
 */

// --- Standard Library Includes ---
#include <Arduino.h>

// -----------------------------------------------------------------------------
// Device Pin Constants
// -----------------------------------------------------------------------------

/// @name Device Pin Assignments
///@{
#define BUTTON_PIN     2   ///< GPIO pin for button input
#define COIL_CTRL_PIN  3   ///< GPIO pin for coil control
#define HEAT_PIN      10   ///< GPIO pin for heating element
///@}

// -----------------------------------------------------------------------------
// Device State Enum
// -----------------------------------------------------------------------------

/**
 * @enum DeviceState
 * @brief Device coil lock state.
 */
enum DeviceState {
  COIL_LOCKED,    ///< Coil is locked
  COIL_UNLOCKED   ///< Coil is unlocked
};

// -----------------------------------------------------------------------------
// Device Class
// -----------------------------------------------------------------------------

/**
 * @class Device
 * @brief Static class providing hardware abstraction for device features.
 */
class Device {
public:
  /**
   * @brief Setup device pins (GPIO initialization).
   */
  static void setupPins();

  /**
   * @brief Lock the coil (disable heating).
   */
  static void lockCoil();

  /**
   * @brief Unlock the coil (enable heating).
   */
  static void unlockCoil();

  /**
   * @brief Set the device coil state.
   */
  static void setState(DeviceState state);

  /**
   * @brief Get the current device coil state.
   */
  static DeviceState getState();
};
