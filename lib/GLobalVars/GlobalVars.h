#include <Arduino.h>

#ifndef GLOBAL_VARS_H
#define GLOBAL_VARS_H

// Need to discuss exactly how many bytes
#define MSG_LEN 20

// State machine for device operation
enum DeviceState {
    COIL_LOCKED,   // BLE active, coil remains locked
    COIL_UNLOCK, // Timer expired: coil unlocked for 5 minutes
    COIL_COUNTDOWN // Timer expired: coil unlocked for 5 minutes
};

RTC_DATA_ATTR extern DeviceState deviceState;

uint64_t getRtcTimeMillis();

#endif // GLOBAL_VARS_H
