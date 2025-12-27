
/**
 * @file main.cpp
 * @brief Entry point for the Vetra firmware application.
 *
 * Initializes the App singleton and delegates to setup/loop.
 */

#include <Arduino.h>
#include "BLEManager.h"
#include "Timer.h"
#include "StateMachine.h"
#include "Device.h"
#include "App.h"

// -----------------------------------------------------------------------------
// Global App Instance
// -----------------------------------------------------------------------------

App app;

// -----------------------------------------------------------------------------
// Arduino Setup/Loop
// -----------------------------------------------------------------------------

void setup() {
    app.setup();
}

void loop() {
    app.loop();
}
