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
	server.on("/connecttest.txt", [](AsyncWebServerRequest *request) { request->redirect("http://logout.net"); });	// windows 11 captive portal workaround
	server.on("/wpad.dat", [](AsyncWebServerRequest *request) { request->send(404); });								// Honestly don't understand what this is but a 404 stops win 10 keep calling this repeatedly and panicking the esp32 :)

	// Background responses: Probably not all are Required, but some are. Others might speed things up?
	// A Tier (commonly used by modern systems)
	server.on("/generate_204", [](AsyncWebServerRequest *request) { request->redirect(LocalIPURL); });		   // android captive portal redirect
	server.on("/redirect", [](AsyncWebServerRequest *request) { request->redirect(LocalIPURL); });			   // microsoft redirect
	server.on("/hotspot-detect.html", [](AsyncWebServerRequest *request) { request->redirect(LocalIPURL); });  // apple call home
	server.on("/canonical.html", [](AsyncWebServerRequest *request) { request->redirect(LocalIPURL); });	   // firefox captive portal call home
	server.on("/success.txt", [](AsyncWebServerRequest *request) { request->send(200); });					   // firefox captive portal call home
	server.on("/ncsi.txt", [](AsyncWebServerRequest *request) { request->redirect(LocalIPURL); });			   // windows call home

	// return 404 to webpage icon
	server.on("/favicon.ico", [](AsyncWebServerRequest *request) { request->send(404); });	// webpage icon

	// Serve index.html
	server.on("/", HTTP_ANY, [&](AsyncWebServerRequest *request) {
		AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", INDEX_HTML, INDEX_HTML_LEN);
		response->addHeader("Content-Encoding", "gzip");
    	request->send(response);
	});

	// Servce logo.svg
	server.on("/logo.svg", HTTP_ANY, [&](AsyncWebServerRequest *request) {
		AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", LOGO_SVG, LOGO_SVG_LEN);
		response->addHeader("Content-Encoding", "gzip");
		response->addHeader("Content-Type", "image/svg+xml");
    	request->send(response);
	});

	
	// server.on("/wifi-info", HTTP_ANY, [&](AsyncWebServerRequest *request) {
	// 	String mac = WiFi.macAddress();
	// 	String message =  "{\"ssid\":\"" + _ssid +"\",\"mac\":\"" + mac +"\"}";
	// 	request->send(200, "application/json", message);
	// });

	server.on("/soft-reset", HTTP_ANY, [&](AsyncWebServerRequest *request) {
		resetSettings();
		request->send(200);
	});

	auto scanGET = server.on("/scan", HTTP_GET, [&](AsyncWebServerRequest *request) {
		String json = "[";
		int n = WiFi.scanComplete();
		if (n == WIFI_SCAN_FAILED) {
			WiFi.scanNetworks(true);
			return request->send(202);
		} else if(n == WIFI_SCAN_RUNNING){
			return request->send(202);
		} else {
			for (int i = 0; i < n; ++i){
				String ssid = WiFi.SSID(i);
				// Escape invalid characters
				ssid.replace("\\","\\\\");
				ssid.replace("\"","\\\"");
				json+= "{";
				json+= "\"name\":\""+ssid+"\",";
				json+= "\"open\":"+String(WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "true": "false");
				json+= "}";
				if(i != n-1) json += ",";
			}
			WiFi.scanDelete();
			
			if (WiFi.scanComplete() == -2){
				WiFi.scanNetworks(true);
			}
		}
		json += "]";
		request->send(200, "application/json", json);
		json = String();
	});

	AsyncCallbackJsonWebHandler *handler = new AsyncCallbackJsonWebHandler("/connect", [&](AsyncWebServerRequest *request, JsonVariant &json) {
		JsonObject data = json.as<JsonObject>();
		String ssid = data["ssid"];
		String pswd = data["pswd"];
		_ssid = ssid;
		_password = pswd;
		String mac = WiFi.macAddress();
		String message =  "{\"ssid\":\"" + _ssid +"\",\"mac\":\"" + mac +"\"}";
		request->send(200, "application/json", message);
	});

	server.addHandler(handler);

	server.onNotFound([](AsyncWebServerRequest *request) {
		request->redirect(LocalIPURL);
	});
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

	// Configure the soft access point with a specific IP and subnet mask
	WiFi.softAPConfig(localIP, gatewayIP, subnetMask);

    // Start the soft access point with the given ssid, password, channel, max number of clients
	WiFi.softAP(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL, 0, MAX_CLIENTS);

	// Disable AMPDU RX on the ESP32 WiFi to fix a bug on Android
	esp_wifi_stop();
	esp_wifi_deinit();
	wifi_init_config_t my_config = WIFI_INIT_CONFIG_DEFAULT();
	my_config.ampdu_rx_enable = false;
	esp_wifi_init(&my_config);
	esp_wifi_start();
	vTaskDelay(100 / portTICK_PERIOD_MS);  // Add a small delay

	// configure DSN and WEB server
    setUpDNSServer(*_dnsServer, localIP);
    setUpWebserver(*_server, localIP);

	// begin serving
	_server->begin();

	bool result = false;
	// wait until SSID is provided
	while (1) {
		_dnsServer->processNextRequest();

		if (_ssid == "") {
			delay(DNS_INTERVAL);
		} else {
			uint8_t res = connectWifi(_ssid, _password, true) == WL_CONNECTED;
			if (res) {
				result = true;
				break;
			} else {
				_ssid = "";
				_password = "";

				WiFi.disconnect();
				WifiEnableSTA(false, false);
				break;
			}
		}
	}

	// SSID provided, stop server

	WiFi.scanDelete();
	WiFi.softAPdisconnect(true);

	delay(1000);

	// if (result && WiFi.status() == WL_IDLE_STATUS) {
	// 	WiFi.reconnect(); // restart wifi since we disconnected it in startconfigportal
	// }
	if(result) {
		WiFi.mode(WIFI_STA);
		WiFi.begin(_ssid.c_str(), _password.c_str());
	}

	// stop dsn
	_dnsServer->stop();
  	delete _dnsServer;
	_dnsServer = nullptr;

	// stop server
	_server->end();
	delete _server;
	_server = nullptr;


    return result;
}

void WifiCaptive::resetSettings() {
    Preferences preferences;
    preferences.begin("wificaptive", false);
    preferences.putString("ssid", "");
    preferences.putString("password", "");
    preferences.end();

    WiFi.disconnect(true, true);

	if (_resetcallback != NULL) {
		_resetcallback(); // @CALLBACK
	}
}

bool WifiCaptive::WifiEnableSTA(bool enable, bool persistent){
  bool ret;
  if (persistent)
    WiFi.persistent(true);
  ret = WiFi.enableSTA(enable); // @todo handle persistent when it is implemented in platform
  if (persistent)
    WiFi.persistent(false);
  return ret;
}

/**
 * connect to a new wifi ap
 * @since $dev
 * @param  String ssid
 * @param  String pass
 * @return bool success
 * @return connect only save if false
 */
bool WifiCaptive::connectWifiAndSave(String ssid, String pass, bool connect)
{
	WifiEnableSTA(true, true); // storeSTAmode will also toggle STA on in default opmode (persistent) if true (default)
	Preferences preferences;
    preferences.begin("wificaptive", false);
    preferences.putString("ssid", ssid.c_str());
    preferences.putString("password", pass.c_str());
    preferences.end();

	bool ret = WiFi.begin(ssid.c_str(), pass.c_str(), 0, NULL, connect);
	return ret;
}

// @todo refactor this up into seperate functions
// one for connecting to flash , one for new client
// clean up, flow is convoluted, and causes bugs
uint8_t WifiCaptive::connectWifi(String ssid, String pass, bool connect)
{
  uint8_t retry = 1;
  uint8_t connRes = (uint8_t)WL_NO_SSID_AVAIL;
  uint8_t _connectRetries = 5;

  //setSTAConfig();
  //@todo catch failures in set_config

  // make sure sta is on before `begin` so it does not call enablesta->mode while persistent is ON ( which would save WM AP state to eeprom !)
  // WiFi.setAutoReconnect(false);
  //if (_cleanConnect)
  //  WiFi_Disconnect(); // disconnect before begin, in case anything is hung, this causes a 2 seconds delay for connect
  // @todo find out what status is when this is needed, can we detect it and handle it, say in between states or idle_status to avoid these

  // if retry without delay (via begin()), the IDF is still busy even after returning status
  // E (5130) wifi:sta is connecting, return error
  // [E][WiFiSTA.cpp:221] begin(): connect failed!

  while (retry <= _connectRetries && (connRes != WL_CONNECTED))
  {
    if (_connectRetries > 1)
    {
        delay(1000); // add idle time before recon
    }
    if (ssid != "")
    {
		connectWifiAndSave(ssid, pass, connect);
    	connRes = waitForConnectResult();
    }
    else
    {
		// connect with saved SSID?
    }

    retry++;
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

String WifiCaptive::getSSID(){
	String ssid = "";
	Preferences preferences;
    preferences.begin("wificaptive", false);
    ssid = preferences.getString("ssid", "");
    preferences.end();
	
  	return ssid;
}

String WifiCaptive::getPassword(){
  	String password = "";
	Preferences preferences;
    preferences.begin("wificaptive", false);
    password = preferences.getString("password", "");
    preferences.end();

  	return password;
}

WifiCaptive WifiCaptivePortal;