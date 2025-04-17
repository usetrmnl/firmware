// - - - - -
// GFXDraw - A Arduino library for drawing shapes on a GFX display using paths describing the borders.
// gfxdrawCommon.h: Library header file
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
// CHANGELOG:
// 15.05.2024  creation of the GFXDraw library.
//
// - - - - -

#pragma once

#ifdef ARDUINO
#include <Arduino.h>
#endif

#include <functional>

namespace gfxDraw {

/// @brief Callback function definition to address a pixel on a display
typedef std::function<void(int16_t x, int16_t y)> fSetPixel;

/// @brief Callback function definition to change a pixel on a display by applying the given color.
typedef std::function<void(int16_t x, int16_t y, ARGB color)> fDrawPixel;

/// @brief Callback function definition to change a pixel on a display by applying the given color.
typedef std::function<ARGB(ARGB color)> fMapColor;


/// @brief Callback function to transform all points in the segments
typedef std::function<void(int16_t &x, int16_t &y)> fTransform;

/// @brief Callback function definition to read a pixel from a display
// typedef std::function<ARGB(int16_t x, int16_t y)> fReadPixel;




/// ===== Points =====

/// POINT_BREAK_Y marks the end of a path when pixels are streamed through a fSetPixel callback.
#define POINT_BREAK_Y INT16_MAX

/// POINT_INVALID_Y marks ivalid pixels that must be ignored for drawing.
#define POINT_INVALID_Y INT16_MAX - 1


/// @brief A Point holds a pixel position and provides some useful static methods.
/// There are 2 "special" locations when the Y value is about INT16_MAX and INT16_MAX-1
/// that cannot be used for drawing.
class Point {
public:
  Point()
    : x(0), y(POINT_INVALID_Y) {};

  Point(int16_t _x, int16_t _y)
    : x(_x), y(_y) {};

  /// @brief X coordinate of the Point
  int16_t x;

  /// @brief Y coordinate of the Point
  int16_t y;

  /// @brief compare function for std::sort to sort points by (y) and ascending (x)
  /// @param p1 first point
  /// @param p2 second point
  /// @return when p1 is lower than p2
  static bool compare(const Point &p1, const Point &p2) {
    if (p1.y != p2.y)
      return (p1.y < p2.y);
    return (p1.x < p2.x);
  };

  friend bool operator==(const Point &p1, const Point &p2) {
    return ((p1.x == p2.x) && (p1.y == p2.y));
  };  // operator=


  /// @brief Add 2 point vectors or move a point by a vector.
  /// @param p1 Point 1
  /// @param p2 Point 2
  /// @return Sum of the 2 point vectors.
  friend Point operator+(const Point &p1, const Point &p2) {
    Point p(p1.x + p2.x, p1.y + p2.y);
    return (p);
  };  // operator+

  /// @brief subtract 2 point vectors or move a point by a opposite vector.
  /// @param p1 Point 1
  /// @param p2 Point 2
  /// @return Difference of the 2 point vectors.
  friend Point operator-(const Point &p1, const Point &p2) {
    Point p(p1.x - p2.x, p1.y - p2.y);
    return (p);
  };  // operator-


  // ===== Circle related functions.

  // Quadrants
  //      y
  //    2 | 3
  //  x---|--->
  //    1 | 0
  //      v

  // return Quadrant of a vector
  static int16_t circleQuadrant(const Point &p) {
    if ((p.x > 0) && (p.y >= 0)) { return (0); }
    if ((p.x <= 0) && (p.y > 0)) { return (1); }
    if ((p.x < 0) && (p.y <= 0)) { return (2); }
    if ((p.x >= 0) && (p.y < 0)) { return (3); }
    return (0);
  }


  /// @brief Compare 2 points on a circle in clockwise direction.
  /// @param p1 Point #1
  /// @param p2 Point #2
  /// @return true if the first is less than the second.
  static bool compareCircle(Point p1, Point p2) {

    if (p1 == p2) return (false);

    int16_t q1 = circleQuadrant(p1);
    int16_t q2 = circleQuadrant(p2);

    if (q1 < q2) return (true);
    if (q1 > q2) return (false);

    if (q1 == 0) {
      return ((p1.x > p2.x) || (p1.y < p2.y));
    } else if (q1 == 1) {
      return ((p1.x > p2.x) || (p1.y > p2.y));
    } else if (q1 == 2) {
      return ((p1.x < p2.x) || (p1.y > p2.y));
    } else if (q1 == 3) {
      return ((p1.x < p2.x) || (p1.y < p2.y));
    }
    return (false);
  }
};


/// @brief Propose a next pixel of the path.
/// @param x X value of the pixel
/// @param y Y value of the pixel
/// @param cbDraw callback function for the effective pixels of the path.
///
/// @details
/// This function buffers up to 3 pixels and analysis for pixel unwanted sequences and missing pixels.
/// * ignore duplicates
/// * remove corner-type pixels
/// * fill missing 1-pixel
/// * draw streight line when more pixels are missing.
void proposePixel(int16_t x, int16_t y, fSetPixel cbDraw);

/// @brief Print a vector of Points on the output.
/// @param points vector of Points.
void dumpPoints(std::vector<Point> &points);

// ===== Fast but non-precise sin / cos functions

int32_t sin256(int32_t degree);

int32_t cos256(int32_t degree);

#define SCALE256(v) ((v + 127) >> 8)


}  // gfxDraw:: namespace


// End.
