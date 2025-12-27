#pragma once
#include <Arduino.h>

#ifndef DEBOUNCE_MS
#define DEBOUNCE_MS 200
#endif

class DebounceManager {
public:
    static DebounceManager& instance();
    void start(volatile bool &target, bool value);
    bool active();
    void touch();
    void poll();
private:
    DebounceManager() = default;
    volatile bool* pendingTarget = nullptr;
    bool pendingValue = false;
    bool isActive = false;
    uint32_t endMs = 0;
};
