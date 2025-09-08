
#pragma once

/**
 * @file StateMachine.h
 * @brief Manages device states, transitions, and puff/phase tracking.
 *
 * Provides APIs for state transitions, puff/phase access, and BLE data framing.
 *
 * @author Vetra Firmware Team
 * @copyright Copyright (c) 2025 Vetra
 */

// --- Standard Library Includes ---
#include <Arduino.h>
#include <cstdint>
#include <vector>

// -----------------------------------------------------------------------------
// State Machine Constants
// -----------------------------------------------------------------------------

/// @name State Machine Constants
///@{
#ifndef MAX_PUFFS
#define MAX_PUFFS                         20            ///< Maximum puffs per phase
#endif
#define NUM_PHASES                        5             ///< Number of phases
// Minimum puff duration (in seconds). Set to 0 for tests; recommended >= 1 in production.
#ifndef MIN_PUFF_DURATION_SECONDS
#define MIN_PUFF_DURATION_SECONDS         1
#endif
// Phase duration (in seconds). Default: 1 hour.
#ifndef PHASE_DURATION_SECONDS
#define PHASE_DURATION_SECONDS            (3600)
#endif
///@}

// -----------------------------------------------------------------------------
// State Machine Types
// -----------------------------------------------------------------------------

/**
 * @enum state_t
 * @brief State machine states.
 */
enum state_t {
    PUFF_COUNTING, ///< Counting puffs
    LOCKDOWN       ///< Lockdown state
};

/**
 * @struct PuffModel
 * @brief Model for a single puff event.
 */
struct PuffModel {
    int puffNumber;             ///< Puff number
    uint32_t timestampSec;      ///< Timestamp (seconds)
    unsigned long puffDuration; ///< Puff duration (seconds)
    int phaseIndex;             ///< Associated phase index
};

/**
 * @struct PhaseModel
 * @brief Model for a single phase event.
 */
struct PhaseModel {
    int phaseIndex;             ///< Phase index
    unsigned long phaseDuration;///< Phase duration (seconds)
    uint32_t phaseStartSec;     ///< Phase start time (seconds)
    int maxPuffs;               ///< Max puffs in phase
    int puffsTaken;             ///< Puffs taken in phase
};

// -----------------------------------------------------------------------------
// StateMachine Class
// -----------------------------------------------------------------------------

/**
 * @class StateMachine
 * @brief Singleton class managing device state transitions and puff/phase tracking.
 */
class StateMachine {
public:
    // Singleton
    static StateMachine& instance();

    // State transitions
    void handle_state_rising();
    void handle_state_falling();
    void incrementValidPhase();

    // Puff/Phase Access
    const PuffModel* getAllPuffs() const { return puffs.data(); }
    const PhaseModel* getAllPhases() const { return phases.data(); }
    size_t getPuffsCount() const { return puffs.size(); }
    size_t getPhasesCount() const { return phases.size(); }
    state_t getCurrentState() const { return currentState; }

    // Reconstruction
    void reconstructFromStorage();

    // BLEManager API helpers
    std::vector<PuffModel> getPuffs(uint16_t startAfter, uint8_t maxCount) const;
    std::vector<PhaseModel> getPhases(uint16_t startAfter, uint8_t maxCount) const;
    bool hasCurrentPuff() const { return currPuff != nullptr; }
    PuffModel currentPuff() const { return currPuff ? *currPuff : PuffModel{}; }
    bool hasCurrentPhase() const { return currPhase != nullptr; }
    PhaseModel currentPhase() const { return currPhase ? *currPhase : PhaseModel{}; }

private:
    // Puff timer
    class PuffTimer {
    public:
        PuffTimer();
        void start();
        void stop();
        long getDuration() const;
        void reset();

    private:
        unsigned long startTime;
        unsigned long endTime;
        bool active;
    };
    PuffTimer puffTimer;

    // Internal state
    StateMachine();
    ~StateMachine();
    std::vector<PhaseModel> phases;
    std::vector<PuffModel> puffs;
    int getPuffNumber() const { return static_cast<int>(puffs.size()) + 1; }
    state_t currentState;
    // Internal current pointers (not exposed directly)
    PuffModel* currPuff;
    PhaseModel* currPhase;
    PuffModel pendingPuff;
    bool hasPendingPuff = false;

    void requireCurrPhase();
};
