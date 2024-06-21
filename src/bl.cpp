#include <Arduino.h>
#include <bl.h>
#include <types.h>
#include <ArduinoLog.h>
#include <WiFiManager.h>
#include <pins.h>
#include <config.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <display.h>
#include <stdlib.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <SPIFFS.h>
#include <ImageData.h>
#include <Preferences.h>
#include <cstdint>

bool pref_clear = false;
;
WiFiManager wm;

uint8_t buffer[48062];    // image buffer
char filename[1024];      // image URL
char binUrl[1024];        // update URL
char log_array[512];      // log
char message_buffer[128]; // message to show on the screen

bool status = false;                                                 // need to download a new image
bool update_firmware = false;                                        // need to download a new firmaware
bool reset_firmware = false;                                         // need to reset credentials
bool send_log = false;                                               // need to send logs
esp_sleep_wakeup_cause_t wakeup_reason = ESP_SLEEP_WAKEUP_UNDEFINED; // wake-up reason
MSG current_msg = NONE;
RTC_DATA_ATTR uint8_t need_to_refresh_display = 1;
// timers
uint32_t button_timer = 0;

Preferences preferences;

static bmp_err_e parseBMPHeader(uint8_t *data, bool &reserved);                     // .bmp file validation
static https_request_err_e downloadAndShow(const char *url);                        // download and show the image
static void getDeviceCredentials(const char *url);                                  // receiveing API key and Friendly ID
static void resetDeviceCredentials(void);                                           // reset device credentials API key, Friendly ID, Wi-Fi SSID and password
static bool readBufferFromFile(const char *name, uint8_t *out_buffer);              // file reading
static bool writeBufferToFile(const char *name, uint8_t *in_buffer, uint16_t size); // filw writing
static void checkAndPerformFirmwareUpdate(void);                                    // OTA update
static void goToSleep(void);                                                        // sleep prepearing
static void setClock(void);                                                         // clock synchrinization
static float readBatteryVoltage(void);                                              // battery voltage reading
static void log_POST(char *log_buffer, size_t size);                                // log sending
static void handleRoute(void);
static void bindServerCallback(void);
static void checkLogNotes(void);
static uint32_t getTime(void);

/**
 * @brief Function to init business logic module
 * @param none
 * @return none
 */
void bl_init(void)
{
  Serial.begin(115200);
  Log.begin(LOG_LEVEL_VERBOSE, &Serial);
  Log.info("%s [%d]: BL init success\r\n", __FILE__, __LINE__);
  Log.info("%s [%d]: Firware version %d.%d.%d\r\n", __FILE__, __LINE__, FW_MAJOR_VERSION, FW_MINOR_VERSION, FW_PATCH_VERSION);
  pins_init();
  button_timer = millis();

  wakeup_reason = esp_sleep_get_wakeup_cause();
  Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);

  Log.info("%s [%d]: preferences start\r\n", __FILE__, __LINE__);
  bool res = preferences.begin("data", false);
  if (res)
  {
    Log.info("%s [%d]: preferences init success\r\n", __FILE__, __LINE__);
    if (pref_clear)
    {
      res = preferences.clear(); // if needed to clear the saved data
      if (res)
        Log.info("%s [%d]: preferences cleared success\r\n", __FILE__, __LINE__);
      else
        Log.fatal("%s [%d]: preferences clearing error\r\n", __FILE__, __LINE__);
    }
  }
  else
  {
    Log.fatal("%s [%d]: preferences init failed\r\n", __FILE__, __LINE__);
    ESP.restart();
  }
  Log.info("%s [%d]: preferences end\r\n", __FILE__, __LINE__);

  Log.info("%s [%d]: button handling start\r\n", __FILE__, __LINE__);
  // handling reset
  while (1)
  {
    if (digitalRead(PIN_INTERRUPT) == LOW && millis() - button_timer > BUTTON_HOLD_TIME)
    {
      Log.info("%s [%d]: WiFi reset\r\n", __FILE__, __LINE__);
      wm.resetSettings();
      break;
    }
    else if (digitalRead(PIN_INTERRUPT) == HIGH)
    {
      Log.info("%s [%d]: WiFi NOT reset\r\n", __FILE__, __LINE__);
      break;
    }
  }
  Log.info("%s [%d]: button handling end\r\n", __FILE__, __LINE__);

  // EPD init
  // EPD clear
  Log.info("%s [%d]: Display init\r\n", __FILE__, __LINE__);
  display_init();

  if (wakeup_reason != 4)
  {
    Log.info("%s [%d]: Display clear\r\n", __FILE__, __LINE__);
    display_reset();
  }

  // Mount SPIFFS
  if (!SPIFFS.begin(true))
  {
    Log.fatal("%s [%d]: Failed to mount SPIFFS\r\n", __FILE__, __LINE__);
    ESP.restart();
  }
  else
  {
    Log.info("%s [%d]: SPIFFS mounted\r\n", __FILE__, __LINE__);
  }

  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  if (wm.getWiFiIsSaved())
  {
    // WiFi saved, connection
    Log.info("%s [%d]: WiFi saved\r\n", __FILE__, __LINE__);
    WiFi.begin(wm.getWiFiSSID(), wm.getWiFiPass());

    uint8_t attepmts = 0;
    Log.info("%s [%d]: wifi connection...\r\n", __FILE__, __LINE__);
    while (WiFi.status() != WL_CONNECTED && attepmts < WIFI_CONNECTION_ATTEMPTS)
    {
      delay(1000);
      attepmts++;
    }
    Log.infoln("");
    // Check if connected
    if (WiFi.status() == WL_CONNECTED)
    {
      String ip = String(WiFi.localIP());
      Log.info("%s [%d]:wifi_connection [DEBUG]: Connected: %s\r\n", __FILE__, __LINE__, ip.c_str());
    }
    else
    {
      Log.fatal("%s [%d]: Connection failed!\r\n", __FILE__, __LINE__);

      if (current_msg != WIFI_FAILED)
      {
        res = readBufferFromFile("/logo.bmp", buffer);
        if (res)
          display_show_msg(buffer, WIFI_FAILED);
        else
          display_show_msg(const_cast<uint8_t *>(default_icon), WIFI_FAILED);
        current_msg = WIFI_FAILED;
      }

      memset(log_array, 0, sizeof(log_array));
      sprintf(log_array, "%d [%d]: wifi connection failed", getTime(), __LINE__);
      log_POST(log_array, strlen(log_array));

      // Go to deep sleep
      display_sleep();
      goToSleep();
    }
  }
  else
  {
    // WiFi credentials are not saved - start captive portal
    Log.info("%s [%d]: WiFi NOT saved\r\n", __FILE__, __LINE__);

    char fw_version[20];

    sprintf(fw_version, "%d.%d.%d", FW_MAJOR_VERSION, FW_MINOR_VERSION, FW_PATCH_VERSION);

    String fw = fw_version;

    Log.info("%s [%d]: FW version %s\r\n", __FILE__, __LINE__, fw_version);
    res = readBufferFromFile("/logo.bmp", buffer);
    if (res)
    {
      Log.info("%s [%d]: logo not exists. Use default\r\n", __FILE__, __LINE__);
      display_show_msg(buffer, WIFI_CONNECT, "", false, fw.c_str(), "");
    }
    else
    {
      Log.info("%s [%d]: logo not exists. Use default\r\n", __FILE__, __LINE__);
      display_show_msg(const_cast<uint8_t *>(default_icon), WIFI_CONNECT, "", false, fw.c_str(), "");
    }

    wm.setClass("invert");
    wm.setConnectTimeout(10);
    // Add a custom text label

    // Optionally, add custom CSS to style the label
    // wm.setCustomHeadElement("<style>h2 {color: blue;}</style>");

    // wm.addParameter();
    wm.setWebServerCallback(bindServerCallback);
    const char *menuhtml = "<form action='/custom' method='get'><button>Soft Reset</button></form><br/>\n";
    wm.setCustomMenuHTML(menuhtml);

    std::vector<const char *> menu = {"wifi", "custom"};
    wm.setMenu(menu);
    wm.setCustomHeadElement("<div style='text-align:center; '><h2>Warning!\nIf the portal closes before you enter the Wi-Fi credentials, please open the portal again</h2></div>");
    res = wm.startConfigPortal("TRMNL"); // password protected ap
    if (!res)
    {
      Log.error("%s [%d]: Failed to connect or hit timeout\r\n", __FILE__, __LINE__);
      wm.disconnect();
      wm.stopWebPortal();

      WiFi.disconnect();

      res = readBufferFromFile("/logo.bmp", buffer);
      if (res)
        display_show_msg(buffer, WIFI_FAILED);
      else
        display_show_msg(const_cast<uint8_t *>(default_icon), WIFI_FAILED);

      memset(log_array, 0, sizeof(log_array));
      sprintf(log_array, "%d [%d]: connection to the new WiFi failed", getTime(), __LINE__);
      log_POST(log_array, strlen(log_array));

      // Go to deep sleep
      display_sleep();
      goToSleep();
    }
    Log.info("%s [%d]: WiFi connected\r\n", __FILE__, __LINE__);
  }

  // clock synchronization
  setClock();

  if (!preferences.isKey(PREFERENCES_API_KEY) || !preferences.isKey(PREFERENCES_FRIENDLY_ID))
  {
    Log.info("%s [%d]: API key or friendly ID not saved\r\n", __FILE__, __LINE__);
    // lets get the api key and friendly ID
    getDeviceCredentials("https://usetrmnl.com");
  }
  else
  {
    Log.info("%s [%d]: API key and friendly ID saved\r\n", __FILE__, __LINE__);
  }

  // OTA checking, image checking and drawing
  https_request_err_e request_result = HTTPS_NO_ERR;
  uint8_t retries = 0;
  while ((request_result != HTTPS_SUCCES && request_result != HTTPS_NO_REGISTER && request_result != HTTPS_RESET) && retries < SERVER_MAX_RETRIES)
  {

    Log.info("%s [%d]: request retry %d...\r\n", __FILE__, __LINE__, retries);
    request_result = downloadAndShow("https://usetrmnl.com");
    Log.info("%s [%d]: request result - %d\r\n", __FILE__, __LINE__, request_result);
    bool logic = (request_result != HTTPS_SUCCES && request_result != HTTPS_NO_REGISTER);
    Log.info("%s [%d]: logic %d...\r\n", __FILE__, __LINE__, logic);
    retries++;
  }

  if (request_result == HTTPS_NO_REGISTER && need_to_refresh_display == 1)
  {
    // show the image
    String friendly_id = preferences.getString(PREFERENCES_FRIENDLY_ID, PREFERENCES_FRIENDLY_ID_DEFAULT);
    bool res = readBufferFromFile("/logo.bmp", buffer);
    if (res)
      display_show_msg(buffer, FRIENDLY_ID, friendly_id, true, "", String(message_buffer));
    else
      display_show_msg(const_cast<uint8_t *>(default_icon), FRIENDLY_ID, friendly_id, true, "", String(message_buffer));
    need_to_refresh_display = 0;
  }

  // reset checking
  if (request_result == HTTPS_RESET)
  {
    resetDeviceCredentials();
  }

  // OTA update checking
  if (update_firmware)
  {
    checkAndPerformFirmwareUpdate();
  }

  // error handling
  switch (request_result)
  {
  case HTTPS_REQUEST_FAILED:
  {
    bool res = readBufferFromFile("/logo.bmp", buffer);
    if (WiFi.RSSI() > WIFI_CONNECTION_RSSI)
    {
      if (res)
        display_show_msg(buffer, API_ERROR);
      else
        display_show_msg(const_cast<uint8_t *>(default_icon), API_ERROR);
    }
    else
    {
      if (res)
        display_show_msg(buffer, WIFI_WEAK);
      else
        display_show_msg(const_cast<uint8_t *>(default_icon), WIFI_WEAK);
    }
  }
  break;
  case HTTPS_RESPONSE_CODE_INVALID:
  {
    bool res = readBufferFromFile("/logo.bmp", buffer);
    if (res)
      display_show_msg(buffer, WIFI_INTERNAL_ERROR);
    else
      display_show_msg(const_cast<uint8_t *>(default_icon), WIFI_INTERNAL_ERROR);
  }
  break;
  case HTTPS_UNABLE_TO_CONNECT:
  {
    bool res = readBufferFromFile("/logo.bmp", buffer);
    if (WiFi.RSSI() > WIFI_CONNECTION_RSSI)
    {
      if (res)
        display_show_msg(buffer, API_ERROR);
      else
        display_show_msg(const_cast<uint8_t *>(default_icon), API_ERROR);
    }
    else
    {
      if (res)
        display_show_msg(buffer, WIFI_WEAK);
      else
        display_show_msg(const_cast<uint8_t *>(default_icon), WIFI_WEAK);
    }
  }
  break;
  case HTTPS_WRONG_IMAGE_FORMAT:
  {
    bool rs = readBufferFromFile("/logo.bmp", buffer);
    if (rs)
      display_show_msg(buffer, BMP_FORMAT_ERROR);
    else
      display_show_msg(const_cast<uint8_t *>(default_icon), BMP_FORMAT_ERROR);
  }
  break;
  case HTTPS_WRONG_IMAGE_SIZE:
  {
    bool res = readBufferFromFile("/logo.bmp", buffer);
    if (WiFi.RSSI() > WIFI_CONNECTION_RSSI)
    {
      if (res)
        display_show_msg(buffer, API_SIZE_ERROR);
      else
        display_show_msg(const_cast<uint8_t *>(default_icon), API_SIZE_ERROR);
    }
    else
    {
      if (res)
        display_show_msg(buffer, WIFI_WEAK);
      else
        display_show_msg(const_cast<uint8_t *>(default_icon), WIFI_WEAK);
    }
  }
  break;
  case HTTPS_CLIENT_FAILED:
  {
    bool res = readBufferFromFile("/logo.bmp", buffer);
    if (res)
      display_show_msg(buffer, WIFI_INTERNAL_ERROR);
    else
      display_show_msg(const_cast<uint8_t *>(default_icon), WIFI_INTERNAL_ERROR);
  }
  break;
  default:
    break;
  }

  if (request_result != HTTPS_NO_ERR)
  {
    checkLogNotes();
  }

  // display go to sleep
  display_sleep();
  if (!update_firmware)
    goToSleep();
  else
    ESP.restart();
}

/**
 * @brief Function to process business logic module
 * @param none
 * @return none
 */
void bl_process(void)
{
}

/**
 * @brief Function to parse .bmp file header
 * @param data pointer to the buffer
 * @param reserved variable address to store parsed color schematic
 * @return bmp_err_e error code
 */
static bmp_err_e parseBMPHeader(uint8_t *data, bool &reversed)
{
  // Check if the file is a BMP image
  if (data[0] != 'B' || data[1] != 'M')
  {
    Log.fatal("%s [%d]: It is not a BMP file\r\n", __FILE__, __LINE__);
    return BMP_NOT_BMP;
  }

  // Get width and height from the header
  uint32_t width = *(uint32_t *)&data[18];
  uint32_t height = *(uint32_t *)&data[22];
  uint16_t bitsPerPixel = *(uint16_t *)&data[28];
  uint32_t compressionMethod = *(uint32_t *)&data[30];
  uint32_t imageDataSize = *(uint32_t *)&data[34];
  uint32_t colorTableEntries = *(uint32_t *)&data[46];

  if (width != 800 || height != 480 || bitsPerPixel != 1 || imageDataSize != 48000 || colorTableEntries != 2)
    return BMP_BAD_SIZE;
  // Get the offset of the pixel data
  uint32_t dataOffset = *(uint32_t *)&data[10];

  // Display BMP information
  Log.info("%s [%d]: BMP Header Information:\r\nWidth: %d\r\nHeight: %d\r\nBits per Pixel: %d\r\nCompression Method: %d\r\nImage Data Size: %d\r\nColor Table Entries: %d\r\nData offset: %d\r\n", __FILE__, __LINE__, width, height, bitsPerPixel, compressionMethod, imageDataSize, colorTableEntries, dataOffset);

  // Check if there's a color table
  if (dataOffset > 54)
  {
    // Read color table entries
    uint32_t colorTableSize = colorTableEntries * 4; // Each color entry is 4 bytes

    // Display color table
    Log.info("%s [%d]: Color table\r\n", __FILE__, __LINE__);
    for (uint32_t i = 0; i < colorTableSize; i += 4)
    {
      Log.info("%s [%d]: Color %d: B-%d, R-%d, G-%d\r\n", __FILE__, __LINE__, i / 4 + 1, data[54 + 4 * i], data[55 + 4 * i], data[56 + 4 * i], data[57 + 4 * i]);
    }

    if (data[54] == 0 && data[55] == 0 && data[56] == 0 && data[57] == 0 && data[58] == 255 && data[59] == 255 && data[60] == 255 && data[61] == 0)
    {
      Log.info("%s [%d]: Color scheme standart\r\n", __FILE__, __LINE__);
      reversed = false;
    }
    else if (data[54] == 255 && data[55] == 255 && data[56] == 255 && data[57] == 0 && data[58] == 0 && data[59] == 0 && data[60] == 0 && data[61] == 0)
    {
      Log.info("%s [%d]: Color scheme reversed\r\n", __FILE__, __LINE__);
      reversed = true;
    }
    else
    {
      Log.info("%s [%d]: Color scheme demaged\r\n", __FILE__, __LINE__);
      return BMP_COLOR_SCHEME_FAILED;
    }
    return BMP_NO_ERR;
  }
  else
  {
    return BMP_INVALID_OFFSET;
  }
}

/**
 * @brief Function to ping server and download and show the image if all is OK
 * @param url Server URL addrees
 * @return https_request_err_e error code
 */
static https_request_err_e downloadAndShow(const char *url)
{
  https_request_err_e result = HTTPS_NO_ERR;
  WiFiClientSecure *client = new WiFiClientSecure;
  if (client)
  {
    client->setInsecure();
    {
      // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is
      HTTPClient https;
      Log.info("%s [%d]: RSSI: %d\r\n", __FILE__, __LINE__, WiFi.RSSI());
      Log.info("%s [%d]: [HTTPS] begin...\r\n", __FILE__, __LINE__);
      char new_url[200];
      strcpy(new_url, url);
      strcat(new_url, "/api/display");

      String api_key = "";
      if (preferences.isKey(PREFERENCES_API_KEY))
      {
        api_key = preferences.getString(PREFERENCES_API_KEY, PREFERENCES_API_KEY_DEFAULT);
        Log.info("%s [%d]: %s key exists. Value - %s\r\n", __FILE__, __LINE__, PREFERENCES_API_KEY, api_key.c_str());
      }
      else
      {
        Log.error("%s [%d]: %s key not exists.\r\n", __FILE__, __LINE__, PREFERENCES_API_KEY);
      }

      String friendly_id = "";
      if (preferences.isKey(PREFERENCES_FRIENDLY_ID))
      {
        friendly_id = preferences.getString(PREFERENCES_FRIENDLY_ID, PREFERENCES_FRIENDLY_ID_DEFAULT);
        Log.info("%s [%d]: %s key exists. Value - %s\r\n", __FILE__, __LINE__, PREFERENCES_FRIENDLY_ID, friendly_id);
      }
      else
      {
        Log.error("%s [%d]: %s key not exists.\r\n", __FILE__, __LINE__, PREFERENCES_FRIENDLY_ID);
      }

      uint32_t refresh_rate = SLEEP_TIME_TO_SLEEP;
      if (preferences.isKey(PREFERENCES_SLEEP_TIME_KEY))
      {
        refresh_rate = preferences.getUInt(PREFERENCES_SLEEP_TIME_KEY, SLEEP_TIME_TO_SLEEP);
        Log.info("%s [%d]: %s key exists. Value - %d\r\n", __FILE__, __LINE__, PREFERENCES_SLEEP_TIME_KEY, refresh_rate);
      }
      else
      {
        Log.error("%s [%d]: %s key not exists.\r\n", __FILE__, __LINE__, PREFERENCES_SLEEP_TIME_KEY);
      }

      String fw_version = String(FW_MAJOR_VERSION) + "." + String(FW_MINOR_VERSION) + "." + String(FW_PATCH_VERSION);

      float battery_voltage = readBatteryVoltage();

      Log.info("%s [%d]: Added headers:\n\rID: %s\n\rAccess-Token: %s\n\rRefresh_Rate: %s\n\rBattery-Voltage: %s\n\rFW-Version: %s\r\nRSSI: %s\r\n", __FILE__, __LINE__, WiFi.macAddress().c_str(), api_key.c_str(), String(refresh_rate).c_str(), String(battery_voltage).c_str(), fw_version.c_str(), String(WiFi.RSSI()));

      // if (https.begin(*client, new_url))
      if (https.begin(*client, "https://usetrmnl.com/api/display"))
      { // HTTPS
        Log.info("%s [%d]: RSSI: %d\r\n", __FILE__, __LINE__, WiFi.RSSI());
        Log.info("%s [%d]: [HTTPS] GET...\r\n", __FILE__, __LINE__);
        // start connection and send HTTP header
        https.addHeader("ID", WiFi.macAddress());
        https.addHeader("Access-Token", api_key);
        https.addHeader("Refresh-Rate", String(refresh_rate));
        https.addHeader("Battery-Voltage", String(battery_voltage));
        https.addHeader("FW-Version", fw_version);
        https.addHeader("RSSI", String(WiFi.RSSI()));

        delay(5);

        int httpCode = https.GET();
        // httpCode will be negative on error
        if (httpCode > 0)
        {
          // HTTP header has been send and Server response header has been handled
          Log.info("%s [%d]: GET... code: %d\r\n", __FILE__, __LINE__, httpCode);
          // file found at server
          if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
          {
            String payload = https.getString();
            size_t size = https.getSize();
            Log.info("%s [%d]: Content size: %d\r\n", __FILE__, __LINE__, size);
            Log.info("%s [%d]: Free heap size: %d\r\n", __FILE__, __LINE__, ESP.getFreeHeap());

            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, payload);
            if (error)
            {
              result = HTTPS_JSON_PARSING_ERR;
            }
            else
            {
              uint64_t request_status = doc["status"];
              Log.info("%s [%d]: status: %d\r\n", __FILE__, __LINE__, request_status);
              switch (request_status)
              {
              case 0:
              {
                String image_url = doc["image_url"];
                update_firmware = doc["update_firmware"];
                String firmware_url = doc["firmware_url"];
                uint64_t rate = doc["refresh_rate"];
                reset_firmware = doc["reset_firmware"];

                if (image_url.length() > 0)
                {
                  Log.info("%s [%d]: image_url: %s\r\n", __FILE__, __LINE__, image_url.c_str());
                  image_url.toCharArray(filename, image_url.length() + 1);
                }
                Log.info("%s [%d]: update_firmware: %d\r\n", __FILE__, __LINE__, update_firmware);
                if (firmware_url.length() > 0)
                {
                  Log.info("%s [%d]: firmware_url: %s\r\n", __FILE__, __LINE__, firmware_url.c_str());
                  firmware_url.toCharArray(binUrl, firmware_url.length() + 1);
                }
                Log.info("%s [%d]: refresh_rate: %d\r\n", __FILE__, __LINE__, rate);
                if (rate != preferences.getUInt(PREFERENCES_SLEEP_TIME_KEY, SLEEP_TIME_TO_SLEEP))
                {
                  Log.info("%s [%d]: write new refresh rate: %d\r\n", __FILE__, __LINE__, rate);
                  size_t result = preferences.putUInt(PREFERENCES_SLEEP_TIME_KEY, rate);
                  Log.info("%s [%d]: written new refresh rate: %d\r\n", __FILE__, __LINE__, result);
                }

                if (reset_firmware)
                {
                  Log.info("%s [%d]: Reset status is true\r\n", __FILE__, __LINE__);
                }
                status = true;

                if (update_firmware)
                  result = HTTPS_SUCCES;
                if (reset_firmware)
                  result = HTTPS_RESET;
              }
              break;
              case 202:
              {
                result = HTTPS_NO_REGISTER;
                Log.info("%s [%d]: write new refresh rate: %d\r\n", __FILE__, __LINE__, SLEEP_TIME_WHILE_NOT_CONNECTED);
                size_t result = preferences.putUInt(PREFERENCES_SLEEP_TIME_KEY, SLEEP_TIME_WHILE_NOT_CONNECTED);
                Log.info("%s [%d]: written new refresh rate: %d\r\n", __FILE__, __LINE__, result);
                status = false;
              }
              break;
              case 500:
              {
                result = HTTPS_RESET;
                Log.info("%s [%d]: write new refresh rate: %d\r\n", __FILE__, __LINE__, SLEEP_TIME_WHILE_NOT_CONNECTED);
                size_t result = preferences.putUInt(PREFERENCES_SLEEP_TIME_KEY, SLEEP_TIME_WHILE_NOT_CONNECTED);
                Log.info("%s [%d]: written new refresh rate: %d\r\n", __FILE__, __LINE__, result);
                status = false;
              }
              break;

              default:
                break;
              }
            }
          }
          else
          {
            Log.info("%s [%d]: [HTTPS] Unable to connect\r\n", __FILE__, __LINE__);
            result = HTTPS_REQUEST_FAILED;
            memset(log_array, 0, sizeof(log_array));
            sprintf(log_array, "%d [%d]: returned code is mot OK", getTime(), __LINE__);
            log_POST(log_array, strlen(log_array));
          }
        }
        else
        {
          Log.error("%s [%d]: [HTTPS] GET... failed, error: %s\r\n", __FILE__, __LINE__, https.errorToString(httpCode).c_str());
          result = HTTPS_RESPONSE_CODE_INVALID;

          memset(log_array, 0, sizeof(log_array));
          sprintf(log_array, "%d [%d]: HTTPS returned code is less then 0", getTime(), __LINE__);
          log_POST(log_array, strlen(log_array));
        }

        https.end();
      }
      else
      {
        Log.error("%s [%d]: [HTTPS] Unable to connect\r\n", __FILE__, __LINE__);
        result = HTTPS_UNABLE_TO_CONNECT;
        memset(log_array, 0, sizeof(log_array));
        sprintf(log_array, "%d [%d]: unable to connect to the API endpoint", getTime(), __LINE__);
        log_POST(log_array, strlen(log_array));
      }

      if (status && !update_firmware && !reset_firmware)
      {
        status = false;

        Log.info("%s [%d]: [HTTPS] Request to %s\r\n", __FILE__, __LINE__, filename);
        if (https.begin(*client, filename))
        { // HTTPS
          Log.info("%s [%d]: [HTTPS] GET..\r\n", __FILE__, __LINE__);
          Log.info("%s [%d]: RSSI: %d\r\n", __FILE__, __LINE__, WiFi.RSSI());
          // start connection and send HTTP header
          int httpCode = https.GET();

          // httpCode will be negative on error
          if (httpCode > 0)
          {
            // HTTP header has been send and Server response header has been handled
            Log.error("%s [%d]: [HTTPS] GET... code: %d\r\n", __FILE__, __LINE__, httpCode);
            Log.info("%s [%d]: RSSI: %d\r\n", __FILE__, __LINE__, WiFi.RSSI());
            // file found at server
            if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
            {
              Log.info("%s [%d]: Content size: %d\r\n", __FILE__, __LINE__, https.getSize());
              uint32_t counter = 0;
              if (https.getSize() == DISPLAY_BMP_IMAGE_SIZE)
              {
                WiFiClient *stream = https.getStreamPtr();
                Log.info("%s [%d]: RSSI: %d\r\n", __FILE__, __LINE__, WiFi.RSSI());
                Log.info("%s [%d]: Stream timeout: %d\r\n", __FILE__, __LINE__, stream->getTimeout());

                Log.info("%s [%d]: Stream available: %d\r\n", __FILE__, __LINE__, stream->available());

                uint32_t timer = millis();
                while (stream->available() < 4000 && millis() - timer < 1000)
                  ;

                Log.info("%s [%d]: Stream available: %d\r\n", __FILE__, __LINE__, stream->available());
                //  Read and save BMP data to buffer
                if (stream->available() && https.getSize() == DISPLAY_BMP_IMAGE_SIZE)
                {
                  counter = stream->readBytes(buffer, sizeof(buffer));
                }

                if (counter == DISPLAY_BMP_IMAGE_SIZE)
                {
                  Log.info("%s [%d]: Received successfully\r\n", __FILE__, __LINE__);

                  bool image_reverse = false;
                  bmp_err_e res = parseBMPHeader(buffer, image_reverse);
                  String error = "";
                  switch (res)
                  {
                  case BMP_NO_ERR:
                  {
                    // show the image
                    display_show_image(buffer, image_reverse);
                    result = HTTPS_SUCCES;
                  }
                  break;
                  case BMP_NOT_BMP:
                  {
                    error = "First two header bytes are invalid!";
                  }
                  break;
                  case BMP_BAD_SIZE:
                  {
                    error = "BMP width, height or size are invalid";
                  }
                  break;
                  case BMP_COLOR_SCHEME_FAILED:
                  {
                    error = "BMP color scheme is invalid";
                  }
                  break;
                  case BMP_INVALID_OFFSET:
                  {
                    error = "BMP header offset is invalid";
                  }
                  break;
                  default:
                    break;
                  }

                  if (res != BMP_NO_ERR)
                  {
                    memset(log_array, 0, sizeof(log_array));
                    sprintf(log_array, "%d [%d]: error parsing bmp file - %d", getTime(), __LINE__, error.c_str());
                    log_POST(log_array, strlen(log_array));

                    result = HTTPS_WRONG_IMAGE_FORMAT;
                  }
                }
                else
                {

                  Log.error("%s [%d]: Receiving failed. Readed: %d\r\n", __FILE__, __LINE__, counter);

                  // display_show_msg(const_cast<uint8_t *>(default_icon), API_SIZE_ERROR);
                  memset(log_array, 0, sizeof(log_array));
                  sprintf(log_array, "%d [%d]: HTTPS request error. Returned code - %d, available bytes - %d, received bytes - %d", getTime(), __LINE__, httpCode, https.getSize(), counter);
                  log_POST(log_array, strlen(log_array));

                  result = HTTPS_WRONG_IMAGE_SIZE;
                }
              }
              else
              {
                Log.error("%s [%d]: Receiving failed. Bad file size\r\n", __FILE__, __LINE__);

                memset(log_array, 0, sizeof(log_array));
                sprintf(log_array, "%d [%d]: HTTPS request error. Returned code - %d, available bytes - %d, received bytes - %d", getTime(), __LINE__, httpCode, https.getSize(), counter);
                log_POST(log_array, strlen(log_array));

                result = HTTPS_REQUEST_FAILED;
              }
            }
            else
            {
              Log.error("%s [%d]: [HTTPS] GET... failed, error: %s\r\n", __FILE__, __LINE__, https.errorToString(httpCode).c_str());

              result = HTTPS_REQUEST_FAILED;
              memset(log_array, 0, sizeof(log_array));
              sprintf(log_array, "%d [%d]: HTTPS returned code is not OK", getTime(), __LINE__);
              log_POST(log_array, strlen(log_array));
            }
          }
          else
          {
            Log.error("%s [%d]: [HTTPS] GET... failed, error: %s\r\n", __FILE__, __LINE__, https.errorToString(httpCode).c_str());

            memset(log_array, 0, sizeof(log_array));
            sprintf(log_array, "%d [%d]: HTTPS request failed with error - %d, %s", getTime(), __LINE__, httpCode, https.errorToString(httpCode).c_str());
            log_POST(log_array, strlen(log_array));

            result = HTTPS_REQUEST_FAILED;
          }

          https.end();
        }
        else
        {
          Log.error("%s [%d]: unable to connect\r\n", __FILE__, __LINE__);

          memset(log_array, 0, sizeof(log_array));
          sprintf(log_array, "%d [%d]: unable to connect to the API", getTime(), __LINE__);
          log_POST(log_array, strlen(log_array));

          result = HTTPS_UNABLE_TO_CONNECT;
        }
      }
      // End extra scoping block
    }

    delete client;
  }
  else
  {
    Log.error("%s [%d]: Unable to create client\r\n", __FILE__, __LINE__);

    result = HTTPS_UNABLE_TO_CONNECT;
  }

  if (send_log)
  {
    send_log = false;
  }
  Log.info("%s [%d]: Returned result - %d\r\n", __FILE__, __LINE__, result);
  return result;
}

/**
 * @brief Function to getting the friendly id and API key
 * @param url Server URL addrees
 * @return none
 */
static void getDeviceCredentials(const char *url)
{
  WiFiClientSecure *client = new WiFiClientSecure;
  if (client)
  {
    client->setInsecure();
    {
      // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is
      HTTPClient https;

      Log.info("%s [%d]: [HTTPS] begin...\r\n", __FILE__, __LINE__);
      char new_url[200];
      strcpy(new_url, url);
      strcat(new_url, "/api/setup");
      https.addHeader("ID", WiFi.macAddress());
      if (https.begin(*client, new_url))
      { // HTTPS
        Log.info("%s [%d]: RSSI: %d\r\n", __FILE__, __LINE__, WiFi.RSSI());
        Log.info("%s [%d]: [HTTPS] GET...\r\n", __FILE__, __LINE__);
        // start connection and send HTTP header

        Log.info("%s [%d]: Device MAC address: %s\r\n", __FILE__, __LINE__, WiFi.macAddress().c_str());
        int httpCode = https.GET();

        // httpCode will be negative on error
        if (httpCode > 0)
        {
          // HTTP header has been send and Server response header has been handled
          Log.info("%s [%d]: GET... code: %d\r\n", __FILE__, __LINE__, httpCode);
          // file found at server
          Log.info("%s [%d]: RSSI: %d\r\n", __FILE__, __LINE__, WiFi.RSSI());
          if (httpCode == HTTP_CODE_OK)
          {
            Log.info("%s [%d]: Content size: %d\r\n", __FILE__, __LINE__, https.getSize());
            String payload = https.getString();
            Log.info("%s [%d]: Payload: %s\r\n", __FILE__, __LINE__, payload.c_str());
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, payload);
            if (error)
            {
              Log.error("%s [%d]: JSON deserialization error.\r\n", __FILE__, __LINE__);
              https.end();
              client->stop();
              return;
            }
            uint16_t url_status = doc["status"];
            if (url_status == 200)
            {
              status = true;
              Log.info("%s [%d]: status OK.\r\n", __FILE__, __LINE__);

              String api_key = doc["api_key"];
              Log.info("%s [%d]: API key - %s\r\n", __FILE__, __LINE__, api_key.c_str());
              size_t res = preferences.putString(PREFERENCES_API_KEY, api_key);
              Log.info("%s [%d]: api key saved in the preferences - %d\r\n", __FILE__, __LINE__, res);

              String friendly_id = doc["friendly_id"];
              Log.info("%s [%d]: friendly ID - %s\r\n", __FILE__, __LINE__, friendly_id.c_str());
              res = preferences.putString(PREFERENCES_FRIENDLY_ID, friendly_id);
              Log.info("%s [%d]: friendly ID saved in the preferences - %d\r\n", __FILE__, __LINE__, res);

              String image_url = doc["image_url"];
              Log.info("%s [%d]: image_url - %s\r\n", __FILE__, __LINE__, image_url.c_str());
              image_url.toCharArray(filename, image_url.length() + 1);

              String message_str = doc["message"];
              Log.info("%s [%d]: message - %s\r\n", __FILE__, __LINE__, message_str.c_str());
              message_str.toCharArray(message_buffer, message_str.length() + 1);

              Log.info("%s [%d]: status - %d\r\n", __FILE__, __LINE__, status);
            }
            else
            {
              Log.info("%s [%d]: status FAIL.\r\n", __FILE__, __LINE__);
              status = false;
            }
          }
          else
          {
            Log.info("%s [%d]: [HTTPS] Unable to connect\r\n", __FILE__, __LINE__);

            bool res = readBufferFromFile("/logo.bmp", buffer);
            if (WiFi.RSSI() > WIFI_CONNECTION_RSSI)
            {
              if (res)
                display_show_msg(buffer, API_ERROR);
              else
                display_show_msg(const_cast<uint8_t *>(default_icon), API_ERROR);
            }
            else
            {
              if (res)
                display_show_msg(buffer, WIFI_WEAK);
              else
                display_show_msg(const_cast<uint8_t *>(default_icon), WIFI_WEAK);
            }
            memset(log_array, 0, sizeof(log_array));
            sprintf(log_array, "%d [%d]: returned code is not OK", getTime(), __LINE__);
            log_POST(log_array, strlen(log_array));
          }
        }
        else
        {
          Log.error("%s [%d]: [HTTPS] GET... failed, error: %s\r\n", __FILE__, __LINE__, https.errorToString(httpCode).c_str());
          bool res = readBufferFromFile("/logo.bmp", buffer);
          if (WiFi.RSSI() > WIFI_CONNECTION_RSSI)
          {
            if (res)
              display_show_msg(buffer, API_ERROR);
            else
              display_show_msg(const_cast<uint8_t *>(default_icon), API_ERROR);
          }
          else
          {
            if (res)
              display_show_msg(buffer, WIFI_WEAK);
            else
              display_show_msg(const_cast<uint8_t *>(default_icon), WIFI_WEAK);
          }
          memset(log_array, 0, sizeof(log_array));
          sprintf(log_array, "%d [%d]: HTTPS returned code is less then 0", getTime(), __LINE__);
          log_POST(log_array, strlen(log_array));
        }

        https.end();
      }
      else
      {
        Log.error("%s [%d]: [HTTPS] Unable to connect\r\n", __FILE__, __LINE__);
        bool res = readBufferFromFile("/logo.bmp", buffer);
        if (res)
          display_show_msg(buffer, WIFI_INTERNAL_ERROR);
        else
          display_show_msg(const_cast<uint8_t *>(default_icon), WIFI_INTERNAL_ERROR);
        memset(log_array, 0, sizeof(log_array));
        sprintf(log_array, "%d [%d]: unable to connect to the API", getTime(), __LINE__);
        log_POST(log_array, strlen(log_array));
      }
      Log.info("%s [%d]: status - %d\r\n", __FILE__, __LINE__, status);
      if (status)
      {
        status = false;
        Log.info("%s [%d]: filename - %s\r\n", __FILE__, __LINE__, filename);

        Log.info("%s [%d]: [HTTPS] Request to %s\r\n", __FILE__, __LINE__, filename);
        if (https.begin(*client, filename))
        { // HTTPS
          Log.info("%s [%d]: [HTTPS] GET..\r\n", __FILE__, __LINE__);
          // start connection and send HTTP header
          int httpCode = https.GET();

          // httpCode will be negative on error
          if (httpCode > 0)
          {
            // HTTP header has been send and Server response header has been handled
            Log.error("%s [%d]: [HTTPS] GET... code: %d\r\n", __FILE__, __LINE__, httpCode);
            // file found at server
            if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
            {
              Log.info("%s [%d]: Content size: %d\r\n", __FILE__, __LINE__, https.getSize());

              WiFiClient *stream = https.getStreamPtr();

              uint32_t counter = 0;
              // Read and save BMP data to buffer
              if (stream->available() && https.getSize() == sizeof(buffer))
              {
                counter = stream->readBytes(buffer, sizeof(buffer));
              }
              https.end();
              if (counter == sizeof(buffer))
              {
                Log.info("%s [%d]: Received successfully\r\n", __FILE__, __LINE__);

                bool res = writeBufferToFile("/logo.bmp", buffer, sizeof(buffer));
                if (res)
                  Log.info("%s [%d]: File written!\r\n", __FILE__, __LINE__);
                else
                  Log.error("%s [%d]: File not written!\r\n", __FILE__, __LINE__);

                // show the image
                String friendly_id = preferences.getString(PREFERENCES_FRIENDLY_ID, PREFERENCES_FRIENDLY_ID_DEFAULT);
                display_show_msg(buffer, FRIENDLY_ID, friendly_id, true, "", String(message_buffer));
                need_to_refresh_display = 0;
              }
              else
              {
                Log.error("%s [%d]: Receiving failed. Readed: %d\r\n", __FILE__, __LINE__, counter);
                bool res = readBufferFromFile("/logo.bmp", buffer);
                if (WiFi.RSSI() > WIFI_CONNECTION_RSSI)
                {
                  if (res)
                    display_show_msg(buffer, API_SIZE_ERROR);
                  else
                    display_show_msg(const_cast<uint8_t *>(default_icon), API_SIZE_ERROR);
                }
                else
                {
                  if (res)
                    display_show_msg(buffer, WIFI_WEAK);
                  else
                    display_show_msg(const_cast<uint8_t *>(default_icon), WIFI_WEAK);
                }
                memset(log_array, 0, sizeof(log_array));
                sprintf(log_array, "%d [%d]:Receiving failed. Readed: %d", getTime(), __LINE__, counter);
                log_POST(log_array, strlen(log_array));
              }
            }
            else
            {
              Log.error("%s [%d]: [HTTPS] GET... failed, error: %s\r\n", __FILE__, __LINE__, https.errorToString(httpCode).c_str());
              https.end();
              bool res = readBufferFromFile("/logo.bmp", buffer);
              if (WiFi.RSSI() > WIFI_CONNECTION_RSSI)
              {
                if (res)
                  display_show_msg(buffer, API_ERROR);
                else
                  display_show_msg(const_cast<uint8_t *>(default_icon), API_ERROR);
              }
              else
              {
                if (res)
                  display_show_msg(buffer, WIFI_WEAK);
                else
                  display_show_msg(const_cast<uint8_t *>(default_icon), WIFI_WEAK);
              }
              memset(log_array, 0, sizeof(log_array));
              sprintf(log_array, "%d [%d]: HTTPS received code is not OK", getTime(), __LINE__);
              log_POST(log_array, strlen(log_array));
            }
          }
          else
          {
            Log.error("%s [%d]: [HTTPS] GET... failed, error: %s\r\n", __FILE__, __LINE__, https.errorToString(httpCode).c_str());
            bool res = readBufferFromFile("/logo.bmp", buffer);
            if (WiFi.RSSI() > WIFI_CONNECTION_RSSI)
            {
              if (res)
                display_show_msg(buffer, API_ERROR);
              else
                display_show_msg(const_cast<uint8_t *>(default_icon), API_ERROR);
            }
            else
            {
              if (res)
                display_show_msg(buffer, WIFI_WEAK);
              else
                display_show_msg(const_cast<uint8_t *>(default_icon), WIFI_WEAK);
            }
            memset(log_array, 0, sizeof(log_array));
            sprintf(log_array, "%d [%d]: HTTPS returned code is less then 0", getTime(), __LINE__);
            log_POST(log_array, strlen(log_array));
          }
        }
        else
        {
          Log.error("%s [%d]: unable to connect\r\n", __FILE__, __LINE__);
          bool res = readBufferFromFile("/logo.bmp", buffer);
          if (WiFi.RSSI() > WIFI_CONNECTION_RSSI)
          {
            if (res)
              display_show_msg(buffer, API_ERROR);
            else
              display_show_msg(const_cast<uint8_t *>(default_icon), API_ERROR);
          }
          else
          {
            if (res)
              display_show_msg(buffer, WIFI_WEAK);
            else
              display_show_msg(const_cast<uint8_t *>(default_icon), WIFI_WEAK);
          }
          memset(log_array, 0, sizeof(log_array));
          sprintf(log_array, "%d [%d]: unable to connect to the APU", getTime(), __LINE__);
          log_POST(log_array, strlen(log_array));
        }
      }
      // End extra scoping block
    }
    client->stop();
    delete client;
  }
  else
  {
    Log.error("%s [%d]: Unable to create client\r\n", __FILE__, __LINE__);
    bool res = readBufferFromFile("/logo.bmp", buffer);
    if (res)
      display_show_msg(buffer, WIFI_INTERNAL_ERROR);
    else
      display_show_msg(const_cast<uint8_t *>(default_icon), WIFI_INTERNAL_ERROR);
    memset(log_array, 0, sizeof(log_array));
    sprintf(log_array, "%d [%d]: unable to create the client", getTime(), __LINE__);
    log_POST(log_array, strlen(log_array));
  }
}

/**
 * @brief Function to reset the friendly id, API key, WiFi SSID and password
 * @param url Server URL addrees
 * @return none
 */
static void resetDeviceCredentials(void)
{
  Log.info("%s [%d]: The device will be reset now...\r\n", __FILE__, __LINE__);
  Log.info("%s [%d]: WiFi reseting...\r\n", __FILE__, __LINE__);
  wm.resetSettings();
  need_to_refresh_display = 1;
  bool res = preferences.clear();
  if (res)
    Log.info("%s [%d]: The device reseted success. Restarting...\r\n", __FILE__, __LINE__);
  else
    Log.error("%s [%d]: The device reseting error. The device will be reset now...\r\n", __FILE__, __LINE__);
  preferences.end();
  ESP.restart();
}

/**
 * @brief Function to reading image buffer from file
 * @param out_buffer buffer pointer
 * @return 1 - if success; 0 - if failed
 */
static bool readBufferFromFile(const char *name, uint8_t *out_buffer)
{
  if (SPIFFS.exists(name))
  {
    Log.info("%s [%d]: icon exists\r\n", __FILE__, __LINE__);
    File file = SPIFFS.open("name", FILE_READ);
    if (file)
    {
      if (file.size() == DISPLAY_BMP_IMAGE_SIZE)
      {
        Log.info("%s [%d]: the size is the same\r\n", __FILE__, __LINE__);
        file.readBytes((char *)out_buffer, DISPLAY_BMP_IMAGE_SIZE);
      }
      else
      {
        Log.error("%s [%d]: the size is NOT the same %d\r\n", __FILE__, __LINE__, file.size());
        return false;
      }
      file.close();
      return true;
    }
    else
    {
      Log.error("%s [%d]: File open ERROR\r\n", __FILE__, __LINE__);
      return false;
    }
  }
  else
  {
    Log.error("%s [%d]: icon DOESN\'T exists\r\n", __FILE__, __LINE__);
    return false;
  }
}

/**
 * @brief Function to reading image buffer from file
 * @param name buffer pointer
 * @return 1 - if success; 0 - if failed
 */
static bool writeBufferToFile(const char *name, uint8_t *in_buffer, uint16_t size)
{
  if (SPIFFS.exists(name))
  {
    Log.info("%s [%d]: file %s exists. Deleting...\r\n", __FILE__, __LINE__, name);
    if (SPIFFS.remove(name))
      Log.info("%s [%d]: file %s deleted\r\n", __FILE__, __LINE__, name);
    else
      Log.info("%s [%d]: file %s deleting failed\r\n", __FILE__, __LINE__, name);
  }
  else
  {
    Log.info("%s [%d]: file %s not exists.\r\n", __FILE__, __LINE__, name);
  }
  File file = SPIFFS.open(name, FILE_APPEND);
  Serial.println(file);
  if (file)
  {
    size_t res = file.write(in_buffer, size);
    file.close();
    if (res)
    {
      Log.info("%s [%d]: file %s writing success\r\n", __FILE__, __LINE__, name);
      return true;
    }
    else
    {
      Log.error("%s [%d]: file %s writing error\r\n", __FILE__, __LINE__, name);
      return false;
    }
  }
  else
  {
    Log.error("%s [%d]: File open ERROR\r\n", __FILE__, __LINE__);
    return false;
  }
}

/**
 * @brief Function to check and performing OTA update
 * @param none
 * @return none
 */
static void checkAndPerformFirmwareUpdate(void)
{
  WiFiClientSecure *client = new WiFiClientSecure;
  if (client)
  {
    client->setInsecure();
    HTTPClient https;
    if (https.begin(*client, binUrl))
    {
      int httpCode = https.GET();
      if (httpCode == HTTP_CODE_OK)
      {
        Log.info("%s [%d]: Downloading .bin file...\r\n", __FILE__, __LINE__);

        size_t contentLength = https.getSize();
        // Perform firmware update
        if (Update.begin(contentLength))
        {
          Log.info("%s [%d]: Firmware update start\r\n", __FILE__, __LINE__);
          bool res = readBufferFromFile("/logo.bmp", buffer);
          if (res)
            display_show_msg(buffer, FW_UPDATE);
          else
            display_show_msg(const_cast<uint8_t *>(default_icon), FW_UPDATE);
          if (Update.writeStream(https.getStream()))
          {
            if (Update.end(true))
            {
              Log.info("%s [%d]: Firmware update successful. Rebooting...\r\n", __FILE__, __LINE__);
              bool res = readBufferFromFile("/logo.bmp", buffer);
              if (res)
                display_show_msg(buffer, FW_UPDATE_SUCCESS);
              else
                display_show_msg(const_cast<uint8_t *>(default_icon), FW_UPDATE_SUCCESS);
            }
            else
            {
              Log.fatal("%s [%d]: Firmware update failed!\r\n", __FILE__, __LINE__);
              bool res = readBufferFromFile("/logo.bmp", buffer);
              if (res)
                display_show_msg(buffer, FW_UPDATE_FAILED);
              else
                display_show_msg(const_cast<uint8_t *>(default_icon), FW_UPDATE_FAILED);
            }
          }
          else
          {
            Log.fatal("%s [%d]: Write to firmware update stream failed!\r\n", __FILE__, __LINE__);
            bool res = readBufferFromFile("/logo.bmp", buffer);
            if (res)
              display_show_msg(buffer, FW_UPDATE_FAILED);
            else
              display_show_msg(const_cast<uint8_t *>(default_icon), FW_UPDATE_FAILED);
          }
        }
        else
        {
          Log.fatal("%s [%d]: Begin firmware update failed!\r\n", __FILE__, __LINE__);
          bool res = readBufferFromFile("/logo.bmp", buffer);
          if (res)
            display_show_msg(buffer, FW_UPDATE_FAILED);
          else
            display_show_msg(const_cast<uint8_t *>(default_icon), FW_UPDATE_FAILED);
        }
      }
      else
      {
        Log.fatal("%s [%d]: HTTP GET failed!\r\n", __FILE__, __LINE__);
        bool res = readBufferFromFile("/logo.bmp", buffer);
        if (WiFi.RSSI() > WIFI_CONNECTION_RSSI)
        {
          if (res)
            display_show_msg(buffer, API_ERROR);
          else
            display_show_msg(const_cast<uint8_t *>(default_icon), API_ERROR);
        }
        else
        {
          if (res)
            display_show_msg(buffer, WIFI_WEAK);
          else
            display_show_msg(const_cast<uint8_t *>(default_icon), WIFI_WEAK);
        }
      }
      https.end();
    }
  }
  delete client;
}

/**
 * @brief Function to sleep prepearing and go to sleep
 * @param none
 * @return none
 */
static void goToSleep(void)
{
  uint32_t time_to_sleep = SLEEP_TIME_TO_SLEEP;
  if (preferences.isKey(PREFERENCES_SLEEP_TIME_KEY))
    time_to_sleep = preferences.getUInt(PREFERENCES_SLEEP_TIME_KEY, SLEEP_TIME_TO_SLEEP);
  Log.info("%s [%d]: time to sleep - %d\r\n", __FILE__, __LINE__, time_to_sleep);
  preferences.end();
  esp_sleep_enable_timer_wakeup(time_to_sleep * SLEEP_uS_TO_S_FACTOR);
  esp_deep_sleep_enable_gpio_wakeup(1 << PIN_INTERRUPT,
                                    ESP_GPIO_WAKEUP_GPIO_LOW);
  esp_deep_sleep_start();
}

// Not sure if WiFiClientSecure checks the validity date of the certificate.
// Setting clock just to be sure...
/**
 * @brief Function to clock synchronization
 * @param none
 * @return none
 */
static void setClock()
{
  Log.info("%s [%d]: Time synchronization... Attempt 1...\r\n", __FILE__, __LINE__);
  configTime(0, 0, "pool.ntp.org");
  delay(500);
  // time_t nowSecs = time(nullptr);
  // while (nowSecs < 8 * 3600 * 2)
  // {
  //   delay(500);
  //   yield();
  //   nowSecs = time(nullptr);
  // }

  // struct tm timeinfo;
  // gmtime_r(&nowSecs, &timeinfo);

  // Wait for time to be set
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Log.info("%s [%d]: Time synchronization failed... Attempt 2...\r\n", __FILE__, __LINE__);
    configTime(0, 0, "time.windows.com");
    delay(500);
    if (!getLocalTime(&timeinfo))
    {
      Log.info("%s [%d]: Time synchronization failed... Attempt 3...\r\n", __FILE__, __LINE__);
      configTime(0, 0, "time.google.com");
      delay(500);
      if (!getLocalTime(&timeinfo))
      {
        Log.info("%s [%d]: Time synchronization failed after 3 attempts...\r\n", __FILE__, __LINE__);
        return;
      }
    }
  }

  Log.info("%s [%d]: Current time - %s\r\n", __FILE__, __LINE__, asctime(&timeinfo));
}

/**
 * @brief Function to read the battery voltage
 * @param none
 * @return float voltage in Volts
 */
static float readBatteryVoltage(void)
{
  Log.info("%s [%d]: Battery voltage reading...\r\n", __FILE__, __LINE__);
  int32_t adc = 0;
  for (uint8_t i = 0; i < 128; i++)
  {
    adc += analogReadMilliVolts(PIN_BATTERY);
  }

  int32_t sensorValue = (adc / 128) * 2;

  float voltage = sensorValue / 1000.0;
  return voltage;
}

/**
 * @brief Function to send the log note
 * @param log_buffer pointer to the buffer that contains log note
 * @param size size of buffer
 * @return none
 */
static void log_POST(char *log_buffer, size_t size)
{
  WiFiClientSecure *client = new WiFiClientSecure;
  bool result = false;
  if (client)
  {
    client->setInsecure();

    {
      // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is
      HTTPClient https;
      Log.info("%s [%d]: [HTTPS] begin...\r\n", __FILE__, __LINE__);
      if (https.begin(*client, "https://usetrmnl.com/api/log"))
      { // HTTPS
        Log.info("%s [%d]: [HTTPS] POST...\r\n", __FILE__, __LINE__);

        String api_key = "";
        if (preferences.isKey(PREFERENCES_API_KEY))
        {
          api_key = preferences.getString(PREFERENCES_API_KEY, PREFERENCES_API_KEY_DEFAULT);
          Log.info("%s [%d]: %s key exists. Value - %s\r\n", __FILE__, __LINE__, PREFERENCES_API_KEY, api_key.c_str());
        }
        else
        {
          Log.error("%s [%d]: %s key not exists.\r\n", __FILE__, __LINE__, PREFERENCES_API_KEY);
        }

        https.addHeader("Accept", "application/json");
        https.addHeader("Access-Token", api_key);
        https.addHeader("Content-Type", "application/json");
        char buffer[512] = {0};
        sprintf(buffer, "{\"log\":{\"dump\":{\"error\":\"%s\"}}}", log_buffer);
        Log.info("%s [%d]: Send log - %s\r\n", __FILE__, __LINE__, buffer);
        // start connection and send HTTP header
        int httpCode = https.POST(buffer);

        // httpCode will be negative on error
        if (httpCode > 0)
        {
          // HTTP header has been send and Server response header has been handled
          Log.info("%s [%d]: [HTTPS] POST... code: %d\r\n", __FILE__, __LINE__, httpCode);
          // file found at server
          if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY || httpCode == HTTP_CODE_NO_CONTENT)
          {
            result = true;
            String payload = https.getString();
          }
        }
        else
        {
          Log.error("%s [%d]: [HTTPS] POST... failed, error: %s\r\n", __FILE__, __LINE__, https.errorToString(httpCode).c_str());
          result = false;
        }

        https.end();
      }
      else
      {
        Log.error("%s [%d]: [HTTPS] Unable to connect\r\n", __FILE__, __LINE__);
        result = false;
      }

      // End extra scoping block
    }

    delete client;
  }
  else
  {
    Log.error("%s [%d]: [HTTPS] Unable to create client\r\n", __FILE__, __LINE__);
    result = false;
  }
  if (!result)
  {
    // log not send
    for (uint8_t i = 0; i < LOG_MAX_NOTES_NUMBER; i++)
    {
      String key = PREFERENCES_LOG_KEY + String(i);
      if (preferences.isKey(key.c_str()))
      {
        Log.info("%s [%d]: key %s exists\r\n", __FILE__, __LINE__, key);
        result = false;
      }
      else
      {
        Log.info("%s [%d]: key %s not exists\r\n", __FILE__, __LINE__, key);
        size_t res = preferences.putString(key.c_str(), log_buffer);
        Log.info("%s [%d]: Initial size %d. Received size - %d\r\n", __FILE__, __LINE__, size, res);
        if (res == size)
        {
          Log.info("%s [%d]: log note written success\r\n", __FILE__, __LINE__);
        }
        else
        {
          Log.info("%s [%d]: log note writing failed\r\n", __FILE__, __LINE__);
        }
        result = true;
        break;
      }
    }
    if (!result)
    {
      uint8_t head = 0;
      if (preferences.isKey(PREFERENCES_LOG_BUFFER_HEAD_KEY))
      {
        Log.info("%s [%d]: head exists\r\n", __FILE__, __LINE__);
        head = preferences.getUChar(PREFERENCES_LOG_BUFFER_HEAD_KEY, 0);
      }
      else
      {
        Log.info("%s [%d]: head NOT exists\r\n", __FILE__, __LINE__);
      }

      String key = PREFERENCES_LOG_KEY + String(head);
      size_t res = preferences.putString(key.c_str(), log_buffer);
      if (res == size)
      {
        Log.info("%s [%d]: log note written success\r\n", __FILE__, __LINE__);
      }
      else
      {
        Log.info("%s [%d]: log note writing failed\r\n", __FILE__, __LINE__);
      }

      head += 1;
      if (head == LOG_MAX_NOTES_NUMBER)
      {
        head = 0;
      }

      uint8_t result_write = preferences.putUChar(PREFERENCES_LOG_BUFFER_HEAD_KEY, head);
      if (result_write)
        Log.info("%s [%d]: head written success\r\n", __FILE__, __LINE__);
      else
        Log.info("%s [%d]: head note writing failed\r\n", __FILE__, __LINE__);
    }
  }
}

static void handleRoute(void)
{
  Log.info("%s [%d]: handle portal callback\r\n", __FILE__, __LINE__);

  wm.stopConfigPortal();
  resetDeviceCredentials();
}

static void bindServerCallback(void)
{
  wm.server->on("/custom", handleRoute); // this is now crashing esp32 for some reason
  // wm.server->on("/info",handleRoute); // you can override wm!
}

static uint32_t getTime(void)
{
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    // Serial.println("Failed to obtain time");
    return (0);
  }
  time(&now);
  return now;
}

static void checkLogNotes(void)
{
  String log;
  for (uint8_t i = 0; i < LOG_MAX_NOTES_NUMBER; i++)
  {
    String key = PREFERENCES_LOG_KEY + String(i);
    if (preferences.isKey(key.c_str()))
    {
      Log.info("%s [%d]: log note exists\r\n", __FILE__, __LINE__);
      String note = preferences.getString(key.c_str(), "");
      if (note.length() > 0)
      {
        log += note;
        log += ",";
      }
    }
  }

  bool result = false;
  if (log.length() > 0)
  {
    Log.info("%s [%d]: log string - %s\r\n", __FILE__, __LINE__, log.c_str());
    Log.info("%s [%d]: need to send the log\r\n", __FILE__, __LINE__);
    WiFiClientSecure *client = new WiFiClientSecure;

    if (client)
    {
      client->setInsecure();

      {
        // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is
        HTTPClient https;
        Log.info("%s [%d]: [HTTPS] begin...\r\n", __FILE__, __LINE__);
        if (https.begin(*client, "https://usetrmnl.com/api/log"))
        { // HTTPS
          Log.info("%s [%d]: [HTTPS] POST...\r\n", __FILE__, __LINE__);

          String api_key = "";
          if (preferences.isKey(PREFERENCES_API_KEY))
          {
            api_key = preferences.getString(PREFERENCES_API_KEY, PREFERENCES_API_KEY_DEFAULT);
            Log.info("%s [%d]: %s key exists. Value - %s\r\n", __FILE__, __LINE__, PREFERENCES_API_KEY, api_key.c_str());
          }
          else
          {
            Log.error("%s [%d]: %s key not exists.\r\n", __FILE__, __LINE__, PREFERENCES_API_KEY);
          }

          https.addHeader("Accept", "application/json");
          https.addHeader("Access-Token", api_key);
          https.addHeader("Content-Type", "application/json");
          // char buffer[512 * LOG_MAX_NOTES_NUMBER] = {0};
          String buffer = "{\"log\":{\"dump\":{\"error\":\"" + log;
          buffer = buffer + "\"}}}";
          // sprintf(buffer, %s, log);
          Log.info("%s [%d]: Send log - %s\r\n", __FILE__, __LINE__, buffer.c_str());
          // start connection and send HTTP header
          int httpCode = https.POST(buffer);

          // httpCode will be negative on error
          if (httpCode > 0)
          {
            // HTTP header has been send and Server response header has been handled
            Log.info("%s [%d]: [HTTPS] POST... code: %d\r\n", __FILE__, __LINE__, httpCode);
            // file found at server
            if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY || httpCode == HTTP_CODE_NO_CONTENT)
            {
              result = true;
              Log.info("%s [%d]: [HTTPS] POST OK\r\n", __FILE__, __LINE__);
              // String payload = https.getString();
            }
          }
          else
          {
            Log.error("%s [%d]: [HTTPS] POST... failed, error: %s\r\n", __FILE__, __LINE__, https.errorToString(httpCode).c_str());
            result = false;
          }

          https.end();
        }
        else
        {
          Log.error("%s [%d]: [HTTPS] Unable to connect\r\n", __FILE__, __LINE__);
          result = false;
        }

        // End extra scoping block
      }

      delete client;
    }
    else
    {
      Log.error("%s [%d]: [HTTPS] Unable to create client\r\n", __FILE__, __LINE__);
      result = false;
    }
  }
  else
  {
    Log.info("%s [%d]: no needed to send the log\r\n", __FILE__, __LINE__);
  }
  if (result == true)
  {
    for (uint8_t i = 0; i < LOG_MAX_NOTES_NUMBER; i++)
    {
      String key = PREFERENCES_LOG_KEY + String(i);
      if (preferences.isKey(key.c_str()))
      {
        Log.info("%s [%d]: log note exists\r\n", __FILE__, __LINE__);
        bool note_del = preferences.remove(key.c_str());
        if (note_del)
          Log.info("%s [%d]: log note deleted\r\n", __FILE__, __LINE__);
        else
          Log.info("%s [%d]: log note not deleted\r\n", __FILE__, __LINE__);
      }
    }
  }
}