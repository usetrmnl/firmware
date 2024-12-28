# TRMNL Firmware

created for the [TRMNL](https://usetrmnl.com) e-ink display.

## **Algorithm block scheme**

![Image Alt text](/pics/algorithm.png "Algorithm block scheme")

## **Web Server Endpoints**

following Wifi connection via the captive portal, device swaps its Mac Address for an API Key and Friendly ID from the server (which get saved on device).

```curl
GET /api/setup

headers = {
  'ID' => 'XX:XX:XX:XX:XX' # mac adddress
}

response example (success):
{ "status": 200, "api_key": "2r--SahjsAKCFksVcped2Q", "friendly_id": "917F0B", "image_url": "https://usetrmnl.com/images/setup/setup-logo.bmp", "filename": "empty_state" }

response example (fail, device with this Mac Address not found)
{ "status" => 404, "api_key" => nil, "friendly_id" => nil, "image_url" => nil, "filename" => nil }
```

assuming the Setup endpoint responded successfully, future requests are made solely for image / display content:

```curl
GET /api/display

headers = {
  'ID' => 'XX:XX:XX:XX',
  'Access-Token' => '2r--SahjsAKCFksVcped2Q',
  'Refresh-Rate' => '1800',
  'Battery-Voltage' => '4.1',
  'FW-Version' => '2.1.3',
  'RSSI' => '-69'
}

response example (success, device found with this access token):
{
  "status"=>0, # will be 202 if no user_id is attached to device
  "image_url"=>"https://trmnl.s3.us-east-2.amazonaws.com/path-to-img.bmp",
  "filename"=>"2024-09-20T00:00:00",
  "update_firmware"=>false,
  "firmware_url"=>nil,
  "refresh_rate"=>"1800",
  "reset_firmware"=>false
}

response example (success, device found AND needs soft reset):
{
 "status"=>0,
 "image_url"=>"https://trmnl.s3.us-east-2.amazonaws.com/path-to-img.bmp",
 "filename"=>"name-of-img.bmp",
 "update_firmware"=>false,
 "firmware_url"=>nil,
 "refresh_rate"=>"1800",
 "reset_firmware"=>true
}

response example (success, device found AND needs firmware update):
{
 "status"=>0,
 "image_url"=>"https://trmnl.s3.us-east-2.amazonaws.com/path-to-img.bmp",
 "filename"=>"name-of-img.bmp",
 "update_firmware"=>true,
 "firmware_url"=>"https://trmnl.s3.us-east-2.amazonaws.com/path-to-firmware.bin",
 "refresh_rate"=>"1800",
 "reset_firmware"=>false
}

response example (fail, device not found for this access token):
{"status"=>500, "error"=>"Device not found"}

if 'FW-Version' header != web server `Setting.firmware_download_url`, server will include absolute URL from which to download firmware.
```

if device detects an issue with response data from the `api/display` endpoint, logs are sent to server.

```curl
POST /api/logs

# example request tbd
```

## **Power consumption**

Ths image displays the amount of power consumed during a work cycle that involves downloading and displaying images.

![Image Alt text](/pics/Simple_cycle.jpg "Simple cycle")

This image displays the amount of power consumed while in sleep mode.

![Image Alt text](/pics/Sleep_cycle.jpg "Sleep cycle")

This image displays the amount of power consumed during a work cycle that involves link pinging, new firmware downloading and OTA.

![Image Alt text](/pics/OTA.jpg "OTA")

Full Power Cycle

- Sleep 0.1mA
- Image refresh cycle 32.8mA during 24s

If refreshed continuously, device will refresh 8,231 times (54 hours) on a full charge.
If device is set to sleep continuously, it can sleep for 18,000 hours (750 days).

15 min refresh = 78 days
5 min refresh = 29 days

## **Low Battery Level**

This image shows that the battery disconnects when the voltage reaches 2.75 V:

![Image Alt text](/pics/battery_3v3.jpg "Voltage battery&3.3V")

The pulse on the graph shows the voltage on the divider in sleep mode, further on the graph it can be seen that at the moment of disconnection of the battery on the divider under load the voltage is equal to 1V, i.e. a voltage of 1.2V under load on the divider can be considered extremely critical, which corresponds to a voltage of 1.5V in the state sleep on the divider and 3V on the battery:

![Image Alt text](/pics/battery_divider.jpg "Voltage battery&divider")

## **Version Log**

[v.1.0.0]
   - initial work version

[v.1.0.1]
   - added version control
   - added reading the default logo from flash, not from SPIFFS

[v.1.0.2]
   - added setup request and response handling
   - added battery reading
   - added new errors messages
   - added key to requests
   - added dynamic refresh rate

[v.1.0.3]
   - added default logo storing and showing
   - added OTA

[v.1.0.4]
   - fixeD version that sending to the server
   - changed default refresh rate to 15 minutes

[v.1.0.5]
   - fixed WiFi connection with wrong credentials
   - changed default AP name
   - fixed starting config portal in backend if connection is wrong

[v.1.0.6]
   - changed picture file path to absolute path

[v.1.0.7]
   - fixed an uknown bag with OTA update

[v.1.0.8]
   - changeed BMP header length

[v.1.0.9]
   - fixed loop error on receiving image from the new server(added timeout loop that waiting for stream available)
   - localization problem with inversion of the color with new server(relative with difference headers)

[v.1.1.0]
   - added bmp header parser
   - added new error relative with incorrect bmp format - bad width, height, bpp, color table size, color

[v.1.2.0]
   - added new initial sequence for e-paper display (from RPi prototype). Now in the code are 3 initialization sequences: - default function from ESP32 code; default function from RPi code (2.2 s for Chinese displays, 10 s for waveshare displays); fast function from RPi code (3.4 s for Chinese displays, 1.8 s for waveshare displays).

[v.1.2.1]
   - added log POST request for failed GET requests

[v.1.2.2]
   - changed the sequence of actions when sending logs
   - removed menu items from WiFI manager
   - removed Friendly ID from boot image
   - done small copywriting edits for prompt messages
   - improved battery voltage reading(added information about basic voltage to README)

[v.1.2.3]
   - added retries for https queries

[v.1.2.4]
   - fixed delay after power ON from PC USB
   - debug hidden
   - fixed refresh rate

[v.1.2.5]
   - changed battery read function

[v.1.2.6]
   - code refactored
   - changed friendly ID display logic (now it will show until the device is linked to User at usertrmnl.com)
   - snow effect shows only when power reset or wake up by button

[v.1.2.7]
   - added warning message that if captive portal closed without credential saved - user must re-open it
   - added reset button to the main screen of the captive portal that clear preferences(wifi_credentials, friendly id and API key)
   - added reset_firmaware flag handling - if it will true then the device will reset all credentials(wifi_credentials, friendly id and API key)

[v.1.2.8]
   - fixed error decoding

[v.1.2.9]
   - fixed small bug on soft reset

[v.1.2.10]
   - WiFi deleted after device reset from Captive Portal or from API response
   - Rename "Reset" to "Soft Reset" on Config Portal
   - added reset handling after status=500 received
   - fixed display udating when no needed

[v.1.2.11]
   - fixed issue with the friendly id showing
   - added status 202 for /api/display endpoint

[v.1.2.12]
   - fixed status codes handling

[v.1.2.13]
   - added bad Wi-Fi signal handler while internet connection or HTTPS requesting

[v.1.2.14]
   - added RSSI field into /api/display request

[v.1.2.15]
   - added NTP servers mirrors
   - added loging in offline mode

[v.1.2.16]
   - added HTTPS error code to log note

[v.1.2.17]
   - incresed time of button pressing for reset from 300 ms to 5000 ms
   - decreased ping time after first powering-on and after plugins was deleted from device
   - add empty string parsing while URL parsing

[v.1.2.18]
   - replaced 3 attempts of getting UTC to 1 with multiple NTP servers
   - added double click handler
   - added special functions(Identify, Sleep, Multi WiFi, Restart playlist, Rewind, Send to me)
   - changed core version from 6.5.0 to 6.7.0
   - created filesystem library

[v.1.3.0]
   - fixed "rewind" special function to show "bad bmp format"
   - added new beatiful custom Wi-Fi configuration portal

[v.1.3.1]
   - changed block draw function to non-block 
   - added current image showing in "send to me" special function

[v.1.3.2]
   - added filename saving. It prevent unnecessary screen flicker on dupe content 

[v.1.3.3]
   - added animation while networks scanning
   - added rssi sorting and showing only one ssid with the same name with the highest rssi
   - added filtering for networks with name "TRMNL" 

[v.1.3.4]
   - added multiple WiFi support
   - tested JSON parsing
   - platform update

[v.1.3.5]
   - added tests for api responce parser and bmp reader
   - changed API endpoint
   - Added exponential entry time
   - Fixed several logging bugs

[v.1.3.6]
   - Added soft reset modal confirmation
   - Fixed some build warnings

[v.1.3.7]
   - Added option to download an image in up to 3 chunks
   - Updated dependencies

[v.1.4.0]
   - Renewed logs
   - Changed API ERROR message

[v.1.4.1]
   - Added white screen before wifi connect message
   - Added QR code to wifi captive image and changed text
   - Updated logs, added new fields and fixed some bugs

[v.1.4.2]
   - Test fix of wifi connection failure on some devices
   - Defined device sleep time after failed connection (60 sec)

[v.1.4.3]
   - Static device sleep time after failed wifi connection changed to progressive
   - Made several optimizations to make power cycle last faster (~2.5 seconds)
   - Changed "snow" image to TRMNL logo 

[v.1.4.4]
   - Fix of "send_to_me" special function
   - Added QR code for error messages related to Wi-Fi
   - Added "MAC Address not registered" handling and screen
   - Experimental Wi-FI connection and downloading issues fixes

[v.1.4.5]
   - Fixed issue with friendly ID and setup_logo screens when they wouldn't render after an error
   - Added "empty_state" support


## **Compilation guide**

A more user-friendly (and non developer) guide is available here:
https://help.usetrmnl.com/en/articles/10271569-manually-flash-firmware

If you prefer to skip this build step and simply grab the prepared bin file, download the latest build here: https://usetrmnl.com/api/firmware/latest.

1. Install VS Code: https://code.visualstudio.com
2. Install PlatformIO: https://platformio.org/install/ide?install=vscode
3. Install Git: https://git-scm.com/book/en/v2/Appendix-A%3A-Git-in-Other-Environments-Git-in-Visual-Studio-Code
4. Clone repository: https://github.com/usetrmnl/firmware
5. After cloning, open project in VS Code workspace
6. After configuring the project, click on the PlatformIO -> Build button located at the bottom of the screen

![Image Alt text](/pics/build_icon.JPG "Build")

8. After the compilation process is complete, you should expect to see a message in the console.

![Image Alt text](/pics/console.JPG "Console")

9. You can find the compiled file in the folder shown in the picture.

![Image Alt text](/pics/bin_folder.JPG "Bin")

## **Uploading guide (PlatformIO)**

1. Turn off PCB. Connect PCB to PC using USB-C cable. While holding down the boot button, turn on PCB. Let go of boot button. This puts board in flashing mode.

2. Mac/Windows: Select the proper COM port from drop-down list (or leave on "Auto"). Ubuntu: Look for something like "/dev/ttyACMO USB JTAG/serial debug unit" or "Espressif USB JTAG/serial debug unit" via lsusb.

![Image Alt text](/pics/fs.jpg "FS")

3. Click on "PlatformIO: Upload" button.

## **Uploading guide (ESP32 Flash Download Tool)**

Tools required:

1. Windows OS
2. Flash Tool 3.9.5
3. [Firmware binary file](https://github.com/usetrmnl/firmware/tree/main/builds)
4. [Bootloader binary file](https://github.com/usetrmnl/firmware/tree/main/builds/bin/bootloader.bin)
5. [Partition binary file](https://github.com/usetrmnl/firmware/tree/main/builds/bin/partitions.bin)
6. [Boot app binary file](https://github.com/usetrmnl/firmware/tree/main/builds/bin/boot_app0.bin)

### Step 1 - Configure flash tool
open the Flash Tool (executable file), select these parameters, then clickOK:

![Image Alt text](/pics/select_screen.jpg "select screen")

### Step 2 - Add binaries
1. Beside the top blank space, click “...” dots and select the bootloader binary file then input 
> “0x00000000” 
in the far right space and check the box.

2. Click “...” dots and select the partitions binary file then input 
> “0x00008000” 
in the far right space and check the box.

3. Click “...” dots and select the boot_app0 binary file then input 
> “0x0000e000” 
in the far right space and check the box.

4. Click “...” dots and select the firmware binary file then input 
> “0x00010000” 
in the far right space and check the box.

![Image Alt text](/pics/binaries.jpg "binaries")

finally, set the following parameters at the bottom of the Flash Tool interface:

![Image Alt text](/pics/settings.jpg "settings")

### Step 3 - Connect and flash device
1. Open the Windows “Device Manager” program and scroll to the bottom where the USB devices can be found. each machine will have different available devices, but look for a section like this:

![Image Alt text](/pics/devices.jpg "devices")

2. Next, connect the PCB to the Windows machine with a USB-C cable. make sure the USB port is on the right, and that the PCB’s on/off switch is toggled DOWN for “off.”

3. While holding the BOOT button (below the on/off toggle), toggle the device ON by flipping the above switch UP. you may hear a sound from your Windows machine Inspect the Device Manager connections at the bottom of the interface, and a new device should appear. it may be “USB Component {{ Num }},” or something like below:
 
![Image Alt text](/pics/select_device.jpg "select_device")

4. Take note of this device’s name, that is our TRMNL PCB. then back inside the Flash Tool, click to open the “COM” dropdown in the bottom right and choose the TRMNL PCB. finally, click the “START” button.

![Image Alt text](/pics/start.jpg "start")

### Step 4 - Prepare for new device flashing
Inside the Flash Tool click the “STOP” button.

![Image Alt text](/pics/stop.jpg "stop")

Next turn off (toggle DOWN) and unplug the PCB. you are now ready to flash another device - see Step 1.
