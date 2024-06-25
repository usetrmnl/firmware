#include <Arduino.h>

#include "bl.h"

void setup()
{
  bl_init();
}

void loop()
{
  bl_process();
}
