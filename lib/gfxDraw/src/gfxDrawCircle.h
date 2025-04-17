// - - - - -
// GFXDraw - A Arduino library for drawing shapes on a GFX display using paths describing the borders.
// gfxdrawCircle.h: Header file for functions to calculate all points of a circle, circle quadrant and circle segment.
//
// Copyright (c) 2024-2024 by Matthias Hertel, http://www.mathertel.de
// This work is licensed under a BSD style license. See http://www.mathertel.de/License.aspx
//
// These pixel oriented drawing functions are implemented to use callback functions for the effective drawing
// to make them independent from an specific canvas or GFX implementation and can be used for drawing and un-drawing.
//
// The functions have minimized use of float and arc arithmetics.
// Path drawing is supporting on any given path.
// Filled paths are supported on closed paths only.
//
// Changelog:
// * 22.05.2024 creation 
// * 01.11.2024 full circle with fill 
//
// - - - - -

#pragma once

#include "gfxDraw.h"

namespace gfxDraw {

enum ArcFlags : uint16_t {
  CounterClockwise = 0x00,
  Clockwise = 0x02,
  // ShortPath = 0x00
  LongPath = 0x10,

  FullCircle = LongPath | Clockwise
};

inline ArcFlags operator|(ArcFlags a, ArcFlags b) {
  return static_cast<ArcFlags>(static_cast<int>(a) | static_cast<int>(b));
}


/// ===== Basic draw functions with callback =====

/// @brief draw a whole circle. The draw function is not called in order of the pixels on the circle.
/// @param center center of the circle
/// @param radius radius of the circle
/// @param cbDraw SetPixel callback
void drawCircle(Point center, int16_t radius, fSetPixel cbStroke, fSetPixel cbFill = nullptr);


/// @brief Calculate all points on the specified quadrant of a circle with center 0/0.
/// @param radius radius of the circle
/// @param q number of quadrant (see header file)
/// @param cbDraw will be called for all pixels in the Circle Quadrant
void drawCircleQuadrant(int16_t radius, int16_t q, fSetPixel cbDraw);


/// @brief draw a circle segment or a whole circle.
/// @param center center of the circle
/// @param radius radius of the circle
/// @param startPoint first point of the arc
/// @param endPoint last point of the arc
/// @param clockwise order of pixel drawing.
/// @param cbDraw SetPixel callback
void drawCircleSegment(Point center, int16_t radius, Point startPoint, Point endPoint, ArcFlags flags, fSetPixel cbDraw);


/// @brief Draw an arc using the most efficient algorithm
void drawArc(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t rx, int16_t ry, int16_t phi, int16_t flags, fSetPixel cbDraw);


/// ====== internally used functions - maybe helpful for generating paths

/// @brief Calculate the angle of a vector in degrees
/// @param dx x value of the vector
/// @param dy y value of the vector
/// @return the angle n range 0...359
int16_t vectorAngle(int16_t dx, int16_t dy);


}  // gfxDraw:: namespace




// End.
