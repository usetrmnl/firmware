#include <api-client/display.h>
#include <ArduinoLog.h>
#include <HTTPClient.h>
#include <trmnl_log.h>
#include <WiFiClientSecure.h>
#include <config.h>
#include <api_response_parsing.h>
#include <http_client.h>

void addHeaders(HTTPClient &https, ApiDisplayInputs &inputs)
{
  Log_info("Added headers:\n\r"
           "ID: %s\n\r"
           "Special function: %d\n\r"
           "Access-Token: %s\n\r"
           "Refresh_Rate: %s\n\r"
           "Battery-Voltage: %s\n\r"
           "FW-Version: %s\r\n"
           "RSSI: %s\r\n",
           inputs.macAddress.c_str(),
           inputs.specialFunction,
           inputs.apiKey.c_str(),
           String(inputs.refreshRate).c_str(),
           String(inputs.batteryVoltage).c_str(),
           inputs.firmwareVersion.c_str(),
           String(inputs.rssi));

  https.addHeader("ID", inputs.macAddress);
  https.addHeader("Access-Token", inputs.apiKey);
  https.addHeader("Refresh-Rate", String(inputs.refreshRate));
  https.addHeader("Battery-Voltage", String(inputs.batteryVoltage));
  https.addHeader("FW-Version", inputs.firmwareVersion);
  https.addHeader("RSSI", String(inputs.rssi));
  https.addHeader("Width", String(inputs.displayWidth));
  https.addHeader("Height", String(inputs.displayHeight));

  if (inputs.specialFunction != SF_NONE)
  {
    Log.info("%s [%d]: Add special function: true (%d)\r\n", __FILE__, __LINE__, inputs.specialFunction);
    https.addHeader("special_function", "true");
  }
}

ApiDisplayResult fetchApiDisplay(ApiDisplayInputs &apiDisplayInputs)
{

  return withHttp(
      apiDisplayInputs.baseUrl + "/api/display",
      [&apiDisplayInputs](HTTPClient *https, HttpError error) -> ApiDisplayResult
      {
        if (error == HttpError::HTTPCLIENT_WIFICLIENT_ERROR)
        {
          Log.error("%s [%d]: Unable to create WiFiClient\r\n", __FILE__, __LINE__);
          return ApiDisplayResult{
              .error = https_request_err_e::HTTPS_UNABLE_TO_CONNECT,
              .response = {},
              .error_detail = "Unable to create WiFiClient",
          };
        }
        if (error == HttpError::HTTPCLIENT_HTTPCLIENT_ERROR)
        {
          Log.error("%s [%d]: Unable to create HTTPClient\r\n", __FILE__, __LINE__);
          return ApiDisplayResult{
              .error = https_request_err_e::HTTPS_UNABLE_TO_CONNECT,
              .response = {},
              .error_detail = "Unable to create HTTPClient",
          };
        }

        addHeaders(*https, apiDisplayInputs);

        delay(5);

        int httpCode = https->GET();

        if (httpCode < 0 ||
            !(httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY))
        {
          Log.error("%s [%d]: [HTTPS] GET... failed, error: %s\r\n", __FILE__, __LINE__, https->errorToString(httpCode).c_str());

          return ApiDisplayResult{
              .error = https_request_err_e::HTTPS_RESPONSE_CODE_INVALID,
              .response = {},
              .error_detail = "HTTP Client failed with error: " + https->errorToString(httpCode) +
                              "(" + String(httpCode) + ")"};
        }

        // HTTP header has been send and Server response header has been handled
        Log.info("%s [%d]: GET... code: %d\r\n", __FILE__, __LINE__, httpCode);

        String payload = https->getString();
        size_t size = https->getSize();
        Log.info("%s [%d]: Content size: %d\r\n", __FILE__, __LINE__, size);
        Log.info("%s [%d]: Free heap size: %d\r\n", __FILE__, __LINE__, ESP.getMaxAllocHeap());
        Log.info("%s [%d]: Payload - %s\r\n", __FILE__, __LINE__, payload.c_str());

        auto apiResponse = parseResponse_apiDisplay(payload);

        if (apiResponse.outcome == ApiDisplayOutcome::DeserializationError)
        {
          return ApiDisplayResult{
              .error = https_request_err_e::HTTPS_JSON_PARSING_ERR,
              .response = {},
              .error_detail = "JSON parse failed with error: " +
                              apiResponse.error_detail};
        }
        else
        {
          return ApiDisplayResult{
              .error = https_request_err_e::HTTPS_NO_ERR,
              .response = apiResponse,
              .error_detail = ""};
        }
      });
}