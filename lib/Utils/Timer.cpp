#include "Timer.h"
#include "Logger.h"
#include "PersistenceManager.h"
#include <ctime>

bool epochToTimestamp(uint32_t epochSec, char* out, size_t outSize) {
    if (!out || outSize == 0) {
        Logger::error("[Timer] Invalid output buffer or size");
        return false;
    }
    time_t rawtime = (time_t)epochSec;
    struct tm * timeinfo = localtime(&rawtime);
    if (!timeinfo) return false;
    size_t n = strftime(out, outSize, "%Y-%m-%d %H:%M:%S", timeinfo);
    return n > 0;
}

bool updateSystemTime(uint32_t newEpochSeconds) {
    struct timeval tv;
    tv.tv_sec = newEpochSeconds;
    tv.tv_usec = 0;

    uint32_t nowEpoch = epochSeconds();
    if (nowEpoch >= newEpochSeconds) {
        Logger::info("[Timer] Current system time is up-to-date or ahead; no update needed.");
        return false;
    }
    

    if (settimeofday(&tv,nullptr) != 0) {
        Logger::error("[Timer] Failed to update system time.");
        return false;
    }
    PersistenceManager::instance().recordEpoch(newEpochSeconds);
    char ts[32];
    if (epochToTimestamp(newEpochSeconds, ts, sizeof(ts))) {
        char line[80];
        snprintf(line, sizeof(line), "[Timer] System time updated: %s", ts);
        Logger::info(line);
    } else {
        Logger::infof("[Timer] System time updated: %u", newEpochSeconds);
    }
    return true;
}