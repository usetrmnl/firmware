#include "api-client/submit_log.h"
#include <stdio.h>
#include "trmnl_log.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

bool submitLogToApi(LogApiInput &input, const char *api_url)
{
  String payload = "{\"log\":{\"dump\":{\"error\":" + String(input.log_buffer) + "}}}";

  WiFiClientSecure *client = new WiFiClientSecure;
  bool result = false;
  if (client)
  {
    client->setInsecure();

    {
      // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is
      HTTPClient https;
      Log_info("[HTTPS] begin /api/log ...");

      char new_url[200];
      strcpy(new_url, api_url);
      strcat(new_url, "/api/log");
      
      if (https.begin(*client, new_url))
      { // HTTPS
        Log_info("[HTTPS] POST...");

        https.addHeader("Accept", "application/json");
        https.addHeader("Access-Token", input.api_key);
        https.addHeader("Content-Type", "application/json");
        Log_info("Send log - %s", payload.c_str());
        // start connection and send HTTP header
        int httpCode = https.POST(payload);

        // httpCode will be negative on error
        if (httpCode > 0)
        {
          // HTTP header has been send and Server response header has been handled
          Log_info("[HTTPS] POST... code: %d", httpCode);
          // file found at server
          if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY || httpCode == HTTP_CODE_NO_CONTENT)
          {
            Log_info("[HTTPS] POST OK");
            result = true;
          }
        }
        else
        {
          Log_error("[HTTPS] POST... failed, error: %d %s", httpCode, https.errorToString(httpCode).c_str());
          result = false;
        }

        https.end();
      }
      else
      {
        Log_error("[HTTPS] Unable to connect");
        result = false;
      }
    }

    delete client;
  }
  else
  {
    Log_error("[HTTPS] Unable to create client");
    result = false;
  }
  return result;
}