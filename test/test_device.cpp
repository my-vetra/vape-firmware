#include <Arduino.h>
#include <unity.h>
#include "Device.h"

void test_device_state() {
    Device::setState(COIL_UNLOCKED);
    TEST_ASSERT_EQUAL(Device::getState(), COIL_UNLOCKED);
    Device::lockCoil();
    TEST_ASSERT_EQUAL(Device::getState(), COIL_LOCKED);
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_device_state);
    UNITY_END();
}

void loop() {}
