#ifndef WiFiCaptive_h
#define WiFiCaptive_h

#include <AsyncTCP.h> //https://github.com/me-no-dev/AsyncTCP using the latest dev version from @me-no-dev
#include <DNSServer.h>
#include <ESPAsyncWebServer.h> //https://github.com/me-no-dev/ESPAsyncWebServer using the latest dev version from @me-no-dev
#include <esp_wifi.h>          //Used for mpdu_rx_disable android workaround
#include <AsyncJson.h>
#include "WifiCaptivePage.h"
#include "WifiCommon.h"

#define WIFI_SSID "TRMNL"
#define WIFI_PASSWORD NULL

// Define the DNS interval in milliseconds between processing DNS requests
#define DNS_INTERVAL 60
// Define the maximum number of clients that can connect to the server
#define MAX_CLIENTS 1
// Define the WiFi channel to be used (channel 6 in this case)
#define WIFI_CHANNEL 6
// Local IP URL
#define LocalIPURL "http://4.3.2.1"

class WifiCaptive : public WifiCommon
{
private:

    DNSServer *_dnsServer;
    AsyncWebServer *_server;

    void setUpDNSServer(DNSServer &dnsServer, const IPAddress &localIP);
    void setUpWebserver(AsyncWebServer &server, const IPAddress &localIP);

public:
    /// @brief Starts WiFi configuration portal.
    /// @return True if successfully connected to provided SSID, false otherwise.
    bool startPortal();
};

extern WifiCaptive WifiCaptivePortal;

#endif
