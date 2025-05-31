#include "GlobalVars.h"
DeviceState deviceState = COIL_LOCKED;

uint64_t getRtcTimeMillis() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return ((uint64_t)tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}
