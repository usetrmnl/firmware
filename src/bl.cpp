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

static const char *TAG = "bl.cpp";

bool pref_clear = false;
WiFiManager wm;

// timers
uint8_t buffer[48130];
char filename[100];

bool status = false;

// timers
uint32_t timer = 0;
uint32_t button_timer = 0;

static void downloadAndSaveToFile(const char *url);
static bool readBufferFromFile(uint8_t *out_buffer);
static void goToSleep(void);

/**
 * @brief Function to init business logic module
 * @param none
 * @return none
 */
void bl_init(void)
{
  Serial.begin(115200);
  Log.begin(LOG_LEVEL_VERBOSE, &Serial);
  Log.info("%s [%d]: BL inited success\r\n", TAG, __LINE__);
  pins_init();
  button_timer = millis();

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
    readBufferFromFile(buffer);
    display_show_msg(buffer, WIFI_CONNECT);
  }

  wm.setClass("invert");
  wm.setConnectRetries(1);
  wm.setConnectTimeout(10);
  wm.setBreakAfterConfig(true);       // always exit configportal even if wifi save fails
  bool res = wm.autoConnect("trmnl"); // password protected ap
  if (!res)
  {
    Log.error("%s [%d]: Failed to connect or hit timeout\r\n", TAG, __LINE__);
    wm.disconnect();
    wm.stopWebPortal();
    wm.resetSettings();
    // show logo with string
    readBufferFromFile(buffer);
    display_show_msg(buffer, WIFI_FAILED);
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

static bool readBufferFromFile(uint8_t *out_buffer)
{
  if (!SPIFFS.exists("logo_white.bmp"))
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
      if (https.begin(*client, new_url))
      { // HTTPS
        Log.info("%s [%d]: [HTTPS] GET...\r\n", TAG, __LINE__);
        // start connection and send HTTP header
        int httpCode = https.GET();

        // httpCode will be negative on error
        if (httpCode > 0)
        {
          // HTTP header has been send and Server response header has been handled
          Log.info("%s [%d]: GET... code: %d\r\n", TAG, __LINE__, httpCode);
          // file found at server
          if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
          {
            Log.info("%s [%d]: Content size: %d\r\n", TAG, __LINE__, https.getSize());
            String payload = https.getString();

            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, payload);
            if (error)
              return;
            String name = doc["content"];
            Log.info("%s [%d]: Content name: %s\r\n", TAG, __LINE__, name);
            name.toCharArray(filename, name.length() + 1);
            status = true;
          }
          else
          {
            Log.info("%s [%d]: [HTTPS] Unable to connect\r\n", TAG, __LINE__);
            readBufferFromFile(buffer);
            display_show_msg(buffer, API_ERROR);
            // display_sleep();
          }
        }
        else
        {
          Log.error("%s [%d]: [HTTPS] GET... failed, error: %s\r\n", TAG, __LINE__, https.errorToString(httpCode).c_str());
          readBufferFromFile(buffer);
          display_show_msg(buffer, API_ERROR);
          // display_sleep();
        }

        https.end();
      }
      else
      {
        Log.error("%s [%d]: [HTTPS] Unable to connect\r\n", TAG, __LINE__);
        readBufferFromFile(buffer);
        display_show_msg(buffer, API_ERROR);
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
                readBufferFromFile(buffer);
                display_show_msg(buffer, API_SIZE_ERROR);
              }
            }
            else
            {
              Log.error("%s [%d]: [HTTPS] GET... failed, error: %s\r\n", TAG, __LINE__, https.errorToString(httpCode).c_str());
              readBufferFromFile(buffer);
              display_show_msg(buffer, API_ERROR);
            }
          }
          else
          {
            Log.error("%s [%d]: [HTTPS] GET... failed, error: %s\r\n", TAG, __LINE__, https.errorToString(httpCode).c_str());
            readBufferFromFile(buffer);
            display_show_msg(buffer, API_ERROR);
          }

          https.end();
        }
        else
        {
          Log.error("%s [%d]: unable to connect\r\n", TAG, __LINE__);
          readBufferFromFile(buffer);
          display_show_msg(buffer, API_ERROR);
        }
      }
      // End extra scoping block
    }

    delete client;
  }
  else
  {
    Log.error("%s [%d]: Unable to create client\r\n", TAG, __LINE__);
  }
  // display_sleep();
}

static void goToSleep(void)
{
  esp_sleep_enable_timer_wakeup(SLEEP_TIME_TO_SLEEP * SLEEP_uS_TO_S_FACTOR);
  esp_deep_sleep_enable_gpio_wakeup(1 << PIN_INTERRUPT,
                                    ESP_GPIO_WAKEUP_GPIO_LOW);
  esp_deep_sleep_start();
}