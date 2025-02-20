#ifndef WiFiCaptive_h
#define WiFiCaptive_h

#include <AsyncTCP.h> //https://github.com/me-no-dev/AsyncTCP using the latest dev version from @me-no-dev
#include <DNSServer.h>
#include <ESPAsyncWebServer.h> //https://github.com/me-no-dev/ESPAsyncWebServer using the latest dev version from @me-no-dev
#include <esp_wifi.h>          //Used for mpdu_rx_disable android workaround
#include <AsyncJson.h>
#include "Preferences.h"
#include "WifiCaptivePage.h"
#include <ArduinoJson.h>
#include <ArduinoLog.h>

#define WIFI_SSID "TRMNL"
#define WIFI_PASSWORD NULL

// Define the DNS interval in milliseconds between processing DNS requests
#define DNS_INTERVAL 60
// Define the maximum number of clients that can connect to the server
#define MAX_CLIENTS 1
// Define the WiFi channel to be used (channel 6 in this case)
#define WIFI_CHANNEL 6
// Define the maximum number of possible saved credentials
#define WIFI_MAX_SAVED_CREDS 5
// Define the maximum number of connection attempts
#define WIFI_CONNECTION_ATTEMPTS 3
// Define max connection timeout
#define CONNECTION_TIMEOUT 15000
// Local IP URL
#define LocalIPURL "http://4.3.2.1"

#define WIFI_SSID_KEY(i) ("wifi_" + String(i) + "_ssid").c_str()
#define WIFI_PSWD_KEY(i) ("wifi_" + String(i) + "_pswd").c_str()
#define WIFI_LAST_USED_SSID_KEY "wifi_ssid"
#define WIFI_LAST_USED_PSWD_KEY "wifi_pswd"

class WifiCaptive
{
private:
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

    DNSServer *_dnsServer;
    AsyncWebServer *_server;
    String _ssid = "";
    String _password = "";
    String _api_server = "";

    std::function<void()> _resetcallback;

    WifiCredentials _savedWifis[WIFI_MAX_SAVED_CREDS];
    WifiCredentials _lastUsed;

    void setUpDNSServer(DNSServer &dnsServer, const IPAddress &localIP);
    void setUpWebserver(AsyncWebServer &server, const IPAddress &localIP);
    uint8_t connect(String ssid, String pass);
    uint8_t waitForConnectResult(uint32_t timeout);
    uint8_t waitForConnectResult();
    void readWifiCredentials();
    void saveWifiCredentials(String ssid, String pass);
    void saveLastUsed(String ssid, String pass);
    void saveApiServer(String url);
    std::vector<WifiCredentials> matchNetworks(std::vector<Network> &scanResults, WifiCaptive::WifiCredentials wifiCredentials[]);
    std::vector<Network> getScannedUniqueNetworks(bool runScan);
    std::vector<Network> combineNetworks(std::vector<Network> &scanResults, WifiCaptive::WifiCredentials wifiCredentials[]);

public:
    /// @brief Starts WiFi configuration portal.
    /// @return True if successfully connected to provided SSID, false otherwise.
    bool startPortal();

    /// @brief Checks if any ssid is saved
    /// @return True if any ssis is saved, false otherwise
    bool isSaved();

    /// @brief Resets all saved credentials
    void resetSettings();

    /// @brief sets the function callback that is triggered when uses performs soft reset
    /// @param func reset callback
    void setResetSettingsCallback(std::function<void()> func);

    /// @brief Connects to the saved SSID with the best signal strength
    /// @return True if successfully connected to saved SSID, false otherwise.
    bool autoConnect();
};

extern WifiCaptive WifiCaptivePortal;

#endif
