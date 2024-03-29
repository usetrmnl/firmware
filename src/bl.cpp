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
uint8_t buffer[48062];
char filename[1024];
char binUrl[1024];

bool status = false;
bool keys_stored = false;
bool update_firmware = false;

// timers
uint32_t timer = 0;
uint32_t button_timer = 0;

Preferences preferences;

static bool parseBMPHeader(uint8_t *data, bool &reserved);
static void downloadAndSaveToFile(const char *url);
static void getDeviceCredentials(const char *url);
static bool readBufferFromFile(uint8_t *out_buffer);
static bool writeBufferToFile(const char *name, uint8_t *in_buffer, uint16_t size);
static void checkAndPerformFirmwareUpdate(void);
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

void listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
  Serial.printf("Listing directory: %s\r\n", dirname);
  File root = fs.open(dirname);
  if (!root)
  {
    Serial.println("- failed to open directory");
    return;
  }
  if (!root.isDirectory())
  {
    Serial.println(" - not a directory");
    return;
  }
  File file = root.openNextFile();
  while (file)
  {
    if (file.isDirectory())
    {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels)
      {
        listDir(fs, file.name(), levels - 1);
      }
    }
    else
    {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("\tSIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
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
    listDir(SPIFFS, "/", 0);
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
    WiFi.begin(wm.getWiFiSSID(), wm.getWiFiPass());

    uint8_t attepmts = 0;
    Log.info("%s [%d]: wifi connection...\r\n", TAG, __LINE__);
    while (WiFi.status() != WL_CONNECTED && attepmts < WIFI_CONNECTION_ATTEMPTS)
    {
      delay(1000);
      attepmts++;
    }
    Serial.println();
    // Check if connected
    if (WiFi.status() == WL_CONNECTED)
    {
      String ip = String(WiFi.localIP());
      Log.info("%s [%d]:wifi_connection [DEBUG]: Connected: %s\r\n", TAG, __LINE__, ip.c_str());
    }
    else
    {
      Log.fatal("%s [%d]: Connection failed!\r\n", TAG, __LINE__);
      WiFi.disconnect();
      res = readBufferFromFile(buffer);
      if (res)
        display_show_msg(buffer, WIFI_FAILED);
      else
        display_show_msg(const_cast<uint8_t *>(default_icon), WIFI_FAILED);
      // Go to deep sleep
      display_sleep();
      goToSleep();
    }
  }
  else
  {
    Log.info("%s [%d]: WiFi NOT saved\r\n", TAG, __LINE__);

    char fw_version[20];

    sprintf(fw_version, "%d.%d.%d", FW_MAJOR_VERSION, FW_MINOR_VERSION, FW_PATCH_VERSION);

    String fw = fw_version;

    Log.info("%s [%d]: FW version %s\r\n", TAG, __LINE__, fw_version);
    Log.info("%s [%d]: FW version %s\r\n", TAG, __LINE__, fw.c_str());
    res = readBufferFromFile(buffer);
    if (res)
    {
      Log.info("%s [%d]: logo not exists. Use default\r\n", TAG, __LINE__);
      if (preferences.isKey(PREFERENCES_FRIENDLY_ID))
      {
        Log.info("%s [%d]: friendly ID exists\r\n", TAG, __LINE__);
        String friendly_id = preferences.getString(PREFERENCES_FRIENDLY_ID, PREFERENCES_FRIENDLY_ID_DEFAULT);

        display_show_msg(buffer, WIFI_CONNECT, friendly_id, fw.c_str());
      }
      else
      {
        display_show_msg(buffer, WIFI_CONNECT, "NOT SAVED", fw.c_str());
      }
    }
    else
    {
      Log.info("%s [%d]: logo not exists. Use default\r\n", TAG, __LINE__);
      if (preferences.isKey(PREFERENCES_FRIENDLY_ID))
      {
        Log.info("%s [%d]: friendly ID exists\r\n", TAG, __LINE__);
        String friendly_id = preferences.getString(PREFERENCES_FRIENDLY_ID, PREFERENCES_FRIENDLY_ID_DEFAULT);
        display_show_msg(const_cast<uint8_t *>(default_icon), WIFI_CONNECT, friendly_id, fw.c_str());
      }
      else
      {
        Log.error("%s [%d]: friendly ID NOT exists\r\n", TAG, __LINE__);
        display_show_msg(const_cast<uint8_t *>(default_icon), WIFI_CONNECT, "NOT SAVED", fw.c_str());
      }
    }
    wm.setClass("invert");
    // wm.setConnectRetries(1);
    wm.setConnectTimeout(10);
    // wm.setBreakAfterConfig(true);  // always exit configportal even if wifi save fails
    res = wm.startConfigPortal("TRMNL"); // password protected ap
    if (!res)
    {
      Log.error("%s [%d]: Failed to connect or hit timeout\r\n", TAG, __LINE__);
      wm.disconnect();
      wm.stopWebPortal();
      WiFi.disconnect();
      // wm.resetSettings();
      //  show logo with string
      res = readBufferFromFile(buffer);
      if (res)
        display_show_msg(buffer, WIFI_FAILED);
      else
        display_show_msg(const_cast<uint8_t *>(default_icon), WIFI_FAILED);
      // Go to deep sleep
      display_sleep();
      goToSleep();
    }
    Log.info("%s [%d]: WiFi connected\r\n", TAG, __LINE__);
  }

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

  if (update_firmware)
  {
    checkAndPerformFirmwareUpdate();
  }

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
  else
  {
    Log.info("%s [%d]: file %s not exists.\r\n", TAG, __LINE__, name);
  }
  File file = SPIFFS.open(name, FILE_APPEND);
  Serial.println(file);
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
  if (SPIFFS.exists("/logo.bmp"))
  {
    Log.info("%s [%d]: icon exists\r\n", TAG, __LINE__);
    File file = SPIFFS.open("/logo.bmp", FILE_READ);
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

      String fw_version = String(FW_MAJOR_VERSION) + "." + String(FW_MINOR_VERSION) + "." + String(FW_PATCH_VERSION);

      float battery_voltage = readBatteryVoltage();
      Log.info("%s [%d]: %s battery voltage - %f\r\n", TAG, __LINE__, PREFERENCES_SLEEP_TIME_KEY, battery_voltage);

      Log.info("%s [%d]: Added headers:\n\rID: %s\n\rAccess-Token: %s\n\rRefresh_Rate: %s\n\rBattery-Voltage: %s\n\rFW-Version: %s\r\n", TAG, __LINE__, WiFi.macAddress().c_str(), api_key.c_str(), String(refresh_rate).c_str(), String(battery_voltage).c_str(), fw_version.c_str());

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

        delay(5);

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
            update_firmware = doc["update_firmware"];
            String firmware_url = doc["firmware_url"];
            uint64_t rate = doc["refresh_rate"];

            if (image_url.length() > 0)
            {
              Log.info("%s [%d]: image_url: %s\r\n", TAG, __LINE__, image_url.c_str());
              image_url.toCharArray(filename, image_url.length() + 1);
            }
            Log.info("%s [%d]: update_firmware: %d\r\n", TAG, __LINE__, update_firmware);
            if (firmware_url.length() > 0)
            {
              Log.info("%s [%d]: firmware_url: %s\r\n", TAG, __LINE__, firmware_url.c_str());
              firmware_url.toCharArray(binUrl, firmware_url.length() + 1);
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

      if (status && !update_firmware)
      {
        status = false;
        // memset(new_url, 0, sizeof(new_url));
        // strcpy(new_url, url);
        // strcat(new_url, filename);

        Log.info("%s [%d]: [HTTPS] Request to %s\r\n", TAG, __LINE__, filename);
        if (https.begin(*client, filename))
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
              Log.info("%s [%d]: Stream timeout: %d\r\n", TAG, __LINE__, stream->getTimeout());
              uint32_t counter = 0;
              Log.info("%s [%d]: Stream available: %d\r\n", TAG, __LINE__, stream->available());

              uint32_t timer = millis();
              while (!stream->available() && millis() - timer < 1000)
                ;

              Log.info("%s [%d]: Stream available: %d\r\n", TAG, __LINE__, stream->available());
              // Read and save BMP data to buffer
              if (stream->available() && https.getSize() == DISPLAY_BMP_IMAGE_SIZE)
              {
                counter = stream->readBytes(buffer, sizeof(buffer));
              }

              if (counter == DISPLAY_BMP_IMAGE_SIZE)
              {
                Log.info("%s [%d]: Received successfully\r\n", TAG, __LINE__);
                // EPD_7IN5_V2_Clear();
                // DEV_Delay_ms(500);
                bool image_reverse = false;
                bool res = parseBMPHeader(buffer, image_reverse);
                if (res)
                {
                  // show the image
                  display_show_image(buffer, image_reverse);
                }
                else
                {
                  res = readBufferFromFile(buffer);
                  if (res)
                    display_show_msg(buffer, BMP_FORMAT_ERROR);
                  else
                    display_show_msg(const_cast<uint8_t *>(default_icon), BMP_FORMAT_ERROR);
                }
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
        Log.info("%s [%d]: Downloading .bin file...\r\n", TAG, __LINE__);

        size_t contentLength = https.getSize();
        // Perform firmware update
        if (Update.begin(contentLength))
        {
          Log.info("%s [%d]: Firmware update start\r\n", TAG, __LINE__);
          bool res = readBufferFromFile(buffer);
          if (res)
            display_show_msg(buffer, FW_UPDATE);
          else
            display_show_msg(const_cast<uint8_t *>(default_icon), FW_UPDATE);
          if (Update.writeStream(https.getStream()))
          {
            if (Update.end(true))
            {
              Log.info("%s [%d]: Firmware update successful. Rebooting...\r\n", TAG, __LINE__);
              bool res = readBufferFromFile(buffer);
              if (res)
                display_show_msg(buffer, FW_UPDATE_SUCCESS);
              else
                display_show_msg(const_cast<uint8_t *>(default_icon), FW_UPDATE_SUCCESS);
            }
            else
            {
              Log.fatal("%s [%d]: Firmware update failed!\r\n", TAG, __LINE__);
              bool res = readBufferFromFile(buffer);
              if (res)
                display_show_msg(buffer, FW_UPDATE_FAILED);
              else
                display_show_msg(const_cast<uint8_t *>(default_icon), FW_UPDATE_FAILED);
            }
          }
          else
          {
            Log.fatal("%s [%d]: Write to firmware update stream failed!\r\n", TAG, __LINE__);
            bool res = readBufferFromFile(buffer);
            if (res)
              display_show_msg(buffer, FW_UPDATE_FAILED);
            else
              display_show_msg(const_cast<uint8_t *>(default_icon), FW_UPDATE_FAILED);
          }
        }
        else
        {
          Log.fatal("%s [%d]: Begin firmware update failed!\r\n", TAG, __LINE__);
          bool res = readBufferFromFile(buffer);
          if (res)
            display_show_msg(buffer, FW_UPDATE_FAILED);
          else
            display_show_msg(const_cast<uint8_t *>(default_icon), FW_UPDATE_FAILED);
        }
      }
      else
      {
        Log.fatal("%s [%d]: HTTP GET failed!\r\n", TAG, __LINE__);
        bool res = readBufferFromFile(buffer);
        if (res)
          display_show_msg(buffer, API_ERROR);
        else
          display_show_msg(const_cast<uint8_t *>(default_icon), API_ERROR);
      }
      https.end();
    }
  }
  delete client;
}

static bool parseBMPHeader(uint8_t *data, bool &reserved)
{
  // Check if the file is a BMP image
  if (data[0] != 'B' || data[1] != 'M')
  {
    Log.fatal("%s [%d]: It is not a BMP file\r\n", __FILE__, __LINE__);
    return false;
  }

  // Get width and height from the header
  uint32_t width = *(uint32_t *)&data[18];
  uint32_t height = *(uint32_t *)&data[22];
  uint16_t bitsPerPixel = *(uint16_t *)&data[28];
  uint32_t compressionMethod = *(uint32_t *)&data[30];
  uint32_t imageDataSize = *(uint32_t *)&data[34];
  uint32_t colorTableEntries = *(uint32_t *)&data[46];

  if (width != 800 || height != 480 || bitsPerPixel != 1 || imageDataSize != 48000 || colorTableEntries != 2)
    return false;
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
      reserved = false;
    }
    else if (data[54] == 255 && data[55] == 255 && data[56] == 255 && data[57] == 0 && data[58] == 0 && data[59] == 0 && data[60] == 0 && data[61] == 0)
    {
      Log.info("%s [%d]: Color scheme reversed\r\n", __FILE__, __LINE__);
      reserved = true;
    }
    else
    {
      Log.info("%s [%d]: Color scheme demaged\r\n", __FILE__, __LINE__);
      return false;
    }
    return true;
  }
  else
  {
    return false;
  }
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
        // memset(new_url, 0, sizeof(new_url));
        // strcpy(new_url, url);
        // strcat(new_url, filename);
        Log.info("%s [%d]: filename - %s\r\n", TAG, __LINE__, filename);

        Log.info("%s [%d]: [HTTPS] Request to %s\r\n", TAG, __LINE__, filename);
        if (https.begin(*client, filename))
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

                bool res = writeBufferToFile("/logo.bmp", buffer, sizeof(buffer));
                if (res)
                  Log.info("%s [%d]: File written!\r\n", TAG, __LINE__);
                else
                  Log.error("%s [%d]: File not written!\r\n", TAG, __LINE__);

                // show the image
                display_show_image(buffer, false);
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