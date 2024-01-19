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
// #include "imagedata.h"
#include <stdlib.h>
#include <SPIFFS.h>

static const char *TAG = "bl.cpp";

bool pref_clear = false;
WiFiManager wifi_manager;
bool wm_nonblocking = false; // change to true to use non blocking
char filename[100];
uint8_t buffer[48130];
static void downloadAndSaveToFile(const char *url);
bool status = false;

static void configModeCallback(WiFiManager *myWiFiManager);

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

              WiFiClient *stream = https.getStreamPtr();

              uint32_t counter = 0;
              // Read and save BMP data to SPIFFS

              // Find the last occurrence of '/'
              // const char *lastSlash = strrchr(filename, '/');
              // String bmp_name;
              // if (lastSlash != nullptr)
              //{
              // Extract the filename (substring after the last '/')
              //  bmp_name = lastSlash;

              //  Serial.print("Filename: ");
              //  Serial.println(bmp_name);
              //}
              // else
              //{
              //  Serial.println("Invalid file path");
              //}
              // if (!SPIFFS.exists(bmp_name))
              //{
              //  File file = SPIFFS.open(bmp_name, "w");
              if (stream->available() && https.getSize() == sizeof(buffer))
              {
                /*Serial.print("0x");
                Serial.print(stream->read(), HEX);
                Serial.print(",");
                */
                counter = stream->readBytes(buffer, sizeof(buffer));
                // file.write(buffer, sizeof(buffer));
              }
              // file.close();
              if (counter == sizeof(buffer))
              {
                Serial.print("Written succes: ");
                Serial.println(counter);
                Serial.println("Time to draw");
                // EPD_7IN5_V2_Clear();
                // DEV_Delay_ms(500);
                //  Create a new image cache
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

                printf("show image for array\r\n");
                Paint_SelectImage(BlackImage);
                Serial.println("Select image");
                Paint_Clear(WHITE);
                Serial.println("clear");
                Paint_DrawBitMap(buffer + 130);
                Serial.println("Draw bitmap");
                EPD_7IN5_V2_Display(BlackImage);
                Serial.println("Display");
                printf("Goto Sleep...\r\n");
                DEV_Delay_ms(2000);
                // EPD_7IN5_V2_Sleep();
                free(BlackImage);
                BlackImage = NULL;
              }
              else
              {
                Serial.print("Receiving failed. Readed: ");
                Serial.println(counter);
                // if (SPIFFS.remove(bmp_name))
                //   Serial.println("File deleted");
                // else
                //   Serial.println("File deleting error");
              }
              //}
              // else
              //{
              //  Serial.println("File exists");
              //  if (SPIFFS.remove(bmp_name))
              //    Serial.println("File deleted");
              //  else
              //    Serial.println("File deleting error");
              //}
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

void test_init(void)
{
  // put your setup code here, to run once:
  Serial.begin(115200);

  // WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wm;

  // reset settings - wipe stored credentials for testing
  // these are stored by the esp library
  wm.resetSettings();

  // Automatically connect using saved credentials,
  // if connection fails, it starts an access point with the specified name ( "AutoConnectAP"),
  // if empty will auto generate SSID, if password is blank it will be anonymous AP (wm.autoConnect())
  // then goes into a blocking loop awaiting configuration and will return success result
  delay(4000);
  Serial.println("Start AP");
  wm.setConnectTimeout(15);
  bool res;
  // res = wm.autoConnect(); // auto generated AP name from chipid
  // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
  res = wm.autoConnect("AutoConnectAP", "password"); // password protected ap

  if (!res)
  {
    Serial.println("Failed to connect");
    // ESP.restart();
  }
  else
  {
    // if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");
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
  // Mount SPIFFS
  if (!SPIFFS.begin(true))
  {
    Serial.println("Failed to mount SPIFFS");
    return;
  }

  Serial.println("SPIFFS mounted");
  printf("EPD_7IN5_V2_test Demo\r\n");
  DEV_Module_Init();

  printf("e-Paper Init and Clear...\r\n");
  EPD_7IN5_V2_Init();
  printf("e-Paper Clear...\r\n");
  EPD_7IN5_V2_Clear();
  // Download file and save to SPIFFS
  // downloadAndSaveToFile("https://usetrmnl.com");
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP

  // DEV_Delay_ms(500);
  if (wifi_manager.getWiFiIsSaved())
  {
    Serial.println("WiFi saved");
  }
  else
  {
    Serial.println("WiFi NOT saved");
    if (!SPIFFS.exists("logo_white.bmp"))
    {
      Log.info("%s [%d]: icon exists\r\n", TAG, __LINE__);
      File file = SPIFFS.open("/logo_white.bmp", FILE_READ);
      if (file)
      {
        if (file.size() == sizeof(buffer))
        {
          Log.info("%s [%d]: the size is the same\r\n", TAG, __LINE__);
          file.readBytes((char *)buffer, sizeof(buffer));
        }
        else
        {
          Log.info("%s [%d]: the size is NOT the same %d\r\n", TAG, __LINE__, file.size());
        }
        file.close();
      }
      else
      {
        Log.info("%s [%d]: File open ERROR\r\n", TAG, __LINE__);
      }
    }
    else
    {
      Log.info("%s [%d]: icon DOESN\'T exists\r\n", TAG, __LINE__);
    }

    /* you have to edit the startup_stm32fxxx.s file and set a big enough heap size */
    uint8_t *BlackImage;
    uint16_t Imagesize = ((EPD_7IN5_V2_WIDTH % 8 == 0) ? (EPD_7IN5_V2_WIDTH / 8) : (EPD_7IN5_V2_WIDTH / 8 + 1)) * EPD_7IN5_V2_HEIGHT;
    if ((BlackImage = (UBYTE *)malloc(Imagesize)) == NULL)
    {
      printf("Failed to apply for black memory...\r\n");
      while (1)
        ;
    }
    printf("Paint_NewImage\r\n");
    Paint_NewImage(BlackImage, EPD_7IN5_V2_WIDTH, EPD_7IN5_V2_HEIGHT, 0, WHITE);

    printf("show image for array\r\n");
    Paint_SelectImage(BlackImage);
    Paint_Clear(WHITE);
    Paint_DrawBitMap(buffer + 130);
    Paint_DrawString_EN(225, 400, "Connect to trmnl WiFi", &Font24, WHITE, BLACK);
    EPD_7IN5_V2_Display(BlackImage);
    // DEV_Delay_ms(500);
    free(BlackImage);
    BlackImage = NULL;
  }

  // wifi_manager.setConnectRetries(1);
  wifi_manager.setConnectTimeout(10);
  // wifi_manager.setBreakAfterConfig(true);
  // wifi_manager.setAPCallback(configModeCallback);
  // wifi_manager.setBreakAfterConfig(true);
  // wifi_manager.setConfigPortalTimeout(10);
  delay(3000);
  Serial.println("Start AP");
  bool res = wifi_manager.autoConnect("trmnl"); // password protected ap
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  if (!res)
  {
    Log.error("%s [%d]: Failed to connect or hit timeout\r\n", TAG, __LINE__);
    // show logo with string
    if (!SPIFFS.exists("logo_white.bmp"))
    {
      Log.info("%s [%d]: icon exists\r\n", TAG, __LINE__);
      File file = SPIFFS.open("/logo_white.bmp", FILE_READ);
      if (file)
      {
        if (file.size() == sizeof(buffer))
        {
          Log.info("%s [%d]: the size is the same\r\n", TAG, __LINE__);
          file.readBytes((char *)buffer, sizeof(buffer));
        }
        else
        {
          Log.info("%s [%d]: the size is NOT the same %d\r\n", TAG, __LINE__, file.size());
        }
        file.close();
      }
      else
      {
        Log.info("%s [%d]: File open ERROR\r\n", TAG, __LINE__);
      }
    }
    else
    {
      Log.info("%s [%d]: icon DOESN\'T exists\r\n", TAG, __LINE__);
    }

    /* you have to edit the startup_stm32fxxx.s file and set a big enough heap size */
    uint8_t *BlackImage;
    uint16_t Imagesize = ((EPD_7IN5_V2_WIDTH % 8 == 0) ? (EPD_7IN5_V2_WIDTH / 8) : (EPD_7IN5_V2_WIDTH / 8 + 1)) * EPD_7IN5_V2_HEIGHT;
    if ((BlackImage = (UBYTE *)malloc(Imagesize)) == NULL)
    {
      printf("Failed to apply for black memory...\r\n");
      while (1)
        ;
    }
    printf("Paint_NewImage\r\n");
    Paint_NewImage(BlackImage, EPD_7IN5_V2_WIDTH, EPD_7IN5_V2_HEIGHT, 0, WHITE);

    printf("show image for array\r\n");
    Paint_SelectImage(BlackImage);
    Paint_Clear(WHITE);
    Paint_DrawBitMap(buffer + 130);
    Paint_DrawString_EN(150, 400, "Can't establsih WiFi connection with saved WiFi", &Font24, WHITE, BLACK);
    Paint_DrawString_EN(175, 430, "Hold button on the back to reset WiFi", &Font24, WHITE, BLACK);
    EPD_7IN5_V2_Display(BlackImage);
    // DEV_Delay_ms(500);
    free(BlackImage);
    BlackImage = NULL;

    // Go to deep sleep
  }
  else
  {
    // if you get here you have connected to the WiFi
    Log.error("%s [%d]: connected...\r\n)", TAG, __LINE__);
  }
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
  if (pref_clear)
  {
    pref_clear = false;
    wifi_manager.resetSettings();
    if (wifi_manager.getWiFiIsSaved())
      Serial.println("WiFi saved");
    else
      Serial.println("WiFi NOT saved");
    ESP.restart();
  }

  downloadAndSaveToFile("https://usetrmnl.com");

  delay(10000);
}

static void configModeCallback(WiFiManager *myWiFiManager)
{
  Serial.println("Entered config mode");

  Serial.println(myWiFiManager->disconnect());
}