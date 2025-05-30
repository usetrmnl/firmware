// - - - - -
// GFXDraw - A Arduino library for drawing shapes on a GFX display using paths describing the borders.
// gfxdrawPath.h: Header file for functions to calculate all points of a path segment.
//
// Copyright (c) 2024-2024 by Matthias Hertel, http://www.mathertel.de
// This work is licensed under a BSD style license. See http://www.mathertel.de/License.aspx
//
// These pixel oriented drawing functions are implemented to use callback functions for the effective drawing
// to make them independent from an specific canvas or GFX implementation and can be used for drawing and un-drawing.
//
// Filled paths are supported on closed paths only.
//
// The functions have minimized use of float and arc arithmetics.
//
// Changelog:
// * 27.11.2024 creation
//
// - - - - -

#pragma once

#include "gfxDraw.h"

namespace gfxDraw {

// All internal path processing and drawing is based on a vector of Segments `std::vector<Segment>`
// that can be constructed by creating individual segments and adding them to the vector using
//
// * createMove(...);
// * createLine(...);
// * createArc(...);
// * createClose(...);
//
// or by passing textual path definitions to
//
// * parsePath(...)


/// @brief The Segment struct holds all information about a segment of a path.
class Segment {
public:
  enum Type : uint16_t {
    Move = 0x0100 + 2,
    Line = 0x0200 + 2,
    Curve = 0x0300 + 6,
    Arc = 0x0400 + 7,
    Circle = 0x0800 + 3,
    Close = 0xFF00 + 0,
  };

  Segment() = default;
  Segment(Type _type, int16_t x = 0, int16_t y = 0);

  Type type;

  union {
    int16_t p[6];  // for parameter scanning

    struct {  // x,y for Lines, and move
      int16_t x1;
      int16_t y1;
    };

    struct {  // for Arcs
      int16_t rx;
      int16_t ry;
      int16_t rotation;
      int16_t f1f2;
      int16_t xEnd;
      int16_t yEnd;
    };
  };

  static Segment createMove(int16_t x, int16_t y);
  static Segment createMove(Point p);

  static Segment createLine(int16_t x, int16_t y);
  static Segment createLine(Point p);

  static Segment createClose();
  static Segment createArc(int16_t radius, bool f1, bool f2, int16_t xEnd, int16_t yEnd);
};

// ===== create and manipulate segments

/// @brief Scan a path using the svg/path/d syntax to create a vector(array) of Segments.
/// @param pathText path definition as String
/// @return Vector with Segments.
/// @example pathText="M4 8l12-6l10 10h-8v4h-6z"
std::vector<Segment> parsePath(const char *pathText);


/// @brief Draw the border line of a path.
// void kath(const char *pathText, fSetPixel cbDraw);

/// @brief scale all points by factor / base.
/// @param segments Segment vector to be changed
/// @param factor scaling factor
/// @param base scaling base, defaults to 100.
void scaleSegments(std::vector<Segment> &segments, int16_t factor, int16_t base = 100);

/// @brief rotate all points by the given angle.
/// @param segments Segment vector to be changed
/// @param angle angle 0...360
void rotateSegments(std::vector<Segment> &segments, int16_t angle);

/// @brief move all points by the given offset in x and y.
/// @param segments Segment vector to be changed
/// @param dx X-Offset
/// @param dy Y-Offset
void moveSegments(std::vector<Segment> &segments, int16_t dx, int16_t dy);

/// @brief move all points by the given offset in x and y.
/// @param segments Segment vector to be changed
/// @param moveVector X- and Y- Offset as Point-Vector.
void moveSegments(std::vector<Segment> &segments, Point moveVector);


/// @brief Transform all points in the segments
void transformSegments(std::vector<Segment> &segments, fTransform cbTransform);


/// @brief Draw a path without filling.
/// @param segments Vector of the segments of the path.
/// @param cbDraw Callback with coordinates of line pixels.
void drawSegments(std::vector<Segment> &segments, fSetPixel cbDraw);

/// @brief Draw a path with filling.
// void fillSegments(std::vector<Segment> &segments, int16_t dx, int16_t dy, fSetPixel cbBorder, fSetPixel cbFill = nullptr);
void fillSegments(std::vector<Segment> &segments, fSetPixel cbBorder, fSetPixel cbFill = nullptr);


/// @brief draw a path using a border and optional fill drawing function.
/// @param path The path definition using SVG path syntax.
/// @param x Starting Point X coordinate.
/// @param y Starting Point Y coordinate.
/// @param scale scaling factor * 100.
/// @param cbBorder Draw function for border pixels. cbFill is used when cbBorder is null.
/// @param cbFill Draw function for filling pixels.
void pathByText(const char *pathText, int16_t x, int16_t y, int16_t scale100, fSetPixel cbBorder, fSetPixel cbFill);



}  // gfxDraw:: namespace


// End.
