
#pragma once

/**
 * @file BLEManager.h
 * @brief BLEManager handles Bluetooth Low Energy server, characteristics, and notifications for the device.
 *
 * This class manages BLE service setup, characteristic notifications, and client subscriptions.
 *
 * @author Vetra Firmware Team
 * @copyright Copyright (c) 2025 Vetra
 */

// --- Standard Library Includes ---
#include <Arduino.h>
#include <memory>

// --- ESP32 BLE Library Includes ---
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLECharacteristic.h>
#include <BLEDescriptor.h>
#include <BLEAdvertising.h>

// --- Project Includes ---
#include "Logger.h"
#include "StateMachine.h"

// -----------------------------------------------------------------------------
// BLE Constants (UUIDs, MTU, Timeouts)
// -----------------------------------------------------------------------------

/// @name BLE UUIDs (Service and Characteristics)
///@{
#define SERVICE_UUID        "56a63ec7-0623-4242-9a66-f2ad8f9f270b"   ///< Main BLE service UUID
#define NTP_CHAR_UUID       "c8646c82-aa4b-4ac8-b6d5-cb45677ebcaa" ///< NTP time sync characteristic UUID
#define KEEPALIVE_CHAR_UUID "ac4678ba-8131-4a70-8ffd-a7c7f0ed23b0" ///< Keepalive characteristic UUID
#define PUFFS_CHAR_UUID     "cedf9ce5-2953-4d18-b38c-100a3a90f987" ///< Puff data characteristic UUID
#define PHASES_CHAR_UUID    "9016b7fe-7192-40ce-8a83-451fc2ae5a97" ///< Phase data characteristic UUID
#define LOGGER_CHAR_UUID    "332e04f5-7a8a-491d-a730-f4748a6116e2" ///< Logger characteristic UUID
///@}

#define PEER_MTU            185   ///< Default peer MTU size

/// @brief Timeout in ms for BLE inactivity
static constexpr uint32_t BLE_TIMEOUT = 60 * 1000UL;

/// @brief Max number of log lines to send per pumpLogs() call
#define kBurst 5

// -----------------------------------------------------------------------------
// BLEManager Class
// -----------------------------------------------------------------------------

/**
 * @class BLEManager
 * @brief Singleton class managing BLE server, characteristics, and notifications.
 *
 * Handles BLE service lifecycle, client subscriptions, and notification framing for puffs, phases, and logs.
 */
class BLEManager {
public:
    // -------------------------------------------------------------------------
    // Framing Constants (Puffs / Phases)
    // -------------------------------------------------------------------------

    /// @name Puff/Phase Framing
    ///@{
    static constexpr size_t PUFF_FRAME_MAX  = PEER_MTU - 3; ///< Max puff frame payload
    static constexpr size_t PUFF_HEADER     = 4;            ///< Puff frame header size (type + firstPuff(2) + count)
    static constexpr size_t PUFF_ENTRY      = 9;            ///< Puff entry size (puffNumber(2) + timestamp(4) + duration(2) + phase(1))
    static constexpr size_t PHASE_FRAME_MAX = PEER_MTU - 3; ///< Max phase frame payload
    static constexpr size_t PHASE_HEADER    = 4;            ///< Phase frame header size (type + firstPhase(2) + count)
    static constexpr size_t PHASE_ENTRY     = 5;            ///< Phase entry size (phaseIndex(1) + startSec(4))
    ///@}

    /**
     * @brief Notify BLE client of a new puff event.
     * @param puff PuffModel containing puff data.
     */
    void notifyNewPuff(const PuffModel& puff);

    /**
     * @brief Notify BLE client of a new phase event.
     * @param phase PhaseModel containing phase data.
     */
    void notifyNewPhase(const PhaseModel& phase);

    /**
     * @brief Pump any queued logs over BLE (called in loop or on subscription/connect).
     */
    void pumpLogs();

    /**
     * @brief Get singleton instance of BLEManager.
     */
    static BLEManager& instance();

    /**
     * @brief Query whether a client has subscribed to the logger characteristic.
     * @return True if logger notifications are enabled.
     */
    bool isLoggerSubscribed() const { return loggerSubscribed; }

    /**
     * @brief Set subscription status for logger notifications (internal CCCD callback).
     * @param subscribed True if client subscribed.
     */
    void setSubscriptionStatus(bool subscribed);

    /**
     * @brief Start BLE service and advertising.
     */
    void startService();

    /**
     * @brief Cleanup BLE service and release resources.
     */
    void cleanupService();

    /**
     * @brief Query if BLE is currently active.
     * @return True if BLE is enabled.
     */
    bool isActive() const { return bleEnabled; }

    /**
     * @brief Update last interaction timestamp (for timeout tracking).
     */
    void updateInteraction();

    /**
     * @brief Check if BLE connection has timed out.
     * @return True if timeout has occurred.
     */
    bool connectionTimeOut() const;

    /**
     * @brief Get maximum payload available for a single notify/indicate (negotiated MTU minus ATT header).
     * @return Payload size in bytes (fallback to 20 if unknown).
     */
    size_t maxNotifyPayload() const;

    // ...existing code...
    void setLoggerCccd(bool notifyEnabled, bool indicateEnabled) {
        loggerNotifyEnabled = notifyEnabled;
        loggerIndicateEnabled = indicateEnabled;
        loggerSubscribed = (notifyEnabled || indicateEnabled);
    }
    // Expose indication preferences for Puffs/Phases (used by callbacks)
    bool usePuffsIndicate() const { return puffsIndicateEnabled; }
    bool usePhasesIndicate() const { return phasesIndicateEnabled; }
    void setPuffsCccd(bool notifyEnabled, bool indicateEnabled) {
        puffsNotifyEnabled = notifyEnabled; puffsIndicateEnabled = indicateEnabled;
    }
    void setPhasesCccd(bool notifyEnabled, bool indicateEnabled) {
        phasesNotifyEnabled = notifyEnabled; phasesIndicateEnabled = indicateEnabled;
    }

private:
    BLEManager();
    ~BLEManager();
    BLEManager(const BLEManager&) = delete;
    BLEManager& operator=(const BLEManager&) = delete;

    BLEServer* pServer;
    BLECharacteristic* ntpChar;
    BLECharacteristic* puffsChar;
    BLECharacteristic* phasesChar;
    BLECharacteristic* loggerChar;
    BLECharacteristic* keepAliveChar;
    bool bleEnabled;
    unsigned long lastInteractionTime;
    bool loggerSubscribed = false;
    bool loggerNotifyEnabled = false;
    bool loggerIndicateEnabled = false;
    bool puffsNotifyEnabled = false;
    bool puffsIndicateEnabled = false;
    bool phasesNotifyEnabled = false;
    bool phasesIndicateEnabled = false;

    // Inline little-endian writers
    static inline void writeLE(uint8_t* buf, uint16_t val) {
        buf[0] = val & 0xFF; buf[1] = (val >> 8) & 0xFF;
    }
    static inline void writeLE32(uint8_t* buf, uint32_t val) {
        buf[0] = val & 0xFF; buf[1] = (val >> 8) & 0xFF; buf[2] = (val >> 16) & 0xFF; buf[3] = (val >> 24) & 0xFF;
    }
    // Inline helper to send a standard one-byte done frame (0x02) with logging
    static inline void sendDone(BLECharacteristic* c, const char* label) {
        if (!c) return;
        uint8_t doneFrame[1] = {0x02};
        c->setValue(doneFrame, 1);
        c->notify();
        Logger::infof("[BLEManager] Sent %s done frame.", label);
    }

    class MyServerCallbacks : public BLEServerCallbacks {
    public:
        MyServerCallbacks();
        void onConnect(BLEServer* pServer) override;
        void onDisconnect(BLEServer* pServer) override;
    };
    // NTP characteristic callbacks
    class NTPCallbacks : public BLECharacteristicCallbacks {
    public:
        NTPCallbacks();
        void onWrite(BLECharacteristic* pCharacteristic) override;
    };
    class PuffsCallbacks : public BLECharacteristicCallbacks {
    public:
        PuffsCallbacks();
        void onWrite(BLECharacteristic* pCharacteristic) override;
    };
    class PhasesCallbacks : public BLECharacteristicCallbacks {
    public:
        PhasesCallbacks();
        void onWrite(BLECharacteristic* pCharacteristic) override;
    };
    class KeepAliveCallbacks : public BLECharacteristicCallbacks {
    public:
        KeepAliveCallbacks();
        void onRead(BLECharacteristic* pCharacteristic) override;
    };
    // Descriptor callbacks for CCCD (0x2902)
    class LoggerCccdCallbacks : public BLEDescriptorCallbacks {
    public:
        void onWrite(BLEDescriptor* pDescriptor) override;
    };
    class PuffsCccdCallbacks : public BLEDescriptorCallbacks {
    public:
        void onWrite(BLEDescriptor* pDescriptor) override;
    };
    class PhasesCccdCallbacks : public BLEDescriptorCallbacks {
    public:
        void onWrite(BLEDescriptor* pDescriptor) override;
    };
    // Callback instances managed by smart pointers for automatic cleanup
    std::unique_ptr<MyServerCallbacks> serverCallbacks;
    std::unique_ptr<NTPCallbacks> ntpCallbacks;
    std::unique_ptr<PuffsCallbacks> puffsCallbacks;
    std::unique_ptr<PhasesCallbacks> phasesCallbacks;
    std::unique_ptr<KeepAliveCallbacks> keepAliveCallbacks;
    std::unique_ptr<LoggerCccdCallbacks> loggerCccdCallbacks;
    std::unique_ptr<PuffsCccdCallbacks> puffsCccdCallbacks;
    std::unique_ptr<PhasesCccdCallbacks> phasesCccdCallbacks;
};