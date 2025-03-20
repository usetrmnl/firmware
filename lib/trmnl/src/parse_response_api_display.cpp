
#include <ArduinoJson.h>
#include "api_response_parsing.h"
#include <trmnl_log.h>
#include <special_function.h>

ApiDisplayResponse parseResponse_apiDisplay(String &payload)
{
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, payload);

  if (error)
  {
    Log_error("JSON deserialization error.");
    return {.outcome = ApiDisplayOutcome::DeserializationError};
  }
  String special_function_str = doc["special_function"];

  return ApiDisplayResponse{
      .status = doc["status"],
      .image_url = doc["image_url"] | "",
      .image_url_timeout = doc["image_url_timeout"],
      .filename = doc["filename"] | "",
      .update_firmware = doc["update_firmware"],
      .firmware_url = doc["firmware_url"] | "",
      .refresh_rate = doc["refresh_rate"],
      .reset_firmware = doc["reset_firmware"],
      .special_function = parseSpecialFunction(special_function_str),
      .action = doc["action"] | "",
      };
}