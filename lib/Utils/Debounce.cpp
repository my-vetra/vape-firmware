#include "Debounce.h"
#include "Logger.h"

DebounceManager& DebounceManager::instance(){ static DebounceManager inst; return inst; }

void DebounceManager::start(volatile bool &target, bool value){
    pendingTarget = &target;
    pendingValue = value;
    isActive = true;
    endMs = millis() + DEBOUNCE_MS;
    Logger::infof("[Debounce] start window=%ums target=%p value=%d", (unsigned)DEBOUNCE_MS, (void*)pendingTarget, (int)pendingValue);
}

bool DebounceManager::active(){
    return isActive;
}

void DebounceManager::touch(){
    // if(isActive){
    endMs = millis() + DEBOUNCE_MS; // reset window
    // Logger::infof("[Debounce] extend window=%ums", (unsigned)DEBOUNCE_MS);
    // }
}

void DebounceManager::poll(){
    if(!isActive) return;
    uint32_t now = millis();
    if((int32_t)(now - endMs) >= 0){
        if(pendingTarget) *pendingTarget = pendingValue;
        Logger::infof("[Debounce] poll expire apply value=%d target=%p", (int)pendingValue, (void*)pendingTarget);
        isActive = false;
        pendingTarget = nullptr;
    }
}
