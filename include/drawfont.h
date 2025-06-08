#ifndef DRAWFONT_H
#define DRAWFONT_H

#include "config.h"
#include <cstdint>
#include <gfxfont.h>

enum ANCHOR {
  TOPLEFT,
  TOP,
  TOPRIGHT,
  RIGHT,
  BOTTOMRIGTH,
  BOTTOM,
  BOTTOMLEFT,
  LEFT,
  CENTER
};

const GFXfont *Paint_GetDefaultFont();
const GFXfont *Paint_GetFont();
void Paint_SetFont(const GFXfont *font);

const GFXglyph *Paint_GetGlyph(uint16_t code);

// returns horizontal advance to next char
int16_t Paint_DrawGlyph(uint16_t c, int16_t x, int16_t y, uint8_t color);
/// UTF8 public functions
// draws a single line
void Paint_DrawTextAt(const char *text, int16_t x, int16_t y, uint8_t color,
                      ANCHOR anchor = CENTER);

void Paint_DrawTextRect(const char *text, int16_t x, int16_t y, uint16_t width,
                        uint16_t height, uint8_t color, ANCHOR anchor = CENTER);

void Paint_GetTextInfo(const char *utf8text, uint16_t *numlines,
                       uint16_t *maxwidth, uint16_t *maxheight,
                       uint16_t *lineheight);
// utf8
char *Utf8ToLatin(const char **utf8text, uint16_t *len); // makes a copy
void Paint_DrawLatinText(const char *s, int16_t len, int16_t x, int16_t y, uint8_t color = BLACK);
// demo
void Demo_Charset();

#endif