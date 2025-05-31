#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLECharacteristic.h>
#include <BLEAdvertising.h>
#include <Arduino.h>
#include "TimerManager.h"

// BLE UUIDs (service and characteristics)
#define SERVICE_UUID        "12345678-1234-5678-1234-56789abcdef0"
#define TIMER_CHAR_UUID     "12345678-1234-5678-1234-56789abcdef1"
#define NTP_CHAR_UUID       "12345678-1234-5678-1234-56789abcdef2"
#define SYNC_CHAR_UUID      "12345678-1234-5678-1234-56789abcdef3"
#define KEEPALIVE_CHAR_UUID "12345678-1234-5678-1234-56789abcdef4"

class BLEManager {
public:
    BLEManager();
    ~BLEManager();

    // Starts the BLE service and advertising
    void startService();

    // Stops and cleans up BLE resources
    void cleanupService();

    void updateSyncCharacteristic();

    // Returns true if BLE is currently active
    bool isActive() const { return bleEnabled; }

    // Update the last interaction time (used for idle timeout)
    static void updateInteraction();
    static unsigned long getLastInteraction();

private:
    BLEServer* pServer;
    BLECharacteristic* timerChar;
    BLECharacteristic* syncChar;
    BLECharacteristic* ntpChar;
    BLECharacteristic* keepAliveChar;
    bool bleEnabled;

    // Internal callback classes
    class MyServerCallbacks : public BLEServerCallbacks {
    public:
        MyServerCallbacks();
        void onConnect(BLEServer* pServer) override;
        void onDisconnect(BLEServer* pServer) override;
    };

    class TimerWriteCallbacks : public BLECharacteristicCallbacks {
    public:
        TimerWriteCallbacks();
        void onWrite(BLECharacteristic* pCharacteristic) override;  
    };

    class SyncWriteCallbacks : public BLECharacteristicCallbacks {
    public:
        SyncWriteCallbacks();
        void onWrite(BLECharacteristic* pCharacteristic) override;
        void onRead(BLECharacteristic* pCharacteristic) override;
    };

    class NTPWriteCallbacks : public BLECharacteristicCallbacks {
        public:
        NTPWriteCallbacks();
        void onWrite(BLECharacteristic* pCharacteristic) override;
    };

    class KeepAliveCallbacks : public BLECharacteristicCallbacks {
        public:
        KeepAliveCallbacks();
        void onRead(BLECharacteristic* pCharacteristic) override;
    };
};

static void packTimerData(uint8_t data[16]);

#endif // BLE_MANAGER_H
