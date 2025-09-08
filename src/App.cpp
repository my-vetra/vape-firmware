
/**
 * @file App.cpp
 * @brief Implementation of main application logic and lifecycle management.
 */

#include "App.h"
#include <Arduino.h>
#include <esp_sleep.h>
#include "PersistenceManager.h"
#include "Timer.h"
#include "LogBuffer.h"

// -----------------------------------------------------------------------------
// Global ISR Flags
// -----------------------------------------------------------------------------

static volatile bool s_wakeup_pending = false;
static volatile bool s_puff_rising_pending = false;
static volatile bool s_puff_falling_pending = false;

// -----------------------------------------------------------------------------
// ISR Implementations
// -----------------------------------------------------------------------------

/**
 * @brief Unified ISR for HEAT_PIN.
 */
void IRAM_ATTR heatIsr() {
    int state = digitalRead(HEAT_PIN);
    if (state == HIGH) {
        s_puff_rising_pending = true;
    } else {
        s_puff_falling_pending = true;
    }
}

/**
 * @brief Wakeup ISR.
 */
void IRAM_ATTR wakeupISR() {
    s_wakeup_pending = true;
}

// -----------------------------------------------------------------------------
// App Method Implementations
// -----------------------------------------------------------------------------

void App::setup() {
    // Initialize singletons and services
    Device::setupPins();
    puffCounterSm = &StateMachine::instance();
    bleManager = &BLEManager::instance();

    auto& persistenceManager = PersistenceManager::instance();
    persistenceManager.init();

    // Restore system time from persisted epoch with detailed branch logs
    uint32_t lastEpoch = persistenceManager.getLastEpoch(0);
    uint32_t nowEpoch = epochSeconds();
    if (lastEpoch == 0) {
        Logger::info("[App] No persisted epoch found; skipping time restore.");
    } else if (nowEpoch < lastEpoch) {
        char nowTs[32], lastTs[32];
        epochToTimestamp(nowEpoch, nowTs, sizeof(nowTs));
        epochToTimestamp(lastEpoch, lastTs, sizeof(lastTs));
        Logger::infof("[App] System time behind (now=%s/%u, persisted=%s/%u); restoring.", nowTs, nowEpoch, lastTs, lastEpoch);
        updateSystemTime(lastEpoch);
    } else {
        char nowTs[32], lastTs[32];
        epochToTimestamp(nowEpoch, nowTs, sizeof(nowTs));
        epochToTimestamp(lastEpoch, lastTs, sizeof(lastTs));
        Logger::infof("[App] System time up-to-date (now=%s/%u, persisted=%s/%u); skipping restore.", nowTs, nowEpoch, lastTs, lastEpoch);
    }

    // Configure deep-sleep wake on BUTTON_PIN going HIGH
#if SOC_GPIO_SUPPORT_DEEPSLEEP_WAKEUP
    esp_err_t wakeErr = esp_deep_sleep_enable_gpio_wakeup(1ULL << BUTTON_PIN, ESP_GPIO_WAKEUP_GPIO_HIGH);
    if (wakeErr == ESP_OK) {
        Logger::infof("[App] Wake source configured: ext1 GPIO %d HIGH", BUTTON_PIN);
    } else {
        Logger::errorf("[App] Failed to configure wake source (ext1) on GPIO %d, err=%d", BUTTON_PIN, (int)wakeErr);
    }
    #endif  

    attachInterrupt(BUTTON_PIN, wakeupISR, RISING);
    attachInterrupt(HEAT_PIN, heatIsr, CHANGE);

    bleManager->startService();
}

void App::loop() {
    if (!bleManager) bleManager = &BLEManager::instance();
    if (!puffCounterSm) puffCounterSm = &StateMachine::instance();

    // Drain ISR events in task context
    bool wake = false, rise = false, fall = false;
    noInterrupts();
    if (s_wakeup_pending) { wake = true; s_wakeup_pending = false; }
    if (s_puff_rising_pending) { rise = true; s_puff_rising_pending = false; }
    if (s_puff_falling_pending) { fall = true; s_puff_falling_pending = false; }
    interrupts();
    if (wake) handleWakeup();
    if (rise) handlePuffCountRising();
    if (fall) handlePuffCountFalling();

    if (bleManager->connectionTimeOut()) {
        if (bleManager->isActive()) bleManager->cleanupService();
        // Store current epoch (requires prior NTP for accuracy)
        PersistenceManager::instance().recordEpoch(epochSeconds());
        Logger::info("[App] Entering deep sleep");
        esp_deep_sleep_start();
    }
    updateDeviceState();
    puffCounterSm->incrementValidPhase();

    bleManager->pumpLogs();
    delay(WAKE_DELAY_MS);
}

void App::handleWakeup() {
    if (!puffCounterSm) puffCounterSm = &StateMachine::instance();
    updateDeviceState();
}

void App::handlePuffCountRising() {
    if (!puffCounterSm) puffCounterSm = &StateMachine::instance();
    puffCounterSm->handle_state_rising();
}

void App::handlePuffCountFalling() {
    if (!puffCounterSm) puffCounterSm = &StateMachine::instance();
    puffCounterSm->handle_state_falling();
}

void App::updateDeviceState() {
    if (puffCounterSm->getCurrentState() == LOCKDOWN) {
        Device::lockCoil();
    } else if (puffCounterSm->getCurrentState() == PUFF_COUNTING) {
        Device::unlockCoil();
    }
}