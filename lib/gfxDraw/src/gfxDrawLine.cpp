// - - - - -
// GFXDraw - A Arduino library for drawing shapes on a GFX display using paths describing the borders.
// gfxDrawLine.cpp: Library implementation file for functions to calculate all points of a staight line.
//
// Copyright (c) 2024-2024 by Matthias Hertel, http://www.mathertel.de
// This work is licensed under a BSD style license. See http://www.mathertel.de/License.aspx
//
// Changelog: See gfxDrawLine.h and documentation files in this library.
//
// - - - - -

#include "gfxDrawLine.h"

#ifndef GFX_TRACE
#define GFX_TRACE(...)  // GFXDRAWTRACE(__VA_ARGS__)
#endif


namespace gfxDraw {

void drawLine(Point &p1, Point &p2, fSetPixel cbDraw) {
  drawLine(p1.x, p1.y, p2.x, p2.y, cbDraw);
}


/// @brief Draw a line using the most efficient algorithm
/// @param x0 Starting Point X coordinate.
/// @param y0 Starting Point Y coordinate.
/// @param x1 Ending Point X coordinate.
/// @param y1 Ending Point Y coordinate.
/// @param cbDraw Callback with coordinates of line pixels.
void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, fSetPixel cbDraw) {
  GFX_TRACE("Draw Line (%d/%d)--(%d/%d)", x0, y0, x1, y1);

  int16_t delta_x = abs(x1 - x0);
  int16_t delta_y = abs(y1 - y0);
  int16_t step_x = (x0 < x1) ? 1 : -1;
  int16_t step_y = (y0 < y1) ? 1 : -1;

  if (x0 == x1) {
    // fast draw vertical lines
    int16_t endY = y1 + step_y;
    for (int16_t y = y0; y != endY; y += step_y) {
      cbDraw(x0, y);
    }

  } else if (y0 == y1) {
    // fast draw horizontal lines
    int16_t endX = x1 + step_x;
    for (int16_t x = x0; x != endX; x += step_x) {
      cbDraw(x, y0);
    }

  } else {
    int16_t err = delta_x - delta_y;

    while (true) {
      cbDraw(x0, y0);
      if ((x0 == x1) && (y0 == y1)) break;

      int16_t err2 = err << 1;

      if (err2 > -delta_y) {
        err -= delta_y;
        x0 += step_x;
      }
      if (err2 < delta_x) {
        err += delta_x;
        y0 += step_y;
      }
    }
  }
};

}  // gfxDraw:: namespace

// End.
