# TRMNL Firmware

[insert something helpful]

**Web Server Endpoints**

following Wifi connection, swap Mac Address for API Key and Friendly ID (which get saved on device).

```curl
GET `/api/setup`

headers = {
  'ID' => 'XX:XX:XX:XX'
}

response example (success):
{ "status" => 200, "api_key" => "2r--SahjsAKCFksVcped2Q", friendly_id: "917F0B", image_url: '/images/setup/setup-logo.png' }

response example (fail, device with this Mac Address not found)
{ "status" => 404, "api_key" => nil, friendly_id: nil }
```
request for image / display content

```curl
GET `/api/display`

headers = {
  'ID' => 'XX:XX:XX:XX',
  'Access-Token' => '2r--SahjsAKCFksVcped2Q',
  'Refresh-Rate' => '1800' 
  'Battery-Voltage' => '4100' # not currently in use by web server
  'FW-Version' => '0.1.3' 
}

response example (success, device found with this access token):
{
  "image_url"=>"/images/sample_screens/shopify_orders_black.bmp",
  "firmware_url"=>nil,
  "refresh_rate"=>"1800"
}

response example (success, device found AND needs firmware update):
{
 "image_url"=>"/images/sample_screens/close_crm_stats.bmp",
 "firmware_url"=>"/some-firmware-endpoint-here",
 "refresh_rate"=>"1800"
}

response example (fail, device not found for this access token):
{"image_url"=>nil, "firmware_url"=>nil, "refresh_rate"=>nil}

if 'FW-Version' header and web server `Device::FIRMWARE_VERSION` do not match, server will respond with endpoint from which to download new Firmware.
```

**Power consumption**

The image displays the amount of power consumed during a work cycle that involves downloading and displaying images.

![Image Alt text](/pics/Simple_cycle.jpg "Simple cycle")

The image displays the amount of power consumed while in sleep mode

![Image Alt text](/pics/Sleep_cycle.jpg "Sleep cycle")

The image displays the amount of power consumed during a work cycle that involves link pinging, new firmware downloading and OTA.

![Image Alt text](/pics/OTA.jpg "OTA")

In this repo readme you can see actual power consumption test results
https://github.com/usetrmnl/firmware/tree/main

Full Power Cycle

- Sleep 0.1mA
- Image refresh cycle 32.8mA during 24s

In case it will do a continuous refresh it will refresh 8231 times on full charge or 54 hours of continuous refreshing
In case it will sleep all the time it can sleep 18000 hours which is 750 days

15 min refresh = 78 days
5 min refresh = 29 days

**Version Log**

[v.1.0.0]
    - initial work version;

[v.1.0.1]
    - added version control;
    - added reading the default logo from flash, not from SPIFFS; 

[v.1.0.2]
    - added setup request and response handling;
    - added battery reading;
    - added new errors messages;
    - added key to requests;
    - add dynamic refresh rate;

[v.1.0.3]
    - added default logo storing and showing;
    - added OTA

[v.1.0.4]
    - fix version that sending to the server;
    - change default refresh rate to 15 minutes;

[v.1.0.5]
    - fix WiFi connection with wrong credentials;
    - change default AP name;
    - fix starting config portal in backend if connection is wrong;
