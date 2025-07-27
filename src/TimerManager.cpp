#include "TimerManager.h"
#include "esp_timer.h"
#include "GlobalVars.h"

// Persistent timer variables
RTC_DATA_ATTR unsigned long TimerManager::persistentDuration =  1 * 60 * 1000UL;  // Default: 10 minutes
RTC_DATA_ATTR unsigned long TimerManager::persistentStartTime = 0;
// RTC_DATA_ATTR unsigned long TimerManager::lastUpdate = 0;
unsigned long TimerManager::minimumTime = 10 * 60 * 1000UL;   // 10 minutes in ms

// Coil timer variables
unsigned long TimerManager::coilDuration = 2 * 60 * 1000UL; // Default: 2 minutes
RTC_DATA_ATTR unsigned long TimerManager::coilStartTime = 0;

unsigned long ONE_DAY = 24 * 60 * 60 * 1000UL;     // 24 hours in ms

bool TimerManager::setPersistentDuration(unsigned long newTime) {
    // if (newTime < minimumTime | ((getRtcTimeMillis() - lastUpdate) < ONE_DAY)) {
    if (newTime < minimumTime) {
        return false;
    }
    persistentDuration = newTime;
    // lastUpdate = getRtcTimeMillis();
    return true;
}

unsigned long TimerManager::getPersistentDuration() {
    return persistentDuration;
}

unsigned long TimerManager::getPersistentElapsed() {
    unsigned long elapsed = getRtcTimeMillis() - persistentStartTime;
    return (elapsed > persistentDuration) ? persistentDuration : elapsed;
}

void TimerManager::startTimer() {
    persistentStartTime = getRtcTimeMillis();
}

// --- Coil Timer functions ---
void TimerManager::startCoilTimer() {
    coilStartTime = getRtcTimeMillis();
}

unsigned long TimerManager::getCoilDuration() {
    return coilDuration;
}

unsigned long TimerManager::getCoilRemaining() {
    if (deviceState != COIL_COUNTDOWN)
    {
        return coilDuration;
    }

    unsigned long elapsed = getRtcTimeMillis() - coilStartTime;
    if (elapsed >= coilDuration) {
        return 0;
    }
    return coilDuration - elapsed;
}

bool TimerManager::isCoilTimerExpired() {
    return ((getRtcTimeMillis() - coilStartTime) >= coilDuration);
}


bool TimerManager::updateSystemTime(unsigned long newNTPTime) {
    // newNTPTime is expected to be in milliseconds.
    struct timeval tv;
    tv.tv_sec = newNTPTime / 1000;  // Convert milliseconds to seconds.
    tv.tv_usec = 0;
    if (settimeofday(&tv, NULL) != 0) {
        return false;
    }
    return true;
}