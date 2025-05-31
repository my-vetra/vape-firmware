#include "BLEManager.h"
#include "TimerManager.h"
#include "GlobalVars.h"
#include <BLE2902.h>

// Static variable to track the last BLE interaction
static RTC_DATA_ATTR unsigned long lastInteractionTime = 0;

unsigned long BLEManager::getLastInteraction() {
    return lastInteractionTime;
}

void BLEManager::updateInteraction() {
    lastInteractionTime = getRtcTimeMillis();
}

// --------------------- BLEManager Implementation ---------------------

BLEManager::BLEManager() : pServer(nullptr), timerChar(nullptr),  syncChar(nullptr), ntpChar(nullptr), keepAliveChar(nullptr), bleEnabled(false) {}

BLEManager::~BLEManager() {
    cleanupService();
}

void BLEManager::startService() {
    bleEnabled = true;

    // Initialize BLE and create a server
    BLEDevice::init("Sylo");
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    // Create service and characteristics
    BLEService* timerService = pServer->createService(SERVICE_UUID);

    // TIMER characteristic: used to set the timer value
    timerChar = timerService->createCharacteristic(
        TIMER_CHAR_UUID,
        BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY
    );
    timerChar->setCallbacks(new TimerWriteCallbacks());
    timerChar->addDescriptor(new BLE2902());


    // NTP characteristic: used to update the NTP time to compensate for deep sleep drift.
    ntpChar = timerService->createCharacteristic(
        NTP_CHAR_UUID,
        BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY
    );
    ntpChar->setCallbacks(new NTPWriteCallbacks());
    ntpChar->addDescriptor(new BLE2902());


    // SYNC characteristic: used to read/write the timer data
    syncChar = timerService->createCharacteristic(
        SYNC_CHAR_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY
    );
    syncChar->setCallbacks(new SyncWriteCallbacks());
    syncChar->addDescriptor(new BLE2902());

    keepAliveChar = timerService->createCharacteristic(
        KEEPALIVE_CHAR_UUID,
        BLECharacteristic::PROPERTY_READ
    );

    keepAliveChar->setCallbacks(new KeepAliveCallbacks());

    timerService->start();

    // Start advertising the service
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(false);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();

    updateInteraction();
}

void BLEManager::cleanupService() {

    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    if (pAdvertising) {
        pAdvertising->stop();
    }
    BLEDevice::deinit();

    if (pServer) {
        delete pServer;
        pServer = nullptr;
    }
    timerChar = nullptr;
    ntpChar = nullptr;
    syncChar = nullptr;
    bleEnabled = false;
}


// --- NotifyImplementation ---
void BLEManager::updateSyncCharacteristic() {
    if (!syncChar) {
        return;
    }
    uint8_t data[16];
    packTimerData(data);
    syncChar->setValue(data, sizeof(data));
    syncChar->notify();
}

// --------------------- Callback Implementations ---------------------

// --- MyServerCallbacks ---
BLEManager::MyServerCallbacks::MyServerCallbacks() {}

void BLEManager::MyServerCallbacks::onConnect(BLEServer* pServer) {
    BLEManager::updateInteraction();
}

void BLEManager::MyServerCallbacks::onDisconnect(BLEServer* pServer) {
    BLEDevice::startAdvertising();
}

// --- TimerWriteCallbacks ---
BLEManager::TimerWriteCallbacks::TimerWriteCallbacks() {}

void BLEManager::TimerWriteCallbacks::onWrite(BLECharacteristic* pCharacteristic) {
    BLEManager::updateInteraction();
    std::string value = pCharacteristic->getValue();
    if (!value.empty()) {
        try {
            unsigned long newDuration = std::stoul(value);
            bool success = TimerManager::setPersistentDuration(newDuration);
            if (success) {
                pCharacteristic->setValue("Timer update success");
            } else {
                pCharacteristic->setValue("Timer update failed");
            }
        } catch (const std::exception &e) {
            pCharacteristic->setValue(e.what());
        }
        pCharacteristic->notify();
    }
}

// Helper function to pack timer data into a 16-byte array.
// The 4 fields are:
//  Field 1: Persistent Timer Total (ms)
//  Field 2: Persistent Timer Elapsed (ms)
//  Field 3: Coil Unlock Total Duration (ms)
//  Field 4: Coil Unlock Remaining (ms)
static void packTimerData(uint8_t data[16]) {
    uint32_t persistentTotal   = TimerManager::getPersistentDuration();
    uint32_t persistentElapsed = TimerManager::getPersistentElapsed();
    uint32_t coilTotal         = TimerManager::getCoilDuration();
    uint32_t coilRemaining     = TimerManager::getCoilRemaining();

    data[0]  = persistentTotal & 0xFF;
    data[1]  = (persistentTotal >> 8) & 0xFF;
    data[2]  = (persistentTotal >> 16) & 0xFF;
    data[3]  = (persistentTotal >> 24) & 0xFF;

    data[4]  = persistentElapsed & 0xFF;
    data[5]  = (persistentElapsed >> 8) & 0xFF;
    data[6]  = (persistentElapsed >> 16) & 0xFF;
    data[7]  = (persistentElapsed >> 24) & 0xFF;

    data[8]  = coilTotal & 0xFF;
    data[9]  = (coilTotal >> 8) & 0xFF;
    data[10] = (coilTotal >> 16) & 0xFF;
    data[11] = (coilTotal >> 24) & 0xFF;

    data[12] = coilRemaining & 0xFF;
    data[13] = (coilRemaining >> 8) & 0xFF;
    data[14] = (coilRemaining >> 16) & 0xFF;
    data[15] = (coilRemaining >> 24) & 0xFF;
}


// --- SyncWriteCallbacks ---
BLEManager::SyncWriteCallbacks::SyncWriteCallbacks() {}

void BLEManager::SyncWriteCallbacks::onWrite(BLECharacteristic* pCharacteristic) {
    uint8_t data[16];
    packTimerData(data);
    pCharacteristic->setValue(data, sizeof(data));
    pCharacteristic->notify();
}

void BLEManager::SyncWriteCallbacks::onRead(BLECharacteristic* pCharacteristic) {
    uint8_t data[16];
    packTimerData(data);
    pCharacteristic->setValue(data, sizeof(data));
}


// --- NTPWriteCallbacks ---
BLEManager::NTPWriteCallbacks::NTPWriteCallbacks() {}

void BLEManager::NTPWriteCallbacks::onWrite(BLECharacteristic* pCharacteristic) {
    BLEManager::updateInteraction();
    std::string value = pCharacteristic->getValue();
    if (!value.empty()) {
        try {
            unsigned long newNTPTime = std::stoul(value);
            // Call the TimerManager function to update the NTP time.
            bool success = TimerManager::updateSystemTime(newNTPTime);
            if (success) {
                pCharacteristic->setValue("NTP update success");
            } else {
                pCharacteristic->setValue("NTP update failed");
            }
        } catch (const std::exception &e) {
            pCharacteristic->setValue(e.what());
        }
        pCharacteristic->notify();
    }
}


// --- KeepAliveReadCallbacks ---
BLEManager::KeepAliveCallbacks::KeepAliveCallbacks() {}

void BLEManager::KeepAliveCallbacks::onRead(BLECharacteristic* pCharacteristic) {
    BLEManager::updateInteraction();
    uint32_t value = static_cast<uint32_t>(lastInteractionTime);
    pCharacteristic->setValue((uint8_t*)&value, sizeof(value));
}
