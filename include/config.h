#ifndef CONFIG_H
#define CONFIG_H

#define FW_MAJOR_VERSION 1
#define FW_MINOR_VERSION 4
#define FW_PATCH_VERSION 5

#define LOG_MAX_NOTES_NUMBER 5

#define PREFERENCES_API_KEY "api_key"
#define PREFERENCES_API_KEY_DEFAULT ""
#define PREFERENCES_API_URL "api_url"
#define PREFERENCES_FRIENDLY_ID "friendly_id"
#define PREFERENCES_FRIENDLY_ID_DEFAULT ""
#define PREFERENCES_SLEEP_TIME_KEY "refresh_rate"
#define PREFERENCES_LOG_KEY "log_"
#define PREFERENCES_LOG_BUFFER_HEAD_KEY "log_head"
#define PREFERENCES_LOG_ID_KEY "log_id"
#define PREFERENCES_DEVICE_REGISTRED_KEY "plugin"
#define PREFERENCES_SF_KEY "sf"
#define PREFERENCES_FILENAME_KEY "filename"
#define PREFERENCES_LAST_SLEEP_TIME "last_sleep"
#define PREFERENCES_CONNECT_API_RETRY_COUNT "retry_count"
#define PREFERENCES_CONNECT_WIFI_RETRY_COUNT "wifi_retry"

#define WIFI_CONNECTION_RSSI (-100)

#define DISPLAY_BMP_IMAGE_SIZE 48062 // in bytes - 62 bytes - header; 48000 bytes - bitmap (480*800 1bpp) / 8

#define SLEEP_uS_TO_S_FACTOR 1000000           /* Conversion factor for micro seconds to seconds */
#define SLEEP_TIME_TO_SLEEP 900                /* Time ESP32 will go to sleep (in seconds) */
#define SLEEP_TIME_WHILE_NOT_CONNECTED 5       /* Time ESP32 will go to sleep (in seconds) */
#define SLEEP_TIME_WHILE_PLUGIN_NOT_ATTACHED 5 /* Time ESP32 will go to sleep (in seconds) */

enum API_CONNECT_RETRY_TIME // Time to sleep before trying to connect to the API (in seconds)
{
    API_FIRST_RETRY = 5,
    API_SECOND_RETRY = 10,
    API_THIRD_RETRY = 30
};

enum WIFI_CONNECT_RETRY_TIME // Time to sleep before trying to connect to the Wi-Fi (in seconds)
{
    WIFI_FIRST_RETRY = 60,
    WIFI_SECOND_RETRY = 180,
    WIFI_THIRD_RETRY = 300
};

#define PIN_RESET 9
#define PIN_INTERRUPT 2
#define PIN_BATTERY 3

#define BUTTON_HOLD_TIME 5000

#define SERVER_MAX_RETRIES 3

#define API_BASE_URL "https://trmnl.app"

#endif
