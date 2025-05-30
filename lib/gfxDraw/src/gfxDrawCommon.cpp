// - - - - -
// GFXDraw - A Arduino library for drawing shapes on a GFX display using paths describing the borders.
// gfxDrawCommon.cpp: Library implementation for common functionality.
//
// Copyright (c) 2024-2024 by Matthias Hertel, http://www.mathertel.de
// This work is licensed under a BSD style license. See http://www.mathertel.de/License.aspx
//
// Changelog: See gfxDrawCommon.h and documentation files in this library.
//
// - - - - -

#include "gfxDraw.h"
#include "gfxDrawCommon.h"

#ifndef GFX_TRACE
#define GFX_TRACE(...)  // GFXDRAWTRACE(__VA_ARGS__)
#endif


namespace gfxDraw {

// Proposes a next pixel of the path.

void proposePixel(int16_t x, int16_t y, fSetPixel cbDraw) {
  static Point lastPoints[3];

  GFX_TRACE("proposePixel(%d, %d)", x, y);

  if ((x == lastPoints[0].x) && (y == lastPoints[0].y)) {
    // don't collect duplicates
    GFX_TRACE("  duplicate!");

  } else if (y == POINT_BREAK_Y) {
    // draw all remaining points and invalidate lastPoints
    GFX_TRACE("  flush!");
    for (int n = 2; n >= 0; n--) {
      if (lastPoints[n].y != POINT_INVALID_Y)
        cbDraw(lastPoints[n].x, lastPoints[n].y);
      lastPoints[n].y = POINT_INVALID_Y;
    }  // for

  } else if (lastPoints[0].y == POINT_BREAK_Y) {
    lastPoints[0].x = x;
    lastPoints[0].y = y;

  } else {
    // draw oldest point and shift new point in
    if (lastPoints[2].y != POINT_INVALID_Y)
      cbDraw(lastPoints[2].x, lastPoints[2].y);
    lastPoints[2] = lastPoints[1];
    lastPoints[1] = lastPoints[0];
    lastPoints[0].x = x;
    lastPoints[0].y = y;

    if (lastPoints[1].y != POINT_INVALID_Y) {
      bool delFlag = false;

      // don't draw "corner" points
      if ((lastPoints[0].y == lastPoints[1].y) && (abs(lastPoints[0].x - lastPoints[1].x) == 1)) {
        delFlag = (lastPoints[1].x == lastPoints[2].x);
      } else if ((lastPoints[0].x == lastPoints[1].x) && (abs(lastPoints[0].y - lastPoints[1].y) == 1)) {
        delFlag = (lastPoints[1].y == lastPoints[2].y);
      }

      // draw between unconnected points
      if (!delFlag) {
        if ((abs(lastPoints[0].x - lastPoints[1].x) <= 1) && (abs(lastPoints[0].y - lastPoints[1].y) <= 1)) {
          // points are connected -> no additional draw required

        } else if ((abs(lastPoints[0].x - lastPoints[1].x) <= 2) && (abs(lastPoints[0].y - lastPoints[1].y) <= 2)) {
          // simple interpolate new lastPoints[1]
          // GFX_TRACE("  gap!");
          if (lastPoints[2].y != POINT_INVALID_Y)
            cbDraw(lastPoints[2].x, lastPoints[2].y);
          lastPoints[2] = lastPoints[1];
          lastPoints[1].x = (lastPoints[0].x + lastPoints[1].x) / 2;
          lastPoints[1].y = (lastPoints[0].y + lastPoints[1].y) / 2;
        } else if ((abs(lastPoints[0].x - lastPoints[1].x) > 2) || (abs(lastPoints[0].y - lastPoints[1].y) > 2)) {
          // GFX_TRACE("  big gap!");

          // draw a streight line from lastPoints[1] to lastPoints[0]
          if (lastPoints[2].y != POINT_INVALID_Y)
            cbDraw(lastPoints[2].x, lastPoints[2].y);
          drawLine(lastPoints[1].x, lastPoints[1].y, lastPoints[0].x, lastPoints[0].y, cbDraw);
          lastPoints[2].y = POINT_INVALID_Y;
          lastPoints[1].y = POINT_INVALID_Y;
          // lastPoints[0] stays.
        }
      }

      if (delFlag) {
        // remove lastPoints[1];
        lastPoints[1] = lastPoints[2];
        lastPoints[2].y = POINT_INVALID_Y;
      }
    }
  }
};

// ===== Fast but non-precise sin / cos functions

const int32_t tab_sin256[] = {
  0, 4, 9, 13, 18, 22, 27, 31, 35, 40, 44,
  49, 53, 57, 62, 66, 70, 75, 79, 83, 87,
  91, 96, 100, 104, 108, 112, 116, 120, 124, 128,
  131, 135, 139, 143, 146, 150, 153, 157, 160, 164,
  167, 171, 174, 177, 180, 183, 186, 190, 192, 195,
  198, 201, 204, 206, 209, 211, 214, 216, 219, 221,
  223, 225, 227, 229, 231, 233, 235, 236, 238, 240,
  241, 243, 244, 245, 246, 247, 248, 249, 250, 251,
  252, 253, 253, 254, 254, 254, 255, 255, 255, 255
};



/// @brief table drive sin(degree) function
/// @param degree Degree in ruan 0...360
/// @return non-precise sin value as integer in range 0...256
int32_t sin256(int32_t degree) {
  degree = ((degree % 360) + 360) % 360;  // Math. Modulo Operator
  if (degree <= 90) {
    return (tab_sin256[degree]);
  } else if (degree <= 180) {
    return (tab_sin256[90 - (degree - 90)]);
  } else if (degree <= 270) {
    return (-tab_sin256[degree - 180]);
  } else {
    return (-tab_sin256[90 - (degree - 270)]);
  }
}

int32_t cos256(int32_t degree) {
  return (sin256(degree + 90));
}

// ===== Debug helping functions... =====

void dumpPoints(std::vector<Point> &points) {
  GFX_TRACE("\nPoints:");
  size_t size = points.size();
  for (size_t i = 0; i < size; i++) {
    if (i % 10 == 0) {
      if (i > 0) { GFX_TRACE(""); }
      GFX_TRACE("  p%02d:", i);
    }
    GFX_TRACE(" (%2d/%2d)", points[i].x, points[i].y);
  }
  GFX_TRACE("");
}

}  // gfxDraw:: namespace

// End.
