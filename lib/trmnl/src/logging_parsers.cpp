#include <string.h>

#include "logging_parcers.h"


struct WifiStatusNode
{
  const char *name;
  wl_status_t value;
};

struct WakeupReasonNode
{
  const char *name;
  esp_sleep_source_t value;
};

static const WifiStatusNode wifiStatusMap[] = {
    {"no_shield", WL_NO_SHIELD},
    {"idle_status", WL_IDLE_STATUS},
    {"no_ssid_avail", WL_NO_SSID_AVAIL},
    {"scan_completed", WL_SCAN_COMPLETED},
    {"connected", WL_CONNECTED},
    {"connect_failed", WL_CONNECT_FAILED},
    {"connection_lost", WL_CONNECTION_LOST},
    {"disconnected", WL_DISCONNECTED},
};

static const WakeupReasonNode wakeupReasonMap[] = {
    {"undefined", ESP_SLEEP_WAKEUP_UNDEFINED},
    {"all", ESP_SLEEP_WAKEUP_ALL},
    {"EXT0", ESP_SLEEP_WAKEUP_EXT0},
    {"EXT1", ESP_SLEEP_WAKEUP_EXT1},
    {"timer", ESP_SLEEP_WAKEUP_TIMER},
    {"touchpad", ESP_SLEEP_WAKEUP_TOUCHPAD},
    {"ulp", ESP_SLEEP_WAKEUP_ULP},
    {"gpio", ESP_SLEEP_WAKEUP_GPIO},
    {"uart", ESP_SLEEP_WAKEUP_UART},
    {"wifi", ESP_SLEEP_WAKEUP_WIFI},
    {"cocpu", ESP_SLEEP_WAKEUP_COCPU},
    {"cocpu_trap_trig", ESP_SLEEP_WAKEUP_COCPU_TRAP_TRIG},
    {"bt", ESP_SLEEP_WAKEUP_BT},
};

bool parseWifiStatusToStr(char *buffer, size_t buffer_size, wl_status_t wifi_status)
{
  for (const WifiStatusNode &entry : wifiStatusMap)
  {
    if (wifi_status == entry.value)
    {
      strncpy(buffer, entry.name, buffer_size);
      return true;
    }
  }
  return false;
}

bool parseWakeupReasonToStr(char *buffer, size_t buffer_size, esp_sleep_source_t wakeup_reason)
{
  for (const WakeupReasonNode &entry : wakeupReasonMap)
  {
    if (wakeup_reason == entry.value)
    {
      strncpy(buffer, entry.name, buffer_size);
      return true;
    }
  }
  return false;
}