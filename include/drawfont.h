#ifndef DRAWFONT_H
#define DRAWFONT_H

#include <cstdint>
#include <gfxfont.h>


const GFXfont* getDefaultFont();
const GFXfont* getCurrentFont();

int16_t Paint_DrawLatin9Char(int16_t x, int16_t y, uint8_t c, uint8_t colorfg);
void Paint_DrawUtf8String(int16_t x, int16_t y, const char *utf8, uint8_t colorfg);


void displayCharset();
void displaySampleText();


#endif