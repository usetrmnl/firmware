#include <Arduino.h>
#include <ArduinoJson.h>

enum class ApiSetupOutcome
{
  Ok,
  DeserializationError,
  StatusError
};

struct ApiSetupResponse
{
  ApiSetupOutcome outcome;
  uint16_t status;
  String api_key;
  String friendly_id;
  String image_url;
  String message;
};
