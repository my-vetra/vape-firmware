
/**
 * @file StateMachine.cpp
 * @brief Implementation of StateMachine for device state and puff/phase tracking.
 */

#include "StateMachine.h"
#include <Arduino.h>
#include <algorithm>
#include <cstdint>
#include "Logger.h"
#include "BLEManager.h"
#include "PersistenceManager.h"
#include "Timer.h"

// -----------------------------------------------------------------------------
// PuffTimer Implementation
// -----------------------------------------------------------------------------

StateMachine::PuffTimer::PuffTimer() : startTime(0), active(false) {}

void StateMachine::PuffTimer::start() {
    if (active) return;
    startTime = (unsigned long)epochMillis();
    active = true;
}

long StateMachine::PuffTimer::getDuration() const {
    if (startTime == 0) {
        Logger::error("[PuffTimer] getDuration() called before start()");
        return -1;
    }
    unsigned long now = (unsigned long)epochMillis();
    if (now < startTime) {
        Logger::warning("[PuffTimer] current epoch earlier than startTime; returning 0");
        return 0;
    }
    // Return elapsed milliseconds.
    return static_cast<long>(now - startTime);
}

void StateMachine::PuffTimer::reset() {
    startTime = 0;
    active = false;
}

// -----------------------------------------------------------------------------
// StateMachine Core Implementation
// -----------------------------------------------------------------------------

StateMachine& StateMachine::instance() { static StateMachine inst;  return inst; }

StateMachine::StateMachine() {
    phases.reserve(NUM_PHASES + 1);
    for (int i = 0; i <= NUM_PHASES; i++) {
        PhaseModel newPm;
        newPm.phaseIndex = i;
    newPm.phaseDuration = PHASE_DURATION_SECONDS;
        newPm.maxPuffs = MAX_PUFFS;
        newPm.puffsTaken = 0;
        phases.push_back(newPm);
    }
    currPhase = &phases[0];
    currPhase->phaseStartSec = epochSeconds();
    puffs.clear();
    currPuff = nullptr;
    currentState = PUFF_COUNTING;
    Logger::info("[StateMachine] Base initialized. Reconstructing from storage...");
    reconstructFromStorage();
}

StateMachine::~StateMachine() {
    puffs.clear();
    phases.clear();
    currPhase = nullptr;
    currPuff = nullptr;
    Logger::info("[StateMachine] Destroyed.");
}

void StateMachine::requireCurrPhase() {
    if (!currPhase || currPhase->phaseIndex > NUM_PHASES) {
        Logger::error("[StateMachine] currPhase unexpectedly null or out-of-range. Resetting to phase[0].");
        if (!phases.empty()) {
            currPhase = &phases[0];
        }
    }
}

// --- State Machine Control ---
void StateMachine::handle_state_rising() {
    switch (currentState) {
        case PUFF_COUNTING:
            Logger::info("[StateMachine] Puff attempt detected.");
            requireCurrPhase();
            hasPendingPuff = true;
            puffTimer.start();
            pendingPuff = PuffModel{};
            pendingPuff.phaseIndex = currPhase->phaseIndex;
            pendingPuff.timestampSec = epochSeconds();
            PersistenceManager::instance().recordEpoch(epochSeconds());
            break;
        case LOCKDOWN:
            Logger::error("[StateMachine] CRITICAL - Rising edge on blocked gate.");
    }
}

void StateMachine::handle_state_falling() {
    switch (currentState) {
        case PUFF_COUNTING: {
            if (!hasPendingPuff) {
                Logger::warning("[StateMachine] Falling edge detected before rising edge.");
                return;
            }
            long duration = puffTimer.getDuration();
            puffTimer.reset();
            if (duration != -1 && duration >= (MIN_PUFF_DURATION_MILLISECONDS)) {
                pendingPuff.puffDuration = (unsigned long)duration;
                pendingPuff.puffNumber = getPuffNumber();
                puffs.push_back(pendingPuff);
                currPuff = &puffs.back();
                PersistenceManager::instance().appendPuff(*currPuff);
                char ts[32];
                if (epochToTimestamp(currPuff->timestampSec, ts, sizeof(ts))) {
                    Logger::infof("[StateMachine] New Puff recorded (%d). Duration(ms): %lu ms at %s", currPuff->puffNumber, currPuff->puffDuration, ts);
                } else {
                    Logger::infof("[StateMachine] New Puff recorded (%d). Duration(ms): %lu ms at (%u)", currPuff->puffNumber, currPuff->puffDuration, currPuff->timestampSec);
                }
                BLEManager::instance().notifyNewPuff(*currPuff);
                if (currPhase) {
                    currPhase->puffsTaken++;
                    PersistenceManager::instance().updateCurrentPhasePuffsTaken((uint16_t)currPhase->phaseIndex, (uint16_t)currPhase->puffsTaken);
                    if (currPhase->puffsTaken == currPhase->maxPuffs) {
                        currentState = LOCKDOWN;
                        Logger::infof("[StateMachine] Max puffs %d reached, state changed to LOCKDOWN.", currPhase->maxPuffs);
                    }
                    else if (currPhase->puffsTaken > currPhase->maxPuffs)
                    {
                        currentState = LOCKDOWN;
                        Logger::errorf("[StateMachine] Exceeded max puffs %d, malfunction detected.", currPhase->maxPuffs);
                    }
                }
            } else {
                Logger::infof("Invalid puff duration (%ld ms); ignoring.", duration);
            }
            hasPendingPuff = false;
            pendingPuff = PuffModel{};
            break;
        }
        case LOCKDOWN: {
            Logger::error("[StateMachine] CRITICAL - Falling edge on blocked gate.");
            break;
        }
    }
}

// --- Puff/Phase Access ---
std::vector<PuffModel> StateMachine::getPuffs(uint16_t startAfter, uint8_t maxCount) const {
    std::vector<PuffModel> result;
    if (puffs.empty()) return result;

    // Use lower_bound to find the first puff with puffNumber > startAfter
    auto it = std::lower_bound(
        puffs.begin(), puffs.end(), static_cast<uint16_t>(startAfter + 1),
        [](const PuffModel& puff, uint16_t val) {
        return puff.puffNumber < val;
        }
    );

    // Collect up to maxCount puffs
    for (; it != puffs.end() && (!maxCount || result.size() < (size_t)maxCount); ++it) {
        result.push_back(*it);
    }
    return result;
}

std::vector<PhaseModel> StateMachine::getPhases(uint16_t startAfter, uint8_t maxCount) const {
    std::vector<PhaseModel> result;
    if (phases.empty()) return result;
    int endPhaseIndex = currPhase ? currPhase->phaseIndex : 1;
    for (const auto& phase : phases) {
        if (phase.phaseIndex > startAfter && phase.phaseIndex <= endPhaseIndex) {
            result.push_back(phase);
            if (maxCount && result.size() >= maxCount) break;
        }
    }
    return result;
}

// --- Phase Control ---
static bool s_lastPhaseLogMuted = false;  // prevents log spam

void StateMachine::incrementValidPhase() {
    // WARNING: unsigned integer comparison (ensure epochSeconds() is always greater)
    requireCurrPhase();
    if ((epochSeconds() - currPhase->phaseStartSec) >= currPhase->phaseDuration) {
        currentState = PUFF_COUNTING;
        if (currPhase->phaseIndex < NUM_PHASES) {
            Logger::infof("[StateMachine] Elapsed (%u) > Phase duration, incrementing from phase (%d).", (unsigned)(epochSeconds() - currPhase->phaseStartSec), currPhase->phaseIndex);
            currPhase = &phases[currPhase->phaseIndex + 1];
            currPhase->phaseStartSec = epochSeconds();
            PersistenceManager::instance().appendPhaseStart(*currPhase);
            char ts[32];
            if (epochToTimestamp(currPhase->phaseStartSec, ts, sizeof(ts))) {
                Logger::infof("[StateMachine] Phase incremented to (%d) at %s", currPhase->phaseIndex, ts);
            } else {
                Logger::infof("[StateMachine] Phase incremented to (%d) at (%u)", currPhase->phaseIndex, currPhase->phaseStartSec);
            }
            PersistenceManager::instance().recordEpoch(epochSeconds());
            BLEManager::instance().notifyNewPhase(*currPhase);
        } else {
            if (!s_lastPhaseLogMuted) {
                Logger::warningf("[StateMachine] Already at last phase (%d), cannot increment.", currPhase->phaseIndex);
                s_lastPhaseLogMuted = true;
            }
        };
    }
}

// --- Reconstruction from storage ---
void StateMachine::reconstructFromStorage() {
    // Rebuild phases data from storage (track if anything was loaded)
    bool loadedAnyPhase = false;
    PersistenceManager::instance().forEachPhase([this, &loadedAnyPhase](const PersistenceManager::PhaseRecord& rec){
        if (rec.phaseIndex < phases.size()) {
            phases[rec.phaseIndex].phaseStartSec = rec.startSec;
            phases[rec.phaseIndex].maxPuffs = rec.maxPuffs;
            phases[rec.phaseIndex].puffsTaken = rec.puffsTaken;
            currPhase = &phases[rec.phaseIndex];
            loadedAnyPhase = true;
        }
    });

    requireCurrPhase();

    // Rebuild puff list from storage
    puffs.clear();
    PersistenceManager::instance().forEachPuff([this](const PersistenceManager::PuffRecord& rec){
        PuffModel pm; pm.puffNumber = rec.puffNumber; pm.phaseIndex = rec.phaseIndex; pm.puffDuration = rec.durationMs; pm.timestampSec = rec.tSec; puffs.push_back(pm); });
    if (!puffs.empty()) { currPuff = &puffs.back(); }

    // If nothing was loaded at all, keep constructor-initialized defaults
    if (!loadedAnyPhase && puffs.empty()) {
        currentState = PUFF_COUNTING;
        Logger::infof("[StateMachine] No persisted data. Using defaults. Current Phase: %d, Current Puff: %d", currPhase->phaseIndex, currPuff ? currPuff->puffNumber : 0);
        return;
    }

    // Determine current state
    int idx = currPhase->phaseIndex;
    currentState = (phases[idx].puffsTaken >= phases[idx].maxPuffs) ? LOCKDOWN : PUFF_COUNTING;
    Logger::infof("[StateMachine] Reconstruction complete. Current Phase: %d, Current Puff: %d", currPhase->phaseIndex, currPuff ? currPuff->puffNumber : 0);
}