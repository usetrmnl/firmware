#include "WifiCaptive.h"

void WifiCaptive::setUpDNSServer(DNSServer &dnsServer, const IPAddress &localIP)
{
    dnsServer.setTTL(3600);
    dnsServer.start(53, "*", localIP);
}

void WifiCaptive::setUpWebserver(AsyncWebServer &server, const IPAddress &localIP)
{
    //======================== Webserver ========================
    // WARNING IOS (and maybe macos) WILL NOT POP UP IF IT CONTAINS THE WORD "Success" https://www.esp8266.com/viewtopic.php?f=34&t=4398
    // SAFARI (IOS) IS STUPID, G-ZIPPED FILES CAN'T END IN .GZ https://github.com/homieiot/homie-esp8266/issues/476 this is fixed by the webserver serve static function.
    // SAFARI (IOS) there is a 128KB limit to the size of the HTML. The HTML can reference external resources/images that bring the total over 128KB
    // SAFARI (IOS) popup browser has some severe limitations (javascript disabled, cookies disabled)

    // Required
    server.on("/connecttest.txt", [](AsyncWebServerRequest *request)
              { request->redirect("http://logout.net"); }); // windows 11 captive portal workaround
    server.on("/wpad.dat", [](AsyncWebServerRequest *request)
              { request->send(404); }); // Honestly don't understand what this is but a 404 stops win 10 keep calling this repeatedly and panicking the esp32 :)

    // Background responses: Probably not all are Required, but some are. Others might speed things up?
    // A Tier (commonly used by modern systems)
    server.on("/generate_204", [](AsyncWebServerRequest *request)
              { request->redirect(LocalIPURL); }); // android captive portal redirect
    server.on("/redirect", [](AsyncWebServerRequest *request)
              { request->redirect(LocalIPURL); }); // microsoft redirect
    server.on("/hotspot-detect.html", [](AsyncWebServerRequest *request)
              { request->redirect(LocalIPURL); }); // apple call home
    server.on("/canonical.html", [](AsyncWebServerRequest *request)
              { request->redirect(LocalIPURL); }); // firefox captive portal call home
    server.on("/success.txt", [](AsyncWebServerRequest *request)
              { request->send(200); }); // firefox captive portal call home
    server.on("/ncsi.txt", [](AsyncWebServerRequest *request)
              { request->redirect(LocalIPURL); }); // windows call home

    // return 404 to webpage icon
    server.on("/favicon.ico", [](AsyncWebServerRequest *request)
              { request->send(404); }); // webpage icon

    // Serve index.html
    server.on("/", HTTP_ANY, [&](AsyncWebServerRequest *request)
              {
		AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", INDEX_HTML, INDEX_HTML_LEN);
		response->addHeader("Content-Encoding", "gzip");
    	request->send(response); });

    // Servce logo.svg
    server.on("/logo.svg", HTTP_ANY, [&](AsyncWebServerRequest *request)
              {
		AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", LOGO_SVG, LOGO_SVG_LEN);
		response->addHeader("Content-Encoding", "gzip");
		response->addHeader("Content-Type", "image/svg+xml");
    	request->send(response); });

    server.on("/soft-reset", HTTP_ANY, [&](AsyncWebServerRequest *request)
              {
		resetSettings();
		if (_resetcallback != NULL) {
			_resetcallback(); // @CALLBACK
		}
		request->send(200); });

    auto scanGET = server.on("/scan", HTTP_GET, [&](AsyncWebServerRequest *request)
                             {
		String json = "[";
		int n = WiFi.scanComplete();
		if (n == WIFI_SCAN_FAILED) {
			WiFi.scanNetworks(true);
			return request->send(202);
		} else if(n == WIFI_SCAN_RUNNING){
			return request->send(202);
		} else {
			// Data structure to store the highest RSSI for each SSID
			std::vector<Network> uniqueNetworks = getScannedUniqueNetworks(false);
            std::vector<Network> combinedNetworks = combineNetworks(uniqueNetworks, _savedWifis);

			// Generate JSON response
			size_t size = 0;
			for (const auto& network : combinedNetworks)
			{
				String ssid = network.ssid;
				String rssi = String(network.rssi);

				// Escape invalid characters
				ssid.replace("\\","\\\\");
				ssid.replace("\"","\\\"");
				json+= "{";
				json+= "\"name\":\""+ssid+"\",";
				json+= "\"rssi\":\""+rssi+"\",";
				json+= "\"open\":"+String(network.open == WIFI_AUTH_OPEN ? "true,": "false,");
                json+= "\"saved\":"+String(network.saved ? "true": "false");
				json+= "}";

				size += 1;

				if (size != combinedNetworks.size())
				{
					json+= ",";
				}
			}

            WiFi.scanDelete();
			Serial.println(json);
            
			if (WiFi.scanComplete() == -2){
				WiFi.scanNetworks(true);
			}
		}
		json += "]";
		request->send(200, "application/json", json);
		json = String(); });

    AsyncCallbackJsonWebHandler *handler = new AsyncCallbackJsonWebHandler("/connect",[&](AsyncWebServerRequest *request, JsonVariant &json)
                                                                           {
		JsonObject data = json.as<JsonObject>();
		String ssid = data["ssid"];
		String pswd = data["pswd"];
		_ssid = ssid;
		_password = pswd;
		String mac = WiFi.macAddress();
		String message =  "{\"ssid\":\"" + _ssid +"\",\"mac\":\"" + mac +"\"}";
		request->send(200, "application/json", message); });

    server.addHandler(handler);

    server.onNotFound([](AsyncWebServerRequest *request)
                      { request->redirect(LocalIPURL); });
}

bool WifiCaptive::startPortal()
{
    _dnsServer = new DNSServer();
    _server = new AsyncWebServer(80);

    // Set the WiFi mode to access point and station
    WiFi.mode(WIFI_MODE_AP);

    // Define the subnet mask for the WiFi network
    const IPAddress subnetMask(255, 255, 255, 0);
    const IPAddress localIP(4, 3, 2, 1);
    const IPAddress gatewayIP(4, 3, 2, 1);

    WiFi.disconnect();
    delay(50);

    // Configure the soft access point with a specific IP and subnet mask
    WiFi.softAPConfig(localIP, gatewayIP, subnetMask);
    delay(50);

    // Start the soft access point with the given ssid, password, channel, max number of clients
    WiFi.softAP(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL, 0, MAX_CLIENTS);
    delay(50);

    // Disable AMPDU RX on the ESP32 WiFi to fix a bug on Android
    esp_wifi_stop();
    esp_wifi_deinit();
    wifi_init_config_t my_config = WIFI_INIT_CONFIG_DEFAULT();
    my_config.ampdu_rx_enable = false;
    esp_wifi_init(&my_config);
    esp_wifi_start();
    vTaskDelay(100 / portTICK_PERIOD_MS); // Add a small delay

    // configure DSN and WEB server
    setUpDNSServer(*_dnsServer, localIP);
    setUpWebserver(*_server, localIP);

    // begin serving
    _server->begin();

    // start async network scan
    WiFi.scanNetworks(true);

    readWifiCredentials();

    bool succesfullyConnected = false;
    // wait until SSID is provided
    while (1)
    {
        _dnsServer->processNextRequest();

        if (_ssid == "")
        {
            delay(DNS_INTERVAL);
        }
        else
        {
            bool res = connect(_ssid, _password) == WL_CONNECTED;
            if (res)
            {
                saveWifiCredentials(_ssid, _password);
                succesfullyConnected = true;
                break;
            }
            else
            {
                _ssid = "";
                _password = "";

                WiFi.disconnect();
                WiFi.enableSTA(false);
                break;
            }
        }
    }

    // SSID provided, stop server
    WiFi.scanDelete();
    WiFi.softAPdisconnect(true);
    delay(1000);

    auto status = WiFi.status();
    if (status != WL_CONNECTED)
    {
        Log.info("Not connected after AP disconnect\r\n");
        WiFi.mode(WIFI_STA);
        WiFi.begin(_ssid.c_str(), _password.c_str());
        waitForConnectResult();
    }

    // stop dsn
    _dnsServer->stop();
    delete _dnsServer;
    _dnsServer = nullptr;

    // stop server
    _server->end();
    delete _server;
    _server = nullptr;

    return succesfullyConnected;
}

void WifiCaptive::resetSettings()
{
    Preferences preferences;
    preferences.begin("wificaptive", false);
    for(int i = 0; i < WIFI_MAX_SAVED_CREDS; i++)
    {
        preferences.remove(WIFI_SSID_KEY(i));
        preferences.remove(WIFI_PSWD_KEY(i));
    }
    preferences.remove(WIFI_LAST_USED_SSID_KEY);
    preferences.remove(WIFI_LAST_USED_PSWD_KEY);
    preferences.end();

    for (int i = 0; i < WIFI_MAX_SAVED_CREDS; i++)
    {
        _savedWifis[i] = {"", ""};
    }
    _lastUsed = {"", ""};

    WiFi.disconnect(true, true);
}

uint8_t WifiCaptive::connect(String ssid, String pass)
{
    uint8_t connRes = (uint8_t)WL_NO_SSID_AVAIL;

    if (ssid != "")
    {
        WiFi.enableSTA(true);
        WiFi.begin(ssid.c_str(), pass.c_str());
        connRes = waitForConnectResult();
    }

    return connRes;
}

/**
 * waitForConnectResult
 * @param  uint16_t timeout  in seconds
 * @return uint8_t  WL Status
 */
uint8_t WifiCaptive::waitForConnectResult(uint32_t timeout)
{
    if (timeout == 0)
    {
        return WiFi.waitForConnectResult();
    }

    unsigned long timeoutmillis = millis() + timeout;
    uint8_t status = WiFi.status();

    while (millis() < timeoutmillis)
    {
        status = WiFi.status();
        // @todo detect additional states, connect happens, then dhcp then get ip, there is some delay here, make sure not to timeout if waiting on IP
        if (status == WL_CONNECTED || status == WL_CONNECT_FAILED)
        {
            return status;
        }
        delay(100);
    }

    return status;
}

uint8_t WifiCaptive::waitForConnectResult()
{
    return waitForConnectResult(CONNECTION_TIMEOUT);
}

void WifiCaptive::setResetSettingsCallback(std::function<void()> func)
{
    _resetcallback = func;
}

bool WifiCaptive::isSaved()
{
    readWifiCredentials();
    return _savedWifis[0].ssid != "";
}

void WifiCaptive::readWifiCredentials()
{
    Preferences preferences;
    preferences.begin("wificaptive", true);
    size_t len = preferences.getBytesLength(WIFI_SSID_KEY(0));
    for(int i = 0; i < WIFI_MAX_SAVED_CREDS; i++)
    {
        _savedWifis[i].ssid = preferences.getString(WIFI_SSID_KEY(i), "");
        _savedWifis[i].pswd = preferences.getString(WIFI_PSWD_KEY(i), "");
    }
    _lastUsed.ssid = preferences.getString(WIFI_LAST_USED_SSID_KEY, "");
    _lastUsed.pswd = preferences.getString(WIFI_LAST_USED_PSWD_KEY, "");
    preferences.end();
}

void WifiCaptive::saveWifiCredentials(String ssid, String pass)
{
    Log.info("Saving wifi credentials: %s\r\n", ssid.c_str());

    for (u16_t i = WIFI_MAX_SAVED_CREDS - 1; i >= 1; i--)
    {
        _savedWifis[i].ssid = _savedWifis[i-1].ssid;
        _savedWifis[i].pswd = _savedWifis[i-1].pswd;
    }

    _savedWifis[0].ssid = ssid;
    _savedWifis[0].pswd = pass;
    _lastUsed.ssid = ssid;
    _lastUsed.pswd = pass;

    Preferences preferences;
    preferences.begin("wificaptive", false);
    for(int i = 0; i < WIFI_MAX_SAVED_CREDS; i++)
    {
        preferences.putString(WIFI_SSID_KEY(i), _savedWifis[i].ssid);
        preferences.putString(WIFI_PSWD_KEY(i), _savedWifis[i].pswd);
    }
    preferences.putString(WIFI_LAST_USED_SSID_KEY, _lastUsed.ssid);
    preferences.putString(WIFI_LAST_USED_PSWD_KEY, _lastUsed.pswd);
    preferences.end();
}

void WifiCaptive::saveLastUsed(String ssid, String pass)
{
    _lastUsed.ssid = ssid;
    _lastUsed.pswd = pass;

    Preferences preferences;
    preferences.begin("wificaptive", false);
    preferences.putString(WIFI_LAST_USED_SSID_KEY, ssid);
    preferences.putString(WIFI_LAST_USED_PSWD_KEY, pass);
    preferences.end();
}

std::vector<WifiCaptive::Network> WifiCaptive::getScannedUniqueNetworks(bool runScan)
{
    std::vector<Network> uniqueNetworks;
    int n = WiFi.scanComplete();
    if (runScan == true)
    {
        WiFi.scanNetworks(true);
        delay(100);
        int n = WiFi.scanComplete();
        while (n == WIFI_SCAN_RUNNING || n == WIFI_SCAN_FAILED)
        {
            delay(100);
            if (n == WIFI_SCAN_RUNNING)
            {
                n = WiFi.scanComplete();
            }
            else if (n == WIFI_SCAN_FAILED)
            {
                WiFi.scanNetworks(true);
                delay(100);
                n = WiFi.scanComplete();
            }
        }
    }

    n = WiFi.scanComplete();

    // Process each found network
    for (int i = 0; i < n; ++i)
    {
        if (!WiFi.SSID(i).equals("TRMNL"))
        {
            String ssid = WiFi.SSID(i);
            int32_t rssi = WiFi.RSSI(i);
            bool open = WiFi.encryptionType(i);
            bool found = false;
            for (auto &network : uniqueNetworks)
            {
                if (network.ssid == ssid)
                {
                    Serial.println("Equal SSID");
                    found = true;
                    if (network.rssi < rssi)
                    {
                        network.rssi = rssi; // Update to higher RSSI
                    }
                    break;
                }
            }
            if (!found)
            {
                uniqueNetworks.push_back({ssid, rssi, open});
            }
        }
    }

    return uniqueNetworks;
}

std::vector<WifiCaptive::WifiCredentials> WifiCaptive::matchNetworks(
    std::vector<WifiCaptive::Network> &scanResults,
    WifiCaptive::WifiCredentials savedWifis[])
{
    // sort scan results by RSSI
    std::sort(scanResults.begin(), scanResults.end(), [](const Network &a, const Network &b)
              { return a.rssi > b.rssi; });

    std::vector<WifiCredentials> sortedWifis;
    for (auto &network : scanResults)
    {
        for (int i = 0; i < WIFI_MAX_SAVED_CREDS; i++)
        {
            if (network.ssid == savedWifis[i].ssid)
            {
                sortedWifis.push_back(savedWifis[i]);
            }
        }
    }

    return sortedWifis;
}

std::vector<WifiCaptive::Network> WifiCaptive::combineNetworks(
    std::vector<WifiCaptive::Network> &scanResults,
    WifiCaptive::WifiCredentials savedWifis[])
{
    std::vector<Network> combinedNetworks;
    for (auto &network : scanResults)
    {
        bool found = false;
        for (int i = 0; i < WIFI_MAX_SAVED_CREDS; i++)
        {
            if (network.ssid == savedWifis[i].ssid)
            {
                combinedNetworks.push_back({network.ssid, network.rssi, network.open, true});
                found = true;
                break;
            }
        }
        if (!found)
        {
            combinedNetworks.push_back({network.ssid, network.rssi, network.open, false});
        }
    }
    // add saved wifis that are not combinedNetworks
    for (int i = 0; i < WIFI_MAX_SAVED_CREDS; i++)
    {
        bool found = false;
        for (auto &network : combinedNetworks)
        {
            if (network.ssid == savedWifis[i].ssid)
            {
                found = true;
                break;
            }
        }
        if (!found && savedWifis[i].ssid != "")
        {
            combinedNetworks.push_back({savedWifis[i].ssid, -200, false, true});
        }
    }

    return combinedNetworks; 
}

bool WifiCaptive::autoConnect()
{
    Log.info("Trying to autoconnect to wifi...\r\n");
    readWifiCredentials();

    if (_lastUsed.ssid != "") {
        Log.info("Trying to connect to last used %s...\r\n", _lastUsed.ssid.c_str());
        WiFi.setSleep(0);
        WiFi.setMinSecurity(WIFI_AUTH_OPEN);
        WiFi.mode(WIFI_STA);
        connect(_lastUsed.ssid, _lastUsed.pswd);
        
        // Check if connected
        if (WiFi.status() == WL_CONNECTED)
        {
            Log.info("Connected to %s\r\n", _lastUsed.ssid.c_str());
            return true;
        }
        WiFi.disconnect();
    }

    std::vector<Network> scanResults = getScannedUniqueNetworks(true);
    std::vector<WifiCaptive::WifiCredentials> sortedNetworks = matchNetworks(scanResults, _savedWifis);
    // if no networks found, try to connect to saved wifis
    if (sortedNetworks.size() == 0)
    {
        Log.info("No matched networks...\r\n");
        sortedNetworks = std::vector<WifiCredentials>(_savedWifis, _savedWifis + WIFI_MAX_SAVED_CREDS);
    }

    WiFi.mode(WIFI_STA);
    for (auto &network : sortedNetworks)
    {
        if (network.ssid == "")
        {
            continue;
        }
        
        connect(network.ssid, network.pswd);

        // Check if connected
        if (WiFi.status() == WL_CONNECTED)
        {
            saveLastUsed(network.ssid, network.pswd);
            return true;
        }
    }

    Log.info("Failed to connect to any network\r\n");
    return false;
}

WifiCaptive WifiCaptivePortal;