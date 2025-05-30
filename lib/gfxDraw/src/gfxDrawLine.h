// - - - - -
// GFXDraw - A Arduino library for drawing shapes on a GFX display using paths describing the borders.
// gfxDrawLine.h: Header file for functions to calculate all points of a staight line.
//
// Copyright (c) 2024-2024 by Matthias Hertel, http://www.mathertel.de
// This work is licensed under a BSD style license. See http://www.mathertel.de/License.aspx
//
// These pixel oriented drawing functions are implemented to use callback functions for the effective drawing
// to make them independent from an specific canvas or GFX implementation and can be used for drawing and un-drawing.
//
// Changelog:
// * 18.08.2024 creation
//
// - - - - -

#pragma once

#include "gfxDraw.h"

namespace gfxDraw {

/// @brief Draw a line using the most efficient algorithm
/// @param x0 Starting Point X coordinate.
/// @param y0 Starting Point Y coordinate.
/// @param x1 Ending Point X coordinate.
/// @param y1 Ending Point Y coordinate.
/// @param cbDraw Callback with coordinates of line pixels.
/// @param w Width of line.
void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, fSetPixel cbDraw);


/// @brief Draw a line using the most efficient algorithm
/// @param p1 Starting Point
/// @param p2 Ending Point
/// @param cbDraw Callback with coordinates of line pixels.
void drawLine(Point &p1, Point &p2, fSetPixel cbDraw);

}  // gfxDraw:: namespace


// End.
