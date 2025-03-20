#include "api-client/submit_log.h"
#include <stdio.h>
#include "trmnl_log.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <memory>

bool submitLogToApi(LogApiInput &input, const char *api_url)
{
  String payload = "{\"log\":{\"logs_array\":[" + String(input.log_buffer) + "]}}";

  bool isHttps = String(api_url).indexOf("https://") == 0;

  // because of the lack of virtual destructor in the derived class WiFiClientSecure we have to do some trickery...
  std::unique_ptr<WiFiClientSecure> secureClient(new WiFiClientSecure());
  std::unique_ptr<WiFiClient> baseClient(new WiFiClient());

  secureClient->setInsecure();
  WiFiClient *client;
  
  if (isHttps)
    client = dynamic_cast<WiFiClient *>(secureClient.get());
  else
    client = dynamic_cast<WiFiClient *>(baseClient.get());;
  
  HTTPClient https;
  Log_info("[HTTPS] begin /api/log ...");

  char new_url[200];
  strcpy(new_url, api_url);
  strcat(new_url, "/api/log");

  if (!https.begin(*client, new_url))
  {
    Log_error("[HTTPS] Unable to connect");
    return false;
  }

  Log_info("[HTTPS] POST...");

        https.addHeader("ID", WiFi.macAddress());
        https.addHeader("Accept", "application/json");
        https.addHeader("Access-Token", input.api_key);
        https.addHeader("Content-Type", "application/json");
        Log_info("Send log - %s", payload.c_str());
        // start connection and send HTTP header
        int httpCode = https.POST(payload);

  // httpCode will be negative on error
  if (httpCode < 0)
  {
    Log_error("[HTTPS] POST... failed, error: %d %s", httpCode, https.errorToString(httpCode).c_str());
    return false;
  }
  else if (httpCode != HTTP_CODE_OK && httpCode != HTTP_CODE_MOVED_PERMANENTLY && httpCode != HTTP_CODE_NO_CONTENT)
  {
    Log_error("[HTTPS] POST... failed, returned HTTP code unknown: %d %s", httpCode, https.errorToString(httpCode).c_str());
    return false;
  }
  
  // HTTP header has been send and Server response header has been handled
  Log_info("[HTTPS] POST OK, code: %d", httpCode);

  https.end();

  return true;
}
