#ifndef WiFiCaptive_h
#define WiFiCaptive_h


#include <AsyncTCP.h>  //https://github.com/me-no-dev/AsyncTCP using the latest dev version from @me-no-dev
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>	//https://github.com/me-no-dev/ESPAsyncWebServer using the latest dev version from @me-no-dev
#include <esp_wifi.h>			//Used for mpdu_rx_disable android workaround
#include <AsyncJson.h>
#include "Preferences.h"
#include "WifiCaptivePage.h"
#include <ArduinoJson.h>


#define WIFI_SSID "TRMNL"
#define WIFI_PASSWORD NULL

// Define the DNS interval in milliseconds between processing DNS requests
#define DNS_INTERVAL 60
// Define the maximum number of clients that can connect to the server
#define MAX_CLIENTS 1
// Define the WiFi channel to be used (channel 6 in this case)
#define WIFI_CHANNEL 6

// Define max connection timeout
#define CONNECTION_TIMEOUT 100000

#define LocalIPURL "http://4.3.2.1"

class WifiCaptive {

    private:
        DNSServer* _dnsServer;
        AsyncWebServer* _server;

        String _ssid = "";
        String _password = "";

        std::function<void()> _resetcallback;


        void setUpDNSServer(DNSServer &dnsServer, const IPAddress &localIP);
        void setUpWebserver(AsyncWebServer &server, const IPAddress &localIP);
        bool connectWifiAndSave(String ssid, String pass, bool connect);
        uint8_t connectWifi(String ssid, String pass, bool connect);
        uint8_t waitForConnectResult(uint32_t timeout);
        uint8_t waitForConnectResult();
        bool WifiEnableSTA(bool enable, bool persistent);

    public:
        /// @brief Starts WiFi configuration portal.
        /// @return True if successfully connected to provided SSID, false otherwise.
        bool startPortal();
        String getSSID();
        String getPassword();
        bool isSaved();
        void resetSettings();
        void setResetSettingsCallback(std::function<void()> func);
};

extern WifiCaptive WifiCaptivePortal;

#endif