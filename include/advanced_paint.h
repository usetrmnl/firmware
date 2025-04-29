#if !defined(ADVANCED_PAINT)
#define AVANCED_PAINT

#include <bb_truetype.h>


void AdvancedPaint_TrueType_Init(const uint8_t* buffer, uint32_t buffersize);
void AdvancedPaint_TrueType_SetSize(uint16_t size);
void AdvancedPaint_TrueType_DrawText(int16_t x, int16_t y, const char *text);

#endif