#pragma once

#include "esp_sleep.h"

typedef enum {
    WL_NO_SHIELD        = 255,   // for compatibility with WiFi Shield library
    WL_IDLE_STATUS      = 0,
    WL_NO_SSID_AVAIL    = 1,
    WL_SCAN_COMPLETED   = 2,
    WL_CONNECTED        = 3,
    WL_CONNECT_FAILED   = 4,
    WL_CONNECTION_LOST  = 5,
    WL_DISCONNECTED     = 6
} wl_status_t;

typedef enum {
    ESP_SLEEP_WAKEUP_UNDEFINED,    //!< In case of deep sleep, reset was not caused by exit from deep sleep
    ESP_SLEEP_WAKEUP_ALL,          //!< Not a wakeup cause, used to disable all wakeup sources with esp_sleep_disable_wakeup_source
    ESP_SLEEP_WAKEUP_EXT0,         //!< Wakeup caused by external signal using RTC_IO
    ESP_SLEEP_WAKEUP_EXT1,         //!< Wakeup caused by external signal using RTC_CNTL
    ESP_SLEEP_WAKEUP_TIMER,        //!< Wakeup caused by timer
    ESP_SLEEP_WAKEUP_TOUCHPAD,     //!< Wakeup caused by touchpad
    ESP_SLEEP_WAKEUP_ULP,          //!< Wakeup caused by ULP program
    ESP_SLEEP_WAKEUP_GPIO,         //!< Wakeup caused by GPIO (light sleep only on ESP32, S2 and S3)
    ESP_SLEEP_WAKEUP_UART,         //!< Wakeup caused by UART (light sleep only)
    ESP_SLEEP_WAKEUP_WIFI,              //!< Wakeup caused by WIFI (light sleep only)
    ESP_SLEEP_WAKEUP_COCPU,             //!< Wakeup caused by COCPU int
    ESP_SLEEP_WAKEUP_COCPU_TRAP_TRIG,   //!< Wakeup caused by COCPU crash
    ESP_SLEEP_WAKEUP_BT,           //!< Wakeup caused by BT (light sleep only)
} esp_sleep_source_t;

bool parseWifiStatusToStr(char *buffer, size_t buffer_size, wl_status_t wifi_status);
bool parseWakeupReasonToStr(char *buffer, size_t buffer_size, esp_sleep_source_t wakeup_reason);

