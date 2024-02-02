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

static const char *TAG = "bl.cpp";

bool pref_clear = false;
WiFiManager wm;

// timers
uint8_t buffer[48130];
char filename[100];

bool status = false;
bool keys_stored = false;

// timers
uint32_t timer = 0;
uint32_t button_timer = 0;

Preferences preferences;

static void downloadAndSaveToFile(const char *url);
static void getDeviceCredentials(const char *url);
static bool readBufferFromFile(uint8_t *out_buffer);
static bool writeBufferToFile(const char *name, uint8_t *in_buffer, uint16_t size);
static void goToSleep(void);
static void setClock(void);
static float readBatteryVoltage(void);

static float readBatteryVoltage(void)
{
  uint32_t adc = 0;
  for (uint8_t i = 0; i < 255; i++)
  {
    adc += analogRead(PIN_BATTERY);
  }
  adc = adc / 255;

  float voltage = adc * 3.3 / 4096 * 2;
  return voltage;
}

// Not sure if WiFiClientSecure checks the validity date of the certificate.
// Setting clock just to be sure...
static void setClock()
{
  configTime(0, 0, "pool.ntp.org");

  Serial.print(F("Waiting for NTP time sync: "));
  time_t nowSecs = time(nullptr);
  while (nowSecs < 8 * 3600 * 2)
  {
    delay(500);
    Serial.print(F("."));
    yield();
    nowSecs = time(nullptr);
  }

  Serial.println();
  struct tm timeinfo;
  gmtime_r(&nowSecs, &timeinfo);
  Serial.print(F("Current time: "));
  Serial.print(asctime(&timeinfo));
}

/**
 * @brief Function to init business logic module
 * @param none
 * @return none
 */
void bl_init(void)
{
  Serial.begin(115200);
  Log.begin(LOG_LEVEL_VERBOSE, &Serial);
  Log.info("%s [%d]: BL init success\r\n", TAG, __LINE__);
  Log.info("%s [%d]: Firware version %d.%d.%d\r\n", TAG, __LINE__, FW_MAJOR_VERSION, FW_MINOR_VERSION, FW_PATCH_VERSION);
  pins_init();
  button_timer = millis();

  bool res = preferences.begin("data", false);
  if (res)
  {
    Log.info("%s [%d]: preferences init success\r\n", TAG, __LINE__);
    // preferences.clear();
  }
  else
  {
    Log.fatal("%s [%d]: preferences init failed\r\n", TAG, __LINE__);
    while (1)
      ;
  }

  // handling reset
  while (1)
  {
    if (digitalRead(PIN_INTERRUPT) == LOW && millis() - button_timer > BUTTON_HOLD_TIME)
    {
      Log.info("%s [%d]: WiFi reset\r\n", TAG, __LINE__);
      wm.resetSettings();
      break;
    }
    else if (digitalRead(PIN_INTERRUPT) == HIGH)
    {
      Log.info("%s [%d]: WiFi NOT reset\r\n", TAG, __LINE__);
      break;
    }
  }

  // Mount SPIFFS
  if (!SPIFFS.begin(true))
  {
    Log.fatal("%s [%d]: Failed to mount SPIFFS\r\n", TAG, __LINE__);
    while (1)
      ;
  }
  else
  {
    Log.info("%s [%d]: SPIFFS mounted\r\n", TAG, __LINE__);
  }

  // EPD init
  // EPD clear
  Log.info("%s [%d]: Display init\r\n", TAG, __LINE__);
  display_init();

  Log.info("%s [%d]: Display clear\r\n", TAG, __LINE__);
  display_reset();

  // Download file and save to SPIFFS
  // downloadAndSaveToFile("https://usetrmnl.com");
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  // DEV_Delay_ms(500);
  if (wm.getWiFiIsSaved())
  {
    Log.info("%s [%d]: WiFi saved\r\n", TAG, __LINE__);
  }
  else
  {
    Log.info("%s [%d]: WiFi NOT saved\r\n", TAG, __LINE__);
    bool res = readBufferFromFile(buffer);
    if (res)
    {
      Log.info("%s [%d]: logo not exists. Use default\r\n", TAG, __LINE__);
      if (preferences.isKey(PREFERENCES_FRIENDLY_ID))
      {
        Log.info("%s [%d]: friendly ID exists\r\n", TAG, __LINE__);
        String friendly_id = preferences.getString(PREFERENCES_FRIENDLY_ID, PREFERENCES_FRIENDLY_ID_DEFAULT);
        display_show_msg(buffer, WIFI_CONNECT, friendly_id);
      }
      else
      {
        display_show_msg(buffer, WIFI_CONNECT, "NOT SAVED");
      }
    }
    else
    {
      Log.info("%s [%d]: logo not exists. Use default\r\n", TAG, __LINE__);
      if (preferences.isKey(PREFERENCES_FRIENDLY_ID))
      {
        Log.info("%s [%d]: friendly ID exists\r\n", TAG, __LINE__);
        String friendly_id = preferences.getString(PREFERENCES_FRIENDLY_ID, PREFERENCES_FRIENDLY_ID_DEFAULT);
        display_show_msg(const_cast<uint8_t *>(default_icon), WIFI_CONNECT, friendly_id);
      }
      else
      {
        Log.error("%s [%d]: friendly ID exists\r\n", TAG, __LINE__);
        display_show_msg(const_cast<uint8_t *>(default_icon), WIFI_CONNECT, "NOT SAVED");
      }
    }
  }

  wm.setClass("invert");
  wm.setConnectRetries(1);
  wm.setConnectTimeout(10);
  wm.setBreakAfterConfig(true);  // always exit configportal even if wifi save fails
  res = wm.autoConnect("trmnl"); // password protected ap
  if (!res)
  {
    Log.error("%s [%d]: Failed to connect or hit timeout\r\n", TAG, __LINE__);
    wm.disconnect();
    wm.stopWebPortal();
    // wm.resetSettings();
    //  show logo with string
    res = readBufferFromFile(buffer);
    if (res && keys_stored)
      display_show_msg(buffer, WIFI_FAILED);
    else
      display_show_msg(const_cast<uint8_t *>(default_icon), WIFI_FAILED);
    // Go to deep sleep
    display_sleep();
    goToSleep();
  }
  else
  {
    // if you get here you have connected to the WiFi
    Log.info("%s [%d]: connected...\r\n)", TAG, __LINE__);
  }

  // timer = millis();
  Log.info("%s [%d]: WiFi connected\r\n", TAG, __LINE__);
  setClock();

  if (!preferences.isKey(PREFERENCES_API_KEY) || !preferences.isKey(PREFERENCES_FRIENDLY_ID))
  {
    Log.info("%s [%d]: API key or friendly ID not saved\r\n", TAG, __LINE__);
    // lets get the api key and friendly ID
    getDeviceCredentials("https://usetrmnl.com");
  }
  else
  {
    Log.info("%s [%d]: API key and friendly ID saved\r\n", TAG, __LINE__);
  }

  downloadAndSaveToFile("https://usetrmnl.com");
  display_sleep();
  goToSleep();
}

/**
 * @brief Function to process business logic module
 * @param none
 * @return none
 */
void bl_process(void)
{
  /*
  if (wm_nonblocking)
    wifi_manager.process();
*/
}

static bool writeBufferToFile(const char *name, uint8_t *in_buffer, uint16_t size)
{
  if (SPIFFS.exists(name))
  {
    Log.info("%s [%d]: file %s exists. Deleting...\r\n", TAG, __LINE__, name);
    if (SPIFFS.remove(name))
      Log.info("%s [%d]: file %s deleted\r\n", TAG, __LINE__, name);
    else
      Log.info("%s [%d]: file %s deleting failed\r\n", TAG, __LINE__, name);
  }
  File file = SPIFFS.open(name, FILE_WRITE);
  if (file)
  {
    size_t res = file.write(in_buffer, size);
    file.close();
    if (res)
    {
      Log.info("%s [%d]: file %s writing success\r\n", TAG, __LINE__, name);
      return true;
    }
    else
    {
      Log.error("%s [%d]: file %s writing error\r\n", TAG, __LINE__, name);
      return false;
    }
  }
  else
  {
    Log.error("%s [%d]: File open ERROR\r\n", TAG, __LINE__);
    return false;
  }
}

static bool readBufferFromFile(uint8_t *out_buffer)
{
  if (SPIFFS.exists("logo_white.bmp"))
  {
    Log.info("%s [%d]: icon exists\r\n", TAG, __LINE__);
    File file = SPIFFS.open("/logo_white.bmp", FILE_READ);
    if (file)
    {
      if (file.size() == DISPLAY_BMP_IMAGE_SIZE)
      {
        Log.info("%s [%d]: the size is the same\r\n", TAG, __LINE__);
        file.readBytes((char *)out_buffer, DISPLAY_BMP_IMAGE_SIZE);
      }
      else
      {
        Log.error("%s [%d]: the size is NOT the same %d\r\n", TAG, __LINE__, file.size());
        return false;
      }
      file.close();
      return true;
    }
    else
    {
      Log.error("%s [%d]: File open ERROR\r\n", TAG, __LINE__);
      return false;
    }
  }
  else
  {
    Log.error("%s [%d]: icon DOESN\'T exists\r\n", TAG, __LINE__);
    return false;
  }
}

static void downloadAndSaveToFile(const char *url)
{
  WiFiClientSecure *client = new WiFiClientSecure;
  if (client)
  {
    // client->setCACert(rootCACertificate);
    client->setInsecure();
    {
      // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is
      HTTPClient https;

      Log.info("%s [%d]: [HTTPS] begin...\r\n", TAG, __LINE__);
      char new_url[200];
      strcpy(new_url, url);
      strcat(new_url, "/api/display");

      String api_key = "";
      if (preferences.isKey(PREFERENCES_API_KEY))
      {
        api_key = preferences.getString(PREFERENCES_API_KEY, PREFERENCES_API_KEY_DEFAULT);
        Log.info("%s [%d]: %s key exists. Value - %s\r\n", TAG, __LINE__, PREFERENCES_API_KEY, api_key.c_str());
      }
      else
      {
        Log.error("%s [%d]: %s key not exists.\r\n", TAG, __LINE__, PREFERENCES_API_KEY);
      }

      String friendly_id = "";
      if (preferences.isKey(PREFERENCES_FRIENDLY_ID))
      {
        friendly_id = preferences.getString(PREFERENCES_FRIENDLY_ID, PREFERENCES_FRIENDLY_ID_DEFAULT);
        Log.info("%s [%d]: %s key exists. Value - %s\r\n", TAG, __LINE__, PREFERENCES_FRIENDLY_ID, friendly_id);
      }
      else
      {
        Log.error("%s [%d]: %s key not exists.\r\n", TAG, __LINE__, PREFERENCES_FRIENDLY_ID);
      }

      uint64_t refresh_rate = SLEEP_TIME_TO_SLEEP;
      if (preferences.isKey(PREFERENCES_SLEEP_TIME_KEY))
      {
        refresh_rate = preferences.getLong64(PREFERENCES_SLEEP_TIME_KEY, SLEEP_TIME_TO_SLEEP);
        Log.info("%s [%d]: %s key exists. Value - %s\r\n", TAG, __LINE__, PREFERENCES_SLEEP_TIME_KEY, api_key);
      }
      else
      {
        Log.error("%s [%d]: %s key not exists.\r\n", TAG, __LINE__, PREFERENCES_SLEEP_TIME_KEY);
      }

      String fw_version = String(FW_MAJOR_VERSION) + "." + String(FW_MINOR_VERSION) + "." + String(FW_PATCH_VERSION) + ".";

      float battery_voltage = readBatteryVoltage();
      Log.info("%s [%d]: %s battery voltage - %f\r\n", TAG, __LINE__, PREFERENCES_SLEEP_TIME_KEY, battery_voltage);

      Log.info("%s [%d]: Added headers:\n\rID: %s\n\rAccess-Token: %s\n\rRefresh_Rate: %s\n\rBattery-Voltage: %s\n\rFW-Version: %s", TAG, __LINE__, WiFi.macAddress().c_str(), api_key.c_str(), String(refresh_rate).c_str(), String(battery_voltage).c_str(), fw_version.c_str());

      // if (https.begin(*client, new_url))
      if (https.begin(*client, "https://usetrmnl.com/api/display"))
      { // HTTPS
        Log.info("%s [%d]: [HTTPS] GET...\r\n", TAG, __LINE__);
        // start connection and send HTTP header
        https.addHeader("ID", WiFi.macAddress());
        https.addHeader("Access-Token", api_key);
        https.addHeader("Refresh-Rate", String(refresh_rate));
        https.addHeader("Battery-Voltage", String(battery_voltage));
        https.addHeader("FW-Version", fw_version);

        int httpCode = https.GET();

        // httpCode will be negative on error
        if (httpCode > 0)
        {
          // HTTP header has been send and Server response header has been handled
          Log.info("%s [%d]: GET... code: %d\r\n", TAG, __LINE__, httpCode);
          // file found at server
          if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
          {
            String payload = https.getString();
            Serial.print("Payload: ");
            Serial.println(payload);
            size_t size = https.getSize();
            Log.info("%s [%d]: Content size: %d\r\n", TAG, __LINE__, size);
            Log.info("%s [%d]: Free heap size: %d\r\n", TAG, __LINE__, ESP.getFreeHeap());

            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, payload);
            if (error)
              return;
            String image_url = doc["image_url"];
            String firmware_url = doc["firmware_url"];
            uint64_t rate = doc["refresh_rate"];

            if (image_url.length() > 0)
            {
              Log.info("%s [%d]: image_url: %s\r\n", TAG, __LINE__, image_url.c_str());
              image_url.toCharArray(filename, image_url.length() + 1);
            }
            if (firmware_url.length() > 0)
            {
              Log.info("%s [%d]: firmware_url: %s\r\n", TAG, __LINE__, firmware_url.c_str());
            }
            Log.info("%s [%d]: refresh_rate: %d\r\n", TAG, __LINE__, rate);
            if (rate != preferences.getLong64(PREFERENCES_SLEEP_TIME_KEY, SLEEP_TIME_TO_SLEEP))
            {
              Log.info("%s [%d]: write new refresh rate: %d\r\n", TAG, __LINE__, rate);
              preferences.putULong64(PREFERENCES_SLEEP_TIME_KEY, rate);
            }
            status = true;
          }
          else
          {
            Log.info("%s [%d]: [HTTPS] Unable to connect\r\n", TAG, __LINE__);
            bool res = readBufferFromFile(buffer);
            if (res)
              display_show_msg(buffer, API_ERROR);
            else
              display_show_msg(const_cast<uint8_t *>(default_icon), API_ERROR);
            // display_sleep();
          }
        }
        else
        {
          Log.error("%s [%d]: [HTTPS] GET... failed, error: %s\r\n", TAG, __LINE__, https.errorToString(httpCode).c_str());
          bool res = readBufferFromFile(buffer);
          if (res)
            display_show_msg(buffer, WIFI_INTERNAL_ERROR);
          else
            display_show_msg(const_cast<uint8_t *>(default_icon), WIFI_INTERNAL_ERROR);
        }

        https.end();
      }
      else
      {
        Log.error("%s [%d]: [HTTPS] Unable to connect\r\n", TAG, __LINE__);
        bool res = readBufferFromFile(buffer);
        if (res)
          display_show_msg(buffer, API_ERROR);
        else
          display_show_msg(const_cast<uint8_t *>(default_icon), API_ERROR);
        // display_sleep();
      }

      if (status)
      {
        status = false;
        memset(new_url, 0, sizeof(new_url));
        strcpy(new_url, url);
        strcat(new_url, filename);

        Log.info("%s [%d]: [HTTPS] Request to %s\r\n", TAG, __LINE__, new_url);
        if (https.begin(*client, new_url))
        { // HTTPS
          Log.info("%s [%d]: [HTTPS] GET..\r\n", TAG, __LINE__);
          // start connection and send HTTP header
          int httpCode = https.GET();

          // httpCode will be negative on error
          if (httpCode > 0)
          {
            // HTTP header has been send and Server response header has been handled
            Log.error("%s [%d]: [HTTPS] GET... code: %d\r\n", TAG, __LINE__, httpCode);
            // file found at server
            if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
            {
              Log.info("%s [%d]: Content size: %d\r\n", TAG, __LINE__, https.getSize());

              WiFiClient *stream = https.getStreamPtr();

              uint32_t counter = 0;
              // Read and save BMP data to buffer
              if (stream->available() && https.getSize() == sizeof(buffer))
              {
                counter = stream->readBytes(buffer, sizeof(buffer));
              }

              if (counter == sizeof(buffer))
              {
                Log.info("%s [%d]: Received successfully\r\n", TAG, __LINE__);
                // EPD_7IN5_V2_Clear();
                // DEV_Delay_ms(500);

                // show the image
                display_show_image(buffer);
              }
              else
              {
                Log.error("%s [%d]: Receiving failed. Readed: %d\r\n", TAG, __LINE__, counter);
                bool res = readBufferFromFile(buffer);
                if (res)
                  display_show_msg(buffer, API_SIZE_ERROR);
                else
                  display_show_msg(const_cast<uint8_t *>(default_icon), API_SIZE_ERROR);
              }
            }
            else
            {
              Log.error("%s [%d]: [HTTPS] GET... failed, error: %s\r\n", TAG, __LINE__, https.errorToString(httpCode).c_str());
              bool res = readBufferFromFile(buffer);
              if (res)
                display_show_msg(buffer, API_ERROR);
              else
                display_show_msg(const_cast<uint8_t *>(default_icon), API_ERROR);
            }
          }
          else
          {
            Log.error("%s [%d]: [HTTPS] GET... failed, error: %s\r\n", TAG, __LINE__, https.errorToString(httpCode).c_str());
            bool res = readBufferFromFile(buffer);
            if (res)
              display_show_msg(buffer, API_ERROR);
            else
              display_show_msg(const_cast<uint8_t *>(default_icon), API_ERROR);
          }

          https.end();
        }
        else
        {
          Log.error("%s [%d]: unable to connect\r\n", TAG, __LINE__);
          bool res = readBufferFromFile(buffer);
          if (res)
            display_show_msg(buffer, API_ERROR);
          else
            display_show_msg(const_cast<uint8_t *>(default_icon), API_ERROR);
        }
      }
      // End extra scoping block
    }

    delete client;
  }
  else
  {
    Log.error("%s [%d]: Unable to create client\r\n", TAG, __LINE__);
    bool res = readBufferFromFile(buffer);
    if (res)
      display_show_msg(buffer, WIFI_INTERNAL_ERROR);
    else
      display_show_msg(const_cast<uint8_t *>(default_icon), WIFI_INTERNAL_ERROR);
  }
  // display_sleep();
}

static void getDeviceCredentials(const char *url)
{
  WiFiClientSecure *client = new WiFiClientSecure;
  if (client)
  {
    // client->setCACert(rootCACertificate);
    client->setInsecure();
    {
      // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is
      HTTPClient https;

      Log.info("%s [%d]: [HTTPS] begin...\r\n", TAG, __LINE__);
      char new_url[200];
      strcpy(new_url, url);
      strcat(new_url, "/api/setup");
      https.addHeader("ID", WiFi.macAddress());
      if (https.begin(*client, new_url))
      { // HTTPS
        Log.info("%s [%d]: [HTTPS] GET...\r\n", TAG, __LINE__);
        // start connection and send HTTP header

        Log.info("%s [%d]: Device MAC address: %s\r\n", TAG, __LINE__, WiFi.macAddress().c_str());
        int httpCode = https.GET();

        // httpCode will be negative on error
        if (httpCode > 0)
        {
          // HTTP header has been send and Server response header has been handled
          Log.info("%s [%d]: GET... code: %d\r\n", TAG, __LINE__, httpCode);
          // file found at server
          if (httpCode == HTTP_CODE_OK)
          {
            Log.info("%s [%d]: Content size: %d\r\n", TAG, __LINE__, https.getSize());
            String payload = https.getString();
            Log.info("%s [%d]: Payload: %s\r\n", TAG, __LINE__, payload.c_str());
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, payload);
            if (error)
            {
              Log.error("%s [%d]: JSON deserialization error.\r\n", TAG, __LINE__);
              https.end();
              client->stop();
              return;
            }
            uint16_t url_status = doc["status"];
            if (url_status == 200)
            {
              status = true;
              Log.info("%s [%d]: status OK.\r\n", TAG, __LINE__);

              String api_key = doc["api_key"];
              Log.info("%s [%d]: API key - %s\r\n", TAG, __LINE__, api_key.c_str());
              size_t res = preferences.putString(PREFERENCES_API_KEY, api_key);
              Log.info("%s [%d]: api key saved in the preferences - %d\r\n", TAG, __LINE__, res);

              String friendly_id = doc["friendly_id"];
              Log.info("%s [%d]: friendly ID - %s\r\n", TAG, __LINE__, friendly_id.c_str());
              res = preferences.putString(PREFERENCES_FRIENDLY_ID, friendly_id);
              Log.info("%s [%d]: friendly ID saved in the preferences - %d\r\n", TAG, __LINE__, res);

              String image_url = doc["image_url"];
              Log.info("%s [%d]: image_url - %s\r\n", TAG, __LINE__, image_url.c_str());
              image_url.toCharArray(filename, image_url.length() + 1);
              Log.info("%s [%d]: status - %d\r\n", TAG, __LINE__, status);
            }
            else
            {
              Log.info("%s [%d]: status FAIL.\r\n", TAG, __LINE__);
              status = false;
            }
          }
          else
          {
            Log.info("%s [%d]: [HTTPS] Unable to connect\r\n", TAG, __LINE__);
            bool res = readBufferFromFile(buffer);
            if (res)
              display_show_msg(buffer, API_ERROR);
            else
              display_show_msg(const_cast<uint8_t *>(default_icon), API_ERROR);
            // display_sleep();
          }
        }
        else
        {
          Log.error("%s [%d]: [HTTPS] GET... failed, error: %s\r\n", TAG, __LINE__, https.errorToString(httpCode).c_str());
          bool res = readBufferFromFile(buffer);
          if (res)
            display_show_msg(buffer, API_ERROR);
          else
            display_show_msg(const_cast<uint8_t *>(default_icon), API_ERROR);
          // display_sleep();
        }

        https.end();
      }
      else
      {
        Log.error("%s [%d]: [HTTPS] Unable to connect\r\n", TAG, __LINE__);
        bool res = readBufferFromFile(buffer);
        if (res)
          display_show_msg(buffer, WIFI_INTERNAL_ERROR);
        else
          display_show_msg(const_cast<uint8_t *>(default_icon), WIFI_INTERNAL_ERROR);
        // display_sleep();
      }
      Log.info("%s [%d]: status - %d\r\n", TAG, __LINE__, status);
      if (status)
      {
        status = false;
        memset(new_url, 0, sizeof(new_url));
        strcpy(new_url, url);
        strcat(new_url, filename);
        Log.info("%s [%d]: filename - %s\r\n", TAG, __LINE__, filename);

        Log.info("%s [%d]: [HTTPS] Request to %s\r\n", TAG, __LINE__, new_url);
        if (https.begin(*client, new_url))
        { // HTTPS
          Log.info("%s [%d]: [HTTPS] GET..\r\n", TAG, __LINE__);
          // start connection and send HTTP header
          int httpCode = https.GET();

          // httpCode will be negative on error
          if (httpCode > 0)
          {
            // HTTP header has been send and Server response header has been handled
            Log.error("%s [%d]: [HTTPS] GET... code: %d\r\n", TAG, __LINE__, httpCode);
            // file found at server
            if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
            {
              Log.info("%s [%d]: Content size: %d\r\n", TAG, __LINE__, https.getSize());

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
                Log.info("%s [%d]: Received successfully\r\n", TAG, __LINE__);
                // EPD_7IN5_V2_Clear();
                // DEV_Delay_ms(500);

                writeBufferToFile("logo_white.bmp", buffer, sizeof(buffer));

                // show the image
                display_show_image(buffer);
              }
              else
              {
                Log.error("%s [%d]: Receiving failed. Readed: %d\r\n", TAG, __LINE__, counter);
                bool res = readBufferFromFile(buffer);
                if (res)
                  display_show_msg(buffer, API_SIZE_ERROR);
                else
                  display_show_msg(const_cast<uint8_t *>(default_icon), API_SIZE_ERROR);
              }
            }
            else
            {
              Log.error("%s [%d]: [HTTPS] GET... failed, error: %s\r\n", TAG, __LINE__, https.errorToString(httpCode).c_str());
              https.end();
              bool res = readBufferFromFile(buffer);
              if (res)
                display_show_msg(buffer, API_ERROR);
              else
                display_show_msg(const_cast<uint8_t *>(default_icon), API_ERROR);
            }
          }
          else
          {
            Log.error("%s [%d]: [HTTPS] GET... failed, error: %s\r\n", TAG, __LINE__, https.errorToString(httpCode).c_str());
            bool res = readBufferFromFile(buffer);
            if (res)
              display_show_msg(buffer, API_ERROR);
            else
              display_show_msg(const_cast<uint8_t *>(default_icon), API_ERROR);
          }
        }
        else
        {
          Log.error("%s [%d]: unable to connect\r\n", TAG, __LINE__);
          bool res = readBufferFromFile(buffer);
          if (res)
            display_show_msg(buffer, API_ERROR);
          else
            display_show_msg(const_cast<uint8_t *>(default_icon), API_ERROR);
        }
      }
      // End extra scoping block
    }
    client->stop();
    delete client;
  }
  else
  {
    Log.error("%s [%d]: Unable to create client\r\n", TAG, __LINE__);
    bool res = readBufferFromFile(buffer);
    if (res)
      display_show_msg(buffer, WIFI_INTERNAL_ERROR);
    else
      display_show_msg(const_cast<uint8_t *>(default_icon), WIFI_INTERNAL_ERROR);
  }
}

static void goToSleep(void)
{
  uint64_t time_to_sleep = preferences.getLong64(PREFERENCES_SLEEP_TIME_KEY, SLEEP_TIME_TO_SLEEP);
  Log.info("%s [%d]: time to sleep - %d\r\n", TAG, __LINE__, time_to_sleep);
  preferences.end();
  esp_sleep_enable_timer_wakeup(time_to_sleep * SLEEP_uS_TO_S_FACTOR);
  esp_deep_sleep_enable_gpio_wakeup(1 << PIN_INTERRUPT,
                                    ESP_GPIO_WAKEUP_GPIO_LOW);
  esp_deep_sleep_start();
}