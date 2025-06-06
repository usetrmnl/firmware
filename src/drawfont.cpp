#include <GUI_Paint.h>
#include <NicoClean-Regular8.h>
#include <Inter10.h>
#include <display.h>
#include <drawfont.h>
#include <trmnl_log.h>

// internal
void GetLatinTextBounds(const uint8_t *latintext, uint16_t *w, uint16_t *h);

// makes a copy
uint8_t *Utf8ToLatin(const uint8_t *utf8text, uint16_t *len);

const GFXfont *Paint_GetDefaultFont() { return &Inter_28pt_SemiBold10pt8b; } //&NicoClean_Regular8pt8b; }

static const GFXfont *current_font = Paint_GetDefaultFont();
const GFXfont *Paint_GetFont() { return current_font; }
void Paint_SetFont(const GFXfont *font) { current_font = font; }

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
// retuens a new converted string
uint8_t *Utf8ToLatin(const uint8_t *utf8text, uint16_t *lenlat) {
  uint16_t lenutf8 = strlen((const char *)utf8text);
  uint8_t *latintext = (uint8_t *)malloc(lenutf8 + 1);
  strcpy((char *)latintext, (const char *)utf8text);
  uint16_t k = 0; // length of latin1
  uint16_t ucs2;
  resetUTF8decoder();
  for (uint16_t i = 0; i < lenutf8; i++) {
    ucs2 = decodeUTF8(utf8text[i]);
    if (ucs2 == 0x0d ||
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

int16_t Paint_DrawLatinText(int16_t x, int16_t y, const uint8_t *latin_text,
                            uint8_t color) {
  uint16_t latin_length = strlen((const char *)latin_text);
  for (uint16_t c = 0; c < latin_length; ++c) {
    x += DrawLatin9Char(x, y, latin_text[c], color);
  }
  return Paint_GetFont()->yAdvance;
}

// can be passed non 0 terminated strings
void DrawLatinStringN(int16_t x, int16_t y, const uint8_t *latin_text,
                      uint16_t len, uint8_t color) {
  for (uint16_t c = 0; c < len; ++c) {
    x += DrawLatin9Char(x, y, latin_text[c], color);
  }
}

void Paint_Charset() {
  static char hex[] = "0123456789ABCDEF";
  const GFXfont *font = Paint_GetFont();
  Paint_Clear(WHITE);
  uint16_t gridx = display_width() / 17;
  uint16_t gridy = display_height() / 17;
  for (uint16_t y = 0; y < 16; ++y)

    DrawLatin9Char(0, (gridy * 2) + y * gridy, hex[y], BLACK);
  for (uint16_t x = 0; x < 16; ++x)
    DrawLatin9Char(gridx + x * gridx, gridy, hex[x], 0);
  for (uint16_t y = 0; y < 16; ++y) {
    for (uint16_t x = 0; x < 16; ++x) {
      DrawLatin9Char(gridx + x * gridx, (gridy * 2) + y * gridy, y * 16 + x,
                     BLACK);
    }
  }
}

// utility function to get max width of message
uint16_t MaxWidth(const uint8_t *lines[], uint16_t numtext) {
  uint16_t width = 0;
  for (uint16_t t = 0; t < numtext; ++t) {
    uint16_t twidth = 0, theight = 0;
    Paint_GetTextBounds(lines[t], &twidth, &theight);
    if (twidth > width) {
      width = twidth;
    }
  }
  return width;
}

uint16_t LatinTextWidth(const uint8_t *latintext) {
  uint16_t width = 0;

  for (uint16_t c = 0; c < strlen((const char *)latintext); ++c) {
    uint16_t tw = 0;
    uint16_t th = 0;
    Paint_GetGlyphBounds(latintext[c], &tw, &th);
    width += tw;
  }
  return width;
}

// Displays all chars and a sample text
const char *sampletext[] = {"Lorem ipsum dolor sit amet",
                            "Départ à l'heure français",
                            "pingüino mañana emoción", "glück lächeln groß"};

void Paint_Layout() {
  Paint_Clear(WHITE);
  const GFXfont *font = Paint_GetFont();
  uint8_t numtext = sizeof(sampletext) / sizeof(const char *);
  // 3x3 zones
  uint16_t width = display_width() / 3;
  uint16_t height = display_height() / 3;

  uint16_t leftx = 0;
  uint16_t topy = 0;

  uint16_t middlex = width;
  uint16_t middley = height;

  uint16_t rightx = 2 * width;
  uint16_t bottomy = 2 * height;

  Paint_DrawTextLines(leftx, topy, width, height, (const uint8_t **)sampletext,
                      numtext, BLACK, LEFT, TOP);
  Paint_DrawTextLines(middlex, topy, width, height,
                      (const uint8_t **)sampletext, numtext, BLACK, CENTER,
                      TOP);
  Paint_DrawTextLines(rightx, topy, width, height, (const uint8_t **)sampletext,
                      numtext, BLACK, RIGHT, TOP);

  Paint_DrawTextLines(leftx, middley, width, height,
                      (const uint8_t **)sampletext, numtext, LEFT, CENTER);
  Paint_DrawTextLines(middlex, middley, width, height,
                      (const uint8_t **)sampletext, numtext, CENTER, CENTER);
  Paint_DrawTextLines(rightx, middley, width, height,
                      (const uint8_t **)sampletext, numtext, RIGHT, CENTER);

  Paint_DrawTextLines(leftx, bottomy, width, height,
                      (const uint8_t **)sampletext, numtext, LEFT, BOTTOM);
  Paint_DrawTextLines(middlex, bottomy, width, height,
                      (const uint8_t **)sampletext, numtext, BLACK, CENTER,
                      BOTTOM);
  Paint_DrawTextLines(rightx, bottomy, width, height,
                      (const uint8_t **)sampletext, numtext, BLACK, RIGHT,
                      BOTTOM);
}

// single public function
int16_t Paint_DrawText(int16_t x, int16_t y, uint8_t *text, uint8_t color) {
  const GFXfont *font = Paint_GetFont();
  uint16_t width = 0;
  uint16_t height = 0;
  uint16_t latin_length;
  uint8_t *latintext = Utf8ToLatin(text, &latin_length);
  GetLatinTextBounds(latintext, &width, &height);
  Paint_DrawLatinText(x, y + font->yAdvance, latintext, color);
  free(latintext);
  return font->yAdvance;
}

void Paint_GetGlyphBounds(uint8_t latinchar, uint16_t *width,
                          uint16_t *height) {
  const GFXfont *font = Paint_GetFont();
  if ((latinchar < font->first) || (latinchar > font->last)) {
    return;
  }
  latinchar -= (uint8_t)font->first;
  GFXglyph *glyph = font->glyph + latinchar;
  *width = glyph->xAdvance;
  *height = font->yAdvance;
}

void GetLatinTextBounds(const uint8_t *latintext, uint16_t *width, uint16_t *height) {
  uint16_t latin_length = strlen((const char *)latintext);
  uint16_t w = 0;
  uint16_t wtotal = 0;
  uint16_t h = 0;
  for (uint16_t c = 0; c < latin_length; ++c) {
    Paint_GetGlyphBounds(latintext[c], &w, &h);
    wtotal += w;
  }
  *width = wtotal;
  *height = h;
}

// make a copy
void Paint_GetTextBounds(const uint8_t *utf8text, uint16_t *width,
                         uint16_t *height) {
  uint16_t latinlength = 0;
  uint8_t *latintext = Utf8ToLatin(utf8text, &latinlength);
  GetLatinTextBounds(latintext, width, height);
  free(latintext);
}

// complex ! txt must fit in bbox, handles cr and word-wrap
void Paint_DrawMultilineText(int16_t x, int16_t y, uint16_t maxwidth,
                             uint16_t maxheight, uint8_t *utf8text,
                             uint8_t color, JUSTIFICATION hjustification,
                             JUSTIFICATION vjustification) {
  const GFXfont *font = Paint_GetFont();
#if defined(DEBUG_TEXT_LAYOUT)
  Paint_DrawRectangle(x, y, x + maxwidth, y + maxheight, BLACK, DOT_PIXEL_DFT,
                      DRAW_FILL_EMPTY);
#endif
  // converts to  latin
  uint16_t numlatin = 0;
  uint8_t *latintext = Utf8ToLatin(utf8text, &numlatin);
  uint16_t curwidth;
  bool oversize = false;
  // first pass : compute lines number and max width

  uint16_t lines = 1;
  uint16_t max_line_width = 0;
  for (uint16_t c = 0; c < numlatin; ++c) {
    uint8_t lc = latintext[c];
    if (lc == '\n') {
      ++lines;
      if (maxwidth < curwidth)
        max_line_width = curwidth;
      curwidth = 0;
    } else if (oversize == true) {
      if (!isalnum(lc)) {
        ++lines;
        if (max_line_width < curwidth)
          max_line_width = curwidth;
        curwidth = 0;
      }
    }
    uint16_t lcw = 0;
    uint16_t lch = 0;
    Paint_GetGlyphBounds(lc, &lcw, &lch);
    curwidth += lcw;
    oversize = curwidth > maxwidth - 50;
  }

  // now computebox
  uint16_t textwidth = max_line_width;
  uint16_t maxlines = maxheight / font->yAdvance;
  // if (lines > maxlines) lines = maxlines;
  uint16_t textheight = lines * font->yAdvance;

  int16_t newx = x + (maxwidth - textwidth) / 2;
  int16_t newy = y + (maxheight - textheight) / 2;

#if defined(DEBUG_TEXT_LAYOUT)
  Paint_DrawRectangle(newx, newy, newx + textwidth, newy + textheight, BLACK,
                      DOT_PIXEL_DFT, DRAW_FILL_EMPTY);
#endif
  // now draw
  newy += font->yAdvance;
  oversize = false;
  curwidth = 0;
  uint8_t *cpos = (uint8_t *)latintext;
  uint8_t clen = 0;
  lines = 1;
  for (uint16_t c = 0; c < numlatin; ++c, ++clen) {
    uint8_t lc = latintext[c];
    if (lc == '\n') {
      DrawLatinStringN(newx, newy, cpos, clen, color);
      newy += font->yAdvance;
      ++lines;
      cpos += clen;
      clen = 0;
      curwidth = 0;
    } else if (oversize == true) {
      if (!isalnum(lc)) {
        DrawLatinStringN(newx, newy, cpos, clen, color);
        newy += font->yAdvance;
        ++lines;
        cpos += clen;
        clen = 0;
        curwidth = 0;
      }
    }
    uint16_t lcw = 0;
    uint16_t lch = 0;
    Paint_GetGlyphBounds(lc, &lcw, &lch);
    curwidth += lcw;
    oversize = curwidth > maxwidth;
  }
  free(latintext);
}

// utility function to display text in a rectangle
void Paint_DrawTextLines(int16_t x, int16_t y, uint16_t width, uint16_t height,
                         const uint8_t *lines[], uint16_t numlines,
                         uint8_t color, JUSTIFICATION hjustification,
                         JUSTIFICATION vjustification) {
  const GFXfont *font = Paint_GetFont();
#if defined(DEBUG_TEXT_LAYOUT)
  Log_info(" %d %d %dx%d", x, y, width, height);
  Paint_DrawRectangle(x, y, x + width, y + height, BLACK, DOT_PIXEL_1X1,
                      DRAW_FILL_EMPTY);
  Paint_DrawLine(x, y, x + width, y + height, BLACK, DOT_PIXEL_DFT,
                 LINE_STYLE_DOTTED);

  Paint_DrawLine(x, y + height, x + width, y, BLACK, DOT_PIXEL_DFT,
                 LINE_STYLE_DOTTED);
#endif
  uint16_t textheight = numlines * font->yAdvance;
  // find max width
  uint16_t textwidth = MaxWidth(lines, numlines);
  int16_t ypos = 0;
  switch (vjustification) {
  case TOP:
    ypos = y;
    break;
  case BOTTOM:
    ypos = y + height - textheight;
    break;
  case CENTER:
    ypos = y + (height - textheight) / 2;
    break;
  }
  int16_t xpos = x + (width - textwidth) / 2;
#if defined(DEBUG_TEXT_LAYOUT)
  Paint_DrawRectangle(xpos, ypos, xpos + textwidth, ypos + textheight, BLACK,
                      DOT_PIXEL_DFT, DRAW_FILL_EMPTY);
#endif
  for (uint8_t t = 0; t < numlines; ++t) {
    xpos = 0;
    int16_t xposfull = 0;
    uint16_t twidth, theight;
    Paint_GetTextBounds((const uint8_t *)lines[t], &twidth, &theight);
    switch (hjustification) {
    case LEFT:
      xpos = x;
      xposfull = x;
      break;
    case RIGHT:
      xpos = x + width - twidth;
      xposfull = x + width - textwidth;
      break;
    case CENTER:
      xpos = x + width / 2 - twidth / 2;
      xposfull = x + width / 2 - textwidth / 2;
      break;
    }
#if defined(DEBUG_TEXT_LAYOUT)
    Paint_DrawPoint(xpos, ypos, BLACK, DOT_PIXEL_3X3, DOT_FILL_RIGHTUP);
    Paint_DrawRectangle(xposfull, ypos, xposfull + textwidth, ypos + theight,
                        BLACK, DOT_PIXEL_DFT, DRAW_FILL_EMPTY);

#endif
    ypos += Paint_DrawText(xpos, ypos, (uint8_t *)lines[t], color);
  }
}
