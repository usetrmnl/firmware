#ifndef WIFI_COMMON_H
#define WIFI_COMMON_H

#include <esp_wifi.h>          //Used for mpdu_rx_disable android workaround
#include "Preferences.h"
#include <ArduinoJson.h>
#include <ArduinoLog.h>
#include <WiFi.h>
#include <vector>
#include <functional>

// Define the maximum number of possible saved credentials
#define WIFI_MAX_SAVED_CREDS 5
// Define the maximum number of connection attempts
#define WIFI_CONNECTION_ATTEMPTS 3
// Define max connection timeout
#define CONNECTION_TIMEOUT 15000
// Preference namespace
#define WIFI_PREF_NAMESPACE "wificaptive"
// Last used wifi index key
#define WIFI_LAST_INDEX "wifi_last_index"

#define WIFI_SSID_KEY(i) ("wifi_" + String(i) + "_ssid").c_str()
#define WIFI_PSWD_KEY(i) ("wifi_" + String(i) + "_pswd").c_str()

class WifiCommon {
public:
    struct WifiCredentials
    {
        String ssid;
        String pswd;
    };

    struct Network
    {
        String ssid;
        int32_t rssi;
        bool open;
        bool saved;
    };

    /// @brief Resets all saved credentials
    void resetSettings();

    /// @brief Checks if any ssid is saved
    /// @return True if any ssis is saved, false otherwise
    bool isSaved();

    /// @brief Connects to the saved SSID with the best signal strength
    /// @return True if successfully connected to saved SSID, false otherwise.
    bool autoConnect();

    /// @brief sets the function callback that is triggered when uses performs soft reset
    /// @param func reset callback
    void setResetSettingsCallback(std::function<void()> func);

protected:
    String _ssid = "";
    String _password = "";
    String _api_server = "";

    std::function<void()> _resetcallback;

    WifiCredentials _savedWifis[WIFI_MAX_SAVED_CREDS];

    /// @brief Connect to WiFi with given credentials
    /// @param ssid SSID to connect to
    /// @param pass Password for the SSID
    /// @return Status code (WL_CONNECTED on success)
    uint8_t connect(String ssid, String pass);

    /// @brief Wait for WiFi connection with custom timeout
    /// @param timeout Timeout in milliseconds
    /// @return Status code (WL_CONNECTED on success)
    uint8_t waitForConnectResult(uint32_t timeout);

    /// @brief Wait for WiFi connection with default timeout
    /// @return Status code (WL_CONNECTED on success)
    uint8_t waitForConnectResult();

    /// @brief Read saved WiFi credentials from preferences
    void readWifiCredentials();

    /// @brief Save WiFi credentials to preferences
    /// @param ssid SSID to save
    /// @param pass Password to save
    void saveWifiCredentials(String ssid, String pass);

    /// @brief Save index of last successfully used WiFi network
    /// @param index Index to save
    void saveLastUsedWifiIndex(int index);

    /// @brief Read last successfully used WiFi network index
    /// @return Index of last used network
    int readLastUsedWifiIndex();

    /// @brief Save API server URL to preferences
    /// @param url API server URL
    void saveApiServer(String url);

    /// @brief Match scan results with saved credentials
    /// @param scanResults Vector of scanned networks
    /// @param wifiCredentials Array of saved credentials
    /// @return Vector of matched networks sorted by signal strength
    std::vector<WifiCredentials> matchNetworks(std::vector<Network> &scanResults, WifiCredentials wifiCredentials[]);

    /// @brief Get unique networks from scan
    /// @param runScan Whether to run a new scan or use existing results
    /// @return Vector of unique networks
    std::vector<Network> getScannedUniqueNetworks(bool runScan);

    /// @brief Combine scanned networks with saved networks
    /// @param scanResults Vector of scanned networks
    /// @param wifiCredentials Array of saved credentials
    /// @return Combined vector with saved status
    std::vector<Network> combineNetworks(std::vector<Network> &scanResults, WifiCredentials wifiCredentials[]);
};

#endif
