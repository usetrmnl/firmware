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
  // API connection settings
  String apiUrl;
  String apiKey;
  String friendlyId;

  // Device state information
  uint32_t refreshRate;
  bool deviceRegistered;

  // System information for API headers
  String macAddress;
  float batteryVoltage;
  String firmwareVersion;
  int rssi;
  int displayWidth;
  int displayHeight;
  SPECIAL_FUNCTION specialFunction;
};
