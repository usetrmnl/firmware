#include <Arduino.h>
#include <bl.h>
#include <types.h>
#include <ArduinoLog.h>
#include <WifiCaptive.h>
#include <pins.h>
#include <config.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <display.h>
#include <stdlib.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ImageData.h>
#include <Preferences.h>
#include <cstdint>
#include <bmp.h>
#include <Update.h>
#include <math.h>
#include <filesystem.h>
#include "log.h"
#include <stored_logs.h>

bool pref_clear = false;

uint8_t buffer[48062];    // image buffer
char filename[1024];      // image URL
char binUrl[1024];        // update URL
char log_array[512];      // log
char message_buffer[128]; // message to show on the screen

bool status = false;          // need to download a new image
bool update_firmware = false; // need to download a new firmaware
bool reset_firmware = false;  // need to reset credentials
bool send_log = false;        // need to send logs
bool not_reset = false;
bool double_click = false;
esp_sleep_wakeup_cause_t wakeup_reason = ESP_SLEEP_WAKEUP_UNDEFINED; // wake-up reason
MSG current_msg = NONE;
SPECIAL_FUNCTION special_function = SF_NONE;
RTC_DATA_ATTR uint8_t need_to_refresh_display = 1;

// timers
uint32_t button_timer = 0;
bool twice = false;

Preferences preferences;

static https_request_err_e downloadAndShow(const char *url); // download and show the image
static void getDeviceCredentials(const char *url);           // receiveing API key and Friendly ID
static void resetDeviceCredentials(void);                    // reset device credentials API key, Friendly ID, Wi-Fi SSID and password
static void checkAndPerformFirmwareUpdate(void);             // OTA update
static void goToSleep(void);                                 // sleep prepearing
static void setClock(void);                                  // clock synchrinization
static float readBatteryVoltage(void);                       // battery voltage reading
static void log_POST(char *log_buffer, size_t size);         // log sending
static void handleRoute(void);
static void checkLogNotes(void);
static void writeSpecialFunction(SPECIAL_FUNCTION function);
static void writeImageToFile(const char *name, uint8_t *in_buffer, size_t size);
static uint32_t getTime(void);
static void showMessageWithLogo(MSG message_type);
static void showMessageWithLogo(MSG message_type, String friendly_id, bool id, const char *fw_version, String message);
static uint8_t *storedLogoOrDefault(void);
static bool saveCurrentFileName(String &name);
static bool checkCureentFileName(String &newName);

/**
 * @brief Function to init business logic module
 * @param none
 * @return none
 */
void bl_init(void)
{

  Serial.begin(115200);
  Log.begin(LOG_LEVEL_VERBOSE, &Serial);
  Log_info("BL init success");
  Log_info("Firware version %d.%d.%d", FW_MAJOR_VERSION, FW_MINOR_VERSION, FW_PATCH_VERSION);
  pins_init();
  button_timer = millis();

  wakeup_reason = esp_sleep_get_wakeup_cause();
  Log.info("%s [%d]: Wakeup was not caused by deep sleep: %d\r\n", __FILE__, __LINE__, wakeup_reason);

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
        Log_fatal("preferences clearing error");
    }
  }
  else
  {
    Log.fatal("%s [%d]: preferences init failed\r\n", __FILE__, __LINE__);
    ESP.restart();
  }
  Log.info("%s [%d]: preferences end\r\n", __FILE__, __LINE__);

  Log.info("%s [%d]: button handling start\r\n", __FILE__, __LINE__);

  special_function = SF_NONE;

  Log.info("%s [%d]: Wakeup was not caused by deep sleep: %d\r\n", __FILE__, __LINE__, wakeup_reason);
  if (wakeup_reason == ESP_SLEEP_WAKEUP_GPIO)
  {
    while (1)
    {
      if (digitalRead(PIN_INTERRUPT) == LOW && millis() - button_timer > BUTTON_HOLD_TIME)
      {
        Log.info("%s [%d]: WiFi reset\r\n", __FILE__, __LINE__);
        WifiCaptivePortal.resetSettings();
        special_function = SF_NONE;
        break;
      }
      else if (digitalRead(PIN_INTERRUPT) == HIGH && !not_reset)
      {
        Log.info("%s [%d]: WiFi NOT reset\r\n", __FILE__, __LINE__);
        not_reset = true;
      }
      else if (digitalRead(PIN_INTERRUPT) == LOW && not_reset)
      {
        Log.info("%s [%d]: double click\r\n", __FILE__, __LINE__);
        double_click = true;
        break;
      }
      else if (digitalRead(PIN_INTERRUPT) == HIGH && millis() - button_timer > 2000)
      {
        Log.info("%s [%d]: NOT reset. Out\r\n", __FILE__, __LINE__);
        special_function = SF_NONE;
        break;
      }
    }
    Log.info("%s [%d]: button handling end\r\n", __FILE__, __LINE__);
  }

  if (double_click)
  { // special function reading
    if (preferences.isKey(PREFERENCES_SF_KEY))
    {
      Log.info("%s [%d]: SF saved. Reading...\r\n", __FILE__, __LINE__);
      special_function = (SPECIAL_FUNCTION)preferences.getUInt(PREFERENCES_SF_KEY, 0);
      Log.info("%s [%d]: Readed special function - %d\r\n", __FILE__, __LINE__, special_function);
      switch (special_function)
      {
      case SF_IDENTIFY:
      {
        Log.info("%s [%d]: Identify special function...It will be handled while API ping...\r\n", __FILE__, __LINE__);
      }
      break;
      case SF_SLEEP:
      {
        Log.info("%s [%d]: Sleep special function...\r\n", __FILE__, __LINE__);
        // still in progress
      }
      break;
      case SF_ADD_WIFI:
      {
        Log.info("%s [%d]: Add WiFi function...\r\n", __FILE__, __LINE__);
        // still in progress
      }
      break;
      case SF_RESTART_PLAYLIST:
      {
        Log.info("%s [%d]: Identify special function...It will be handled while API ping...\r\n", __FILE__, __LINE__);
      }
      break;
      case SF_REWIND:
      {
        Log.info("%s [%d]: Rewind special function...\r\n", __FILE__, __LINE__);
      }
      break;
      case SF_SEND_TO_ME:
      {
        Log.info("%s [%d]: Send to me special function...It will be handled while API ping...\r\n", __FILE__, __LINE__);
      }
      break;
      default:
        break;
      }
    }
    else
    {
      Log_error("SF not saved");
    }
  }
  // EPD init
  // EPD clear
  Log.info("%s [%d]: Display init\r\n", __FILE__, __LINE__);
  display_init();

  if (wakeup_reason != ESP_SLEEP_WAKEUP_TIMER)
  {
    Log.info("%s [%d]: Display clear\r\n", __FILE__, __LINE__);
    display_reset();
  }

  // Mount SPIFFS
  filesystem_init();

  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  if (WifiCaptivePortal.isSaved())
  {
    // WiFi saved, connection
    Log.info("%s [%d]: WiFi saved\r\n", __FILE__, __LINE__);
    WiFi.begin(WifiCaptivePortal.getSSID().c_str(), WifiCaptivePortal.getPassword().c_str());

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
        showMessageWithLogo(WIFI_FAILED);
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

    showMessageWithLogo(WIFI_CONNECT, "", false, fw.c_str(), "");
    WifiCaptivePortal.setResetSettingsCallback(resetDeviceCredentials);
    res = WifiCaptivePortal.startPortal();
    if (!res)
    {
      Log.error("%s [%d]: Failed to connect or hit timeout\r\n", __FILE__, __LINE__);

      WiFi.disconnect();

      showMessageWithLogo(WIFI_FAILED);

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
  while ((request_result != HTTPS_SUCCES && request_result != HTTPS_NO_REGISTER && request_result != HTTPS_RESET && request_result != HTTPS_PLUGIN_NOT_ATTACHED) && retries < SERVER_MAX_RETRIES)
  {

    Log.info("%s [%d]: request retry %d...\r\n", __FILE__, __LINE__, retries);
    request_result = downloadAndShow("https://usetrmnl.com");
    Log.info("%s [%d]: request result - %d\r\n", __FILE__, __LINE__, request_result);
    retries++;
  }

  if (request_result == HTTPS_NO_REGISTER && need_to_refresh_display == 1)
  {
    // show the image
    String friendly_id = preferences.getString(PREFERENCES_FRIENDLY_ID, PREFERENCES_FRIENDLY_ID_DEFAULT);
    showMessageWithLogo(FRIENDLY_ID, friendly_id, true, "", String(message_buffer));
    need_to_refresh_display = 0;
  }

  // reset checking
  if (request_result == HTTPS_RESET)
  {
    Log.info("%s [%d]: Device reseting...\r\n", __FILE__, __LINE__);
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
    if (WiFi.RSSI() > WIFI_CONNECTION_RSSI)
    {
      showMessageWithLogo(API_ERROR);
    }
    else
    {
      showMessageWithLogo(WIFI_WEAK);
    }
  }
  break;
  case HTTPS_RESPONSE_CODE_INVALID:
  {
    showMessageWithLogo(WIFI_INTERNAL_ERROR);
  }
  break;
  case HTTPS_UNABLE_TO_CONNECT:
  {
    if (WiFi.RSSI() > WIFI_CONNECTION_RSSI)
    {
      showMessageWithLogo(API_ERROR);
    }
    else
    {
      showMessageWithLogo(WIFI_WEAK);
    }
  }
  break;
  case HTTPS_WRONG_IMAGE_FORMAT:
  {
    showMessageWithLogo(BMP_FORMAT_ERROR);
  }
  break;
  case HTTPS_WRONG_IMAGE_SIZE:
  {
    if (WiFi.RSSI() > WIFI_CONNECTION_RSSI)
    {
      showMessageWithLogo(API_SIZE_ERROR);
    }
    else
    {
      showMessageWithLogo(WIFI_WEAK);
    }
  }
  break;
  case HTTPS_CLIENT_FAILED:
  {
    showMessageWithLogo(WIFI_INTERNAL_ERROR);
  }
  break;
  case HTTPS_PLUGIN_NOT_ATTACHED:
  {
    if (preferences.getInt(PREFERENCES_SLEEP_TIME_KEY, 0) != SLEEP_TIME_WHILE_PLUGIN_NOT_ATTACHED)
    {
      Log.info("%s [%d]: write new refresh rate: %d\r\n", __FILE__, __LINE__, SLEEP_TIME_WHILE_PLUGIN_NOT_ATTACHED);
      size_t result = preferences.putUInt(PREFERENCES_SLEEP_TIME_KEY, SLEEP_TIME_WHILE_PLUGIN_NOT_ATTACHED);
      Log.info("%s [%d]: written new refresh rate: %d\r\n", __FILE__, __LINE__, SLEEP_TIME_WHILE_PLUGIN_NOT_ATTACHED);
    }
  }
  break;
  default:
    break;
  }

  if (request_result != HTTPS_NO_ERR && request_result != HTTPS_PLUGIN_NOT_ATTACHED)
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

      Log.info("%s [%d]: Added headers:\n\rID: %s\n\rSpecial function: %d\n\rAccess-Token: %s\n\rRefresh_Rate: %s\n\rBattery-Voltage: %s\n\rFW-Version: %s\r\nRSSI: %s\r\n", __FILE__, __LINE__, WiFi.macAddress().c_str(), special_function, api_key.c_str(), String(refresh_rate).c_str(), String(battery_voltage).c_str(), fw_version.c_str(), String(WiFi.RSSI()));

      // if (https.begin(*client, new_url))
      if (https.begin(*client, "https://usetrmnl.com/api/display"))
      { // HTTPS
        Log.info("%s [%d]: [HTTPS] GET...\r\n", __FILE__, __LINE__);
        // start connection and send HTTP header
        https.addHeader("ID", WiFi.macAddress());
        https.addHeader("Access-Token", api_key);
        https.addHeader("Refresh-Rate", String(refresh_rate));
        https.addHeader("Battery-Voltage", String(battery_voltage));
        https.addHeader("FW-Version", fw_version);
        https.addHeader("RSSI", String(WiFi.RSSI()));

        Log.info("%s [%d]: Special function - %d\r\n", __FILE__, __LINE__, special_function);
        if (special_function != SF_NONE)
        {
          Log.info("%s [%d]: Add special function - true\r\n", __FILE__, __LINE__);
          https.addHeader("special_function", "true");
        }

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
            Log.info("%s [%d]: Payload - %s\r\n", __FILE__, __LINE__, payload.c_str());
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, payload);
            if (error)
            {
              result = HTTPS_JSON_PARSING_ERR;
            }
            else if (!error && special_function == SF_NONE)
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
                bool sleep_5_seconds = false;
                String special_function_str = doc["special_function"];

                if (special_function_str.equals("identify"))
                {
                  Log.info("%s [%d]: New special function - identify\r\n", __FILE__, __LINE__);
                  special_function = SF_IDENTIFY;
                }
                else if (special_function_str.equals("sleep"))
                {
                  Log.info("%s [%d]: New special function - sleep\r\n", __FILE__, __LINE__);
                  special_function = SF_SLEEP;
                }
                else if (special_function_str.equals("add_wifi"))
                {
                  Log.info("%s [%d]: New special function - add_wifi\r\n", __FILE__, __LINE__);
                  special_function = SF_ADD_WIFI;
                }
                else if (special_function_str.equals("restart_playlist"))
                {
                  Log.info("%s [%d]: New special function - restart_playlist\r\n", __FILE__, __LINE__);
                  special_function = SF_RESTART_PLAYLIST;
                }
                else if (special_function_str.equals("rewind"))
                {
                  Log.info("%s [%d]: New special function - rewind\r\n", __FILE__, __LINE__);
                  special_function = SF_REWIND;
                }
                else if (special_function_str.equals("send_to_me"))
                {
                  Log.info("%s [%d]: New special function - send_to_me\r\n", __FILE__, __LINE__);
                  special_function = SF_SEND_TO_ME;
                }
                else
                {
                  Log.info("%s [%d]: No new special function\r\n", __FILE__, __LINE__);
                  special_function = SF_NONE;
                }
                writeSpecialFunction(special_function);
                special_function = SF_NONE;

                if (update_firmware)
                {
                  Log.info("%s [%d]: update firfware. Check URL\r\n", __FILE__, __LINE__);
                  if (firmware_url.length() == 0)
                  {
                    Log.error("%s [%d]: Empty URL\r\n", __FILE__, __LINE__);
                    update_firmware = false;
                  }
                }
                if (image_url.length() > 0)
                {
                  Log.info("%s [%d]: image_url: %s\r\n", __FILE__, __LINE__, image_url.c_str());
                  Log.info("%s [%d]: image url end with: %d\r\n", __FILE__, __LINE__, image_url.endsWith("/setup-logo.bmp"));

                  image_url.toCharArray(filename, image_url.length() + 1);
                  // check if plugin is applied
                  bool flag = preferences.getBool(PREFERENCES_DEVICE_REGISTRED_KEY, false);
                  Log.info("%s [%d]: flag: %d\r\n", __FILE__, __LINE__, flag);

                  if (image_url.endsWith("/setup-logo.bmp"))
                  {
                    Log.info("%s [%d]: End with logo.bmp\r\n", __FILE__, __LINE__);
                    if (!flag)
                    {
                      // draw received logo
                      status = true;
                      // set flag to true
                      if (preferences.getBool(PREFERENCES_DEVICE_REGISTRED_KEY, false) != true) // check the flag to avoid the re-writing
                      {
                        bool res = preferences.putBool(PREFERENCES_DEVICE_REGISTRED_KEY, true);
                        if (res)
                          Log.info("%s [%d]: Flag written true successfully\r\n", __FILE__, __LINE__);
                        else
                          Log.error("%s [%d]: FLag writing failed\r\n", __FILE__, __LINE__);
                      }
                    }
                    else
                    {
                      // don't draw received logo
                      status = false;
                    }
                    // sleep 5 seconds
                    sleep_5_seconds = true;
                  }
                  else
                  {
                    Log.info("%s [%d]: End with NO logo.bmp\r\n", __FILE__, __LINE__);
                    if (flag)
                    {
                      if (preferences.getBool(PREFERENCES_DEVICE_REGISTRED_KEY, false) != false) // check the flag to avoid the re-writing
                      {
                        bool res = preferences.putBool(PREFERENCES_DEVICE_REGISTRED_KEY, false);
                        if (res)
                          Log.info("%s [%d]: Flag written false successfully\r\n", __FILE__, __LINE__);
                        else
                          Log.error("%s [%d]: FLag writing failed\r\n", __FILE__, __LINE__);
                      }
                    }
                    // Find the start and end positions of the string to extract
                    int startPos = image_url.indexOf(".com/") + 5; // ".com/" is 5 characters long
                    int endPos = image_url.indexOf("?", startPos);

                    // Extract the string
                    String extractedString = image_url.substring(startPos, endPos);

                    // Print the extracted string
                    Log.info("%s [%d]: New filename - %s\r\n", __FILE__, __LINE__, extractedString.c_str());
                    if (!checkCureentFileName(extractedString) || wakeup_reason == ESP_SLEEP_WAKEUP_GPIO || wakeup_reason == ESP_SLEEP_WAKEUP_UNDEFINED)
                    {
                      Log.info("%s [%d]: New image. Show it.\r\n", __FILE__, __LINE__);
                      status = true;
                    }
                    else
                    {
                      Log.info("%s [%d]: Old image. No needed to show it.\r\n", __FILE__, __LINE__);
                      status = false;
                      result = HTTPS_SUCCES;
                    }
                  }
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

                if (update_firmware)
                  result = HTTPS_SUCCES;
                if (reset_firmware)
                  result = HTTPS_RESET;
                if (sleep_5_seconds)
                  result = HTTPS_PLUGIN_NOT_ATTACHED;
                Log.info("%s [%d]: result - %d\r\n", __FILE__, __LINE__, result);
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
            else if (!error && special_function != SF_NONE)
            {
              uint64_t request_status = doc["status"];
              Log.info("%s [%d]: status: %d\r\n", __FILE__, __LINE__, request_status);
              switch (request_status)
              {
              case 0:
              {
                switch (special_function)
                {
                case SF_IDENTIFY:
                {
                  String action = doc["action"];
                  if (action.equals("identify"))
                  {
                    Log.info("%s [%d]:Identify success\r\n", __FILE__, __LINE__);
                    String image_url = doc["image_url"];
                    if (image_url.length() > 0)
                    {
                      Log.info("%s [%d]: image_url: %s\r\n", __FILE__, __LINE__, image_url.c_str());
                      Log.info("%s [%d]: image url end with: %d\r\n", __FILE__, __LINE__, image_url.endsWith("/setup-logo.bmp"));

                      image_url.toCharArray(filename, image_url.length() + 1);
                      // check if plugin is applied
                      bool flag = preferences.getBool(PREFERENCES_DEVICE_REGISTRED_KEY, false);
                      Log.info("%s [%d]: flag: %d\r\n", __FILE__, __LINE__, flag);

                      if (image_url.endsWith("/setup-logo.bmp"))
                      {
                        Log.info("%s [%d]: End with logo.bmp\r\n", __FILE__, __LINE__);
                        if (!flag)
                        {
                          // draw received logo
                          status = true;
                          // set flag to true
                          if (preferences.getBool(PREFERENCES_DEVICE_REGISTRED_KEY, false) != true) // check the flag to avoid the re-writing
                          {
                            bool res = preferences.putBool(PREFERENCES_DEVICE_REGISTRED_KEY, true);
                            if (res)
                              Log.info("%s [%d]: Flag written true successfully\r\n", __FILE__, __LINE__);
                            else
                              Log.error("%s [%d]: FLag writing failed\r\n", __FILE__, __LINE__);
                          }
                        }
                        else
                        {
                          // don't draw received logo
                          status = false;
                        }
                      }
                      else
                      {
                        Log.info("%s [%d]: End with NO logo.bmp\r\n", __FILE__, __LINE__);
                        if (flag)
                        {
                          if (preferences.getBool(PREFERENCES_DEVICE_REGISTRED_KEY, false) != false) // check the flag to avoid the re-writing
                          {
                            bool res = preferences.putBool(PREFERENCES_DEVICE_REGISTRED_KEY, false);
                            if (res)
                              Log.info("%s [%d]: Flag written false successfully\r\n", __FILE__, __LINE__);
                            else
                              Log.error("%s [%d]: FLag writing failed\r\n", __FILE__, __LINE__);
                          }
                        }
                        status = true;
                      }
                    }
                  }
                  else
                  {
                    Log.error("%s [%d]: identify failed\r\n", __FILE__, __LINE__);
                  }
                }
                break;
                case SF_SLEEP:
                {
                  String action = doc["action"];
                  if (action.equals("sleep"))
                  {
                    uint64_t rate = doc["refresh_rate"];
                    Log.info("%s [%d]: refresh_rate: %d\r\n", __FILE__, __LINE__, rate);
                    if (rate != preferences.getUInt(PREFERENCES_SLEEP_TIME_KEY, SLEEP_TIME_TO_SLEEP))
                    {
                      Log.info("%s [%d]: write new refresh rate: %d\r\n", __FILE__, __LINE__, rate);
                      size_t result = preferences.putUInt(PREFERENCES_SLEEP_TIME_KEY, rate);
                      Log.info("%s [%d]: written new refresh rate: %d\r\n", __FILE__, __LINE__, result);
                    }
                    status = false;
                    result = HTTPS_SUCCES;
                    Log.info("%s [%d]: sleep success\r\n", __FILE__, __LINE__);
                  }
                  else
                  {
                    Log.error("%s [%d]: sleep failed\r\n", __FILE__, __LINE__);
                    // need to add error
                  }
                }
                break;
                case SF_ADD_WIFI:
                {
                  String action = doc["action"];
                  if (action.equals("add_wifi"))
                  {
                    status = false;
                    result = HTTPS_SUCCES;
                    Log.info("%s [%d]: Add wifi success\r\n", __FILE__, __LINE__);
                  }
                  else
                  {
                    Log.error("%s [%d]: Add wifi failed\r\n", __FILE__, __LINE__);
                  }
                }
                break;
                case SF_RESTART_PLAYLIST:
                {
                  String action = doc["action"];
                  if (action.equals("restart_playlist"))
                  {
                    Log.info("%s [%d]:Restart playlist success\r\n", __FILE__, __LINE__);
                    String image_url = doc["image_url"];
                    if (image_url.length() > 0)
                    {
                      Log.info("%s [%d]: image_url: %s\r\n", __FILE__, __LINE__, image_url.c_str());
                      Log.info("%s [%d]: image url end with: %d\r\n", __FILE__, __LINE__, image_url.endsWith("/setup-logo.bmp"));

                      image_url.toCharArray(filename, image_url.length() + 1);
                      // check if plugin is applied
                      bool flag = preferences.getBool(PREFERENCES_DEVICE_REGISTRED_KEY, false);
                      Log.info("%s [%d]: flag: %d\r\n", __FILE__, __LINE__, flag);

                      if (image_url.endsWith("/setup-logo.bmp"))
                      {
                        Log.info("%s [%d]: End with logo.bmp\r\n", __FILE__, __LINE__);
                        if (!flag)
                        {
                          // draw received logo
                          status = true;
                          // set flag to true
                          if (preferences.getBool(PREFERENCES_DEVICE_REGISTRED_KEY, false) != true) // check the flag to avoid the re-writing
                          {
                            bool res = preferences.putBool(PREFERENCES_DEVICE_REGISTRED_KEY, true);
                            if (res)
                              Log.info("%s [%d]: Flag written true successfully\r\n", __FILE__, __LINE__);
                            else
                              Log.error("%s [%d]: FLag writing failed\r\n", __FILE__, __LINE__);
                          }
                        }
                        else
                        {
                          // don't draw received logo
                          status = false;
                        }
                      }
                      else
                      {
                        Log.info("%s [%d]: End with NO logo.bmp\r\n", __FILE__, __LINE__);
                        if (flag)
                        {
                          if (preferences.getBool(PREFERENCES_DEVICE_REGISTRED_KEY, false) != false) // check the flag to avoid the re-writing
                          {
                            bool res = preferences.putBool(PREFERENCES_DEVICE_REGISTRED_KEY, false);
                            if (res)
                              Log.info("%s [%d]: Flag written false successfully\r\n", __FILE__, __LINE__);
                            else
                              Log.error("%s [%d]: FLag writing failed\r\n", __FILE__, __LINE__);
                          }
                        }
                        status = true;
                      }
                    }
                  }
                  else
                  {
                    Log.error("%s [%d]: identify failed\r\n", __FILE__, __LINE__);
                  }
                }
                break;
                case SF_REWIND:
                {
                  String action = doc["action"];
                  if (action.equals("rewind"))
                  {
                    status = false;
                    result = HTTPS_SUCCES;
                    Log.info("%s [%d]: rewind success\r\n", __FILE__, __LINE__);
                    // showMessageWithLogo(BMP_FORMAT_ERROR);

                    bool result = filesystem_read_from_file("/last.bmp", buffer, sizeof(buffer));
                    if (result)
                    {
                      bool image_reverse = false;
                      bmp_err_e res = parseBMPHeader(buffer, image_reverse);
                      String error = "";
                      switch (res)
                      {
                      case BMP_NO_ERR:
                      {
                        // show the image
                        display_show_image(buffer, image_reverse);
                      }
                      break;
                      default:
                      {
                      }
                      break;
                      }
                    }
                    else
                    {
                      showMessageWithLogo(BMP_FORMAT_ERROR);
                    }
                  }
                  else
                  {
                    Log.error("%s [%d]: rewind failed\r\n", __FILE__, __LINE__);
                  }
                }
                break;
                case SF_SEND_TO_ME:
                {
                  String action = doc["action"];
                  if (action.equals("send_to_me"))
                  {
                    status = false;
                    result = HTTPS_SUCCES;
                    Log.info("%s [%d]: send_to_me success\r\n", __FILE__, __LINE__);
                    bool result = filesystem_read_from_file("/current.bmp", buffer, sizeof(buffer));
                    if (result)
                    {
                      bool image_reverse = false;
                      bmp_err_e res = parseBMPHeader(buffer, image_reverse);
                      String error = "";
                      switch (res)
                      {
                      case BMP_NO_ERR:
                      {
                        // show the image
                        display_show_image(buffer, image_reverse);
                      }
                      break;
                      default:
                      {
                      }
                      break;
                      }
                    }
                    else
                    {
                      showMessageWithLogo(BMP_FORMAT_ERROR);
                    }
                  }
                  else
                  {
                    Log.error("%s [%d]: send_to_me failed\r\n", __FILE__, __LINE__);
                  }
                }
                break;
                default:
                  break;
                }
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
            sprintf(log_array, "%d [%d]: returned code is not OK - %d", getTime(), __LINE__, httpCode);
            log_POST(log_array, strlen(log_array));
          }
        }
        else
        {
          Log.error("%s [%d]: [HTTPS] GET... failed, error: %s\r\n", __FILE__, __LINE__, https.errorToString(httpCode).c_str());
          result = HTTPS_RESPONSE_CODE_INVALID;

          memset(log_array, 0, sizeof(log_array));
          sprintf(log_array, "%d [%d]: HTTPS returned code is less then 0. Code - %d", getTime(), __LINE__, httpCode);
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
                  Serial.println();
                  String error = "";
                  switch (res)
                  {
                  case BMP_NO_ERR:
                  {

                    if (filesystem_file_exists("/last.bmp") && filesystem_file_exists("/current.bmp"))
                    {
                      Log.info("%s [%d]: Last and current exist!\r\n", __FILE__, __LINE__);
                      if (filesystem_file_delete("/last.bmp"))
                      {
                        if (filesystem_file_rename("/current.bmp", "/last.bmp"))
                        {
                          Log.info("%s [%d]: Current renamed to last!\r\n", __FILE__, __LINE__);
                          delay(10);
                          writeImageToFile("/current.bmp", buffer, sizeof(buffer));
                        }
                      }
                    }
                    else
                    {
                      Log.info("%s [%d]: Last and current don't exist!\r\n", __FILE__, __LINE__);
                      writeImageToFile("/current.bmp", buffer, sizeof(buffer));
                      writeImageToFile("/last.bmp", buffer, sizeof(buffer));
                    }
                    // show the image
                    display_show_image(buffer, image_reverse);

                    String image_url = String(filename);

                    // Find the start and end positions of the string to extract
                    int startPos = image_url.indexOf(".com/") + 5; // ".com/" is 5 characters long
                    int endPos = image_url.indexOf("?", startPos);

                    // Extract the string
                    String extractedString = image_url.substring(startPos, endPos);

                    // Print the extracted string
                    Log.info("%s [%d]: New filename - %s\r\n", __FILE__, __LINE__, extractedString.c_str());

                    bool res = saveCurrentFileName(extractedString);
                    if (res)
                      Log.info("%s [%d]: New filename saved\r\n", __FILE__, __LINE__);
                    else
                      Log.error("%s [%d]: New image name saving error!", __FILE__, __LINE__);

                    if (result != HTTPS_PLUGIN_NOT_ATTACHED)
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
              sprintf(log_array, "%d [%d]: HTTPS returned code is not OK. Code - %d", getTime(), __LINE__, httpCode);
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

            if (WiFi.RSSI() > WIFI_CONNECTION_RSSI)
            {
              showMessageWithLogo(API_ERROR);
            }
            else
            {
              showMessageWithLogo(WIFI_WEAK);
            }
            memset(log_array, 0, sizeof(log_array));
            sprintf(log_array, "%d [%d]: returned code is not OK. Code - %d", getTime(), __LINE__, httpCode);
            log_POST(log_array, strlen(log_array));
          }
        }
        else
        {
          Log.error("%s [%d]: [HTTPS] GET... failed, error: %s\r\n", __FILE__, __LINE__, https.errorToString(httpCode).c_str());
          if (WiFi.RSSI() > WIFI_CONNECTION_RSSI)
          {
            showMessageWithLogo(API_ERROR);
          }
          else
          {
            showMessageWithLogo(WIFI_WEAK);
          }
          memset(log_array, 0, sizeof(log_array));
          sprintf(log_array, "%d [%d]: HTTPS returned code is less then 0. Code - %d", getTime(), __LINE__, httpCode);
          log_POST(log_array, strlen(log_array));
        }

        https.end();
      }
      else
      {
        Log.error("%s [%d]: [HTTPS] Unable to connect\r\n", __FILE__, __LINE__);
        showMessageWithLogo(WIFI_INTERNAL_ERROR);

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

                writeImageToFile("/logo.bmp", buffer, sizeof(buffer));

                // show the image
                String friendly_id = preferences.getString(PREFERENCES_FRIENDLY_ID, PREFERENCES_FRIENDLY_ID_DEFAULT);
                display_show_msg(buffer, FRIENDLY_ID, friendly_id, true, "", String(message_buffer));
                need_to_refresh_display = 0;
              }
              else
              {
                Log.error("%s [%d]: Receiving failed. Readed: %d\r\n", __FILE__, __LINE__, counter);
                if (WiFi.RSSI() > WIFI_CONNECTION_RSSI)
                {
                  showMessageWithLogo(API_SIZE_ERROR);
                }
                else
                {
                  showMessageWithLogo(WIFI_WEAK);
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
              if (WiFi.RSSI() > WIFI_CONNECTION_RSSI)
              {
                showMessageWithLogo(API_ERROR);
              }
              else
              {
                showMessageWithLogo(WIFI_WEAK);
              }

              memset(log_array, 0, sizeof(log_array));
              sprintf(log_array, "%d [%d]: HTTPS received code is not OK. Code - %d", getTime(), __LINE__, httpCode);
              log_POST(log_array, strlen(log_array));
            }
          }
          else
          {
            Log.error("%s [%d]: [HTTPS] GET... failed, error: %s\r\n", __FILE__, __LINE__, https.errorToString(httpCode).c_str());
            if (WiFi.RSSI() > WIFI_CONNECTION_RSSI)
            {
              showMessageWithLogo(API_ERROR);
            }
            else
            {
              showMessageWithLogo(WIFI_WEAK);
            }

            memset(log_array, 0, sizeof(log_array));
            sprintf(log_array, "%d [%d]: HTTPS returned code is less then 0. Code - %d", getTime(), __LINE__, httpCode);
            log_POST(log_array, strlen(log_array));
          }
        }
        else
        {
          Log.error("%s [%d]: unable to connect\r\n", __FILE__, __LINE__);
          if (WiFi.RSSI() > WIFI_CONNECTION_RSSI)
          {
            showMessageWithLogo(API_ERROR);
          }
          else
          {
            showMessageWithLogo(WIFI_WEAK);
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
    showMessageWithLogo(WIFI_INTERNAL_ERROR);

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
  WifiCaptivePortal.resetSettings();
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
          showMessageWithLogo(FW_UPDATE);

          if (Update.writeStream(https.getStream()))
          {
            if (Update.end(true))
            {
              Log.info("%s [%d]: Firmware update successful. Rebooting...\r\n", __FILE__, __LINE__);
              showMessageWithLogo(FW_UPDATE_SUCCESS);
            }
            else
            {
              Log.fatal("%s [%d]: Firmware update failed!\r\n", __FILE__, __LINE__);
              showMessageWithLogo(FW_UPDATE_FAILED);
            }
          }
          else
          {
            Log.fatal("%s [%d]: Write to firmware update stream failed!\r\n", __FILE__, __LINE__);
            showMessageWithLogo(FW_UPDATE_FAILED);
          }
        }
        else
        {
          Log.fatal("%s [%d]: Begin firmware update failed!\r\n", __FILE__, __LINE__);
          showMessageWithLogo(FW_UPDATE_FAILED);
        }
      }
      else
      {
        Log.fatal("%s [%d]: HTTP GET failed!\r\n", __FILE__, __LINE__);
        if (WiFi.RSSI() > WIFI_CONNECTION_RSSI)
        {
          showMessageWithLogo(API_ERROR);
        }
        else
        {
          showMessageWithLogo(WIFI_WEAK);
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
  filesystem_deinit();
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
  configTime(0, 0, "pool.ntp.org", "time.google.com", "time.windows.com");
  delay(500);

  // Wait for time to be set
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Log.info("%s [%d]: Time synchronization failed... Attempt 2...\r\n", __FILE__, __LINE__);
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
      https.setTimeout(5);
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
            Log.info("%s [%d]: RESULT TRUE\r\n", __FILE__, __LINE__);
            result = true;
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
    store_log(log_buffer, size, preferences);
  }
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
  gather_stored_logs(log, preferences);

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

          String buffer = "{\"log\":{\"dump\":{\"error\":\"" + log;
          buffer = buffer + "\"}}}";

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
    clear_stored_logs(preferences);
  }
}

static void writeImageToFile(const char *name, uint8_t *in_buffer, size_t size)
{
  size_t res = filesystem_write_to_file(name, in_buffer, size);
  if (res != size)
  {
    Log.error("%s [%d]: File writing ERROR. Result - %d\r\n", __FILE__, __LINE__, res);
    sprintf(log_array, "%d [%d]: error writing file - %s. Written - %d bytes", getTime(), __LINE__, name, res);
    log_POST(log_array, strlen(log_array));
  }
  else
  {
    Log.info("%s [%d]: file %s writing success - %d bytes\r\n", __FILE__, __LINE__, name, res);
  }
}

static void writeSpecialFunction(SPECIAL_FUNCTION function)
{
  if (preferences.isKey(PREFERENCES_SF_KEY))
  {
    Log.info("%s [%d]: SF saved. Reading...\r\n", __FILE__, __LINE__);
    if ((SPECIAL_FUNCTION)preferences.getUInt(PREFERENCES_SF_KEY, 0) == function)
    {
      Log.info("%s [%d]: No needed to re-write\r\n", __FILE__, __LINE__);
    }
    else
    {
      Log.info("%s [%d]: Writing new special function\r\n", __FILE__, __LINE__);
      bool res = preferences.putUInt(PREFERENCES_SF_KEY, function);
      if (res)
        Log.info("%s [%d]: Written new special function successfully\r\n", __FILE__, __LINE__);
      else
        Log.error("%s [%d]: Writing new special function failed\r\n", __FILE__, __LINE__);
    }
  }
  else
  {
    Log.error("%s [%d]: SF not saved\r\n", __FILE__, __LINE__);
    bool res = preferences.putUInt(PREFERENCES_SF_KEY, function);
    if (res)
      Log.info("%s [%d]: Written new special function successfully\r\n", __FILE__, __LINE__);
    else
      Log.error("%s [%d]: Writing new special function failed\r\n", __FILE__, __LINE__);
  }
}

static void showMessageWithLogo(MSG message_type)
{
  display_show_msg(storedLogoOrDefault(), message_type);
}

static void showMessageWithLogo(MSG message_type, String friendly_id, bool id, const char *fw_version, String message)
{
  display_show_msg(storedLogoOrDefault(), message_type, friendly_id, id, fw_version, message);
}

static uint8_t *storedLogoOrDefault(void)
{
  if (filesystem_read_from_file("/logo.bmp", buffer, sizeof(buffer)))
  {
    return buffer;
  }
  return const_cast<uint8_t *>(default_icon);
}

static bool saveCurrentFileName(String &name)
{
  if (!preferences.getString(PREFERENCES_FILENAME_KEY, "").equals(name))
  {
    size_t res = preferences.putString(PREFERENCES_FILENAME_KEY, name);
    if (res > 0)
    {
      Log.info("%s [%d]: New filename saved in the preferences - %d\r\n", __FILE__, __LINE__, res);
      return true;
    }
    else
    {
      Log.error("%s [%d]: New filename saving error!\r\n", __FILE__, __LINE__);
      return false;
    }
  }
  else
  {
    Log.info("%s [%d]: No needed to re-write\r\n", __FILE__, __LINE__);
    return true;
  }
}

static bool checkCureentFileName(String &newName)
{
  String currentFilename = preferences.getString(PREFERENCES_FILENAME_KEY, "");
  if (currentFilename.equals(newName))
  {
    Log.info("%s [%d]: Currrent filename equals to the new filename\r\n", __FILE__, __LINE__);
    return true;
  }
  else
  {
    Log.error("%s [%d]: Currrent filename doesn't equal to the new filename\r\n", __FILE__, __LINE__);
    return false;
  }
}