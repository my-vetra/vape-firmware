#include <Arduino.h>
#include <unity.h>
#include "StateMachine.h"

void test_state_machine_init() {
    TEST_ASSERT_EQUAL(StateMachine::instance().getCurrentState(), PUFF_COUNTING);
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_state_machine_init);
    UNITY_END();
}

void loop() {}