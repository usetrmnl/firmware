#include <GUI_Paint.h>
#include <NicoClean-Regular8.h>
//#include <NicoClean-Regular16.h>
//#include <NicoClean-Regular32.h>
#include <display.h>
#include <drawfont.h>


// internal
int16_t DrawLatin9Char(int16_t x, int16_t y, uint8_t c, uint8_t colorfg);
void DrawUtf8String(int16_t x, int16_t y, const uint8_t *utf8, uint8_t colorfg);
void DrawLatinString(int16_t x, int16_t y, const uint8_t *latin, uint8_t colorfg);
void GetLatinTextBounds(const uint8_t *latintext, uint16_t *w, uint16_t *h);


const uint8_t *GetLatinText();
uint16_t Utf8ToLatin(const uint8_t *utf8text);

const GFXfont *Paint_GetDefaultFont() { return &NicoClean_Regular8pt8b; }

static const GFXfont *current_font = Paint_GetDefaultFont();
const GFXfont *Paint_GetFont() { return current_font; }
void Paint_SetFont(const GFXfont *font) { current_font = font; }


// source: https://github.com/Bodmer/Adafruit-GFX-Library/blob/master/Adafruit_GFX.cpp
// fork of Adafruit-GFX-Library by Bodmer
// line 1135 of Adafruit_GFX.cpp


uint8_t  decoderState = 0;   // UTF-8 decoder state
uint16_t decoderBuffer;      // Unicode code-point buffer

// do not show invalid utf8 or latin chars
static bool showUnmapped = false;

void resetUTF8decoder(void) {
  decoderState = 0;  
}

// Returns Unicode code point in the 0 - 0xFFFE range.  0xFFFF is used to signal 
// that more bytes are needed to complete decoding a multi-byte UTF-8 encoding
//   
// This is just a serial decoder, it does not check to see if the code point is 
// actually assigned to a character in Unicode.
#define SKIP_4_BYTE_ENCODINGS
uint16_t decodeUTF8(uint8_t c) {  
 
  if ((c & 0x80) == 0x00) { // 7 bit Unicode Code Point
    decoderState = 0;
    return (uint16_t) c;
  }

  if (decoderState == 0) {

    if ((c & 0xE0) == 0xC0) { // 11 bit Unicode Code Point
        decoderBuffer = ((c & 0x1F)<<6); // Save first 5 bits
        decoderState = 1;
    } else if ((c & 0xF0) == 0xE0) {  // 16 bit Unicode Code Point      {
        decoderBuffer = ((c & 0x0F)<<12);  // Save first 4 bits
        decoderState = 2;
#ifdef SKIP_4_BYTE_ENCODINGS    
    } else if ((c & 0xF8) == 0xF0) { // 21 bit Unicode  Code Point not supported
        decoderState = 12;
#endif    
    }    
  
  } else {
      decoderState--;
#ifdef SKIP_4_BYTE_ENCODINGS
      if (2 < decoderState && decoderState < 10) {
        decoderState = 0; // skipped over remaining 3 bytes of 21 bit code point
        if (showUnmapped)
          return 0x7F;
      }    
      else 
#endif      
      if (decoderState == 1) 
        decoderBuffer |= ((c & 0x3F)<<6); // Add next 6 bits of 16 bit code point
      else if (decoderState == 0) {
        decoderBuffer |= (c & 0x3F); // Add last 6 bits of code point (UTF8-tail)
        return decoderBuffer;
      }
  }
  return 0xFFFF; 
}


// should be enough for TRMNL FW
static uint8_t latinbuffer[256];


// returns number of latin1 chars converted
// not in range are skipped
uint16_t Utf8ToLatin(const uint8_t *utf8text) {
  uint16_t lenutf8 = strlen((const char *)utf8text);
  uint16_t k = 0; // length of latin1
  uint16_t ucs2;
  resetUTF8decoder();
  for (uint16_t i = 0; i < lenutf8; i++) {
    ucs2  = decodeUTF8(utf8text[i]);
   if (0x20 <= ucs2 && ucs2 <= 0x7F) 
      latinbuffer[k++] = (char) ucs2;
    else if (0xA0 <= ucs2 && ucs2 <= 0xFF)
      latinbuffer[k++] = (char) (ucs2 - 32);  
    else if (showUnmapped && 0xFF < ucs2 && ucs2 < 0xFFFF)
      latinbuffer[k++] = (char) 127;   
    if (k >= 256) break; 
  }
  latinbuffer[k]=0;
  return k;
}

// draws char in the latin9 range
//  skips or displays  rectangles
int16_t DrawLatin9Char(int16_t x, int16_t y, uint8_t c, uint8_t color) {
  if (x >= display_width() || y >= display_height())
    return 0;
  const GFXfont *font = Paint_GetFont();
  if ((c < font->first) || (c > font->last)) {
#if 0
    GFXglyph *glyph = font->glyph + 'A' - font->first;
    uint8_t w = glyph->width;
    uint8_t h = glyph->height;
    int8_t xo = glyph->xOffset;
    int8_t yo = glyph->yOffset;
    Paint_DrawRectangle(x + xo, y + yo, x + xo + w, y + yo + h, BLACK,
                        DOT_PIXEL_DFT, DRAW_FILL_EMPTY);
    return glyph->xAdvance;
#else
    return 0;
#endif
  }
  c -= (uint8_t)font->first;
  GFXglyph *glyph = font->glyph + c;
  uint8_t *bitmap = font->bitmap;

  uint16_t bo = glyph->bitmapOffset;
  uint8_t w = glyph->width;
  uint8_t h = glyph->height;
  int8_t xo = glyph->xOffset;
  int8_t yo = glyph->yOffset;
  uint8_t xx, yy, bits = 0, bit = 0;

  for (yy = 0; yy < h; yy++) {
    for (xx = 0; xx < w; xx++) {
      if (!(bit++ & 7)) {
        bits = bitmap[bo++];
      }
      if (bits & 0x80) {
        Paint_SetPixel(x + xo + xx, y + yo + yy, color);
      }
      bits <<= 1;
    }
  }
  return glyph->xAdvance;
}

const uint8_t *GetLatinText() { return latinbuffer; }


void DrawLatinString(int16_t x, int16_t y, const uint8_t *latin_text,
                           uint8_t color) {
  uint16_t latin_length = strlen((const char *)latin_text);
  for (uint16_t c = 0; c < latin_length; ++c) {
    x += DrawLatin9Char(x, y, latin_text[c], color);
  }
}

void displayCharset() {
  static char hex[] = "0123456789ABCDEF";
  const GFXfont *font = Paint_GetFont();

  uint16_t gridx = display_width() / 17;
  uint16_t gridy = display_height() / 17;
  for (uint16_t y = 0; y < 16; ++y)

    DrawLatin9Char(0, (gridy * 2) + y * gridy, hex[y], BLACK);
  for (uint16_t x = 0; x < 16; ++x)
    DrawLatin9Char(gridx + x * gridx, gridy, hex[x], 0);
  for (uint16_t y = 0; y < 16; ++y) {
    for (uint16_t x = 0; x < 16; ++x) {
      DrawLatin9Char(gridx + x * gridx, (gridy * 2) + y * gridy,
                           y * 16 + x, BLACK);
    }
  }
}

// Displays all chars and a sample text
const char *sampletext[] = {"Lorem ipsum dolor sit amet",
                            "Départ à l'heure français",
                            "pingüino mañana emoción", "glück lächeln groß"};

void displaySampleText() {
  const GFXfont *font = Paint_GetFont();
  uint8_t numtext = sizeof(sampletext) / sizeof(const char *);
  uint16_t height = numtext * font->yAdvance;
  uint16_t leftx = 0;
  uint16_t lefty = font->yAdvance;

  uint16_t rightx = display_width();
  uint16_t righty = display_height() - height;

  uint16_t centerx = display_width() / 2;
  uint16_t centery = (display_height() - height) / 2;

  for (uint8_t idx = 0; idx < numtext; ++idx) {
    lefty += Paint_DrawText(leftx, lefty, (const uint8_t*)sampletext[idx], BLACK, LEFT);
    righty += Paint_DrawText(rightx, righty, (const uint8_t*)sampletext[idx], BLACK, RIGHT);
    centery += Paint_DrawText(centerx, centery, (const uint8_t*)sampletext[idx], BLACK, CENTER);
  }
}

// single public function
int16_t Paint_DrawText(int16_t x, int16_t y, const uint8_t *text, uint8_t color,
                       JUSTIFICATION justification) {
  const GFXfont *font = Paint_GetFont();
  uint16_t width = 0;
  uint16_t height = 0;
  uint16_t latin_length = Utf8ToLatin(text);
  const uint8_t *latintext = GetLatinText();
  GetLatinTextBounds(latintext, &width, &height);
  int16_t xpos = (justification == LEFT)
                     ? x
                     : (justification == RIGHT ? (x - width) : x - (width / 2));
  DrawLatinString(xpos, y + font->yAdvance, latintext, color);
  return font->yAdvance;
}

void Paint_GetGlyphBounds(uint8_t latinchar, uint16_t *width, uint16_t *height) {
  const GFXfont *font = Paint_GetFont();
  if ((latinchar < font->first) || (latinchar > font->last)) {
    return;
  }
  latinchar -= (uint8_t)font->first;
  GFXglyph *glyph = font->glyph + latinchar;
  uint16_t bo = glyph->bitmapOffset;
  *width += glyph->xAdvance;
  *height = font->yAdvance;
}

void GetLatinTextBounds(const uint8_t *latintext, uint16_t *w,
                              uint16_t *h) {
  uint16_t latin_length = strlen((const char*)latintext);
  *w = 0;
  *h = 0;
  for (uint16_t c = 0; c < latin_length; ++c) {
    Paint_GetGlyphBounds(latintext[c], w, h);
  }
}

void Paint_GetTextBounds(const uint8_t* text, uint16_t* width, uint16_t* height) {
  uint16_t latinlength = Utf8ToLatin(text);
  const uint8_t *latintext = GetLatinText();
  GetLatinTextBounds(latintext, width, height);
}
