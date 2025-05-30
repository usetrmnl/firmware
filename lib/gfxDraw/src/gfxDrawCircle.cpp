// - - - - -
// GFXDraw - A Arduino library for drawing shapes on a GFX display using paths describing the borders.
// gfxdrawCircle.cpp: Library implementation file for functions to calculate all points of a circle, circle quadrant and circle segment.
//
// Copyright (c) 2024-2024 by Matthias Hertel, http://www.mathertel.de
// This work is licensed under a BSD style license. See http://www.mathertel.de/License.aspx
//
// Changelog: See gfxdrawCircle.h and documentation files in this library.
//
// - - - - -

#include "gfxDraw.h"

#ifndef GFX_TRACE
#define GFX_TRACE(...)  // GFXDRAWTRACE(__VA_ARGS__)
#endif

namespace gfxDraw {

// ===== internal class definitions =====

// draw a circle
// the draw function is not called in order of the pixels on the circle.


// from: http://members.chello.at/easyfilter/bresenham.html

/// @brief Draw the circle quadrant with the pixels in the given order.
/// @param radius radius of the circle
/// @param q number of quadrant (see header file)
/// @param cbDraw will be called for all pixels in the Circle Quadrant
void drawCircleQuadrant(int16_t radius, int16_t q, fSetPixel cbDraw) {
  GFX_TRACE("drawCircleQuadrant(r=%d)", radius);

  int16_t x = -radius, y = 0;
  int16_t err = 2 - 2 * radius; /* II. Quadrant */

  do {
    if (q == 0) {
      cbDraw(-x, y);
    } else if (q == 1) {
      cbDraw(-y, -x);
    } else if (q == 2) {
      cbDraw(x, -y);
    } else if (q == 3) {
      cbDraw(y, x);
    }
    radius = err;
    if (radius <= y) err += ++y * 2 + 1;            // y step and error adjustment
    if (radius > x || err > y) err += ++x * 2 + 1;  // x step and error adjustment
  } while (x <= 0);
}  // drawCircleQuadrant()


/// @brief draw a circle segment
void drawCircleSegment(Point center, int16_t radius, Point startPoint, Point endPoint, ArcFlags flags, fSetPixel cbDraw) {
  GFX_TRACE("drawCircleSegment(%d/%d r=%d)  (%d/%d) -> (%d/%d)", center.x, center.y, radius, startPoint.x, startPoint.y, endPoint.x, endPoint.y);

  int16_t xm = center.x;
  int16_t ym = center.y;

  if ((startPoint == endPoint) && (flags & ArcFlags::LongPath)) {
    // draw a full circle
    for (int16_t q = 0; q < 4; q++)
      drawCircleQuadrant(radius, q, [&](int16_t x, int16_t y) {
        cbDraw(xm + x, ym + y);
      });
    return;
  }

  if (!(flags & ArcFlags::Clockwise)) {
    // flip vertically and adjust angles to use clockwise processing.
    int16_t ym2 = 2 * center.y;
    drawCircleSegment(
      center, radius,
      Point(startPoint.x, ym2 - startPoint.y),
      Point(endPoint.x, ym2 - endPoint.y),
      (ArcFlags)(flags | ArcFlags::Clockwise),
      [&](int16_t x, int16_t y) {
        cbDraw(x, ym2 - y);
      });
    return;
  }  // if (! clockwise)


  // Clockwise processing only from here.

  // Draw from startPoint to endPoint using Quadrants
  startPoint = startPoint - center;
  endPoint = endPoint - center;

  uint16_t sQ = Point::circleQuadrant(startPoint);
  uint16_t eQ = Point::circleQuadrant(endPoint);

  if (sQ == eQ) {
    if (Point::compareCircle(endPoint, startPoint)) {
      eQ += 4;
    }

  } else if (sQ > eQ) {
    eQ += 4;
  }

  // s=1: before the start
  // s=2: draw pixels
  // s=3: after the end
  uint16_t s = 1;

  for (uint16_t q = sQ; q <= eQ; q++) {
    drawCircleQuadrant(radius, q % 4, [&](int16_t x, int16_t y) {
      if (s == 1) {
        if (Point::compareCircle(startPoint, Point(x, y))) {
          // start drawing the points
          s = 2;
        }

      } else if (s == 2) {
        if (q != eQ) {
          // draw

        } else if (q == eQ) {
          if (Point::compareCircle(endPoint, Point(x, y))) {
            // stop drawing the points
            s = 3;
          }
        }
      }

      if (s == 2) {
        cbDraw(xm + x, ym + y);
      }
    });
  }  // for()
  // } while (q <= eQ);

};  // drawCircleSegment()


// Draw a whole circle. The draw function is not called in order of the pixels on the circle.
void drawCircle(Point center, int16_t radius, fSetPixel cbStroke, fSetPixel cbFill) {
  int16_t xm = center.x;
  int16_t ym = center.y;
  int16_t line = -radius;

  drawCircleQuadrant(radius, 3, [&](int16_t x, int16_t y) {
    // GFX_TRACE(" x=%d y=%d", x, y);
    cbStroke(xm - x, ym + y);
    if ((cbFill) && (y != line)) {
      for (int16_t l = xm - x + 1; l < xm + x; l++) {
        cbFill(l, ym + y);
      }
    }
    cbStroke(xm + x, ym + y);

    if (y < 0) {
      cbStroke(xm - x, ym - y);
      if ((cbFill) && (y != line)) {
        for (int16_t l = xm - x + 1; l < xm + x; l++) {
          cbFill(l, ym - y);
        }
      }
      cbStroke(xm + x, ym - y);
    }
    line = y;
  });
}  // drawCircle()

/// @brief Calculate the center parameterization for an arc from endpoints
/// The radius values may be scaled up when there is no arc possible.
void arcCenter(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t &rx, int16_t &ry, int16_t phi, int16_t flags, int32_t &cx256, int32_t &cy256) {
  // Conversion from endpoint to center parameterization
  // see also http://www.w3.org/TR/SVG11/implnote.html#ArcImplementationNotes

  // <https://github.com/canvg/canvg/blob/937668eced93e0335c67a255d0d2277ea708b2cb/src/Document/PathElement.ts#L491>

  double sinphi = sin(phi);
  double cosphi = cos(phi);

  // middle of (x1/y1) to (x2/y2)
  double xMiddle = (x1 - x2) / 2;
  double yMiddle = (y1 - y2) / 2;

  double xTemp = (cosphi * xMiddle) + (sinphi * yMiddle);
  double yTemp = (-sinphi * xMiddle) + (cosphi * yMiddle);

  // adjust x & y radius when too small
  if (rx == 0 || ry == 0) {
    double dist = sqrt(((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2)));
    rx = ry = (int16_t)(dist / 2);
    GFX_TRACE("rx=ry= %d", rx);

  } else {
    double dist2 = (xTemp * xTemp) / (rx * rx) + (yTemp * yTemp) / (ry * ry);

    if (dist2 > 1) {
      double dist = sqrt(dist2);
      rx = static_cast<int16_t>(std::lround(rx * dist));
      ry = static_cast<int16_t>(std::lround(ry * dist));
    }
    GFX_TRACE("rx=%d ry=%d ", rx, ry);
  }

  // center calculation
  double centerDist = 0;
  double distNumerator = ((rx * rx) * (ry * ry) - (rx * rx) * (yTemp * yTemp) - (ry * ry) * (xTemp * xTemp));
  if (distNumerator > 0) {
    centerDist = sqrt(distNumerator / ((rx * rx) * (yTemp * yTemp) + (ry * ry) * (xTemp * xTemp)));
  }

  if ((flags == 0x00) || (flags == 0x03)) {
    centerDist = -centerDist;
  }

  double cX = (centerDist * rx * yTemp) / ry;
  double cY = (centerDist * -ry * xTemp) / rx;

  double centerX = (cosphi * cX) - (sinphi * cY) + (x1 + x2) / 2;
  double centerY = (sinphi * cX) + (cosphi * cY) + (y1 + y2) / 2;

  cx256 = std::lround(256 * centerX);
  cy256 = std::lround(256 * centerY);
}  // arcCenter()


/// Calculate the angle of a vector in degrees.
int16_t vectorAngle(int16_t dx, int16_t dy) {
  // GFX_TRACE("vectorAngle(%d, %d)", dx, dy);
  double rad = atan2(dy, dx);
  int16_t angle = static_cast<int16_t>(std::lround(rad * 180 / M_PI));
  if (angle < 0) angle = 360 + angle;
  return (angle % 360);
}  // vectorAngle()


/// @brief Draw an arc according to svg path arc parameters.
/// @param x1 Starting Point X coordinate.
/// @param y1 Starting Point Y coordinate.
/// @param x2 Ending Point X coordinate.
/// @param y2 Ending Point Y coordinate.
/// @param rx
/// @param ry
/// @param phi  rotation of the ellipsis
/// @param flags
/// @param cbDraw Callback with coordinates of line pixels.
void drawArc(int16_t x1, int16_t y1, int16_t x2, int16_t y2,
             int16_t rx, int16_t ry,
             int16_t phi, int16_t flags,
             fSetPixel cbDraw) {
  GFX_TRACE("drawArc(%d/%d)-(%d/%d)", x1, y1, x2, y2);

  int32_t cx256, cy256;

  arcCenter(x1, y1, x2, y2, rx, ry, phi, flags, cx256, cy256);

  GFX_TRACE("  flags   = 0x%02x", flags);
  GFX_TRACE("  center = %d/%d", SCALE256(cx256), SCALE256(cy256));
  GFX_TRACE("  radius = %d/%d", rx, ry);

  proposePixel(x1, y1, cbDraw);
  if (rx == ry) {
    // draw a circle segment faster. ellipsis rotation can be ignored.
    gfxDraw::drawCircleSegment(gfxDraw::Point(SCALE256(cx256), SCALE256(cy256)), rx,
                               gfxDraw::Point(x1, y1),
                               gfxDraw::Point(x2, y2),
                               (gfxDraw::ArcFlags)(flags & gfxDraw::ArcFlags::Clockwise),
                               [&](int16_t x, int16_t y) {
                                 proposePixel(x, y, cbDraw);
                               });
  } else {
    int startAngle = vectorAngle(256 * x1 - cx256, 256 * y1 - cy256);
    int endAngle = vectorAngle(256 * x2 - cx256, 256 * y2 - cy256);
    int stepAngle = (flags & 0x02) ? 1 : 359;

    // Iterate through the ellipse
    for (int16_t angle = startAngle; angle != endAngle; angle = (angle + stepAngle) % 360) {
      int16_t x = SCALE256(cx256 + (rx * gfxDraw::cos256(angle)));
      int16_t y = SCALE256(cy256 + (ry * gfxDraw::sin256(angle)));
      proposePixel(x, y, cbDraw);
    }
  }
  proposePixel(x2, y2, cbDraw);

  proposePixel(0, POINT_BREAK_Y, cbDraw);

}  // drawArc()

}  // gfxDraw:: namespace

// End.
