
/**
 * @file Device.cpp
 * @brief Implementation of Device hardware abstraction.
 */

#include "Device.h"
#include <Arduino.h>

// -----------------------------------------------------------------------------
// Device State (local static)
// -----------------------------------------------------------------------------

static DeviceState deviceState = COIL_LOCKED;

// -----------------------------------------------------------------------------
// Device Method Implementations
// -----------------------------------------------------------------------------

void Device::setupPins() {
    pinMode(BUTTON_PIN, INPUT_PULLDOWN);
    pinMode(HEAT_PIN, INPUT_PULLDOWN);
    pinMode(COIL_CTRL_PIN, OUTPUT);
    digitalWrite(COIL_CTRL_PIN, HIGH); // Initially locked
}

void Device::lockCoil() {
    digitalWrite(COIL_CTRL_PIN, HIGH);
    deviceState = COIL_LOCKED;
}

void Device::unlockCoil() {
    digitalWrite(COIL_CTRL_PIN, LOW);
    deviceState = COIL_UNLOCKED;
}

void Device::setState(DeviceState state) {
    if (state == COIL_LOCKED) {
        lockCoil();
    } else {
        unlockCoil();
    }
}

DeviceState Device::getState() {
    return deviceState;
}
