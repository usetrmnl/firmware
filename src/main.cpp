#include <Arduino.h>

/* Includes ------------------------------------------------------------------*/
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

/* Entry point ----------------------------------------------------------------*/
void setup()
{
  bl_init();
}

/* The main loop -------------------------------------------------------------*/
void loop()
{
  bl_process();
}