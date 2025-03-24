#ifndef WIFI_BLE_H
#define WIFI_BLE_H

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <NimBLEServer.h>
#include "trmnl_log.h"
#include "WifiCommon.h"

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CREDENTIALS_UUID    "beb5483e-36e1-4688-b7f5-ea07361b26a8"

#define BLE_DEVICE_NAME     "TRMNL_WIFI_SETUP"

class WifiBLE : public BLECharacteristicCallbacks, public BLEServerCallbacks, public WifiCommon {
    public:

    WifiBLE() : _isProvisioning(false), _deviceConnected(false),
                _previouslyConnected(false), _credentialsReceived(false) {}
    ~WifiBLE();

    bool startBLEProvisioning();
    void stopBLEProvisioning();

private:
    // Device state
    bool _isProvisioning;
    bool _deviceConnected;
    bool _previouslyConnected;
    bool _credentialsReceived;

    // BLE objects
    BLEServer* _server = nullptr;
    BLEService* _service = nullptr;
    BLECharacteristic* _dataChar = nullptr;

    // BLE setup and management
    void cleanupBLEResources();
    void setupBLEAdvertising();

    void processCredentials(const String& data);

    // BLECharacteristicCallbacks implementation
    void onWrite(BLECharacteristic* characteristic) override;

    // BLEServerCallbacks implementation
    void onConnect(BLEServer* pServer) override;
    void onDisconnect(BLEServer* pServer) override;
};

extern WifiBLE WifiBLEProvisioning;

#endif // WIFIBLE_H
