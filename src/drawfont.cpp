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

uint8_t c1 = 0; // Last character buffer

// Convert a single char from UTF8 to Extended ASCII
// Return "0" if a byte has to be ignored
uint8_t utf8_to_xtd_ascii(uint8_t ascii) {
  if (ascii < 128) { // Standard ASCII-set 0..0x7F handling
    c1 = 0;
    if (ascii < 32)
      return 0;
    else
      return ascii;
  }
  uint8_t last = c1;
  c1 = ascii;

  if (last == 0xC2) {
    return ascii - 32;
  } else if (last == 0xC3) {
    return ascii + 32;
  }
  return 0;
}

static uint8_t latinbuffer[256];

// returns number of latin1 chars converted
uint16_t Utf8ToLatin(const uint8_t *utf8text) {
  uint16_t lenutf8 = strlen((const char *)utf8text);
  uint16_t k = 0; // length of latin1
  char c;
  c1 = 0;
  for (uint16_t i = 0; i < lenutf8; i++) {
    c = utf8_to_xtd_ascii(utf8text[i]);
    if (c != 0) {
      if (k < 256) {
        latinbuffer[k++] = c;
      } else {
        latinbuffer[k] = 0;
        return k;
      }
    }
  }
  latinbuffer[k] = 0;
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
