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
#include "DEV_Config.h"
#include "EPD.h"
#include "GUI_Paint.h"
#include "imagedata.h"
#include <stdlib.h>

static const char *TAG = "bl.cpp";

bool pref_clear = false;
WiFiManager wifi_manager;
bool wm_nonblocking = false; // change to true to use non blocking
char filename[100];
static void downloadAndSaveToFile(const char *url);
bool status = false;

void IRAM_ATTR button_reset_handler()
{
  pref_clear = true;
  Log.info("%s [%d]: Interrupt\r\n", TAG, __LINE__);
}

void downloadAndSaveToFile(const char *url)
{
  WiFiClientSecure *client = new WiFiClientSecure;
  if (client)
  {
    // client->setCACert(rootCACertificate);
    client->setInsecure();
    {
      // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is
      HTTPClient https;

      Serial.print("[HTTPS] begin...\n");
      char new_url[200];
      strcpy(new_url, url);
      strcat(new_url, "/api/display");
      if (https.begin(*client, new_url))
      { // HTTPS
        Serial.print("[HTTPS] GET...\n");
        // start connection and send HTTP header
        int httpCode = https.GET();

        // httpCode will be negative on error
        if (httpCode > 0)
        {
          // HTTP header has been send and Server response header has been handled
          Serial.printf("[HTTPS] GET... code: %d\n", httpCode);

          // file found at server
          if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
          {
            Serial.print("Content-Size: ");
            Serial.println(https.getSize());
            String payload = https.getString();
            Serial.print("Payload: ");
            Serial.println(payload);

            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, payload);
            if (error)
              return;
            String name = doc["content"];
            Serial.println(name);
            name.toCharArray(filename, name.length() + 1);
            status = true;
          }
        }
        else
        {
          Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
        }

        https.end();
      }
      else
      {
        Serial.printf("[HTTPS] Unable to connect\n");
      }

      if (status)
      {
        status = false;
        memset(new_url, 0, sizeof(new_url));
        strcpy(new_url, url);
        strcat(new_url, filename);
        Serial.print("Request to ");
        Serial.println(new_url);
        if (https.begin(*client, new_url))
        { // HTTPS
          Serial.print("[HTTPS] GET...\n");
          // start connection and send HTTP header
          int httpCode = https.GET();

          // httpCode will be negative on error
          if (httpCode > 0)
          {
            // HTTP header has been send and Server response header has been handled
            Serial.printf("[HTTPS] GET... code: %d\n", httpCode);

            // file found at server
            if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
            {
              Serial.print("Content-Size: ");
              Serial.println(https.getSize());

              WiFiClient* stream = https.getStreamPtr();

              uint32_t counter = 0;
              while (stream->available())
              {
                Serial.print("0x");
                Serial.print(stream->read(), HEX);
                Serial.print(",");
                counter++;
                if(counter % 30 == 0)
                  Serial.println();
                //delay(1);
              }
              Serial.println();
              Serial.println(counter);
            }
          }
          else
          {
            Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
          }

          https.end();
        }
        else
        {
          Serial.printf("[HTTPS] Unable to connect\n");
        }
      }
      // End extra scoping block
    }

    delete client;
  }
  else
  {
    Serial.println("Unable to create client");
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
  Log.info("%s [%d]: BL inited success\r\n", TAG, __LINE__);
  pins_init();
  pins_set_clear_interrupt(button_reset_handler);
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  if (wm_nonblocking)
    wifi_manager.setConfigPortalBlocking(false);

  bool res = wifi_manager.autoConnect("trmnl"); // password protected ap

  if (!res)
  {
    Log.error("%s [%d]: Failed to connect or hit timeout\r\n", TAG, __LINE__);
    // ESP.restart();
  }
  else
  {
    // if you get here you have connected to the WiFi
    Log.error("%s [%d]: connected...\r\n)", TAG, __LINE__);
  }

  // Download file and save to SPIFFS
  downloadAndSaveToFile("https://usetrmnl.com");

  printf("EPD_7IN5_V2_test Demo\r\n");
  DEV_Module_Init();

  printf("e-Paper Init and Clear...\r\n");
  EPD_7IN5_V2_Init();
  EPD_7IN5_V2_Clear();
  DEV_Delay_ms(500);

  // Create a new image cache
  UBYTE *BlackImage;
  /* you have to edit the startup_stm32fxxx.s file and set a big enough heap size */
  UWORD Imagesize = ((EPD_7IN5_V2_WIDTH % 8 == 0) ? (EPD_7IN5_V2_WIDTH / 8) : (EPD_7IN5_V2_WIDTH / 8 + 1)) * EPD_7IN5_V2_HEIGHT;
  if ((BlackImage = (UBYTE *)malloc(Imagesize)) == NULL)
  {
    printf("Failed to apply for black memory...\r\n");
    while (1)
      ;
  }
  printf("Paint_NewImage\r\n");
  Paint_NewImage(BlackImage, EPD_7IN5_V2_WIDTH, EPD_7IN5_V2_HEIGHT, 0, WHITE);

#if 1 // show image for array
  printf("show image for array\r\n");
  Paint_SelectImage(BlackImage);
  Paint_Clear(WHITE);
  Paint_DrawBitMap(gImage_7in5_V2);
  EPD_7IN5_V2_Display(BlackImage);
  DEV_Delay_ms(2000);
#endif
  printf("Clear...\r\n");
  //EPD_7IN5_V2_Clear();

  printf("Goto Sleep...\r\n");
  EPD_7IN5_V2_Sleep();
  free(BlackImage);
  BlackImage = NULL;
}

/**
 * @brief Function to process business logic module
 * @param none
 * @return none
 */
void bl_process(void)
{
  if (wm_nonblocking)
    wifi_manager.process();

  if (pref_clear)
  {
    pref_clear = false;
    wifi_manager.resetSettings();
    ESP.restart();
  }

  downloadAndSaveToFile("https://usetrmnl.com");

  delay(10000);
}