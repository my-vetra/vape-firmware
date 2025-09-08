
#include <memory>
#include <utility>
#if __cplusplus <= 201103L
namespace std {
    template <typename T, typename... Args>
    std::unique_ptr<T> make_unique(Args&&... args) {
        return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
    }
}
#endif
#include "BLEManager.h"
#include "Timer.h"
#include "LogBuffer.h"
#include <BLE2902.h>
#include <cstring>
#include <algorithm>

// -----------------------------------------------------------------------------
// BLEManager Singleton and Core State
// -----------------------------------------------------------------------------

BLEManager& BLEManager::instance() { static BLEManager inst; return inst; }

BLEManager::BLEManager() : pServer(nullptr), ntpChar(nullptr), puffsChar(nullptr), phasesChar(nullptr), loggerChar(nullptr), keepAliveChar(nullptr), bleEnabled(false) {}
BLEManager::~BLEManager() { cleanupService(); }

// -----------------------------------------------------------------------------
// BLE Service Lifecycle
// -----------------------------------------------------------------------------

void BLEManager::startService() {
    bleEnabled = true;
    BLEDevice::init("Vetra");
    pServer = BLEDevice::createServer();
    serverCallbacks = std::make_unique<MyServerCallbacks>();
    pServer->setCallbacks(serverCallbacks.get());
    BLEService* service = pServer->createService(SERVICE_UUID);

    // --- Characteristic Setup ---
    // NTP characteristic: Write
    ntpChar = service->createCharacteristic(
        NTP_CHAR_UUID,
        BLECharacteristic::PROPERTY_WRITE
    );
    ntpCallbacks = std::make_unique<NTPCallbacks>();
    ntpChar->setCallbacks(ntpCallbacks.get());

    // Puffs characteristic: Write With Response + Notify + Indicate
    puffsChar = service->createCharacteristic(
        PUFFS_CHAR_UUID,
        BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_INDICATE
    );
    puffsCallbacks = std::make_unique<PuffsCallbacks>();
    puffsChar->setCallbacks(puffsCallbacks.get());
    auto puffsCccd = std::make_unique<BLE2902>();
    puffsCccdCallbacks = std::make_unique<PuffsCccdCallbacks>();
    puffsCccd->setCallbacks(puffsCccdCallbacks.get());
    puffsChar->addDescriptor(puffsCccd.release());

    // Phases characteristic: Write With Response + Notify + Indicate
    phasesChar = service->createCharacteristic(
        PHASES_CHAR_UUID,
        BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_INDICATE
    );
    phasesCallbacks = std::make_unique<PhasesCallbacks>();
    phasesChar->setCallbacks(phasesCallbacks.get());
    auto phasesCccd = std::make_unique<BLE2902>();
    phasesCccdCallbacks = std::make_unique<PhasesCccdCallbacks>();
    phasesCccd->setCallbacks(phasesCccdCallbacks.get());
    phasesChar->addDescriptor(phasesCccd.release());

    // Logger characteristic: notify/indicate only (no READ)
    loggerChar = service->createCharacteristic(
        LOGGER_CHAR_UUID,
        BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_INDICATE
    );
    auto loggerCccd = std::make_unique<BLE2902>();
    loggerCccdCallbacks = std::make_unique<LoggerCccdCallbacks>();
    loggerCccd->setCallbacks(loggerCccdCallbacks.get());
    loggerChar->addDescriptor(loggerCccd.release());

    // KeepAlive characteristic: Read
    keepAliveChar = service->createCharacteristic(
        KEEPALIVE_CHAR_UUID,
        BLECharacteristic::PROPERTY_READ
    );
    keepAliveCallbacks = std::make_unique<KeepAliveCallbacks>();
    keepAliveChar->setCallbacks(keepAliveCallbacks.get());

    service->start();
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(false);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
    updateInteraction();
    Logger::info("[BLEManager] BLE service started and advertising (iOS spec).");
}

void BLEManager::cleanupService() {
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    if (pAdvertising) pAdvertising->stop();
    BLEDevice::deinit();
    pServer = nullptr;
    ntpChar = nullptr;
    keepAliveChar = nullptr;
    loggerChar = nullptr;
    loggerSubscribed = false;
    loggerNotifyEnabled = false;
    loggerIndicateEnabled = false;
    puffsChar = nullptr;
    phasesChar = nullptr;
    bleEnabled = false;
    // Smart pointers handle cleanup automatically
    Logger::info("[BLEManager] BLE service cleaned up.");
}

// -----------------------------------------------------------------------------
// BLE Connection/Timeout/Interaction
// -----------------------------------------------------------------------------

bool BLEManager::connectionTimeOut() const {
    return (millis() - lastInteractionTime > BLE_TIMEOUT);
}

void BLEManager::updateInteraction() {
    lastInteractionTime = millis();
}

// -----------------------------------------------------------------------------
// BLEManager Callback Implementations
// -----------------------------------------------------------------------------

// --- Server Callbacks ---
BLEManager::MyServerCallbacks::MyServerCallbacks() {}
void BLEManager::MyServerCallbacks::onConnect(BLEServer* pServer) {
    BLEManager::instance().updateInteraction();
    Logger::info("[BLEManager] BLE client connected.");
}
void BLEManager::MyServerCallbacks::onDisconnect(BLEServer* pServer) {
    BLEManager::instance().setSubscriptionStatus(false);
    BLEDevice::startAdvertising();
    Logger::info("[BLEManager] BLE client disconnected, advertising restarted.");
}

// --- NTP Characteristic Callbacks ---
BLEManager::NTPCallbacks::NTPCallbacks() {}
void BLEManager::NTPCallbacks::onWrite(BLECharacteristic* pCharacteristic) {
    BLEManager::instance().updateInteraction();
    std::string value = pCharacteristic->getValue();
    if (value.size() != 4) {
        Logger::warningf("[BLEManager] NTP write invalid length: %u", (unsigned)value.size());
        return;
    }
    // Interpret as little-endian epoch seconds
    const uint8_t* b = reinterpret_cast<const uint8_t*>(value.data());
    uint32_t epoch = (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
    char ts[32];
    if (epochToTimestamp(epoch, ts, sizeof(ts))) {
        Logger::infof("[BLEManager] NTP epoch received (LE): %s (%u)", ts, epoch);
    } else {
        Logger::infof("[BLEManager] NTP epoch received (LE): %u", epoch);
    }
    if (!updateSystemTime(epoch)) {
        Logger::error("[BLEManager] NTP update failed.");
    } else {
        Logger::info("[BLEManager] NTP update complete.");
    }
}

// --- Puffs Characteristic Callbacks ---
BLEManager::PuffsCallbacks::PuffsCallbacks() {}
void BLEManager::PuffsCallbacks::onWrite(BLECharacteristic* pCharacteristic) {
    BLEManager::instance().updateInteraction();
    std::string value = pCharacteristic->getValue();
    if (value.size() != 4 || value[0] != 0x10) {
        Logger::info("[BLEManager] Invalid Puffs request format.");
        return;
    }
    uint16_t startAfter = value[1] | (value[2] << 8);
    uint8_t maxCount = value[3];
    Logger::infof("[BLEManager] Puffs request: startAfter=%u, maxCount=%u", startAfter, maxCount);
    // Derive capacity from framing constants
    const size_t CAPACITY = (BLEManager::PUFF_FRAME_MAX - BLEManager::PUFF_HEADER) / BLEManager::PUFF_ENTRY;
    if (maxCount == 0 || maxCount > CAPACITY) maxCount = (uint8_t)CAPACITY; // 0 => full capacity

    StateMachine& puff_counter_sm = StateMachine::instance();
    std::vector<PuffModel> puffs = puff_counter_sm.getPuffs(startAfter, maxCount);
    if (puffs.empty()) { BLEManager::sendDone(pCharacteristic, "Puffs"); return; }
    uint16_t firstPuffNumber = puffs[0].puffNumber;
    uint8_t payload[BLEManager::PUFF_FRAME_MAX - BLEManager::PUFF_HEADER] = {0};
    size_t payloadIdx = 0; uint8_t encoded = 0;
    for (auto &pf : puffs) {
        if (encoded >= maxCount) break;
        if (payloadIdx + BLEManager::PUFF_ENTRY > sizeof(payload)) break; // (redundant) guard
        BLEManager::writeLE(&payload[payloadIdx], pf.puffNumber);
        BLEManager::writeLE32(&payload[payloadIdx + 2], (uint32_t)pf.timestampSec);
        // Puff duration is seconds; frame stores it as uint16
        BLEManager::writeLE(&payload[payloadIdx + 6], (uint16_t)pf.puffDuration);
        payload[payloadIdx + 8] = (uint8_t)pf.phaseIndex;
        payloadIdx += BLEManager::PUFF_ENTRY;
        encoded++;
    }
    uint8_t frame[BLEManager::PUFF_FRAME_MAX] = {0};
    frame[0] = 0x01;
    BLEManager::writeLE(&frame[1], firstPuffNumber);
    frame[3] = encoded;
    memcpy(&frame[BLEManager::PUFF_HEADER], payload, payloadIdx);
    size_t frameLen = BLEManager::PUFF_HEADER + payloadIdx;
    pCharacteristic->setValue(frame, frameLen);
    if (BLEManager::instance().usePuffsIndicate()) pCharacteristic->indicate(); else pCharacteristic->notify();
    BLEManager::instance().updateInteraction();
    Logger::infof("[BLEManager] Sent Puffs batch: requested=%u encoded=%u", (unsigned)puffs.size(), encoded);
}

// --- Phases Characteristic Callbacks ---
BLEManager::PhasesCallbacks::PhasesCallbacks() {}
void BLEManager::PhasesCallbacks::onWrite(BLECharacteristic* pCharacteristic) {
    BLEManager::instance().updateInteraction();
    std::string value = pCharacteristic->getValue();
    if (value.size() != 4 || value[0] != 0x10) {
        Logger::info("[BLEManager] Invalid Phases request format.");
        return;
    }
    uint16_t startAfter = value[1] | (value[2] << 8);
    uint8_t maxCount = value[3];
    Logger::infof("[BLEManager] Phases request: startAfter=%u, maxCount=%u", startAfter, maxCount);
    const size_t CAPACITY = (BLEManager::PHASE_FRAME_MAX - BLEManager::PHASE_HEADER) / BLEManager::PHASE_ENTRY;
    if (maxCount == 0 || maxCount > CAPACITY) maxCount = (uint8_t)CAPACITY; // 0 => full capacity

    // Fetch bounded set of phases
    StateMachine& puff_counter_sm = StateMachine::instance();
    std::vector<PhaseModel> phases = puff_counter_sm.getPhases(startAfter, maxCount);
    if (phases.empty()) { BLEManager::sendDone(pCharacteristic, "Phases"); return; }
    uint16_t firstPhaseIndex = phases[0].phaseIndex;
    uint8_t payload[BLEManager::PHASE_FRAME_MAX - BLEManager::PHASE_HEADER] = {0};
    size_t payloadIdx = 0;
    uint8_t encoded = 0;
    for (auto &ph : phases) {
        if (encoded >= maxCount) break;
        if (payloadIdx + BLEManager::PHASE_ENTRY > sizeof(payload)) break; // (redundant) guard
        payload[payloadIdx] = (uint8_t)ph.phaseIndex;
        BLEManager::writeLE32(&payload[payloadIdx + 1], (uint32_t)ph.phaseStartSec);
        payloadIdx += BLEManager::PHASE_ENTRY;
        encoded++;
    }
    uint8_t frame[BLEManager::PHASE_FRAME_MAX] = {0};
    frame[0] = 0x01;
    BLEManager::writeLE(&frame[1], firstPhaseIndex);
    frame[3] = encoded;
    memcpy(&frame[4], payload, payloadIdx);
    size_t frameLen = BLEManager::PHASE_HEADER + payloadIdx;
    pCharacteristic->setValue(frame, frameLen);
    if (BLEManager::instance().usePhasesIndicate()) pCharacteristic->indicate(); else pCharacteristic->notify();
    Logger::infof("[BLEManager] Sent Phases batch: requested=%u encoded=%u", (unsigned)phases.size(), encoded);
}

// --- KeepAlive Characteristic Callbacks ---
BLEManager::KeepAliveCallbacks::KeepAliveCallbacks() {}
void BLEManager::KeepAliveCallbacks::onRead(BLECharacteristic* pCharacteristic) {
    BLEManager::instance().updateInteraction();
    Logger::info("[BLEManager] KeepAlive read request received.");
    uint8_t response[2] = {0x01, 0x00};
    pCharacteristic->setValue(response, sizeof(response));
}

// -----------------------------------------------------------------------------
// Notification, Logging, and CCCD State
// -----------------------------------------------------------------------------

void BLEManager::pumpLogs() {
    if (!loggerChar || !loggerSubscribed) return;
#if LOG_LEVEL < 1
    return; // In release builds with LOG_LEVEL 0, skip pumping
#endif
    const size_t maxPayload = maxNotifyPayload();
    int sent = 0;
    std::string line;
    while (sent < kBurst && LogBuffer::instance().pop(line)) {
        updateInteraction();
        size_t offset = 0;
        while (offset < line.size() && sent < kBurst) {
            size_t chunk = std::min(maxPayload, line.size() - offset);
            loggerChar->setValue((uint8_t*)line.data() + offset, chunk);
            if (loggerIndicateEnabled) {
                loggerChar->indicate();
            } else {
                loggerChar->notify();
            }
            offset += chunk;
            sent++;
        }
    }
}

void BLEManager::setSubscriptionStatus(bool subscribed) {
    loggerSubscribed = subscribed;
}

// --- CCCD Callbacks ---
void BLEManager::PuffsCccdCallbacks::onWrite(BLEDescriptor* pDescriptor) {
    uint8_t* val = pDescriptor->getValue();
    bool notifyEn = false;
    bool indicateEn = false;
    if (val) {
        notifyEn   = (val[0] & 0x01) != 0;
        indicateEn = (val[0] & 0x02) != 0;
    }
    Logger::infof("[BLEManager] Puffs CCCD updated: notify=%s indicate=%s", notifyEn ? "true" : "false", indicateEn ? "true" : "false");
    BLEManager::instance().setPuffsCccd(notifyEn, indicateEn);
    if (notifyEn || indicateEn) {
        StateMachine& sm = StateMachine::instance();
        if (sm.hasCurrentPuff()) {
            PuffModel p = sm.currentPuff();
            Logger::infof("[BLEManager] Pushed Puff (%d).", p.puffNumber);
            BLEManager::instance().notifyNewPuff(p);
        }
    }
}

void BLEManager::PhasesCccdCallbacks::onWrite(BLEDescriptor* pDescriptor) {
    uint8_t* val = pDescriptor->getValue();
    bool notifyEn = false;
    bool indicateEn = false;
    if (val) {
        notifyEn   = (val[0] & 0x01) != 0;
        indicateEn = (val[0] & 0x02) != 0;
    }
    BLEManager::instance().setPhasesCccd(notifyEn, indicateEn);
    Logger::infof("[BLEManager] Phases CCCD updated: notify=%s indicate=%s", notifyEn?"true":"false", indicateEn?"true":"false");
    if (notifyEn || indicateEn) {
        StateMachine& sm = StateMachine::instance();
        if (sm.hasCurrentPhase()) {
            PhaseModel ph = sm.currentPhase();
            Logger::infof("[BLEManager] Pushed Phase (%d).", ph.phaseIndex);
            BLEManager::instance().notifyNewPhase(ph);
        }
    }
}

void BLEManager::LoggerCccdCallbacks::onWrite(BLEDescriptor* pDescriptor) {
    uint8_t* val = pDescriptor->getValue();
    bool notifyEn = false;
    bool indicateEn = false;
    if (val) {
        notifyEn   = (val[0] & 0x01) != 0;
        indicateEn = (val[0] & 0x02) != 0;
    }
    BLEManager& mgr = BLEManager::instance();
    mgr.setSubscriptionStatus(notifyEn || indicateEn);
    mgr.setLoggerCccd(notifyEn, indicateEn);
    Logger::infof("[BLEManager] Logger CCCD updated: notify=%s indicate=%s",notifyEn ? "true" : "false", indicateEn ? "true" : "false");
}

// -----------------------------------------------------------------------------
// Notification Helpers
// -----------------------------------------------------------------------------

void BLEManager::notifyNewPuff(const PuffModel& puff) {
    if (!puffsChar) return;
    // Batch-of-one using standard batch framing:
    // Header (4): [type=0x01][firstPuff(2)][count=1]
    // Entry  (9): [puffNumber(2)][timestamp(4)][duration(2)][phaseIndex(1)]
    uint8_t frame[BLEManager::PUFF_HEADER + BLEManager::PUFF_ENTRY] = {0};
    frame[0] = 0x01;
    BLEManager::writeLE(&frame[1], puff.puffNumber); // first puff number
    frame[3] = 1; // count
    BLEManager::writeLE(&frame[BLEManager::PUFF_HEADER + 0], puff.puffNumber);
    BLEManager::writeLE32(&frame[BLEManager::PUFF_HEADER + 2], (uint32_t)puff.timestampSec);
    BLEManager::writeLE(&frame[BLEManager::PUFF_HEADER + 6], (uint16_t)puff.puffDuration);
    frame[BLEManager::PUFF_HEADER + 8] = (uint8_t)puff.phaseIndex;
    puffsChar->setValue(frame, sizeof(frame));
    if (usePuffsIndicate()) puffsChar->indicate(); else puffsChar->notify();
    Logger::infof("[BLEManager] Live Puff notified (%d).", puff.puffNumber);
}

void BLEManager::notifyNewPhase(const PhaseModel& phase) {
    if (!phasesChar) return;
    // Batch-of-one using standard phases batch framing:
    // Header (4): [type=0x01][firstPhase(2)][count=1]
    // Entry  (5): [phaseIndex(1)][startSec(4)]
    uint8_t frame[BLEManager::PHASE_HEADER + BLEManager::PHASE_ENTRY] = {0};
    frame[0] = 0x01;
    BLEManager::writeLE(&frame[1], (uint16_t)phase.phaseIndex);
    frame[3] = 1; // count
    frame[BLEManager::PHASE_HEADER + 0] = (uint8_t)phase.phaseIndex; // entry phase index
    BLEManager::writeLE32(&frame[BLEManager::PHASE_HEADER + 1], (uint32_t)phase.phaseStartSec);
    phasesChar->setValue(frame, sizeof(frame));
    if (usePhasesIndicate()) phasesChar->indicate(); else phasesChar->notify();
    Logger::infof("[BLEManager] Live Phase %d notified.", phase.phaseIndex);
}

size_t BLEManager::maxNotifyPayload() const {
    // ATT header is 3 bytes; default MTU is 23
    uint16_t mtu = PEER_MTU;
    if (mtu < 23) mtu = 23;
    uint16_t payload = (mtu > 3) ? (mtu - 3) : 20;
    if (payload == 0) payload = 20;
    return payload;
}