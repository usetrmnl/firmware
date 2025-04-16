// - - - - -
// GFXDraw - A Arduino library for drawing shapes on a GFX display using paths describing the borders.
// gfxDrawBezier.h: Header file for functions to calculate all points of a cubic bezier curve.
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
// * 29.05.2024 creation
//
// - - - - -

#pragma once

#include "gfxDraw.h"


namespace gfxDraw {

// This implementation of cubic bezier curve with a start and an end point given and by using 2 control points.
// C x1 y1, x2 y2, x y
// good article for reading: <https://pomax.github.io/bezierinfo/>
// Here the Casteljau's algorithm approach is used.

void drawCubicBezier(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3, fSetPixel cbDraw);

}  // gfxDraw:: namespace


// End.
