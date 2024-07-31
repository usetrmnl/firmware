#include <WiFi.h>

const char* ssid = "APTEST";
const char* password = "";

void setup() {
  Serial.begin(115200);
  
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password, 1);  // Change the channel if needed
  
  // Optional: Enable station mode for better compatibility
  WiFi.enableSTA(true);
  
  Serial.println("AP started");
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());
  
  WiFi.onEvent(WiFiEvent);
}

void loop() {
  // Your asynchronous server code here
}

void WiFiEvent(WiFiEvent_t event) {
  Serial.printf("[WiFi-event] event: %d\n", event);
  switch(event) {
    case SYSTEM_EVENT_AP_STACONNECTED:
      Serial.println("Station connected");
      break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:
      Serial.println("Station disconnected");
      break;
    // Add more cases if needed
  }
}
