// - - - - -
// GFXDraw - A Arduino library for drawing shapes on a GFX display using paths describing the borders.
// gfxDrawColors.cpp: Library implementation for handling Color values.
//
// Copyright (c) 2024-2024 by Matthias Hertel, http://www.mathertel.de
// This work is licensed under a BSD style license. See http://www.mathertel.de/License.aspx
//
// Changelog: See gfxDrawColors.h and documentation files in this library.
//
// - - - - -


#include "gfxDrawColors.h"

#ifndef GFX_TRACE
#define GFX_TRACE(...)  // GFXDRAWTRACE(__VA_ARGS__)
#endif


namespace gfxDraw {

// ===== ARGB class members =====

ARGB::ARGB() : raw(0) {};

// RGB+A Color
ARGB::ARGB(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
  : Red(r), Green(g), Blue(b), Alpha(a){};

// raw 32 bit color with no transparency
ARGB::ARGB(uint32_t color)
  : raw(color) {
  Alpha = 0xFF;
};

bool ARGB::operator==(ARGB const &col2) {
  return (raw == col2.raw);
}

bool ARGB::operator!=(ARGB const &col2) {
  return (raw != col2.raw);
}

/// @brief Convert into a 3*8 bit value using #RRGGBB.
/// @return color value.
uint32_t ARGB::toColor24() {
  return (raw & 0x0FFFFFF);
}

// Convert into a 16 bit value using 5(R)+6(G)+5(B)
uint16_t ARGB::toColor16() {
  return ((((Red) & 0xF8) << 8) | (((Green) & 0xFC) << 3) | ((Blue) >> 3));
}

// c lang-format off
ARGB const ARGB_BLACK (    0,    0,    0);
ARGB const ARGB_SILVER( 0xDD, 0xDD, 0xDD);
ARGB const ARGB_GRAY  ( 0xCC, 0xCC, 0xCC);
ARGB const ARGB_RED   ( 0xFF,    0,    0);
ARGB const ARGB_ORANGE( 0xE9, 0x76,    0);
ARGB const ARGB_YELLOW( 0xF6, 0xC7,    0);
ARGB const ARGB_GREEN (    0, 0x80,    0);
ARGB const ARGB_LIME  ( 0x32, 0xCD, 0x32);
ARGB const ARGB_BLUE  (    0,    0, 0xFF);
ARGB const ARGB_CYAN  (    0, 0xFF, 0xFF);
ARGB const ARGB_PURPLE( 0x99, 0x46, 0x80);
ARGB const ARGB_WHITE ( 0xFF, 0xFF, 0xFF);

ARGB const ARGB_TRANSPARENT( 0x00, 0x00, 0x00, 0x00);
// c lang-format on

// ===== gfxDraw helper functions =====

void dumpColor(const char *name, ARGB col) {
  GFX_TRACE(" %-12s: %02x.%02x.%02x.%02x #%06lx", name, col.Alpha, col.Red, col.Green, col.Blue, col.toColor24());
}

void dumpColorTable() {
  GFX_TRACE("        Color: A  R  G  B  #col24");
  dumpColor("Red", gfxDraw::ARGB_RED);
  dumpColor("Green", gfxDraw::ARGB_GREEN);
  dumpColor("Blue", gfxDraw::ARGB_BLUE);
  dumpColor("Orange", gfxDraw::ARGB_ORANGE);
  dumpColor("Transparent", gfxDraw::ARGB_TRANSPARENT);
}





}  // gfxDraw:: namespace

// End.
