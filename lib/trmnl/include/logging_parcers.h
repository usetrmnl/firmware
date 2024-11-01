#pragma once

#include "esp_sleep.h"

bool parseWifiStatusToStr(char *buffer, size_t buffer_size, wl_status_t wifi_status);
bool parseWakeupReasonToStr(char *buffer, size_t buffer_size, esp_sleep_source_t wakeup_reason);

