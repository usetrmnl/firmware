#pragma once

#include <Arduino.h>

typedef struct __attribute__((packed)) clock_settings_t
{
    uint16_t Xstart;
    uint16_t Xend;
    uint16_t Ystart;
    uint16_t Yend;
    uint8_t ColorFg;
    uint8_t ColorBg;
    uint8_t FontSize;
    char Format[21]; // 32 bytes total struct size; and "long enough" to set clock format
} clock_settings_t;
