#ifndef SLEEP_MANAGER_H
#define SLEEP_MANAGER_H

#include <Arduino.h>
#include "esp_sleep.h"

class SleepManager {
public:
    // Puts the device into deep sleep mode.
    static void enterDeepSleep();
};

#endif // SLEEP_MANAGER_H
