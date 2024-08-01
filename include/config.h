#ifndef CONFIG_H
#define CONFIG_H

#define FW_MAJOR_VERSION 1
#define FW_MINOR_VERSION 3
#define FW_PATCH_VERSION 3

#define LOG_MAX_NOTES_NUMBER 5

#define PREFERENCES_API_KEY "api_key"
#define PREFERENCES_API_KEY_DEFAULT ""
#define PREFERENCES_FRIENDLY_ID "friendly_id"
#define PREFERENCES_FRIENDLY_ID_DEFAULT ""
#define PREFERENCES_SLEEP_TIME_KEY "refresh_rate"
#define PREFERENCES_LOG_KEY "log_"
#define PREFERENCES_LOG_BUFFER_HEAD_KEY "log_head"
#define PREFERENCES_DEVICE_REGISTRED_KEY "plugin"
#define PREFERENCES_SF_KEY "sf"
#define PREFERENCES_FILENAME_KEY "filename"

#define WIFI_CONNECTION_ATTEMPTS 5
#define WIFI_PORTAL_TIMEOUT 120
#define WIFI_CONNECTION_RSSI (-100)

#define DISPLAY_BMP_IMAGE_SIZE 48062 // in bytes - 62 bytes - header; 48000 bytes - bitmap (480*800 1bpp) / 8

#define SLEEP_uS_TO_S_FACTOR 1000000 /* Conversion factor for micro seconds to seconds */
#define SLEEP_TIME_TO_SLEEP 900        /* Time ESP32 will go to sleep (in seconds) */
#define SLEEP_TIME_WHILE_NOT_CONNECTED 5        /* Time ESP32 will go to sleep (in seconds) */
#define SLEEP_TIME_WHILE_PLUGIN_NOT_ATTACHED 5        /* Time ESP32 will go to sleep (in seconds) */

#define PIN_RESET 9
#define PIN_INTERRUPT 2
#define PIN_BATTERY 3

#define BUTTON_HOLD_TIME 5000

#define SERVER_MAX_RETRIES 3

#define API_LOG_LINK "https://usetrmnl.com/api/log"
#endif