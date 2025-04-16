// - - - - -
// GFXDraw - A Arduino library for drawing shapes on a GFX display using paths describing the borders.
// gfxDrawBezier.cpp: Library implementation file for functions to calculate all points of a cubic bezier curve.
//
// Copyright (c) 2024-2024 by Matthias Hertel, http://www.mathertel.de
// This work is licensed under a BSD style license. See http://www.mathertel.de/License.aspx
//
// Changelog: See gfxDrawBezier.h and documentation files in this library.
//
// - - - - -

#include "gfxDraw.h"

#ifndef GFX_TRACE
#define GFX_TRACE(...)  // GFXDRAWTRACE(__VA_ARGS__)
#endif


#define SCALEFACTOR 1024
#define SCALESHIFT 10
#define SCALEUP(v) (v << SCALESHIFT)
#define SCALEDOWN(v) ((v + SCALEFACTOR / 2) >> SCALESHIFT)

namespace gfxDraw {

// This implementation of cubic bezier curve with a start and an end point given and by using 2 control points.
// C x1 y1, x2 y2, x y

void drawCubicBezier(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3, fSetPixel cbDraw) {
  // GFX_TRACE("cubicBezier: %d/%d %d/%d %d/%d %d/%d", x0, y0, x1, y1, x2, y2, x3, y3);

  // Line 1 is x0/y0 to x1/y1, dx1/dy1 is the relative vector from x0/y0 to x1/y1
  int16_t dx1 = (x1 - x0);
  int16_t dy1 = (y1 - y0);
  int16_t dx2 = (x2 - x1);
  int16_t dy2 = (y2 - y1);
  int16_t dx3 = (x3 - x2);
  int16_t dy3 = (y3 - y2);

  // heuristic: calc the number of steps we need
  uint16_t steps = (abs(dx1) + abs(dy1) + abs(dx2) + abs(dy2) + abs(dx3) + abs(dy3));  // p0 - 1 - 2 - 3 - 4 - p3
                                                                                       // GFX_TRACE("steps:%d", steps);
  proposePixel(x0, y0, cbDraw);

  for (uint16_t n = 1; n <= steps; n++) {
    int32_t f = (SCALEFACTOR * n) / steps;
    // 3 points and 3 deltas in 1000 units
    int32_t x4 = SCALEUP(x0) + (f * dx1);
    int32_t y4 = SCALEUP(y0) + (f * dy1);
    int32_t x5 = SCALEUP(x1) + (f * dx2);
    int32_t y5 = SCALEUP(y1) + (f * dy2);
    int32_t x6 = SCALEUP(x2) + (f * dx3);
    int32_t y6 = SCALEUP(y2) + (f * dy3);
    int32_t dx5 = (x5 - x4);
    int32_t dy5 = (y5 - y4);
    int32_t dx6 = (x6 - x5);
    int32_t dy6 = (y6 - y5);

    // 2 points
    int32_t x7 = x4 + SCALEDOWN(f * dx5);
    int32_t y7 = y4 + SCALEDOWN(f * dy5);
    int32_t x8 = x5 + SCALEDOWN(f * dx6);
    int32_t y8 = y5 + SCALEDOWN(f * dy6);
    int32_t dx8 = (x8 - x7);
    int32_t dy8 = (y8 - y7);

    // 1 point
    int32_t x9 = x7 + SCALEDOWN(f * dx8);
    int32_t y9 = y7 + SCALEDOWN(f * dy8);

    int16_t nextX = SCALEDOWN(x9);
    int16_t nextY = SCALEDOWN(y9);

    proposePixel(nextX, nextY, cbDraw);
  }  // for
  proposePixel(x3, y3, cbDraw);

  // flush all Pixels
  proposePixel(0, POINT_BREAK_Y, cbDraw);
};  // drawCubicBezier()

}  // gfxDraw:: namespace

// End.
