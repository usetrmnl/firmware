// - - - - -
// GFXDraw - A Arduino library for drawing shapes on a GFX display using paths describing the borders.
// gfxDraw.h: Library header file
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
// 09.06.2024  extended gfxDrawPathWidget
//
// - - - - -

#pragma once

#ifdef ARDUINO
#include <Arduino.h>
#endif

#include <functional>
#include <vector>
#include <algorithm>

#include <cctype>
#include <string.h>

#define _USE_MATH_DEFINES
#include <math.h>

// Alpha+RGB Color implementation
#include "gfxDrawColors.h"

// Points, Trigonometric functions
#include "gfxDrawCommon.h"

// Lines
#include "gfxDrawLine.h"

// Rectanges
//#include "gfxDrawRect.h"

// Circles and arcs
#include "gfxDrawCircle.h"

// Bezier curves
#include "gfxDrawBezier.h"

// Paths combining all of the above, filling algorithm
#include "gfxDrawPath.h"

// Text
//#include "gfxDrawText.h"

#ifdef ARDUINO
#define GFXDRAWTRACE(fmt, ...) Serial.printf(fmt "\n" __VA_OPT__(, ) __VA_ARGS__)
#else
#define GFXDRAWTRACE(fmt, ...) printf(fmt "\n" __VA_OPT__(, ) __VA_ARGS__)
#endif

// un-comment GFX_TRACE globally here to enable all tracing

// #define GFX_TRACE(...) GFXDRAWTRACE(__VA_ARGS__)

// End.
