#include "SleepManager.h"

void SleepManager::enterDeepSleep() {
    Serial.println("Entering Deep Sleep Mode...");
    delay(100);
    esp_deep_sleep_start();
}
