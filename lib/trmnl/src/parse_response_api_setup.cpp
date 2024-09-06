
#include <ArduinoJson.h>
#include "api_response_parsing.h"
#include <trmnl_log.h>

ApiSetupResponse parseResponse_apiSetup(String &payload)
{
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, payload);

  if (error)
  {
    Log_error("JSON deserialization error.");
    return {.outcome = ApiSetupOutcome::DeserializationError};
  }

  if (doc["status"] != 200)
  {
    Log_info("status FAIL.");

    ApiSetupResponse response = {
        .outcome = ApiSetupOutcome::StatusError,
        .status = doc["status"]};
    return response;
  }

  return {
      .outcome = ApiSetupOutcome::Ok,
      .status = doc["status"],
      .api_key = doc["api_key"] | "",
      .friendly_id = doc["friendly_id"] | "",
      .image_url = doc["image_url"] | "",
      .message = doc["message"] | "",
  };
}