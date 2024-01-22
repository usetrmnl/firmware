#include <Arduino.h>

#include "bl.h"
#include "DEV_Config.h"
#include "EPD.h"
#include "GUI_Paint.h"
#include "imagedata.h"
#include <stdlib.h>

#define uS_TO_S_FACTOR 1000000 /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 20       /* Time ESP32 will go to sleep (in seconds) */

#include <ArduinoLog.h>



static const char *TAG = "main.cpp";

void setup()
{
  bl_init();
  //test_init();
}

void loop()
{
  bl_process();
}
