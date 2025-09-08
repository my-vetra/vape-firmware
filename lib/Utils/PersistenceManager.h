
#pragma once

/**
 * @file PersistenceManager.h
 * @brief Handles non-volatile storage and retrieval of device data (puffs, phases, epochs).
 *
 * Provides APIs for appending and iterating over persistent records, as well as CRC and NVS management.
 *
 * @author Vetra Firmware Team
 * @copyright Copyright (c) 2025 Vetra
 */

// --- Standard Library Includes ---
#include <Arduino.h>
#include <functional>
#include <memory>
#include <utility>

// --- ESP32 NVS Includes ---
#include <nvs.h>
#include <nvs_flash.h>
#include <esp_err.h>

// --- Project Includes ---
#include "Logger.h"
#include "StateMachine.h"

// -----------------------------------------------------------------------------
// Persistence Constants (CRC, NVS, Channels, Block Sizes)
// -----------------------------------------------------------------------------

/// @name CRC Constants
///@{
constexpr int CRC_ITER = 8;                  ///< Number of CRC iterations
constexpr uint32_t CRC_POLY = 0xEDB88320;    ///< CRC-32 polynomial
///@}

/// @name NVS Keys and Channels
///@{
static constexpr const char* NAMESPACE = "persist";           ///< NVS namespace for persistence
static constexpr const char* KEY_SLEEP_EPOCH = "sleep_epoch"; ///< Key for storing last sleep epoch
static constexpr uint8_t PUFF_CH = 0;                         ///< Puff channel index
static constexpr uint8_t PHASE_CH = 1;                        ///< Phase channel index
static constexpr uint8_t CHANNEL_COUNT = 2;                   ///< Number of channels
///@}

/// @name Block Capacities
///@{
static constexpr uint16_t PUFF_BLOCK_CAP = 32;  ///< Puffs per block
static constexpr uint16_t PHASE_BLOCK_CAP = 16; ///< Phases per block
///@}

/**
 * @class PersistenceManager
 * @brief Singleton class for managing persistent storage of puffs, phases, and epochs.
 *
 * Handles NVS initialization, record appending, and iteration for device data.
 */
class PersistenceManager {
public:
    /**
     * @brief Get singleton instance of PersistenceManager.
     */
    static PersistenceManager& instance();

    /**
     * @brief Initialize NVS and internal state.
     */
    void init();

    /**
     * @brief Puff record structure (packed).
     */
    struct PuffRecord {
        uint32_t tSec;
        uint32_t durationSec;
        uint16_t puffNumber;
        uint16_t phaseIndex;
    } __attribute__((packed));

    /**
     * @brief Phase record structure (packed).
     */
    struct PhaseRecord {
        uint32_t startSec;
        uint16_t phaseIndex;
        uint16_t maxPuffs;
        uint16_t puffsTaken;
    } __attribute__((packed));

    /**
     * @brief Append a new puff record to persistent storage.
     */
    void appendPuff(const PuffModel& puff);

    /**
     * @brief Append a new phase start record to persistent storage.
     */
    void appendPhaseStart(const PhaseModel& phase);

    /**
     * @brief Update the number of puffs taken for the current phase.
     */
    void updateCurrentPhasePuffsTaken(uint16_t phaseIndex, uint16_t puffsTaken);

    // -------------------------------------------------------------------------
    // Epoch Persistence (migrated from former TimeStore)
    // -------------------------------------------------------------------------

    /**
     * @brief Record the current epoch (seconds since boot).
     * @return True if successful.
     */
    bool recordEpoch(uint32_t epochSec);

    /**
     * @brief Get the last recorded epoch, or fallback if not found.
     * @param fallback Value to return if no epoch is stored.
     * @return Last epoch in seconds.
     */
    uint32_t getLastEpoch(uint32_t fallback = 0);

    /**
     * @brief Iterate over all stored puff records, invoking callback for each.
     */
    void forEachPuff(const std::function<void(const PuffRecord&)>& cb);

    /**
     * @brief Iterate over all stored phase records, invoking callback for each.
     */
    void forEachPhase(const std::function<void(const PhaseRecord&)>& cb);

private:
    PersistenceManager();
    ~PersistenceManager() = default;
    PersistenceManager(const PersistenceManager&) = delete;
    PersistenceManager& operator=(const PersistenceManager&) = delete;

    // Channel meta for each persisted channel (puffs/phases)
    struct ChannelMeta {
        uint32_t magic;          // 'PMCH'
        uint16_t recordSize;     // sizeof(record)
        uint16_t blockCapacity;  // records per block
        uint16_t activeBlockIndex; // current block index
        uint16_t activeCount;      // records used in active block
        uint32_t totalRecords;     // total records persisted
    } __attribute__((packed));

    struct GlobalMeta {
        uint32_t magic;       // 'PMTA'
        uint16_t version;     // 1
        uint16_t channelCount;
        ChannelMeta channels[CHANNEL_COUNT];
        uint32_t crc32;       // over everything except crc32
    } __attribute__((packed));

    GlobalMeta meta;
    bool metaLoaded;

    // Active blocks in RAM
    uint8_t puffBlock[PUFF_BLOCK_CAP * sizeof(PuffRecord)];
    uint8_t phaseBlock[PHASE_BLOCK_CAP * sizeof(PhaseRecord)];
    bool puffBlockLoaded;
    bool phaseBlockLoaded;

    void ensureInit();
    void loadMeta();
    void saveMeta();
    void loadActiveBlock(uint8_t ch);
    void saveActiveBlock(uint8_t ch);
    void rotateBlock(uint8_t ch);
    uint32_t computeCrc(const void* d, size_t len) const;

    uint8_t* getBlockPtr(uint8_t ch) { return (ch==PUFF_CH)? puffBlock : phaseBlock; }
    ChannelMeta& chMeta(uint8_t ch) { return meta.channels[ch]; }

};
