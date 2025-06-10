#include <GUI_Paint.h>
#include <NicoClean-Regular8.h>
#include <display.h>
#include <drawfont.h>
#include <trmnl_log.h>

const GFXfont *Paint_GetDefaultFont() {
  // return &Inter_28pt_SemiBold10pt8b;
  return &NicoClean_Regular8pt8b;
}

static const GFXfont *current_font = Paint_GetDefaultFont();
const GFXfont *Paint_GetFont() { return current_font; }
void Paint_SetFont(const GFXfont *font) { current_font = font; }

uint16_t InternalMaxWidth(const char **lines, uint16_t nlines);
uint16_t InternalTextWidth(const char *str);

// source:
// https://github.com/Bodmer/Adafruit-GFX-Library/blob/master/Adafruit_GFX.cpp
// fork of Adafruit-GFX-Library by Bodmer
// line 1135 of Adafruit_GFX.cpp

uint8_t decoderState = 0; // UTF-8 decoder state
uint16_t decoderBuffer;   // Unicode code-point buffer

// do not show invalid utf8 or latin chars
static bool showUnmapped = false;

void resetUTF8decoder(void) { decoderState = 0; }

// Returns Unicode code point in the 0 - 0xFFFE range.  0xFFFF is used to signal
// that more bytes are needed to complete decoding a multi-byte UTF-8 encoding
//
// This is just a serial decoder, it does not check to see if the code point is
// actually assigned to a character in Unicode.
#define SKIP_4_BYTE_ENCODINGS
uint16_t decodeUTF8(uint8_t c) {
  if ((c & 0x80) == 0x00) { // 7 bit Unicode Code Point
    decoderState = 0;
    return (uint16_t)c;
  }
  if (decoderState == 0) {
    if ((c & 0xE0) == 0xC0) {            // 11 bit Unicode Code Point
      decoderBuffer = ((c & 0x1F) << 6); // Save first 5 bits
      decoderState = 1;
    } else if ((c & 0xF0) == 0xE0) {      // 16 bit Unicode Code Point      {
      decoderBuffer = ((c & 0x0F) << 12); // Save first 4 bits
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
    } else
#endif
        if (decoderState == 1)
      decoderBuffer |=
          ((c & 0x3F) << 6); // Add next 6 bits of 16 bit code point
    else if (decoderState == 0) {
      decoderBuffer |= (c & 0x3F); // Add last 6 bits of code point (UTF8-tail)
      return decoderBuffer;
    }
  }
  return 0xFFFF;
}

// returns number of latin1 chars converted
// returns a new converted string
char *Utf8ToLatin(const char *utf8text, uint16_t *lenlat) {
  uint16_t lenutf8 = strlen((const char *)utf8text);
  // maximal latinlen <= lenlat
  char *latintext = (char *)malloc(lenutf8 + 1);

  uint16_t k = 0; // length of latin1
  uint16_t ucs2;
  resetUTF8decoder();
  for (uint16_t i = 0; i < lenutf8; i++) {
    ucs2 = decodeUTF8(utf8text[i]);
    if (ucs2 == '\n' ||
        (0x20 <= ucs2 && ucs2 <= 0x7F)) // we have to handle \n (0xd)
      latintext[k++] = (char)ucs2;
    else if (0xA0 <= ucs2 && ucs2 <= 0xFF)
      latintext[k++] = (char)(ucs2 - 32);
    else if (showUnmapped && 0xFF < ucs2 && ucs2 < 0xFFFF)
      latintext[k++] = (char)127;
  }
  latintext[k] = 0;
  *lenlat = k;
  return latintext;
}

// draws char in the latin9 range
//  skips or displays  rectangles
int16_t Paint_DrawGlyph(uint16_t c, int16_t x, int16_t y, uint8_t color) {
  if (x >= display_width() || y >= display_height())
    return 0;
  const GFXfont *font = Paint_GetFont();
  if ((c < font->first) || (c > font->last)) {
#if 0
    const GFXglyph *glyph = font->glyph + 'A' - font->first;
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
  const GFXglyph *glyph = font->glyph + c;
  uint8_t *bitmap = font->bitmap;

  uint16_t bo = glyph->bitmapOffset;
  uint8_t w = glyph->width;
  uint8_t h = glyph->height;
  int8_t xo = glyph->xOffset;
  int8_t yo = glyph->yOffset;
  uint8_t xx, yy, bits = 0, bit = 0;

#if defined(DEBUG_TEXT) && defined(DEBUG_GLYPH_LAYOUT)
Paint_DrawRectangle(x, y - font->yAdvance, x + glyph->xAdvance, y, BLACK, DOT_PIXEL_DFT, DRAW_FILL_EMPTY);
Paint_DrawRectangle(x + xo, y + yo, x + xo + w, y + yo + h, BLACK, DOT_PIXEL_DFT, DRAW_FILL_EMPTY);
#endif
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

char **PrepareText(char *latintext, uint16_t nlatin, uint16_t *nlines);
void ComputeBbox(char **lines, uint16_t nlines, int16_t x, int16_t y,
                 int16_t *xmin, int16_t *ymin, uint16_t *width,
                 uint16_t *height);

// public API
void Paint_GetTextInfo(const char *utf8text, uint16_t *numlines,
                       uint16_t *maxwidth, uint16_t *maxheight,
                       uint16_t *lineheight) {
  // convert to latin,
  uint16_t nlatin = 0;
  char *latintext = Utf8ToLatin(utf8text, &nlatin);
  // prepare lines
  uint16_t nlines = 0;
  char **lines = PrepareText(latintext, nlatin, &nlines);

  // compute bbox
  *lineheight = Paint_GetFont()->yAdvance;
  *numlines = nlines;
  *maxwidth = InternalMaxWidth((const char **)lines, nlines);
  *maxheight = nlines * *lineheight;
  // release resources
  free(latintext);
  free(lines);
}

// public API
// x and y are the center of the string
void Paint_DrawTextAt(const char *utf8text, int16_t x, int16_t y, uint8_t color,
                      ANCHOR anchor) {
#if defined(DEBUG_TEXT) && defined(DEBUG_TEXT_LAYOUT)
  Paint_DrawPoint(x, y, BLACK, DOT_PIXEL_3X3, DOT_FILL_AROUND);
#endif
  const GFXfont *font = Paint_GetFont();
  // convert to latin
  uint16_t nlatin = 0;
  char *latintext = Utf8ToLatin(utf8text, &nlatin);
  // prepare lines
  uint16_t nlines = 0;
  char **lines = PrepareText(latintext, nlatin, &nlines);

  // compute bbox
  uint16_t textwidth = InternalMaxWidth((const char **)lines, nlines);
  uint16_t textheight = nlines * font->yAdvance;

  int16_t centerx = x;
  int16_t centery = y;

  int16_t starty = centery - (textheight / 2);


  // position and draw (center)
#if defined(DEBUG_TEXT) && defined(DEBUG_TEXT_LAYOUT)
 Paint_DrawRectangle(centerx - (textwidth / 2), starty,
                      centerx + (textwidth / 2), starty + textheight, BLACK,
                      DOT_PIXEL_DFT, DRAW_FILL_EMPTY);
#endif
  for (uint8_t l = 0; l < nlines; l++) {
    // TODO: not recompute individual widths...
    uint16_t linewidth = InternalTextWidth(lines[l]);
    Paint_DrawLatinText(lines[l], strlen(lines[l]), centerx - (linewidth / 2),
                        starty, BLACK);
    starty += Paint_GetFont()->yAdvance;
  }
  // release resources
  free(latintext);
  free(lines);
}

// public API
void Paint_DrawLatinText(const char *s, int16_t len, int16_t x, int16_t y,
                         uint8_t color) {
  for (int16_t c = 0; c < len; ++c) {
    x += Paint_DrawGlyph(s[c], x, y + Paint_GetFont()->yAdvance, BLACK);
  }
}

char **PrepareText(char *latintext, uint16_t nlatin, uint16_t *nlines) {
  // count lines and put when a new line is found
  uint16_t numlines = 1;
  for (uint16_t idx = 0; idx < nlatin; idx++) {
    if (latintext[idx] == '\n') {
      numlines++;
      latintext[idx] = 0;
    }
  }
  char **lines = (char **)malloc(numlines * sizeof(char *));
  lines[0] = latintext;
  uint8_t curline = 0;
  // parse again to create the string pointers
  for (uint16_t idx = 0; idx < nlatin; idx++) {
    if (latintext[idx] == 0) {
      curline++;
      lines[curline] = latintext + idx + 1;
    }
  }
  *nlines = numlines;
  return lines;
}

// public API
void Paint_DrawTextRect(const char *utf8text, int16_t x, int16_t y,
                        uint16_t width, uint16_t height, uint8_t color,
                        ANCHOR anchor) {
#if defined(DEBUG_TEXT) && defined(DEBUG_TEXT_LAYOUT)
  Paint_DrawRectangle(x, y, x + width, y + height, BLACK, DOT_PIXEL_2X2,
                      DRAW_FILL_EMPTY);
#endif
  // convert to latin
  uint16_t nlatin = 0;
  char *latintext = Utf8ToLatin(utf8text, &nlatin);
  // prepare lines
  uint16_t nlines = 0;
  char **lines = PrepareText(latintext, nlatin, &nlines);

  // compute bbox
  uint16_t textwidth = InternalMaxWidth((const char **)lines, nlines);
  uint16_t textheight = nlines * Paint_GetFont()->yAdvance;

  // position and draw (center)
  int16_t centerx = x + (width / 2);
  int16_t starty = y + (height - textheight) / 2;

#if defined(DEBUG_TEXT) && defined(DEBUG_TEXT_LAYOUT)
  Paint_DrawRectangle(centerx - (textwidth / 2), starty,
                      centerx + (textwidth / 2), starty + textheight, BLACK,
                      DOT_PIXEL_DFT, DRAW_FILL_EMPTY);
#endif
  for (uint8_t l = 0; l < nlines; l++) {
    // TODO: not recompute individual widths...
    uint16_t linewidth = InternalTextWidth(lines[l]);
    Paint_DrawLatinText(lines[l], strlen(lines[l]), centerx - (linewidth / 2),
                        starty, BLACK);
    starty += Paint_GetFont()->yAdvance;
  }
  // release resources
  free(latintext);
  free(lines);
}

const GFXglyph *Paint_GetGlyph(uint16_t code) {
  const GFXfont *font = Paint_GetFont();
  uint16_t first = font->first;
  uint16_t last = font->last;
  if (code >= first && code <= last) {
    GFXglyph *glyph = font->glyph + code - first;
    return glyph;
  }
  return 0;
}

uint16_t InternalMaxWidth(const char **lines, uint16_t nlines) {
  uint16_t maxwidth = 0;
  for (uint16_t l = 0; l < nlines; ++l) {
    int16_t linewidth = InternalTextWidth(lines[l]);
    if (linewidth > maxwidth)
      maxwidth = linewidth;
  }
  return maxwidth;
}

uint16_t ComplexInternalTextWidth(const char *str) {
  char c;
  const GFXglyph *g;
  uint16_t width = 0;
  if ((c = *str++)) {
    g = Paint_GetGlyph(*str++);
    if (g)
      width = g->xAdvance - g->width - g->xOffset;
  } else {
    return 0;
  }
  int16_t last_offset = 0;
  while ((c = *str++)) {
    g = Paint_GetGlyph(c);
    if (g) {
      width += g->xAdvance;
      last_offset = g->xAdvance - g->width - g->xOffset;
    }
  }
  if (g) {
    width -= last_offset;
  }
  return width;
}

uint16_t InternalTextWidth(const char *str) {
  char c;
  const GFXglyph *g;
  uint16_t width = 0;
  while ((c = *str++)) {
    g = Paint_GetGlyph(c);
    if (g) {
      width += g->xAdvance;
    }
  }
      return width;
}

void Demo_Charset() {
  static char hex[] = "0123456789ABCDEF";
  const GFXfont *font = Paint_GetFont();
  Paint_Clear(WHITE);
  uint16_t gridx = display_width() / 17;
  uint16_t gridy = display_height() / 17;
  for (uint16_t y = 0; y < 16; ++y)

    Paint_DrawGlyph(hex[y], 0, (gridy * 2) + y * gridy, BLACK);
  for (uint16_t x = 0; x < 16; ++x)
    Paint_DrawGlyph(hex[x], gridx + x * gridx, gridy, 0);
  for (uint16_t y = 0; y < 16; ++y) {
    for (uint16_t x = 0; x < 16; ++x) {
      Paint_DrawGlyph(y * 16 + x, gridx + x * gridx, (gridy * 2) + y * gridy,
                      BLACK);
    }
  }
}
