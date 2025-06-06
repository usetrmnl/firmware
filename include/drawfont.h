#ifndef DRAWFONT_H
#define DRAWFONT_H

#include <cstdint>
#include <gfxfont.h>

enum JUSTIFICATION {
    LEFT,
    RIGHT,
    TOP,
    BOTTOM,
    CENTER,
    FIT  // nt used yet
};

const GFXfont* Paint_GetDefaultFont();
const GFXfont* Paint_GetFont();
void Paint_SetFont(const GFXfont* font);

uint16_t LatinTextWidth(const uint8_t *text);

/// UTF8 text
// draws a single line, returns the vertical offset to next line
int16_t Paint_DrawText(int16_t x, int16_t y, uint8_t* text, uint8_t color); 
int16_t Paint_DrawLatinText(int16_t x, int16_t y, const uint8_t* text, uint8_t color); 

// draws given lines justified in a box
void Paint_DrawTextLines(int16_t x, int16_t y, uint16_t width, uint16_t height,
                         const uint8_t *lines[], uint16_t numlines, uint8_t color,
                         JUSTIFICATION hjustification = CENTER,
                         JUSTIFICATION vjustification = CENTER);
                        
// draws given lines, handles CR and word-wrap
void Paint_DrawMultilineText(int16_t x, int16_t y, uint16_t width, uint16_t height,
                        uint8_t *text, uint8_t color, JUSTIFICATION justification, JUSTIFICATION vjustification = CENTER);
                        

// fills width
void Paint_GetTextBounds(const uint8_t* text, uint16_t* width, uint16_t* height);
void Paint_GetGlyphBounds(uint8_t latinchar, uint16_t *width, uint16_t *height);

void Paint_Charset();
void Paint_Layout();
#endif