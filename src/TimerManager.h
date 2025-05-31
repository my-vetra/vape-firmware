#ifndef TIMER_MANAGER_H
#define TIMER_MANAGER_H

#include <Arduino.h>

class TimerManager {
public:
    // Persistent timer functions
    static bool setPersistentDuration(unsigned long newTime);
    static unsigned long getPersistentDuration();
    static unsigned long getPersistentElapsed();
    static void startTimer();

    // Coil timer functions
    static void startCoilTimer();
    static unsigned long getCoilDuration();
    static unsigned long getCoilRemaining();
    static bool isCoilTimerExpired();

    // NPT Timer functions
    static bool updateSystemTime(unsigned long newNTPTime);

private:
    // Persistent timer variables (stored in RTC memory)
    static RTC_DATA_ATTR unsigned long persistentDuration;
    static RTC_DATA_ATTR unsigned long persistentStartTime;

    // Coil timer variables (non-persistent)
    static unsigned long coilDuration;
    static RTC_DATA_ATTR unsigned long coilStartTime;
    // static RTC_DATA_ATTR unsigned long lastUpdate;
    static unsigned long minimumTime;

};

#endif
