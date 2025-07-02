#include "SleepManager.h"

void SleepManager::enterDeepSleep() {
    Serial.println("Entering Deep Sleep Mode...");
    delay(100);
    esp_deep_sleep_start();
}
// What data can be used in mvram and rtc
// Every variable not in rtc gets reset after reboot