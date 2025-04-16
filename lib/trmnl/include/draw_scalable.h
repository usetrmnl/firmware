#pragma once

#include "GUI_Paint.h"

void draw_scalable(const char *svgPath, int16_t pathX, int16_t pathY,
                   int16_t pathWidth, uint16_t pathHeight, int16_t centerX,
                   int16_t centerY, int16_t width, UWORD fillColor = BLACK, UWORD strokeColor = WHITE, float rotation = 0.0);

void drawLogo(int16_t centerX, int16_t centerY, int16_t width, float rotation = 0.0);

void drawTRMNL(int16_t centerX, int16_t centerY, int16_t width, float rotation = 0.0);

void drawFullLogo(int16_t centerX, int16_t centerY, int16_t width, float rotation = 0.0);