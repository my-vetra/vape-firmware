#include <Arduino.h>
#include <unity.h>
#include "BLEManager.h"

void test_ble_manager_init() {
    BLEManager* bleManager = &BLEManager::instance();
    bleManager->startService();
    TEST_ASSERT_TRUE(bleManager->isActive());
    bleManager->cleanupService();
    TEST_ASSERT_FALSE(bleManager->isActive());
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_ble_manager_init);
    UNITY_END();
}

void loop() {}
