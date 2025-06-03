#include <drawfont.h>
#include <display.h>
#include <GUI_Paint.h>
#include <NicoClean-Regular8.h>

const GFXfont* getDefaultFont() { return &NicoClean_Regular8pt8b; }

static const GFXfont* current_font  = getDefaultFont();
const GFXfont* getCurrentFont() { return current_font; }


byte c1 = 0; // Last character buffer

// Convert a single char from UTF8 to Extended ASCII
// Return "0" if a byte has to be ignored
byte utf8_to_xtd_ascii(byte ascii) {
  if (ascii < 128) { // Standard ASCII-set 0..0x7F handling
    c1 = 0;
    if (ascii < 32)
      return 0;
    else
      return ascii;
  }

  byte last = c1;
  c1 = ascii;

  if (last == 0xC2) {
    return ascii - 32;
  } else if (last == 0xC3) {
    return ascii + 32;
  }
  return 0;
}

// In place conversion of a UTF8 string to Latin
uint16_t utf8tocp(char *s, uint16_t len) {
  uint16_t k = 0;
  char c;
  c1 = 0;
  for (uint16_t i = 0; i < len; i++) {
    c = utf8_to_xtd_ascii(s[i]);
    if (c != 0) {
      s[k++] = c;
    }
  }
  s[k] = 0;
  return k;
}

// draws char in the latin9 range
//  skips or displays  rectangles
int16_t Paint_DrawLatin9Char(int16_t x, int16_t y, uint8_t c, uint8_t color) {
    if (x >= display_width() || y >= display_height()) return 0;
      const GFXfont *font = getCurrentFont();
  if ((c < font->first) || (c > font->last)) {
    GFXglyph *glyph = font->glyph + 'A' - font->first;
    uint8_t w = glyph->width;
    uint8_t h = glyph->height;
    int8_t xo = glyph->xOffset;
    int8_t yo = glyph->yOffset;
    Paint_DrawRectangle(x + xo, y + yo, x + xo + w, y + yo + h, BLACK,
                        DOT_PIXEL_DFT, DRAW_FILL_EMPTY);
    return glyph->xAdvance;
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

void Paint_DrawUtf8String(int16_t x, int16_t y, const char *utf8text, uint8_t color) {
  // make a copy and convert
  uint16_t lenutf8 = strlen(utf8text);
  char *buf = (char *)malloc(lenutf8 + 1);
  strncpy(buf, utf8text, lenutf8);
  uint16_t len = utf8tocp(buf, lenutf8);
  for (uint16_t c = 0; c < len; ++c) {
    x += Paint_DrawLatin9Char(x, y, buf[c], color);
  }
  free(buf);
}


// Displays all chars and a sample text
char sampletext[] = "Lorem ipsum dolor sit amet,\
consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam,\
quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.\
Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur.\
Excepteur sint occaecat cupidatat non proident,\
sunt in culpa qui officia deserunt mollit anim id est laborum.On the other hand, \
we denounce with righteous indignation and dislike men who are so beguiled and demoralized\
 by the charms of pleasure of the moment, so blinded by desire, that they cannot foresee the pain and trouble\
  that are bound to ensue; and equal blame belongs to those who fail in their duty through weakness of will,\
   which is the same as saying through shrinking from toil and pain. These cases are perfectly simple and easy to distinguish.\
    In a free hour, when our power of choice is untrammelled and when nothing prevents our being able to do what we like best, \
    every pleasure is to be welcomed and every pain avoided. But in certain circumstances and owing to the claims of duty \
    or the obligations of business it will frequently occur that pleasures have to be repudiated and annoyances accepted. \
    The wise man therefore always holds in these matters to this principle of selection:\
     he rejects pleasures to secure other greater pleasures, or else he endures pains to avoid worse pains.";
     

void displayCharset() {
  static char hex[] = "0123456789ABCDEF";
  const GFXfont *font = getCurrentFont();

  uint16_t gridx = display_width() / 17;
  uint16_t gridy = display_height() / 17;
  for (uint16_t y = 0; y < 16; ++y)

    Paint_DrawLatin9Char(0, (gridy * 2) + y * gridy, hex[y], BLACK);
  for (uint16_t x = 0; x < 16; ++x)
    Paint_DrawLatin9Char(gridx + x * gridx, gridy, hex[x], 0);
  for (uint16_t y = 0; y < 16; ++y) {
    for (uint16_t x = 0; x < 16; ++x) {
      Paint_DrawLatin9Char(gridx + x * gridx, (gridy * 2) + y * gridy,
                           y * 16 + x, BLACK);
    }
  }
}

void displaySampleText() {
   const GFXfont *font = getCurrentFont();
    uint16_t posy = font->yAdvance;
  uint16_t posx = 0;
  uint32_t pos = 0;
  do {
    posx += Paint_DrawLatin9Char(posx, posy, sampletext[pos], BLACK);
    if (posx > (display_width() - 20)) {
      posx = 0;
      posy += font->yAdvance;
    }
    ++pos;
  } while (sampletext[pos] != 0);
}


