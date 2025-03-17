#include "WifiBLE.h"

WifiBLE::~WifiBLE() {
    stopBLEProvisioning();
}

bool WifiBLE::startBLEProvisioning() {
    if (_isProvisioning) return false;

    Serial.println("Starting BLE WiFi provisioning...");

    NimBLEDevice::init(BLE_DEVICE_NAME);

    _server = NimBLEDevice::createServer();
    if (!_server) {
        Log.error("Failed to create BLE server");
        return false;
    }

    _server->setCallbacks(this);

    _service = _server->createService(SERVICE_UUID);
    if (!_service) {
        Log.error("Failed to create BLE service");
        cleanupBLEResources();
        return false;
    }

    _dataChar = _service->createCharacteristic(
        CREDENTIALS_UUID,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY
    );

    if (!_dataChar) {
        Log.error("Failed to create ssid/pass data characteristic");
        cleanupBLEResources();
        return false;
    }

    _dataChar->setCallbacks(this);

    _service->start();

    setupBLEAdvertising();

    readWifiCredentials();

    _isProvisioning = true;
    _credentialsReceived = false;
    Log.info("BLE WiFi provisioning started");

    // Block until credentials are received or timeout occurs
    unsigned long startTime = millis();
    const unsigned long timeout = 300000; // 5 minutes timeout
    const unsigned long progressInterval = 30000; // Report progress every 30 seconds
    unsigned long lastProgress = startTime;

    Log.info("Waiting for BLE credentials (timeout: %d seconds)...", timeout/1000);

    while (!_credentialsReceived) {
        // Progress reporting
        if (millis() - lastProgress > progressInterval) {
            lastProgress = millis();
            unsigned long elapsed = (millis() - startTime) / 1000;
            unsigned long remaining = (timeout - (millis() - startTime)) / 1000;
            Log.info("Still waiting for BLE credentials... (%lu seconds elapsed, %lu remaining)",
                     elapsed, remaining);
        }

        // Check for timeout
        if (millis() - startTime > timeout) {
            Log.error("BLE provisioning timed out after %d seconds", timeout/1000);
            stopBLEProvisioning();
            return false;
        }

        // Handle reconnection logic if device disconnects
        if (!_deviceConnected) {
            delay(500);
            NimBLEDevice::startAdvertising();

            if (_previouslyConnected) {
                Log.info("Device disconnected, restarting advertising");
            } else {
                Log.info("No device connected yet, advertising BLE service");
            }
        }

        delay(100);
    }

    Log.info("BLE credentials received successfully");

    // Credentials received, wait briefly to ensure notifications are sent
    delay(1000);

    stopBLEProvisioning();

    return true;
}

void WifiBLE::stopBLEProvisioning() {
    if (!_isProvisioning) return;

    Log.info("Stopping BLE WiFi provisioning...");

    NimBLEDevice::deinit();
    cleanupBLEResources();

    _isProvisioning = false;
    _deviceConnected = false;
    _previouslyConnected = false;
    Log.info("BLE WiFi provisioning stopped");
}

void WifiBLE::cleanupBLEResources() {
    _service = nullptr;
    _dataChar = nullptr;
    _server = nullptr;
}

void WifiBLE::setupBLEAdvertising() {
    NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();
    if (!advertising) {
        Log.error("Failed to get BLE advertising object");
        return;
    }

    advertising->setMinPreferred(0x06);  // Helps with iOS compatibility?
    advertising->setMinInterval(0x20);
    advertising->setMaxInterval(0x40);

    BLEAdvertisementData adData;
    adData.setFlags(ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT);
    adData.setCompleteServices(BLEUUID(SERVICE_UUID));
    adData.setName(BLE_DEVICE_NAME);
    advertising->setAdvertisementData(adData);

    BLEAdvertisementData scanResp;
    scanResp.setName(BLE_DEVICE_NAME);
    uint8_t manData[5] = {'T', 'R', 'M', 'N', 'L'};
    scanResp.setManufacturerData((char*)manData);
    advertising->setScanResponseData(scanResp);

    advertising->setScanResponse(true);
    advertising->start();
}

void WifiBLE::onConnect(BLEServer* server) {
    _deviceConnected = true;
    Log.info("BLE: Client connected");
}

void WifiBLE::onDisconnect(BLEServer* server) {
    _deviceConnected = false;
    _previouslyConnected = true;
    Log.info("BLE: Client disconnected");
}

void WifiBLE::onWrite(BLECharacteristic* characteristic) {
    if (!characteristic) return;

    BLEUUID uuid = characteristic->getUUID();
    String value = characteristic->getValue();

    if (uuid.equals(BLEUUID(CREDENTIALS_UUID))) {
        String receivedData = characteristic->getValue();
        if (receivedData.length() > 0) {
            Log.info("Received Wifi Credentials [%s]\n", receivedData.c_str());

            // Process the received credentials
            processCredentials(receivedData);

            // Send acknowledgment response
            String response = "SUCCESS";
            _dataChar->setValue(response);
            _dataChar->notify();

            _credentialsReceived = true;
        }
    }
}

void WifiBLE::processCredentials(const String& data) {
    size_t separator = data.indexOf(";");
    if (separator == -1) {
        Log.info("Invalid credentials format received\n");
        Log.info(data.c_str());
        return;
    }

    // Get the raw SSID and trim any leading asterisks
    String rawSsid = data.substring(0, separator);
    String ssid = rawSsid;

    // Remove leading asterisks if present
    while (ssid.length() > 0 && ssid.charAt(0) == '*') {
        ssid = ssid.substring(1);
    }

    String password = data.substring(separator + 1);

    Log.info("SSID: [%s], Password: [%s]\n", ssid.c_str(), password.c_str());

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(50);

    bool res = connect(ssid, password) == WL_CONNECTED;
    if (res) {
        Log.info("Successfully connected to %s\n", ssid.c_str());
        saveWifiCredentials(ssid, password);
        saveLastUsedWifiIndex(0);
    } else {
        Log.info("Failed to connect on first attempt\n");
        WiFi.disconnect();
        WiFi.enableSTA(false);
        delay(500);
    }

    auto status = WiFi.status();
    if (status != WL_CONNECTED) {
        Log.info("Not connected after first attempt, retrying...\n");
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid.c_str(), password.c_str());

        waitForConnectResult();
    }
}

WifiBLE WifiBLEProvisioning;