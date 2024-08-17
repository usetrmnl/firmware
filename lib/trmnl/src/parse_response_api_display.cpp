
#include <ArduinoJson.h>
#include "api_response_parsing.h"
#include <trmnl_log.h>

ApiDisplayResponse parseResponse_apiDisplay(String &payload)
{
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, payload);

  if (error)
  {
    Log_error("JSON deserialization error.");
    return {.outcome = ApiDisplayOutcome::DeserializationError};
  }

  return {
      .status = doc["status"],
      .image_url = doc["image_url"],
      .update_firmware = doc["update_firmware"],
      .firmware_url = doc["firmware_url"],
      .refresh_rate = doc["refresh_rate"],
      .reset_firmware = doc["reset_firmware"],
      .special_function = doc["special_function"],
  };
}