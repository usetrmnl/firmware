# TRMNL Firmware

[insert something helpful]

## **Web Server Endpoints**

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
  'Battery-Voltage' => '4.1' # not currently in use by web server
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

## **Power consumption**

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

## **Version Log**

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
    - added dynamic refresh rate;

[v.1.0.3]
    - added default logo storing and showing;
    - added OTA

[v.1.0.4]
    - fixeD version that sending to the server;
    - changed default refresh rate to 15 minutes;

[v.1.0.5]
    - fixed WiFi connection with wrong credentials;
    - changed default AP name;
    - fixed starting config portal in backend if connection is wrong;

[v.1.0.6]
    - changed picture file path to absolute path;

[v.1.0.7]
    - fixed an uknown bag with OTA update;

[v.1.0.8]
    - changeed BMP header length;

[v.1.0.9]
    - fixed loop error on receiving image from the new server(added timeout loop that waiting for stream available);
    - localization problem with inversion of the color with new server(relative with difference headers);

[v.1.1.0]
    - added bmp header parser;
    - added new error relative with incorrect bmp format - bad width, height, bpp, color table size, color; 

    
## **Compilation guide**
1. Install the VScode https://code.visualstudio.com
2. Install PlatformIO https://platformio.org/install/ide?install=vscode
3. Install Git https://git-scm.com/book/en/v2/Appendix-A%3A-Git-in-Other-Environments-Git-in-Visual-Studio-Code
4. Clone repository from github https://github.com/usetrmnl/firmware
5. After cloning “Open” project in workspace
6. After configuring the project, click on the PlatformIO -> Build button located at the bottom of the screen

![Image Alt text](/pics/build_icon.JPG "Build")

8. After the compilation process is complete, you should expect to see a message in the console.

![Image Alt text](/pics/console.JPG "Console")

9. You can find the compiled file in the folder shown in the picture

![Image Alt text](/pics/bin_folder.JPG "Bin")

## **Uploading guide (PlatformIO)**

1. Connect PCB to PC using USB cable.

2. Select the proper COM port from drop-down list

![Image Alt text](/pics/fs.jpg "FS")

3. Click on "PlatformIO: Upload" button 

> [!WARNING]

> If the board does not want to be flashed, then try to enter the flashing mode manually. To do this, turn off the board using the switch, hold the button and turn on the board. After it you can try to upload firmware again

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

### Step 4 - Setup device by Mac Address
The Flash Tool will quickly add the firmware to the TRMNL PCB, and you need to copy the “STA” value from the info panel. this is the device Mac Address:

![Image Alt text](/pics/finish.jpg "finish")

In this example, the device’s Mac Address is:
> DC:DA:0C:C5:7E:4C

This Mac Address should be provided to the TRMNL web application [admin panel > Devices > New Device](https://usetrmnl.com/admin/devices/new) to instantiate this device to be ‘claimed’ by a customer later, when they unbox the product.

![Image Alt text](/pics/new_device.jpg "new_device")

### Step 5 - Prepare for new device flashing
Inside the Flash Tool click the “STOP” button.

![Image Alt text](/pics/stop.jpg "stop")

Next turn off (toggle DOWN) and unplug the PCB. you are now ready to flash another device - see Step 1.
