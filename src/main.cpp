#include <Arduino.h>
#include "BLEManager.h"
#include "TimerManager.h"
#include "SleepManager.h"
#include "GlobalVars.h"
#include "nvs_flash.h"

// GPIO pins
#define BUTTON_PIN      2         // Pin used for waking up the ESP32
#define COIL_CTRL_PIN   3         // Pin used to drive the transistor for the coil

// Timeouts in milliseconds
const unsigned long bleTimeout = 60 * 1000UL;      // 60 second BLE idle timeout

// Flag used by the wakeup ISR
volatile bool puffTaken = false;

// Global BLEManager instance
BLEManager bleManager;

// ISR for the wakeup (boot) trigger
void IRAM_ATTR handleWakeup() {
    if (deviceState == COIL_UNLOCK) {
        puffTaken = true;
    }
}

void setup() {
    delay(100);

    // Set up the wakeup trigger pin.
    pinMode(BUTTON_PIN, INPUT_PULLDOWN);
    attachInterrupt(BUTTON_PIN, handleWakeup, RISING);

    // Set up the coil control pin.
    pinMode(COIL_CTRL_PIN, OUTPUT);
    // Initially lock the coil (assume LOW = locked, HIGH = unlocked)
    digitalWrite(COIL_CTRL_PIN, LOW);

    // Configure deep sleep wakeup on BUTTON_PIN.
#if SOC_GPIO_SUPPORT_DEEPSLEEP_WAKEUP
    esp_deep_sleep_enable_gpio_wakeup(1ULL << BUTTON_PIN, ESP_GPIO_WAKEUP_GPIO_HIGH);
#endif

    bleManager.startService();
}

void loop() {
    // If BLE is active, check for the BLE idle timeout.
    if (bleManager.isActive()) {
        if (getRtcTimeMillis() - BLEManager::getLastInteraction() > bleTimeout) {
            bleManager.cleanupService();
            SleepManager::enterDeepSleep();
        }
    }

    // Check the persistent timer.
    bool persistentExpired = TimerManager::getPersistentElapsed() == TimerManager::getPersistentDuration();


    // State machine: if the timer is expired and we are in COIL_LOCKED, transition to COIL_UNLOCK.
    if (persistentExpired && deviceState == COIL_LOCKED) {
        deviceState = COIL_UNLOCK;
    } 

    // In the COIL_UNLOCK state, check if the unlock period has finished.
    else if (deviceState == COIL_UNLOCK && puffTaken) {
        deviceState = COIL_COUNTDOWN;
        digitalWrite(COIL_CTRL_PIN, HIGH);
        TimerManager::startCoilTimer();
    }
    
    else if (deviceState == COIL_COUNTDOWN) {
        if (TimerManager::getCoilRemaining() == 0) {
            digitalWrite(COIL_CTRL_PIN, LOW);
            deviceState = COIL_LOCKED;
            puffTaken = false;
            TimerManager::startTimer();
        }
    }

    bleManager.updateSyncCharacteristic();
    delay(500);
}
