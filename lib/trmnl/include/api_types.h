#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include "special_function.h"

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

enum class ApiDisplayOutcome
{
  Ok,
  DeserializationError,
};

struct ApiDisplayResponse
{
  ApiDisplayOutcome outcome;
  String error_detail;
  uint64_t status;
  String image_url;
  uint32_t image_url_timeout;
  String filename;
  bool update_firmware;
  String firmware_url;
  uint64_t refresh_rate;
  bool reset_firmware;
  SPECIAL_FUNCTION special_function;
  String action;
};

struct ApiDisplayInputs
{
  String baseUrl;
  String apiKey;
  String friendlyId;
  uint32_t refreshRate;
  String macAddress;
  float batteryVoltage;
  String firmwareVersion;
  int rssi;
  int displayWidth;
  int displayHeight;
  SPECIAL_FUNCTION specialFunction;
};
