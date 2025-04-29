
#include <GUI_Paint.h>
#include <advanced_paint.h>

bb_truetype bbtt;

// callback
void DrawLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint32_t color) {
  Paint_DrawLine(x1, y1, x2, y2, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
}

void AdvancedPaint_TrueType_Init(const uint8_t *buffer, uint32_t buffersize) {
  bbtt.setTtfDrawLine(
      DrawLine); // pass the pointer to our drawline callback function
  bbtt.setTtfPointer((unsigned char*)buffer, buffersize); // use the font from FLASH
  bbtt.setCharacterSize(20);              // 60 pixels tall
  bbtt.setCharacterSpacing(0);
  bbtt.setTextBoundary(0, Paint.Width,
                       Paint.Height); // clip at display boundary
  bbtt.setTextColor(COLOR_NONE, 0xffffff);
}

void AdvancedPaint_TrueType_SetSize(uint16_t size) {
  bbtt.setCharacterSize(size);
}

void AdvancedPaint_TrueType_DrawText(int16_t x, int16_t y, const char *text) {
  bbtt.textDraw(x, y, text);
}
