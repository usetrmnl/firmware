#include <Arduino.h>
#include <ArduinoLog.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <GUI_Paint.h>
#include <HTTPClient.h>
#include <ImageData.h>
#include <Preferences.h>
#include <SPIFFS.h>
#include <Update.h>
#include <WiFiClientSecure.h>
#include <WifiCaptive.h>
#include <api-client/display.h>
#include <api-client/submit_log.h>
#include <api_response_parsing.h>
#include <bl.h>
#include <bmp.h>
#include <button.h>
#include <config.h>
#include <cstdint>
#include <cstdio>
#include <display.h>
#include <filesystem.h>
#include <http_client.h>
#include <image.h>
#include <logging_parcers.h>
#include <math.h>
#include <pins.h>
#include <png.h>
#include <rover_bmp.h>
#include <rover_topdown_bmp.h>
#include <special_function.h>
#include <stdlib.h>
#include <stored_logs.h>
#include <trmnl_logo_bmp.h>
#include <sleep_bmp.h>
#include <trmnl_logo_png.h>
#include <types.h>
#include  "utility/EPD_7in5_V2.h"

bool pref_clear = false;
String new_filename = "";

uint32_t SIZE_MARGIN = 5000;
uint32_t MAX_PAYLOAD_SIZE =
    stride_32(display_width(), 1) * display_height() + SIZE_MARGIN;
char filename[1024];      // image URL
char binUrl[1024];        // update URL
char log_array[1024];     // log
char message_buffer[128]; // message to show on the screen
uint32_t time_since_sleep;

bool status = false;          // need to download a new image
bool update_firmware = false; // need to download a new firmware
bool reset_firmware = false;  // need to reset credentials
bool send_log = false;        // need to send logs
bool double_click = false;
bool log_retry = false; // need to log connection retry
esp_sleep_wakeup_cause_t wakeup_reason =
    ESP_SLEEP_WAKEUP_UNDEFINED; // wake-up reason
MSG current_msg = NONE;
SPECIAL_FUNCTION special_function = SF_NONE;
RTC_DATA_ATTR uint8_t need_to_refresh_display = 1;

Preferences preferences;

static https_request_err_e downloadAndShow(); // download and show the image
static https_request_err_e
handleApiDisplayResponse(ApiDisplayResponse &apiResponse);
static void getDeviceCredentials(); // receiveing API key and Friendly ID
static void
resetDeviceCredentials(void); // reset device credentials API key, Friendly ID,
                              // Wi-Fi SSID and password
static void checkAndPerformFirmwareUpdate(void);     // OTA update
static void goToSleep(void);                         // sleep preparing
static bool setClock(void);                          // clock synchronization
static float readBatteryVoltage(void);               // battery voltage reading
static void log_POST(char *log_buffer, size_t size); // log sending
static void checkLogNotes(void);
static void writeSpecialFunction(SPECIAL_FUNCTION function);
static void writeImageToFile(const char *name, uint8_t *in_buffer, size_t size);
static uint32_t getTime(void);
static void showMessageWithLogo(MSG message_type);
static void showMessageWithLogo(MSG message_type, String friendly_id, bool id,
                                const char *fw_version, String message);
static void wifiErrorDeepSleep();
static bool saveCurrentFileName(String &name);
static bool checkCurrentFileName(String &newName);
static DeviceStatusStamp getDeviceStatusStamp();
bool SerializeJsonLog(DeviceStatusStamp device_status_stamp, time_t timestamp,
                      int codeline, const char *source_file, char *log_message,
                      uint32_t log_id);
int submitLog(const char *format, time_t time, int line, const char *file, ...);

#define submit_log(format, ...)                                                \
  submitLog(format, getTime(), __LINE__, __FILE__, ##__VA_ARGS__);

static trmnl_bitmap *decodeBitmapFromFile(const char *filename,
                                          trmnl_error *error);
static trmnl_bitmap *decodeBitmapFromMemory(const uint8_t *data,
                                            uint32_t datasize,
                                            trmnl_error *error);
static trmnl_bitmap *getLogoBitmap();

void wait_for_serial() {
#ifdef WAIT_FOR_SERIAL
  for (int i = 10; i > 0 && !Serial; i--) {
    Log_info("## Waiting for serial.. %d", i);
    delay(1000);
  }
#endif
}

void double_buffer() {
  trmnl_error error;
  trmnl_bitmap *buffer1 =
      decodeBMPFromMemory(rover_bmp, sizeof(rover_bmp), &error);
  trmnl_bitmap *buffer2 =
      decodeBMPFromMemory(trmnl_logo_bmp, sizeof(trmnl_logo_bmp), &error);

  bool even  = true;
  while (true) {
    display_send_both(even ? buffer1 : buffer2, even ? buffer2 : buffer1);
    even = !even;
    delay(2000);
  }
}

void debug_fred() {
  printf("e-Paper Init and Clear...\r\n");
  display_init();
  delay(300);
  char buff[100] = {0};
 double_buffer();
  while (true) {
    printf("hello\n");
    trmnl_error error;
    debug_text((const uint8_t *)"Logo PNG\n");
    trmnl_bitmap *logo =
    decodePNGFromMemory(trmnl_logo_png, sizeof(trmnl_logo_png), &error);
    if (logo) {
      display_show_bitmap(logo);
      bitmap_delete(logo);
    } else {
      printf("fail\n");
      debug_text(
          (const uint8_t *)"Failure\n", error);
    }
    delay(5000);

    debug_text((const uint8_t *)"Rover\n");
    trmnl_bitmap *rover =
        decodeBMPFromMemory(rover_bmp, sizeof(rover_bmp), &error);
    if (rover) {
      display_show_bitmap(rover);
      bitmap_delete(rover);
    } else {
      debug_text(
          (const uint8_t *)"Failure\n", error);
    }
    delay(5000);
    debug_text((const uint8_t *)"Top Down Rover\n");
    trmnl_bitmap *frover = decodeBMPFromMemory(
        rover_topdown_bmp, sizeof(rover_topdown_bmp), &error);
    if (frover) {
      debug_text(
          (const uint8_t *)"Success\n");

      display_show_bitmap(frover);
      bitmap_delete(frover);
    } else {
      debug_text(
          (const uint8_t *)"Failure", error);
    }
    debug_heap();
    debug_text((const uint8_t*)"end of tests cleaning debug\n");
    debug_heap();
    debug_clean();
    delay(5000);
  }
  trmnl_bitmap *logo = getLogoBitmap();
  if (logo) {
    display_show_bitmap(logo);
    bitmap_delete(logo);
    delay(2000);
  }
  delay(40000);
  // SPIFFS.format();
  uint8_t buffer[128];
  delay(5000);
  Log.info("%s [%d]: Fred's firmware start\r\n", __FILE__, __LINE__);
  uint32_t pass = 1;

  while (true) {
    debug_heap();
    trmnl_error error;
    trmnl_bitmap *logo =
        decodeBMPFromMemory(trmnl_logo_bmp, sizeof(trmnl_logo_bmp), &error);
    if (logo) {
      debug_text((const uint8_t *)"logo.bmp from memory success\n");
      display_show_bitmap(logo);
      bitmap_delete(logo);
    } else {
      debug_text((const uint8_t *)"logo.bmp from memory failed\n", error);
    }
    delay(5000);

    trmnl_bitmap *pnglogo =
        decodePNGFromMemory(trmnl_logo_png, sizeof(trmnl_logo_png), &error);
    if (pnglogo) {
      debug_text((const uint8_t *)"PNG TRMNL logo read from memory success\n");
      display_show_bitmap(pnglogo);
      bitmap_delete(pnglogo);
    } else {
      debug_text((const uint8_t *)"PNG TRMNL logo read from memory failed \n", error);
    }

    delay(2000);
    debug_clean();
  }
  delay(10000);
  while (true) {
    sprintf((char *)buffer, "Firmware\nby Fred\n2025\n\nPass %d", pass);

    trmnl_bitmap *logo = getLogoBitmap();
    bitmap_delete(logo);
    delay(2000);
  }
}

/**
 * @brief Function to init business logic module
 * @param none
 * @return none
 */
void bl_init(void) {
  wait_for_serial();
  Serial.begin(115200);
  Log.begin(LOG_LEVEL_VERBOSE, &Serial);
  Log_info("BL init success");
  Log_info("Firmware version %d.%d.%d", FW_MAJOR_VERSION, FW_MINOR_VERSION,
           FW_PATCH_VERSION);
  pins_init();

  debug_fred();
  wakeup_reason = esp_sleep_get_wakeup_cause();

  Log.info("%s [%d]: preferences start\r\n", __FILE__, __LINE__);
  bool res = preferences.begin("data", false);
  if (res) {
    Log.info("%s [%d]: preferences init success\r\n", __FILE__, __LINE__);
    if (pref_clear) {
      res = preferences.clear(); // if needed to clear the saved data
      if (res)
        Log.info("%s [%d]: preferences cleared success\r\n", __FILE__,
                 __LINE__);
      else
        Log_fatal("preferences clearing error");
    }
  } else {
    Log.fatal("%s [%d]: preferences init failed\r\n", __FILE__, __LINE__);
    ESP.restart();
  }
  Log.info("%s [%d]: preferences end\r\n", __FILE__, __LINE__);

  if (wakeup_reason == ESP_SLEEP_WAKEUP_GPIO) {
    auto button = read_button_presses();
    wait_for_serial();
    Log_info("GPIO wakeup (%d) -> button was read (%s)", wakeup_reason,
             ButtonPressResultNames[button]);
    switch (button) {
    case LongPress:
      Log_info("WiFi reset");
      WifiCaptivePortal.resetSettings();
      break;
    case DoubleClick:
      double_click = true;
      break;
    case NoAction:
      break;
    }
    Log_info("button handling end");
  } else {
    wait_for_serial();
    Log_info("Non-GPIO wakeup (%d) -> didn't read buttons", wakeup_reason);
  }

  if (double_click) { // special function reading
    if (preferences.isKey(PREFERENCES_SF_KEY)) {
      Log.info("%s [%d]: SF saved. Reading...\r\n", __FILE__, __LINE__);
      special_function =
          (SPECIAL_FUNCTION)preferences.getUInt(PREFERENCES_SF_KEY, 0);
      Log.info("%s [%d]: Read special function - %d\r\n", __FILE__, __LINE__,
               special_function);
      switch (special_function) {
      case SF_IDENTIFY: {
        Log.info("%s [%d]: Identify special function...It will be handled "
                 "while API ping...\r\n",
                 __FILE__, __LINE__);
      } break;
      case SF_SLEEP: {
        Log.info("%s [%d]: Sleep special function...\r\n", __FILE__, __LINE__);
        // still in progress
      } break;
      case SF_ADD_WIFI: {
        Log.info("%s [%d]: Add WiFi function...\r\n", __FILE__, __LINE__);
        WifiCaptivePortal.startPortal();
      } break;
      case SF_RESTART_PLAYLIST: {
        Log.info("%s [%d]: Identify special function...It will be handled "
                 "while API ping...\r\n",
                 __FILE__, __LINE__);
      } break;
      case SF_REWIND: {
        Log.info("%s [%d]: Rewind special function...\r\n", __FILE__, __LINE__);
      } break;
      case SF_SEND_TO_ME: {
        Log.info("%s [%d]: Send to me special function...It will be handled "
                 "while API ping...\r\n",
                 __FILE__, __LINE__);
      } break;
      default:
        break;
      }
    } else {
      Log_error("SF not saved");
    }
  }
  // EPD init
  // EPD clear
  Log.info("%s [%d]: Display init\r\n", __FILE__, __LINE__);
  display_init();

  if (wakeup_reason != ESP_SLEEP_WAKEUP_TIMER) {
    Log.info("%s [%d]: Display TRMNL logo start\r\n", __FILE__, __LINE__);
    trmnl_bitmap *logo = getLogoBitmap();
    if (logo) {
      display_show_bitmap(logo);
      bitmap_delete(logo);
    }

    need_to_refresh_display = 1;
    preferences.putBool(PREFERENCES_DEVICE_REGISTERED_KEY, false);
    Log.info("%s [%d]: Display TRMNL logo end\r\n", __FILE__, __LINE__);
    preferences.putString(PREFERENCES_FILENAME_KEY, "");
  }

  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  if (WifiCaptivePortal.isSaved()) {
    // WiFi saved, connection
    Log.info("%s [%d]: WiFi saved\r\n", __FILE__, __LINE__);
    int connection_res = WifiCaptivePortal.autoConnect();

    Log.info("%s [%d]: Connection result: %d, WiFI Status: %d\r\n", __FILE__,
             __LINE__, connection_res, WiFi.status());

    // Check if connected
    if (connection_res) {
      String ip = String(WiFi.localIP());
      Log.info("%s [%d]:wifi_connection [DEBUG]: Connected: %s\r\n", __FILE__,
               __LINE__, ip.c_str());
      preferences.putInt(PREFERENCES_CONNECT_WIFI_RETRY_COUNT, 1);
    } else {
      Log.fatal("%s [%d]: Connection failed! WL Status: %d\r\n", __FILE__,
                __LINE__, WiFi.status());

      if (current_msg != WIFI_FAILED) {
        showMessageWithLogo(WIFI_FAILED);
        current_msg = WIFI_FAILED;
      }

      submit_log("wifi connection failed, current WL Status: %d",
                 WiFi.status());

      // Go to deep sleep
      wifiErrorDeepSleep();
    }
  } else {
    // WiFi credentials are not saved - start captive portal
    Log.info("%s [%d]: WiFi NOT saved\r\n", __FILE__, __LINE__);

    char fw_version[20];

    sprintf(fw_version, "%d.%d.%d", FW_MAJOR_VERSION, FW_MINOR_VERSION,
            FW_PATCH_VERSION);

    String fw = fw_version;

    Log.info("%s [%d]: FW version %s\r\n", __FILE__, __LINE__, fw_version);

    showMessageWithLogo(WIFI_CONNECT, "", false, fw.c_str(), "");
    WifiCaptivePortal.setResetSettingsCallback(resetDeviceCredentials);
    res = WifiCaptivePortal.startPortal();
    if (!res) {
      Log.error("%s [%d]: Failed to connect or hit timeout\r\n", __FILE__,
                __LINE__);

      WiFi.disconnect(true);

      showMessageWithLogo(WIFI_FAILED);

      submit_log("connection to the new WiFi failed");

      // Go to deep sleep
      wifiErrorDeepSleep();
    }
    Log.info("%s [%d]: WiFi connected\r\n", __FILE__, __LINE__);
    preferences.putInt(PREFERENCES_CONNECT_WIFI_RETRY_COUNT, 1);
  }

  // clock synchronization
  if (setClock()) {
    time_since_sleep = preferences.getUInt(PREFERENCES_LAST_SLEEP_TIME, 0);
    time_since_sleep = time_since_sleep
                           ? getTime() - time_since_sleep
                           : 0; // may be can be used even if no sync
  } else {
    time_since_sleep = 0;
    Log.info("%s [%d]: Time wasn't synced.\r\n", __FILE__, __LINE__);
  }

  Log.info("%s [%d]: Time since last sleep: %d\r\n", __FILE__, __LINE__,
           time_since_sleep);

  if (!preferences.isKey(PREFERENCES_API_KEY) ||
      !preferences.isKey(PREFERENCES_FRIENDLY_ID)) {
    Log.info("%s [%d]: API key or friendly ID not saved\r\n", __FILE__,
             __LINE__);
    // lets get the api key and friendly ID
    getDeviceCredentials();
  } else {
    Log.info("%s [%d]: API key and friendly ID saved\r\n", __FILE__, __LINE__);
  }

  log_retry = true;

  // OTA checking, image checking and drawing
  https_request_err_e request_result = downloadAndShow();
  Log.info("%s [%d]: request result - %d\r\n", __FILE__, __LINE__,
           request_result);

  if (!preferences.isKey(PREFERENCES_CONNECT_API_RETRY_COUNT)) {
    preferences.putInt(PREFERENCES_CONNECT_API_RETRY_COUNT, 1);
  }

  if (request_result != HTTPS_SUCCESS && request_result != HTTPS_NO_REGISTER &&
      request_result != HTTPS_RESET &&
      request_result != HTTPS_PLUGIN_NOT_ATTACHED) {
    uint8_t retries = preferences.getInt(PREFERENCES_CONNECT_API_RETRY_COUNT);

    switch (retries) {
    case 1:
      Log.info("%s [%d]: retry: %d - time to sleep: %d\r\n", __FILE__, __LINE__,
               retries, API_CONNECT_RETRY_TIME::API_FIRST_RETRY);
      res = preferences.putUInt(PREFERENCES_SLEEP_TIME_KEY,
                                API_CONNECT_RETRY_TIME::API_FIRST_RETRY);
      preferences.putInt(PREFERENCES_CONNECT_API_RETRY_COUNT, ++retries);
      display_sleep();
      goToSleep();
      break;

    case 2:
      Log.info("%s [%d]: retry:%d - time to sleep: %d\r\n", __FILE__, __LINE__,
               retries, API_CONNECT_RETRY_TIME::API_SECOND_RETRY);
      res = preferences.putUInt(PREFERENCES_SLEEP_TIME_KEY,
                                API_CONNECT_RETRY_TIME::API_SECOND_RETRY);
      preferences.putInt(PREFERENCES_CONNECT_API_RETRY_COUNT, ++retries);
      display_sleep();
      goToSleep();
      break;

    case 3:
      Log.info("%s [%d]: retry:%d - time to sleep: %d\r\n", __FILE__, __LINE__,
               retries, API_CONNECT_RETRY_TIME::API_THIRD_RETRY);
      res = preferences.putUInt(PREFERENCES_SLEEP_TIME_KEY,
                                API_CONNECT_RETRY_TIME::API_THIRD_RETRY);
      preferences.putInt(PREFERENCES_CONNECT_API_RETRY_COUNT, ++retries);
      display_sleep();
      goToSleep();
      break;

    default:
      Log.info("%s [%d]: Max retries done. Time to sleep: %d\r\n", __FILE__,
               __LINE__, SLEEP_TIME_TO_SLEEP);
      preferences.putUInt(PREFERENCES_SLEEP_TIME_KEY, SLEEP_TIME_TO_SLEEP);
      preferences.putInt(PREFERENCES_CONNECT_API_RETRY_COUNT, ++retries);
      break;
    }
  }

  else {
    Log.info(
        "%s [%d]: Connection done successfully. Retries counter reset.\r\n",
        __FILE__, __LINE__);
    preferences.putInt(PREFERENCES_CONNECT_API_RETRY_COUNT, 1);
  }

  if (request_result == HTTPS_NO_REGISTER && need_to_refresh_display == 1) {
    // show the image
    String friendly_id = preferences.getString(PREFERENCES_FRIENDLY_ID,
                                               PREFERENCES_FRIENDLY_ID_DEFAULT);
    showMessageWithLogo(FRIENDLY_ID, friendly_id, true, "",
                        String(message_buffer));
    need_to_refresh_display = 0;
  }

  // reset checking
  if (request_result == HTTPS_RESET) {
    Log.info("%s [%d]: Device reseting...\r\n", __FILE__, __LINE__);
    resetDeviceCredentials();
  }

  // OTA update checking
  if (update_firmware) {
    checkAndPerformFirmwareUpdate();
  }

  // error handling
  switch (request_result) {
  case HTTPS_REQUEST_FAILED: {
    if (WiFi.RSSI() > WIFI_CONNECTION_RSSI) {
      showMessageWithLogo(API_ERROR);
    } else {
      showMessageWithLogo(WIFI_WEAK);
    }
  } break;
  case HTTPS_RESPONSE_CODE_INVALID: {
    showMessageWithLogo(WIFI_INTERNAL_ERROR);
  } break;
  case HTTPS_UNABLE_TO_CONNECT: {
    if (WiFi.RSSI() > WIFI_CONNECTION_RSSI) {
      showMessageWithLogo(API_ERROR);
    } else {
      showMessageWithLogo(WIFI_WEAK);
    }
  } break;
  case HTTPS_WRONG_IMAGE_FORMAT: {
    showMessageWithLogo(BMP_FORMAT_ERROR);
  } break;
  case HTTPS_WRONG_IMAGE_SIZE: {
    if (WiFi.RSSI() > WIFI_CONNECTION_RSSI) {
      showMessageWithLogo(API_SIZE_ERROR);
    } else {
      showMessageWithLogo(WIFI_WEAK);
    }
  } break;
  case HTTPS_CLIENT_FAILED: {
    showMessageWithLogo(WIFI_INTERNAL_ERROR);
  } break;
  case HTTPS_PLUGIN_NOT_ATTACHED: {
    if (preferences.getInt(PREFERENCES_SLEEP_TIME_KEY, 0) !=
        SLEEP_TIME_WHILE_PLUGIN_NOT_ATTACHED) {
      Log.info("%s [%d]: write new refresh rate: %d\r\n", __FILE__, __LINE__,
               SLEEP_TIME_WHILE_PLUGIN_NOT_ATTACHED);
      size_t result = preferences.putUInt(PREFERENCES_SLEEP_TIME_KEY,
                                          SLEEP_TIME_WHILE_PLUGIN_NOT_ATTACHED);
      Log.info("%s [%d]: written new refresh rate: %d\r\n", __FILE__, __LINE__,
               SLEEP_TIME_WHILE_PLUGIN_NOT_ATTACHED);
    }
  } break;
  default:
    break;
  }

  if (request_result != HTTPS_NO_ERR &&
      request_result != HTTPS_PLUGIN_NOT_ATTACHED) {
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
void bl_process(void) {}

ApiDisplayInputs loadApiDisplayInputs(Preferences &preferences) {
  ApiDisplayInputs inputs;

  inputs.baseUrl = preferences.getString(PREFERENCES_API_URL, API_BASE_URL);

  if (preferences.isKey(PREFERENCES_API_KEY)) {
    inputs.apiKey =
        preferences.getString(PREFERENCES_API_KEY, PREFERENCES_API_KEY_DEFAULT);
    Log.info("%s [%d]: %s key exists. Value - %s\r\n", __FILE__, __LINE__,
             PREFERENCES_API_KEY, inputs.apiKey.c_str());
  } else {
    Log.error("%s [%d]: %s key not exists.\r\n", __FILE__, __LINE__,
              PREFERENCES_API_KEY);
  }

  if (preferences.isKey(PREFERENCES_FRIENDLY_ID)) {
    inputs.friendlyId = preferences.getString(PREFERENCES_FRIENDLY_ID,
                                              PREFERENCES_FRIENDLY_ID_DEFAULT);
    Log.info("%s [%d]: %s key exists. Value - %s\r\n", __FILE__, __LINE__,
             PREFERENCES_FRIENDLY_ID, inputs.friendlyId);
  } else {
    Log.error("%s [%d]: %s key not exists.\r\n", __FILE__, __LINE__,
              PREFERENCES_FRIENDLY_ID);
  }

  inputs.refreshRate = SLEEP_TIME_TO_SLEEP;

  if (preferences.isKey(PREFERENCES_SLEEP_TIME_KEY)) {
    inputs.refreshRate =
        preferences.getUInt(PREFERENCES_SLEEP_TIME_KEY, SLEEP_TIME_TO_SLEEP);
    Log.info("%s [%d]: %s key exists. Value - %d\r\n", __FILE__, __LINE__,
             PREFERENCES_SLEEP_TIME_KEY, inputs.refreshRate);
  } else {
    Log.error("%s [%d]: %s key not exists.\r\n", __FILE__, __LINE__,
              PREFERENCES_SLEEP_TIME_KEY);
  }

  inputs.macAddress = WiFi.macAddress();

  inputs.batteryVoltage = readBatteryVoltage();

  inputs.firmwareVersion = String(FW_MAJOR_VERSION) + "." +
                           String(FW_MINOR_VERSION) + "." +
                           String(FW_PATCH_VERSION);

  inputs.rssi = WiFi.RSSI();
  inputs.displayWidth = display_width();
  inputs.displayHeight = display_height();
  inputs.specialFunction = special_function;

  return inputs;
}

/**
 * @brief Function to ping server and download and show the image if all is OK
 * @param url Server URL address
 * @return https_request_err_e error code
 */
static https_request_err_e downloadAndShow() {
  auto apiDisplayInputs = loadApiDisplayInputs(preferences);

  auto apiDisplayResult = fetchApiDisplay(apiDisplayInputs);

  if (apiDisplayResult.error != HTTPS_NO_ERR) {
    Log.error("%s [%d]: Error fetching API display: %d, detail: %s\r\n",
              __FILE__, __LINE__, apiDisplayResult.error,
              apiDisplayResult.error_detail.c_str());
    submit_log("Error fetching API display: %d, detail: %s",
               apiDisplayResult.error, apiDisplayResult.error_detail.c_str());
    return apiDisplayResult.error;
  }

  handleApiDisplayResponse(apiDisplayResult.response);

  https_request_err_e result = HTTPS_NO_ERR;
  WiFiClientSecure *secureClient = new WiFiClientSecure;
  secureClient->setInsecure();
  WiFiClient *insecureClient = new WiFiClient;

  bool isHttps = true;
  if (apiDisplayInputs.baseUrl.indexOf("https://") == -1) {
    isHttps = false;
  }

  // define client depending on the isHttps variable
  WiFiClient *client = isHttps ? secureClient : insecureClient;

  if (!client) {
    Log.error("%s [%d]: Unable to create client\r\n", __FILE__, __LINE__);

    return HTTPS_UNABLE_TO_CONNECT;
  }

  { // Add a scoping block for HTTPClient https to make sure it is destroyed
    // before WiFiClientSecure *client is

    HTTPClient https;
    if (status && !update_firmware && !reset_firmware) {
      status = false;

      // The timeout will be zero if no value was returned, and in that case we
      // just use the default timeout. Otherwise, we set the requested timeout.
      uint32_t requestedTimeout = apiDisplayResult.response.image_url_timeout;
      if (requestedTimeout > 0) {
        // Convert from seconds to milliseconds.
        // A uint32_t should be large enough not to worry about overflow for any
        // reasonable timeout.
        requestedTimeout *= MS_TO_S_FACTOR;
        if (requestedTimeout > UINT16_MAX) {
          // To avoid surprising behaviour if the server returned a timeout of
          // more than 65 seconds we will send a log message back to the server
          // and truncate the timeout to the maximum.
          submit_log("Requested image URL timeout too large (%d ms). Using "
                     "maximum of %d ms.",
                     requestedTimeout, UINT16_MAX);
          https.setTimeout(UINT16_MAX);
        } else {
          https.setTimeout(uint16_t(requestedTimeout));
        }
      }

      Log.info("%s [%d]: [HTTPS] Request to %s\r\n", __FILE__, __LINE__,
               filename);
      client = strstr(filename, "https://") == nullptr ? insecureClient
                                                       : secureClient;
      if (!https.begin(*client, filename)) // HTTPS
      {
        Log.error("%s [%d]: unable to connect\r\n", __FILE__, __LINE__);

        submit_log("unable to connect to the API");

        return HTTPS_UNABLE_TO_CONNECT;
      }
      const char *headers[] = {"Content-Type"};
      https.collectHeaders(headers, 1);
      Log.info("%s [%d]: [HTTPS] GET..\r\n", __FILE__, __LINE__);
      Log.info("%s [%d]: RSSI: %d\r\n", __FILE__, __LINE__, WiFi.RSSI());
      // start connection and send HTTP header
      int httpCode = https.GET();
      int content_size = https.getSize();

      // httpCode will be negative on error
      if (httpCode < 0) {
        Log.error("%s [%d]: [HTTPS] GET... failed, error: %d (%s)\r\n",
                  __FILE__, __LINE__, httpCode,
                  https.errorToString(httpCode).c_str());

        submit_log("HTTP Client failed with error: %s",
                   https.errorToString(httpCode).c_str());

        return HTTPS_REQUEST_FAILED;
      }

      // HTTP header has been send and Server response header has been handled
      Log.error("%s [%d]: [HTTPS] GET... code: %d\r\n", __FILE__, __LINE__,
                httpCode);
      Log.info("%s [%d]: RSSI: %d\r\n", __FILE__, __LINE__, WiFi.RSSI());
      // file found at server
      if (httpCode != HTTP_CODE_OK && httpCode != HTTP_CODE_MOVED_PERMANENTLY) {
        Log.error("%s [%d]: [HTTPS] GET... failed, code: %d (%s)\r\n", __FILE__,
                  __LINE__, httpCode, https.errorToString(httpCode).c_str());

        submit_log("HTTPS returned code is not OK. Code: %d", httpCode);
        return HTTPS_REQUEST_FAILED;
      }
      Log.info("%s [%d]: Content size: %d\r\n", __FILE__, __LINE__,
               https.getSize());

      uint32_t counter = 0;
      if (content_size > MAX_PAYLOAD_SIZE) {
        Log.error(
            "%s [%d]: %d bytes payload response exceeds the %d bytes limit\r\n",
            __FILE__, __LINE__, content_size, MAX_PAYLOAD_SIZE);
        return HTTPS_REQUEST_FAILED;
      }

      WiFiClient *stream = https.getStreamPtr();
      Log.info("%s [%d]: RSSI: %d\r\n", __FILE__, __LINE__, WiFi.RSSI());
      Log.info("%s [%d]: Stream timeout: %d\r\n", __FILE__, __LINE__,
               stream->getTimeout());

      Log.info("%s [%d]: Stream available: %d\r\n", __FILE__, __LINE__,
               stream->available());

      uint32_t timer = millis();
      while (stream->available() < 4000 && millis() - timer < 1000)
        ;

      Log.info("%s [%d]: Stream available: %d\r\n", __FILE__, __LINE__,
               stream->available());

      bool isPNG = https.header("Content-Type") == "image/png";
      int iteration_counter = 0;

      unsigned long download_start = millis();

      Log.info("%s [%d]: Starting a download at: %d\r\n", __FILE__, __LINE__,
               getTime());
      uint8_t *buffer = (uint8_t *)malloc(content_size);
      int counter2 = content_size;
      while (counter != content_size && millis() - download_start < 10000) {
        if (stream->available()) {
          Log.info("%s [%d]: Downloading... Available bytes: %d\r\n", __FILE__,
                   __LINE__, stream->available());
          counter += stream->readBytes(buffer + counter, counter2 -= counter);

          iteration_counter++;
        }

        delay(10);
      }
      Log.info("%s [%d]: Ending a download at: %d, in %d iterations\r\n",
               __FILE__, __LINE__, getTime(), iteration_counter);
      if (counter != content_size) {

        Log.error("%s [%d]: Receiving failed. Read: %d\r\n", __FILE__, __LINE__,
                  counter);

        // display_show_msg(const_cast<uint8_t *>(default_icon),
        // API_SIZE_ERROR);
        submit_log("HTTPS request error. Returned code - %d, available bytes - "
                   "%d, received bytes - %d in %d iterations",
                   httpCode, https.getSize(), counter, iteration_counter);

        return HTTPS_WRONG_IMAGE_SIZE;
      }

      Log.info("%s [%d]: Received successfully\r\n", __FILE__, __LINE__);

      // we can save the file to SPIFFS and free the buffer if we want to save
      // memory
      writeImageToFile("/current.raw", buffer, content_size);
      trmnl_error error;
#if SAVE_MEMORY
      free(buffer);
      trmnl_bitmap *bitmap = decodeBitmapFromFile("/current.raw", &error);
#else
      trmnl_bitmap *bitmap =
          decodeBitmapFromMemory(buffer, content_size, &error);
      free(buffer);
#endif
      // or try to display it as fast as possible
      // we should have now a trml_bitmap object compatible with the display
      if (bitmap && error == NO_ERROR && true) { // IsBitmapDisplayCompatible) {
#if false && defined(FRED_DEBUG)
      // save it as a BMP to check if it works
      encodeBMP(bitmap, "/current_decoded.bmp");
      // read it back to check if it is ok
      trmnl_error = decodeBMP("/current_decoded.bmp", &decoded);
      if (trmnl_error == NO_ERROR) {
        display_show_bitmap(decoded);
        delay(10000);
        bitmap_delete(decoded);
      }
#endif
        // show it on the display
        display_show_bitmap(bitmap);
        delay(100000);

        //    display_show_trml_file("/current.raw");

        if (send_log) {
          send_log = false;
        }

        Log.info("%s [%d]: Returned result - %d\r\n", __FILE__, __LINE__,
                 result);
        return result;
      }
    }
  }
}

https_request_err_e handleApiDisplayResponse(ApiDisplayResponse &apiResponse) {
  https_request_err_e result = HTTPS_NO_ERR;

  if (special_function == SF_NONE) {
    uint64_t request_status = apiResponse.status;
    Log.info("%s [%d]: status: %d\r\n", __FILE__, __LINE__, request_status);
    switch (request_status) {
    case 0: {
      String image_url = apiResponse.image_url;
      update_firmware = apiResponse.update_firmware;
      String firmware_url = apiResponse.firmware_url;
      uint64_t rate = apiResponse.refresh_rate;
      reset_firmware = apiResponse.reset_firmware;

      bool sleep_5_seconds = false;

      writeSpecialFunction(apiResponse.special_function);

      if (update_firmware) {
        Log.info("%s [%d]: update firmware. Check URL\r\n", __FILE__, __LINE__);
        if (firmware_url.length() == 0) {
          Log.error("%s [%d]: Empty URL\r\n", __FILE__, __LINE__);
          update_firmware = false;
        }
      }
      if (image_url.length() > 0) {
        Log.info("%s [%d]: image_url: %s\r\n", __FILE__, __LINE__,
                 image_url.c_str());
        Log.info("%s [%d]: image url end with: %d\r\n", __FILE__, __LINE__,
                 image_url.endsWith("/setup-logo.bmp"));

        image_url.toCharArray(filename, image_url.length() + 1);
        // check if plugin is applied
        bool flag =
            preferences.getBool(PREFERENCES_DEVICE_REGISTERED_KEY, false);
        Log.info("%s [%d]: flag: %d\r\n", __FILE__, __LINE__, flag);

        if (apiResponse.filename == "empty_state") {
          Log.info("%s [%d]: End with empty_state\r\n", __FILE__, __LINE__);
          if (!flag) {
            // draw received logo
            status = true;
            // set flag to true
            if (preferences.getBool(PREFERENCES_DEVICE_REGISTERED_KEY,
                                    false) !=
                true) // check the flag to avoid the re-writing
            {
              bool res =
                  preferences.putBool(PREFERENCES_DEVICE_REGISTERED_KEY, true);
              if (res)
                Log.info("%s [%d]: Flag written true successfully\r\n",
                         __FILE__, __LINE__);
              else
                Log.error("%s [%d]: FLag writing failed\r\n", __FILE__,
                          __LINE__);
            }
          } else {
            // don't draw received logo
            status = false;
          }
          // sleep 5 seconds
          sleep_5_seconds = true;
        } else {
          Log.info("%s [%d]: End with NO empty_state\r\n", __FILE__, __LINE__);
          if (flag) {
            if (preferences.getBool(PREFERENCES_DEVICE_REGISTERED_KEY,
                                    false) !=
                false) // check the flag to avoid the re-writing
            {
              bool res =
                  preferences.putBool(PREFERENCES_DEVICE_REGISTERED_KEY, false);
              if (res)
                Log.info("%s [%d]: Flag written false successfully\r\n",
                         __FILE__, __LINE__);
              else
                Log.error("%s [%d]: Flag writing failed\r\n", __FILE__,
                          __LINE__);
            }
          }
          // Using filename from API response
          new_filename = apiResponse.filename;

          // Print the extracted st(new_filename)) {
          Log.info("%s [%d]: New image. Show it.\r\n", __FILE__, __LINE__);
          status = true;
        }
      } else {
        Log.info("%s [%d]: Old image. No needed to show it.\r\n", __FILE__,
                 __LINE__);
        status = false;
        result = HTTPS_SUCCESS;
      }
      Log.info("%s [%d]: update_firmware: %d\r\n", __FILE__, __LINE__,
               update_firmware);
      if (firmware_url.length() > 0) {
        Log.info("%s [%d]: firmware_url: %s\r\n", __FILE__, __LINE__,
                 firmware_url.c_str());
        firmware_url.toCharArray(binUrl, firmware_url.length() + 1);
      }
      Log.info("%s [%d]: refresh_rate: %d\r\n", __FILE__, __LINE__, rate);
      if (rate != preferences.getUInt(PREFERENCES_SLEEP_TIME_KEY,
                                      SLEEP_TIME_TO_SLEEP)) {
        Log.info("%s [%d]: write new refresh rate: %d\r\n", __FILE__, __LINE__,
                 rate);
        size_t result = preferences.putUInt(PREFERENCES_SLEEP_TIME_KEY, rate);
        Log.info("%s [%d]: written new refresh rate: %d\r\n", __FILE__,
                 __LINE__, result);
      }

      if (reset_firmware) {
        Log.info("%s [%d]: Reset status is true\r\n", __FILE__, __LINE__);
      }

      if (update_firmware)
        result = HTTPS_SUCCESS;
      if (reset_firmware)
        result = HTTPS_RESET;
      if (sleep_5_seconds)
        result = HTTPS_PLUGIN_NOT_ATTACHED;
      Log.info("%s [%d]: result - %d\r\n", __FILE__, __LINE__, result);
    } break;
    case 202: {
      result = HTTPS_NO_REGISTER;
      Log.info("%s [%d]: write new refresh rate: %d\r\n", __FILE__, __LINE__,
               SLEEP_TIME_WHILE_NOT_CONNECTED);
      size_t result = preferences.putUInt(PREFERENCES_SLEEP_TIME_KEY,
                                          SLEEP_TIME_WHILE_NOT_CONNECTED);
      Log.info("%s [%d]: written new refresh rate: %d\r\n", __FILE__, __LINE__,
               result);
      status = false;
    } break;
    case 500: {
      result = HTTPS_RESET;
      Log.info("%s [%d]: write new refresh rate: %d\r\n", __FILE__, __LINE__,
               SLEEP_TIME_WHILE_NOT_CONNECTED);
      size_t result = preferences.putUInt(PREFERENCES_SLEEP_TIME_KEY,
                                          SLEEP_TIME_WHILE_NOT_CONNECTED);
      Log.info("%s [%d]: written new refresh rate: %d\r\n", __FILE__, __LINE__,
               result);
      status = false;
    } break;

    default:
      break;
    }
  } else if (special_function != SF_NONE) {
    uint64_t request_status = apiResponse.status;
    Log.info("%s [%d]: status: %d\r\n", __FILE__, __LINE__, request_status);
    switch (request_status) {
    case 0: {
      switch (special_function) {
      case SF_IDENTIFY: {
        String action = apiResponse.action;
        if (action.equals("identify")) {
          Log.info("%s [%d]:Identify success\r\n", __FILE__, __LINE__);
          String image_url = apiResponse.image_url;
          if (image_url.length() > 0) {
            Log.info("%s [%d]: image_url: %s\r\n", __FILE__, __LINE__,
                     image_url.c_str());
            Log.info("%s [%d]: image url end with: %d\r\n", __FILE__, __LINE__,
                     image_url.endsWith("/setup-logo.bmp"));

            image_url.toCharArray(filename, image_url.length() + 1);
            // check if plugin is applied
            bool flag =
                preferences.getBool(PREFERENCES_DEVICE_REGISTERED_KEY, false);
            Log.info("%s [%d]: flag: %d\r\n", __FILE__, __LINE__, flag);

            if (apiResponse.filename == "empty_state") {
              Log.info("%s [%d]: End with empty_state\r\n", __FILE__, __LINE__);
              if (!flag) {
                // draw received logo
                status = true;
                // set flag to true
                if (preferences.getBool(PREFERENCES_DEVICE_REGISTERED_KEY,
                                        false) !=
                    true) // check the flag to avoid the re-writing
                {
                  bool res = preferences.putBool(
                      PREFERENCES_DEVICE_REGISTERED_KEY, true);
                  if (res)
                    Log.info("%s [%d]: Flag written true successfully\r\n",
                             __FILE__, __LINE__);
                  else
                    Log.error("%s [%d]: FLag writing failed\r\n", __FILE__,
                              __LINE__);
                }
              } else {
                // don't draw received logo
                status = false;
              }
            } else {
              Log.info("%s [%d]: End with NO empty_state\r\n", __FILE__,
                       __LINE__);
              if (flag) {
                if (preferences.getBool(PREFERENCES_DEVICE_REGISTERED_KEY,
                                        false) !=
                    false) // check the flag to avoid the re-writing
                {
                  bool res = preferences.putBool(
                      PREFERENCES_DEVICE_REGISTERED_KEY, false);
                  if (res)
                    Log.info("%s [%d]: Flag written false successfully\r\n",
                             __FILE__, __LINE__);
                  else
                    Log.error("%s [%d]: FLag writing failed\r\n", __FILE__,
                              __LINE__);
                }
              }
              status = true;
            }
          }
        } else {
          Log.error("%s [%d]: identify failed\r\n", __FILE__, __LINE__);
        }
      } break;
      case SF_SLEEP: {
        String action = apiResponse.action;
        if (action.equals("sleep")) {
          uint64_t rate = apiResponse.refresh_rate;
          Log.info("%s [%d]: refresh_rate: %d\r\n", __FILE__, __LINE__, rate);
          if (rate != preferences.getUInt(PREFERENCES_SLEEP_TIME_KEY,
                                          SLEEP_TIME_TO_SLEEP)) {
            Log.info("%s [%d]: write new refresh rate: %d\r\n", __FILE__,
                     __LINE__, rate);
            size_t result =
                preferences.putUInt(PREFERENCES_SLEEP_TIME_KEY, rate);
            Log.info("%s [%d]: written new refresh rate: %d\r\n", __FILE__,
                     __LINE__, result);
          }
          status = false;
          result = HTTPS_SUCCESS;
          Log.info("%s [%d]: sleep success\r\n", __FILE__, __LINE__);
        } else {
          Log.error("%s [%d]: sleep failed\r\n", __FILE__, __LINE__);
          // need to add error
        }
      } break;
      case SF_ADD_WIFI: {
        String action = apiResponse.action;
        if (action.equals("add_wifi")) {
          status = false;
          result = HTTPS_SUCCESS;
          Log.info("%s [%d]: Add wifi success\r\n", __FILE__, __LINE__);
        } else {
          Log.error("%s [%d]: Add wifi failed\r\n", __FILE__, __LINE__);
        }
      } break;
      case SF_RESTART_PLAYLIST: {
        String action = apiResponse.action;
        if (action.equals("restart_playlist")) {
          Log.info("%s [%d]:Restart playlist success\r\n", __FILE__, __LINE__);
          String image_url = apiResponse.image_url;
          if (image_url.length() > 0) {
            Log.info("%s [%d]: image_url: %s\r\n", __FILE__, __LINE__,
                     image_url.c_str());
            Log.info("%s [%d]: image url end with: %d\r\n", __FILE__, __LINE__,
                     image_url.endsWith("/setup-logo.bmp"));

            image_url.toCharArray(filename, image_url.length() + 1);
            // check if plugin is applied
            bool flag =
                preferences.getBool(PREFERENCES_DEVICE_REGISTERED_KEY, false);
            Log.info("%s [%d]: flag: %d\r\n", __FILE__, __LINE__, flag);

            if (apiResponse.filename == "empty_state") {
              Log.info("%s [%d]: End with empty_state\r\n", __FILE__, __LINE__);
              if (!flag) {
                // draw received logo
                status = true;
                // set flag to true
                if (preferences.getBool(PREFERENCES_DEVICE_REGISTERED_KEY,
                                        false) !=
                    true) // check the flag to avoid the re-writing
                {
                  bool res = preferences.putBool(
                      PREFERENCES_DEVICE_REGISTERED_KEY, true);
                  if (res)
                    Log.info("%s [%d]: Flag written true successfully\r\n",
                             __FILE__, __LINE__);
                  else
                    Log.error("%s [%d]: FLag writing failed\r\n", __FILE__,
                              __LINE__);
                }
              } else {
                // don't draw received logo
                status = false;
              }
            } else {
              Log.info("%s [%d]: End with NO empty_state\r\n", __FILE__,
                       __LINE__);
              if (flag) {
                if (preferences.getBool(PREFERENCES_DEVICE_REGISTERED_KEY,
                                        false) !=
                    false) // check the flag to avoid the re-writing
                {
                  bool res = preferences.putBool(
                      PREFERENCES_DEVICE_REGISTERED_KEY, false);
                  if (res)
                    Log.info("%s [%d]: Flag written false successfully\r\n",
                             __FILE__, __LINE__);
                  else
                    Log.error("%s [%d]: FLag writing failed\r\n", __FILE__,
                              __LINE__);
                }
              }
              status = true;
            }
          }
        } else {
          Log.error("%s [%d]: identify failed\r\n", __FILE__, __LINE__);
        }
      } break;
      case SF_REWIND: {
        String action = apiResponse.action;
        if (action.equals("rewind")) {
          bool isPNG = false;
          status = false;
          result = HTTPS_SUCCESS;
          Log.info("%s [%d]: rewind success\r\n", __FILE__, __LINE__);

          Log.info("%s [%d]: New filename - %s\r\n", __FILE__, __LINE__,
                   new_filename.c_str());
          trmnl_error error;
          trmnl_bitmap *lastbitmap = decodeBitmapFromFile("/last.raw", &error);
          if (lastbitmap && error == NO_ERROR) {
            display_show_bitmap(lastbitmap);
            bitmap_delete(lastbitmap);
            need_to_refresh_display = 1;
          }
        } else {
          Log.error("%s [%d]: rewind failed\r\n", __FILE__, __LINE__);
        }
      } break;
      case SF_SEND_TO_ME: {
        String action = apiResponse.action;
        if (action.equals("send_to_me")) {

          result = HTTPS_SUCCESS;
          Log.info("%s [%d]: send_to_me success\r\n", __FILE__, __LINE__);

          trmnl_error error;
          trmnl_bitmap *currentBitmap =
              decodeBitmapFromFile("/current.raw", &error);
          if (currentBitmap == nullptr) {
            return HTTPS_WRONG_IMAGE_FORMAT;
          }
          display_show_bitmap(currentBitmap);
          bitmap_delete(currentBitmap);
          need_to_refresh_display = 1;
        } else {
          Log.error("%s [%d]: send_to_me failed\r\n", __FILE__, __LINE__);
        }
      } break;
      default:
        break;
      }
    } break;
    default:
      break;
    }
  }
  return result;
}

/**
 * @brief Function to getting the friendly id and API key
 * @return none
 */
// static void getDeviceCredentials() {}

static void getDeviceCredentials() {
  WiFiClientSecure *secureClient = new WiFiClientSecure;
  WiFiClient *insecureClient = new WiFiClient;

  secureClient->setInsecure();

  bool isHttps = true;
  if (preferences.getString(PREFERENCES_API_URL, API_BASE_URL)
          .indexOf("https://") == -1) {
    isHttps = false;
  }

  // define client depending on the isHttps variable
  WiFiClient *client = isHttps ? secureClient : insecureClient;

  if (client) {
    {
      // Add a scoping block for HTTPClient https to make sure it is
      // destroyed before WiFiClientSecure *client is
      HTTPClient https;

      Log.info("%s [%d]: [HTTPS] begin /api/setup/ ...\r\n", __FILE__,
               __LINE__);
      char new_url[200];
      strcpy(new_url,
             preferences.getString(PREFERENCES_API_URL, API_BASE_URL).c_str());
      strcat(new_url, "/api/setup/");

      char fw_version[30];
      sprintf(fw_version, "%d.%d.%d", FW_MAJOR_VERSION, FW_MINOR_VERSION,
              FW_PATCH_VERSION);

      if (https.begin(*client, new_url)) { // HTTPS
        Log.info("%s [%d]: RSSI: %d\r\n", __FILE__, __LINE__, WiFi.RSSI());
        Log.info("%s [%d]: [HTTPS] GET...\r\n", __FILE__, __LINE__);
        // start connection and send HTTP header

        https.addHeader("ID", WiFi.macAddress());
        https.addHeader("FW-Version", fw_version);
        Log.info("%s [%d]: Device MAC address: %s\r\n", __FILE__, __LINE__,
                 WiFi.macAddress().c_str());

        int httpCode = https.GET();

        // httpCode will be negative on error
        if (httpCode > 0) {
          // HTTP header has been send and Server response header has been
          // handled
          Log.info("%s [%d]: GET... code: %d\r\n", __FILE__, __LINE__,
                   httpCode);
          // file found at server
          Log.info("%s [%d]: RSSI: %d\r\n", __FILE__, __LINE__, WiFi.RSSI());
          if (httpCode == HTTP_CODE_OK) {
            Log.info("%s [%d]: Content size: %d\r\n", __FILE__, __LINE__,
                     https.getSize());
            String payload = https.getString();
            Log.info("%s [%d]: Payload: %s\r\n", __FILE__, __LINE__,
                     payload.c_str());

            auto apiResponse = parseResponse_apiSetup(payload);

            if (apiResponse.outcome == ApiSetupOutcome::DeserializationError) {
              Log.error("%s [%d]: JSON deserialization error.\r\n", __FILE__,
                        __LINE__);
              https.end();
              client->stop();
              return;
            }
            uint16_t url_status = apiResponse.status;
            if (url_status == 200) {
              status = true;
              Log.info("%s [%d]: status OK.\r\n", __FILE__, __LINE__);

              String api_key = apiResponse.api_key;
              Log.info("%s [%d]: API key - %s\r\n", __FILE__, __LINE__,
                       api_key.c_str());
              size_t res = preferences.putString(PREFERENCES_API_KEY, api_key);
              Log.info("%s [%d]: api key saved in the preferences - %d\r\n",
                       __FILE__, __LINE__, res);

              String friendly_id = apiResponse.friendly_id;
              Log.info("%s [%d]: friendly ID - %s\r\n", __FILE__, __LINE__,
                       friendly_id.c_str());
              res = preferences.putString(PREFERENCES_FRIENDLY_ID, friendly_id);
              Log.info("%s [%d]: friendly ID saved in the preferences - %d\r\n",
                       __FILE__, __LINE__, res);

              String image_url = apiResponse.image_url;
              Log.info("%s [%d]: image_url - %s\r\n", __FILE__, __LINE__,
                       image_url.c_str());
              image_url.toCharArray(filename, image_url.length() + 1);

              String message_str = apiResponse.message;
              Log.info("%s [%d]: message - %s\r\n", __FILE__, __LINE__,
                       message_str.c_str());
              message_str.toCharArray(message_buffer, message_str.length() + 1);

              Log.info("%s [%d]: status - %d\r\n", __FILE__, __LINE__, status);
            } else if (url_status == 404) {
              Log.info("%s [%d]: MAC Address is not registered on server\r\n",
                       __FILE__, __LINE__);
              showMessageWithLogo(MAC_NOT_REGISTERED);

              preferences.putUInt(PREFERENCES_SLEEP_TIME_KEY,
                                  SLEEP_TIME_TO_SLEEP);

              display_sleep();
              goToSleep();
            } else {
              Log.info("%s [%d]: status FAIL.\r\n", __FILE__, __LINE__);
              status = false;
            }
          } else {
            Log.info("%s [%d]: [HTTPS] Unable to connect\r\n", __FILE__,
                     __LINE__);

            if (WiFi.RSSI() > WIFI_CONNECTION_RSSI) {
              showMessageWithLogo(API_ERROR);
            } else {
              showMessageWithLogo(WIFI_WEAK);
            }
            submit_log("returned code is not OK. Code - %d", httpCode);
          }
        } else {
          Log.error("%s [%d]: [HTTPS] GET... failed, error: %s\r\n", __FILE__,
                    __LINE__, https.errorToString(httpCode).c_str());
          if (WiFi.RSSI() > WIFI_CONNECTION_RSSI) {
            showMessageWithLogo(API_ERROR);
          } else {
            showMessageWithLogo(WIFI_WEAK);
          }
          submit_log("HTTP Client failed with error: %s",
                     https.errorToString(httpCode).c_str());
        }

        https.end();
      } else {
        Log.error("%s [%d]: [HTTPS] Unable to connect\r\n", __FILE__, __LINE__);
        showMessageWithLogo(WIFI_INTERNAL_ERROR);
        submit_log("unable to connect to the API");
      }
      Log.info("%s [%d]: status - %d\r\n", __FILE__, __LINE__, status);
      if (status) {
        status = false;
        Log.info("%s [%d]: filename - %s\r\n", __FILE__, __LINE__, filename);

        Log.info("%s [%d]: [HTTPS] Request to %s\r\n", __FILE__, __LINE__,
                 filename);
        if (https.begin(*client, filename)) { // HTTPS
          Log.info("%s [%d]: [HTTPS] GET..\r\n", __FILE__, __LINE__);
          // start connection and send HTTP header
          int httpCode = https.GET();

          // httpCode will be negative on error
          if (httpCode > 0) {
            // HTTP header has been send and Server response header has been
            // handled
            Log.error("%s [%d]: [HTTPS] GET... code: %d\r\n", __FILE__,
                      __LINE__, httpCode);
            // file found at server
            if (httpCode == HTTP_CODE_OK ||
                httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
              uint32_t rsize = https.getSize();
              Log.info("%s [%d]: Content size: %d\r\n", __FILE__, __LINE__,
                       rsize);
              WiFiClient *stream = https.getStreamPtr();
              if (rsize < MAX_PAYLOAD_SIZE) {
                const uint32_t download_cacheSize = 4096;
                const uint32_t timeout = 10000; // ms
                static uint8_t downloadCache[49606];
                uint32_t pass_read_bytes = 0;
                uint32_t total_read_bytes = 0;
                uint32_t total_written_bytes = 0;
                uint32_t pass = 1;
                ulong start = millis();
                // save to file directly
                // TODO: a dowwnload fuunction
                filesystem_open_write_file("/logo.raw");
                while ((stream->available() && (total_read_bytes != rsize) &&
                        ((millis() - start) < timeout))) {
                  pass_read_bytes =
                      stream->readBytes(downloadCache, download_cacheSize);
                  uint32_t pass_written_bytes = pass_read_bytes;
                  filesystem_write_to_file(downloadCache, &pass_written_bytes);
                  // TOD; check write result
                  total_read_bytes += pass_read_bytes;
                  total_written_bytes += pass_written_bytes;
                  ++pass;
                  delay(10);
                };
                https.end();
                filesystem_close_write_file();
                Log.info("%s [%d]: Downloaded %d bytes in % passes\r\n",
                         __FILE__, __LINE__, total_read_bytes, pass);
                Log.info("%s [%d]: Wrote %d bytes to SPIFFS\r\n", __FILE__,
                         __LINE__),
                    total_written_bytes;
                if (total_read_bytes == total_written_bytes) {
                  Log.info("%s [%d]: Received successfully\r\n", __FILE__,
                           __LINE__);
                  // and show the bitmap
                  trmnl_error error;
                  trmnl_bitmap *bitmap =
                      decodeBitmapFromFile("/logo.raw", &error);
                  String friendly_id = preferences.getString(
                      PREFERENCES_FRIENDLY_ID, PREFERENCES_FRIENDLY_ID_DEFAULT);
                  if (bitmap && (error == NO_ERROR)) {
                    display_show_msg(bitmap, FRIENDLY_ID, friendly_id, true, "",
                                     String(message_buffer));
                    bitmap_delete(bitmap);
                  }
                  need_to_refresh_display = 0;
                }
              } else {
                Log.error("%s [%d]: Payload %d bytes exceeds the %d limit\r\n",
                          __FILE__, __LINE__, rsize, MAX_PAYLOAD_SIZE);
                https.end();
                if (WiFi.RSSI() > WIFI_CONNECTION_RSSI) {
                  showMessageWithLogo(API_SIZE_ERROR);
                } else {
                  showMessageWithLogo(WIFI_WEAK);
                }
                submit_log("Receiving failed");
              }
            } else {
              Log.error("%s [%d]: [HTTPS] GET... failed, error: %s\r\n",
                        __FILE__, __LINE__,
                        https.errorToString(httpCode).c_str());
              https.end();
              if (WiFi.RSSI() > WIFI_CONNECTION_RSSI) {
                showMessageWithLogo(API_ERROR);
              } else {
                showMessageWithLogo(WIFI_WEAK);
              }
              submit_log("HTTPS received code is not OK. Code: %d", httpCode);
            }
          } else {
            Log.error("%s [%d]: [HTTPS] GET... failed, error: %s\r\n", __FILE__,
                      __LINE__, https.errorToString(httpCode).c_str());
            if (WiFi.RSSI() > WIFI_CONNECTION_RSSI) {
              showMessageWithLogo(API_ERROR);
            } else {
              showMessageWithLogo(WIFI_WEAK);
            }
            submit_log("HTTP Client failed with error: %s",
                       https.errorToString(httpCode).c_str());
          }
        } else {
          Log.error("%s [%d]: unable to connect\r\n", __FILE__, __LINE__);
          if (WiFi.RSSI() > WIFI_CONNECTION_RSSI) {
            showMessageWithLogo(API_ERROR);
          } else {
            showMessageWithLogo(WIFI_WEAK);
          }
          submit_log("unable to connect to the API");
        }
      }
      // End extra scoping block
    }
    client->stop();
    delete client;
  } else {
    Log.error("%s [%d]: Unable to create client\r\n", __FILE__, __LINE__);
    showMessageWithLogo(WIFI_INTERNAL_ERROR);
    submit_log("unable to create the client");
  }
}

/**
 * @brief Function to reset the friendly id, API key, WiFi SSID and password
 * @param url Server URL address
 * @return none
 */
static void resetDeviceCredentials(void) {
  Log.info("%s [%d]: The device will be reset now...\r\n", __FILE__, __LINE__);
  Log.info("%s [%d]: WiFi reseting...\r\n", __FILE__, __LINE__);
  WifiCaptivePortal.resetSettings();
  need_to_refresh_display = 1;
  bool res = preferences.clear();
  if (res)
    Log.info("%s [%d]: The device reset success. Restarting...\r\n", __FILE__,
             __LINE__);
  else
    Log.error("%s [%d]: The device reseting error. The device will be reset "
              "now...\r\n",
              __FILE__, __LINE__);
  preferences.end();
  ESP.restart();
}

/**
 * @brief Function to check and performing OTA update
 * @param none
 * @return none
 */
static void checkAndPerformFirmwareUpdate(void) {

  withHttp(binUrl, [&](HTTPClient *https, HttpError errorCode) -> bool {
    if (errorCode != HttpError::HTTPCLIENT_SUCCESS || !https) {
      Log.fatal("%s [%d]: Unable to connect for firmware update\r\n", __FILE__,
                __LINE__);
      if (WiFi.RSSI() > WIFI_CONNECTION_RSSI) {
        showMessageWithLogo(API_ERROR);
      } else {
        showMessageWithLogo(WIFI_WEAK);
      }
    }

    int httpCode = https->GET();
    if (httpCode == HTTP_CODE_OK) {
      Log.info("%s [%d]: Downloading .bin file...\r\n", __FILE__, __LINE__);

      size_t contentLength = https->getSize();
      // Perform firmware update
      if (Update.begin(contentLength)) {
        Log.info("%s [%d]: Firmware update start\r\n", __FILE__, __LINE__);
        showMessageWithLogo(FW_UPDATE);

        if (Update.writeStream(https->getStream())) {
          if (Update.end(true)) {
            Log.info("%s [%d]: Firmware update successful. Rebooting...\r\n",
                     __FILE__, __LINE__);
            showMessageWithLogo(FW_UPDATE_SUCCESS);
          } else {
            Log.fatal("%s [%d]: Firmware update failed!\r\n", __FILE__,
                      __LINE__);
            showMessageWithLogo(FW_UPDATE_FAILED);
          }
        } else {
          Log.fatal("%s [%d]: Write to firmware update stream failed!\r\n",
                    __FILE__, __LINE__);
          showMessageWithLogo(FW_UPDATE_FAILED);
        }
      } else {
        Log.fatal("%s [%d]: Begin firmware update failed!\r\n", __FILE__,
                  __LINE__);
        showMessageWithLogo(FW_UPDATE_FAILED);
      }
    }
    return true;
  });
}

/**
 * @brief Function to sleep preparing and go to sleep
 * @param none
 * @return none
 */
static void goToSleep(void) {
  WiFi.disconnect(true);
  filesystem_deinit();
  uint32_t time_to_sleep = SLEEP_TIME_TO_SLEEP;
  if (preferences.isKey(PREFERENCES_SLEEP_TIME_KEY))
    time_to_sleep =
        preferences.getUInt(PREFERENCES_SLEEP_TIME_KEY, SLEEP_TIME_TO_SLEEP);
  Log.info("%s [%d]: time to sleep - %d\r\n", __FILE__, __LINE__,
           time_to_sleep);
  preferences.putUInt(PREFERENCES_LAST_SLEEP_TIME, getTime());
  preferences.end();
  esp_sleep_enable_timer_wakeup((uint64_t)time_to_sleep * SLEEP_uS_TO_S_FACTOR);
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
static bool setClock() {
  bool sync_status = false;
  struct tm timeinfo;

  configTime(0, 0, "pool.ntp.org", "time.google.com", "time.windows.com");
  Log.info("%s [%d]: Time synchronization...\r\n", __FILE__, __LINE__);

  // Wait for time to be set
  if (getLocalTime(&timeinfo)) {
    sync_status = true;
    Log.info("%s [%d]: Time synchronization succeed!\r\n", __FILE__, __LINE__);
  } else {
    Log.info("%s [%d]: Time synchronization failed...\r\n", __FILE__, __LINE__);
  }

  Log.info("%s [%d]: Current time - %s\r\n", __FILE__, __LINE__,
           asctime(&timeinfo));

  return sync_status;
}

/**
 * @brief Function to read the battery voltage
 * @param none
 * @return float voltage in Volts
 */
static float readBatteryVoltage(void) {
  Log.info("%s [%d]: Battery voltage reading...\r\n", __FILE__, __LINE__);
  int32_t adc = 0;
  for (uint8_t i = 0; i < 128; i++) {
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
static void log_POST(char *log_buffer, size_t size) {
  String api_key = "";
  if (preferences.isKey(PREFERENCES_API_KEY)) {
    api_key =
        preferences.getString(PREFERENCES_API_KEY, PREFERENCES_API_KEY_DEFAULT);
    Log.info("%s [%d]: %s key exists. Value - %s\r\n", __FILE__, __LINE__,
             PREFERENCES_API_KEY, api_key.c_str());
  } else {
    Log.error("%s [%d]: %s key not exists.\r\n", __FILE__, __LINE__,
              PREFERENCES_API_KEY);
  }

  LogApiInput input{api_key, log_buffer};
  auto result = submitLogToApi(
      input, preferences.getString(PREFERENCES_API_URL, API_BASE_URL).c_str());
  if (!result) {
    Log_info("Was unable to send log to API; saving locally for later.");
    // log not send
    store_log(log_buffer, size, preferences);
  }
}

static uint32_t getTime(void) {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 200)) {
    Log.info("%s [%d]: Failed to obtain time. \r\n", __FILE__, __LINE__);
    return (0);
  }
  time(&now);
  return now;
}

static void checkLogNotes(void) {
  String log;
  gather_stored_logs(log, preferences);

  String api_key = "";
  if (preferences.isKey(PREFERENCES_API_KEY)) {
    api_key =
        preferences.getString(PREFERENCES_API_KEY, PREFERENCES_API_KEY_DEFAULT);
    Log.info("%s [%d]: %s key exists. Value - %s\r\n", __FILE__, __LINE__,
             PREFERENCES_API_KEY, api_key.c_str());
  } else {
    Log.error("%s [%d]: %s key not exists.\r\n", __FILE__, __LINE__,
              PREFERENCES_API_KEY);
  }

  bool result = false;
  if (log.length() > 0) {
    Log.info("%s [%d]: log string - %s\r\n", __FILE__, __LINE__, log.c_str());
    Log.info("%s [%d]: need to send the log\r\n", __FILE__, __LINE__);

    LogApiInput input{api_key, log.c_str()};
    result = submitLogToApi(
        input,
        preferences.getString(PREFERENCES_API_URL, API_BASE_URL).c_str());
  } else {
    Log.info("%s [%d]: no needed to send the log\r\n", __FILE__, __LINE__);
  }
  if (result == true) {
    clear_stored_logs(preferences);
  }
}

static void writeImageToFile(const char *name, uint8_t *in_buffer,
                             size_t size) {
  size_t res = filesystem_write_file(name, in_buffer, size);
  if (res != size) {
    Log.error("%s [%d]: File writing ERROR. Result - %d\r\n", __FILE__,
              __LINE__, res);
    submit_log("error writing file - %s. Written - %d bytes", name, res);
  } else {
    Log.info("%s [%d]: file %s writing success - %d bytes\r\n", __FILE__,
             __LINE__, name, res);
  }
}

static void writeSpecialFunction(SPECIAL_FUNCTION function) {
  if (preferences.isKey(PREFERENCES_SF_KEY)) {
    Log.info("%s [%d]: SF saved. Reading...\r\n", __FILE__, __LINE__);
    if ((SPECIAL_FUNCTION)preferences.getUInt(PREFERENCES_SF_KEY, 0) ==
        function) {
      Log.info("%s [%d]: No needed to re-write\r\n", __FILE__, __LINE__);
    } else {
      Log.info("%s [%d]: Writing new special function\r\n", __FILE__, __LINE__);
      bool res = preferences.putUInt(PREFERENCES_SF_KEY, function);
      if (res)
        Log.info("%s [%d]: Written new special function successfully\r\n",
                 __FILE__, __LINE__);
      else
        Log.error("%s [%d]: Writing new special function failed\r\n", __FILE__,
                  __LINE__);
    }
  } else {
    Log.error("%s [%d]: SF not saved\r\n", __FILE__, __LINE__);
    bool res = preferences.putUInt(PREFERENCES_SF_KEY, function);
    if (res)
      Log.info("%s [%d]: Written new special function successfully\r\n",
               __FILE__, __LINE__);
    else
      Log.error("%s [%d]: Writing new special function failed\r\n", __FILE__,
                __LINE__);
  }
}

static void showMessageWithLogo(MSG message_type) {
  trmnl_bitmap *logo = getLogoBitmap();
  display_show_msg(logo, message_type);
  bitmap_delete(logo);
  need_to_refresh_display = 1;
  preferences.putBool(PREFERENCES_DEVICE_REGISTERED_KEY, false);
}

static void showMessageWithLogo(MSG message_type, String friendly_id, bool id,
                                const char *fw_version, String message) {
  trmnl_bitmap *logo = getLogoBitmap();
  display_show_msg(logo, message_type, friendly_id, id, fw_version, message);
  bitmap_delete(logo);
  need_to_refresh_display = 1;
  preferences.putBool(PREFERENCES_DEVICE_REGISTERED_KEY, false);
}

static trmnl_bitmap *getLogoBitmap() {
  trmnl_error error;
  printf("ooo\r\n");
  return decodeBitmapFromMemory(trmnl_logo_bmp, sizeof(trmnl_logo_bmp), &error);
  trmnl_bitmap *logo = decodeBitmapFromFile("/logo.png", &error);
  if (logo != nullptr && error == NO_ERROR) {
    return logo;
  }
}

static trmnl_bitmap *decodeBitmapFromFile(const char *filename,
                                          trmnl_error *error) {
  trmnl_bitmap *result = nullptr;
  if (filesystem_file_exists(filename)) {
    // try png
    result = decodePNGFromFile(filename, error);
    if (result != nullptr) {
      *error = NO_ERROR;
      return result;
    }
    // try bmp
    result = decodeBMPFromFile(filename, error);
    return result;

  } else {
    Log.error("%s [%d]: File %s not found\r\n", __FILE__, __LINE__, filename);
    *error = FILE_NOT_FOUND;
    return nullptr;
  }
}

static trmnl_bitmap *decodeBitmapFromMemory(const uint8_t *data,
                                            uint32_t datasize,
                                            trmnl_error *error) {
  trmnl_bitmap *result = nullptr;
  // try PNG
  result = decodeBMPFromMemory(data, datasize, error);
  if ((result != nullptr) && (*error == NO_ERROR)) {
    return result;
  }
  // try bmp
  return decodePNGFromMemory(data, datasize, error);
}

static bool saveCurrentFileName(String &name) {
  if (!preferences.getString(PREFERENCES_FILENAME_KEY, "").equals(name)) {
    Log.info("%s [%d]: New filename:  - %s\r\n", __FILE__, __LINE__,
             name.c_str());
    size_t res = preferences.putString(PREFERENCES_FILENAME_KEY, name);
    if (res > 0) {
      Log.info("%s [%d]: New filename saved in the preferences - %d\r\n",
               __FILE__, __LINE__, res);
      return true;
    } else {
      Log.error("%s [%d]: New filename saving error!\r\n", __FILE__, __LINE__);
      return false;
    }
  } else {
    Log.info("%s [%d]: No needed to re-write\r\n", __FILE__, __LINE__);
    return true;
  }
}

static bool checkCurrentFileName(String &newName) {
  String currentFilename = preferences.getString(PREFERENCES_FILENAME_KEY, "");

  Log.error("%s [%d]: Current filename: %s\r\n", __FILE__, __LINE__,
            currentFilename);

  if (currentFilename.equals(newName)) {
    Log.info("%s [%d]: Current filename equals to the new filename\r\n",
             __FILE__, __LINE__);
    return true;
  } else {
    Log.error("%s [%d]: Current filename doesn't equal to the new filename\r\n",
              __FILE__, __LINE__);
    return false;
  }
}

static void wifiErrorDeepSleep() {
  if (!preferences.isKey(PREFERENCES_CONNECT_WIFI_RETRY_COUNT)) {
    preferences.putInt(PREFERENCES_CONNECT_WIFI_RETRY_COUNT, 1);
  }

  uint8_t retry_count =
      preferences.getInt(PREFERENCES_CONNECT_WIFI_RETRY_COUNT);

  Log_info("WIFI connection failed! Retry count: %d \n", retry_count);

  switch (retry_count) {
  case 1:
    preferences.putUInt(PREFERENCES_SLEEP_TIME_KEY,
                        WIFI_CONNECT_RETRY_TIME::WIFI_FIRST_RETRY);
    break;

  case 2:
    preferences.putUInt(PREFERENCES_SLEEP_TIME_KEY,
                        WIFI_CONNECT_RETRY_TIME::WIFI_SECOND_RETRY);
    break;

  case 3:
    preferences.putUInt(PREFERENCES_SLEEP_TIME_KEY,
                        WIFI_CONNECT_RETRY_TIME::WIFI_THIRD_RETRY);
    break;

  default:
    preferences.putUInt(PREFERENCES_SLEEP_TIME_KEY, SLEEP_TIME_TO_SLEEP);
    break;
  }
  retry_count++;
  preferences.putInt(PREFERENCES_CONNECT_WIFI_RETRY_COUNT, retry_count);

  display_sleep();
  goToSleep();
}

DeviceStatusStamp getDeviceStatusStamp() {
  DeviceStatusStamp deviceStatus = {};

  char fw_version[30];
  sprintf(fw_version, "%d.%d.%d", FW_MAJOR_VERSION, FW_MINOR_VERSION,
          FW_PATCH_VERSION);

  deviceStatus.wifi_rssi_level = WiFi.RSSI();
  parseWifiStatusToStr(deviceStatus.wifi_status,
                       sizeof(deviceStatus.wifi_status), WiFi.status());
  deviceStatus.refresh_rate = preferences.getUInt(PREFERENCES_SLEEP_TIME_KEY);
  deviceStatus.time_since_last_sleep = time_since_sleep;
  snprintf(deviceStatus.current_fw_version,
           sizeof(deviceStatus.current_fw_version), "%d.%d.%d",
           FW_MAJOR_VERSION, FW_MINOR_VERSION, FW_PATCH_VERSION);
  parseSpecialFunctionToStr(deviceStatus.special_function, special_function);
  deviceStatus.battery_voltage = readBatteryVoltage();
  parseWakeupReasonToStr(deviceStatus.wakeup_reason,
                         sizeof(deviceStatus.wakeup_reason),
                         esp_sleep_get_wakeup_cause());
  deviceStatus.free_heap_size = ESP.getFreeHeap();
  deviceStatus.max_alloc_size = ESP.getMaxAllocHeap();

  return deviceStatus;
}

bool SerializeJsonLog(DeviceStatusStamp device_status_stamp, time_t timestamp,
                      int codeline, const char *source_file, char *log_message,
                      uint32_t log_id) {
  JsonDocument json_log;

  json_log["creation_timestamp"] = timestamp;

  json_log["device_status_stamp"]["wifi_rssi_level"] =
      device_status_stamp.wifi_rssi_level;
  json_log["device_status_stamp"]["wifi_status"] =
      device_status_stamp.wifi_status;
  json_log["device_status_stamp"]["refresh_rate"] =
      device_status_stamp.refresh_rate;
  json_log["device_status_stamp"]["time_since_last_sleep_start"] =
      device_status_stamp.time_since_last_sleep;
  json_log["device_status_stamp"]["current_fw_version"] =
      device_status_stamp.current_fw_version;
  json_log["device_status_stamp"]["special_function"] =
      device_status_stamp.special_function;
  json_log["device_status_stamp"]["battery_voltage"] =
      device_status_stamp.battery_voltage;
  json_log["device_status_stamp"]["wakeup_reason"] =
      device_status_stamp.wakeup_reason;
  json_log["device_status_stamp"]["free_heap_size"] =
      device_status_stamp.free_heap_size;
  json_log["device_status_stamp"]["max_alloc_size"] =
      device_status_stamp.max_alloc_size;

  json_log["log_id"] = log_id;
  json_log["log_message"] = log_message;
  json_log["log_codeline"] = codeline;
  json_log["log_sourcefile"] = source_file;

  json_log["additional_info"]["filename_current"] =
      preferences.getString(PREFERENCES_FILENAME_KEY, "");
  json_log["additional_info"]["filename_new"] = new_filename.c_str();

  if (log_retry) {
    json_log["additional_info"]["retry_attempt"] =
        preferences.getInt(PREFERENCES_CONNECT_API_RETRY_COUNT);
  }

  serializeJson(json_log, log_array);

  log_POST(log_array, strlen(log_array));

  return true;
}

int submitLog(const char *format, time_t time, int line, const char *file,
              ...) {
  uint32_t log_id = preferences.getUInt(PREFERENCES_LOG_ID_KEY, 1);

  char log_message[1024];

  va_list args;
  va_start(args, file);

  int result = vsnprintf(log_message, sizeof(log_message), format, args);

  va_end(args);

  SerializeJsonLog(getDeviceStatusStamp(), time, line, file, log_message,
                   log_id);

  preferences.putUInt(PREFERENCES_LOG_ID_KEY, ++log_id);

  return result;
}
