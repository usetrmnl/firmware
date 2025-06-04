#ifndef DRAWFONT_H
#define DRAWFONT_H

#include <cstdint>
#include <gfxfont.h>

enum JUSTIFICATION {
    LEFT,
    RIGHT,
    TOP,
    BOTTOM,
    CENTER
};

const GFXfont* Paint_GetDefaultFont();
const GFXfont* Paint_GetFont();
void Paint_SetFont(const GFXfont* font);

/// draws UTF8 text
int16_t Paint_DrawText(int16_t x, int16_t y, const uint8_t* text, uint8_t color, JUSTIFICATION justification = CENTER); 

// fills width
void Paint_GetTextBounds(const uint8_t* text, uint16_t* width, uint16_t* height);
void Paint_GetGlyphBounds(uint8_t latinchar, uint16_t *width, uint16_t *height);

void displayCharset();
void displaySampleText();


#endif