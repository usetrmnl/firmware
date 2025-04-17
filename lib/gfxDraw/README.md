# Drawing Vector Graphics on Arduino GFX displays

GFXDraw is a powerful and easy-to-use GUI library for Arduino.  It offers simple elements, powerful path-based vector
drawings, ready to use widgets and visual effects.  The library is made especially for displays that support pixel-based
drawing attached to a board that has enough CPU power and memory to run complex drawing algorithms.

**Note: This is an early version of a new Arduino library.**  
**Publishing was required to link other work to this library at this time.**  

The GFXDraw library is published as an Arduino Library using GitHub.  It can be installed via the Arduino IDE Library
Manager.  You can download the latest version of GFXDraw from GitHub and copy it to Arduino's library folder.

Note that you need to choose a board powerful enough to run GFXDraw and a GFX library supporting the specific hardware.
The drawing algorithms can use GFX libraries as the low-level interface to various displays.  There are various GFX
libraries available in the Arduino Community that support many differend displays.  It is recommended to use a ESP32-S3
chip as first choice.

There are examples that demonstrate how to use the

* [Adafruit GFX](https://github.com/adafruit/Adafruit-GFX-Library)
* [GFX Library for Arduino](https://github.com/moononournation/Arduino_GFX).

as the GFX libraries are only used for sending the pixel values to the displays.

Beyond this the GFXDraw library is not chip specific and it will also compile and run on other hardware.  A Project to
run on Windows creating PNG files by using the Visual Code compiler is also included and can be used for more efficient
development and direct debugging.

The most important part of this library is the capability to draw and transform vectorized based graphics specified by a
path of lines, arcs and curves using the svg path notation and a set of high-level classes to draw typical widgets.
This enables complex figures to be drawn and you are no longer limited by the typical built-in graphics of a GFX
library.

In contrast to the rasterization implementations used in Desktop programs or browsers this library is dedicated to
microprocessor conditions with typically limited memory and cpu power. The library avoids arithmetic floating point calculations for many algorithms and doesn't support anti-aliased drawing.

There is still some room for improvement in some drawing algoriothm that is left for further updates.


## Software Architecture

This is a short high level overview on the way how the gfxDraw library is used by sketches or examples, how it works
internally and how the GFX libraries are linked for low level drawing.

The library uses the Namespace `gfxDraw` to implement the types, functions and the classes to avoid conflicts with other
libraries.

![architecture](docs/architecture.drawio.svg)

**Primitives** -- The simple drawing functionality available is covering **lines**, **rectangles**, **arcs**, **circles**, **bezier
curves** and some direct usable combinations like rounded rectangles.  These function "only" calculate the points that
make up the primitive and use callback function to hand them over to further processing or drawing.

**Filling** -- One of the further processing function available is the filling algorithm that can find out what pixels
are inside a closed area and also pass them for further processing or drawing.

**Path** -- For drawings with a more complex border paths with the syntax from svg can be used for defining the border
lines.  Paths can be transformed, resized and rotated.  See further details below.

**Widgets** -- The Widget classes offers defining parameters for drawing of complex functionality like **gauges**.


### General Library Implementation Rules

This library aims to support graphics implementations that are used in microprocessors like ESP32 based boards with
pixel oriented graphic displays.

The functions are optimized for low resolution pixel displays. They do not implement antialiasing and have minimized use of float
and arc arithmetics.  

The library supports up to 16 bit (-32760 ... +32760) display resolutions.

By design the drawing functionionality is independent of the color depth.

The Widget Classes and the internal Bitmap Class supports drawing using a 32-bit (or less) color setup using RGB+Alpha
for a specific pixel.

The output of the drawing routined in the library is prvided by a callback function.
This can be "plugged" not ony to the GFX library for drawing but some "filters" are available also to
support functionality like saving the output in memory or filling a widget with a gradient.


## Drawing on a display

The easiest way to use the output of the drawing routines is by using helper function for drawing a fixed color.
This function then calls the effective drawPixel function of the GFX library.

Then a rectagle  with no border and a SILVER fill color or any other algorithm can be drawn using this callback function:

```cpp
using namespace gfxDraw; // use gfxDraw library namespace

// a callback function implementing the fSetPixel parameters 
drawSilver(int16_t x, int16_t y) {
  gfx->drawPixel(x, y, SILVER);
})

// draw a background rectangle
gfxDraw::drawRect(8, 8, 60, 40, nullptr, drawSilver);

// A SVG path defining the shape of a heard
const char *heardPath = "M43 7 a1 1 0 00-36 36l36 36 36-36a1 1 0 00-36-36z";

gfxDraw::pathByText(heardPath, 8, 8, 100, nullptr, drawSilver);
```

A more flexible way is by providing a factory function that returns a fSetPixel function that draws the color given by
an argument:

```cpp
std::function<void(int16_t x, int16_t y)> drawColor(uint32_t color) {
  return [color](int16_t x, int16_t y) {
    gfx->drawPixel(x, y, color);
  };
}

// A SVG path defining the shape of a heard
const char *heardPath = "M43 7 a1 1 0 00-36 36l36 36 36-36a1 1 0 00-36-36z";

gfxDraw::pathByText(heardPath, 8, 8, 100, drawColor(RED), drawColor(YELLOW));
```

When using Widgets the colors are given by the widget implementation to a different function that takes the color as an
additional parameter:

```cpp
void draw(int16_t x, int16_t y, gfxDraw::ARGB color) {
  gfx->drawPixel(x, y, col565(color));
}

// assemble the configuration for the widget including colors
_gConfig = new gfxDraw::gfxDrawGaugeConfig();
_gConfig->pointerColor = ARGB_BLACK;
_gConfig->segmentColor = ARGB_SILVER;

// create the widget
_gWidget = new gfxDraw::gfxDrawGaugeWidget(_gConfig);

// draw
_gWidget->draw(draw);
```


## Widget classes

The Widget classes offers further functionionality and espacially can handle a fixed color for stroke and fill.

By creating a instance on a class the basic configuration can be passed to the constructor and parameters can be
changed by using methods.

```cpp
using namespace gfxDraw; // use gfxDraw library namespace

// A SVG path defining the shape of a heard
const char *heardPath = "M43 7 a1 1 0 00-36 36l36 36 36-36a1 1 0 00-36-36z";

// use red fill and yellow border color
gfxDraw::gfxDrawPathConfig conf = {
  .strokeColor = gfxDraw::ARGB_YELLOW,
  .fillColor = gfxDraw::ARGB_RED
};

drawCallback(int16_t x, int16_t y, uint32_t color) {
  gfx->setPixel(x, y, color);
})

gfxDraw::gfxDrawPathWidget *heardWidget = new gfxDraw::gfxDrawPathWidget();
heardWidget->setConfig(&conf);
heardWidget->setPath(heardPath);
heardWidget->rotate(45);
heardWidget->move(8, 8);
heardWidget->draw(drawCallback);
```

See [Path Widgets Class](docs/path-widget.md).


## SVG Path Syntax

Drawing using paths is used by some of the widgets to customize e.g.  the pointers for a clock.

To create a vector (array) of segments that build the borders of the vector graphics object the `path` syntax from the
SVG standard is used.  

There are helpful web applications to create or edit such paths definitions:

* The [SVG Path Editor](https://yqnn.github.io/svg-path-editor/) with source available in
  [Github/Yqnn](https://github.com/Yqnn/svg-path-editor) from Yann Armelin

* The [SVG Path Editor](https://aydos.com/svgedit/) with source available in
  [Github/aydos](https://github.com/aydos/svgpath) from Fahri Aydos.

Both tools let you directly change the individual segments of paths and also offer some graphical view or even edit
capabilities.  You can also use full SVG editors and extract the path from there.

For using paths with pixel oriented displays is is important to use integer based coordinates and scalar values only.
Better to use larger numbers as scaling the result down to a smaller size is possible.


Examples for paths are:

* Simple Line: `"M 0,0 L 30,40"`
* Rectange: `"M 10,10 h40 v30 h-40 z"`
* Bezier Curve: `"M16 12c5 0 5 7 0 7"`
* Smiley:  
    `"M12,2h64c4,0 8,4 8,8v48c0,4 -4,8 -8,8h-64c-4,0 -8,-4 -8,-8v-48c0,-4 4,-8 8,-8z"`  
    `"M12,10 h60v20h-60z"`  
    `"M24,36c6,0 12,6 12,12 0,6 -6,12 -12,12-6,0 -12,-6 -12,-12 0,-6 6,-12 12,-12z"`  

More details about the implementation can be found in

[Line Command](docs/line_command.md)
[Bezier Arc Command](docs/bezier_command.md)
[Elliptical Arc Command](docs/elliptical_arc_command.md)
[Filling Paths](docs/filling.md)

Implemented Widgets

[Path Widget](docs/path-widget.md)
[Gauge Widget](docs/gauge-widget.md)


## Debugging and logging

In case something is not going as expected it is recommended to enable the optional TRACE output of the library.  This
can be done using one of these approaches:

All source code files (*.cpp) contain a definition of a `GFXDRAWTRACE` macro that can easily be enabled to print to the
Serial port.  To enable all Trace ouput in one place the `GFXDRAWTRACE` macro in gfxdraw.h can be enabled too.  Please
not that this may cause massive performance impacts and I/O that may hurt your application.


* <https://github.com/lvgl/lvgl>
* <https://docs.lvgl.io/latest/en/html/index.html>
* <https://docs.lvgl.io/master/details/integration/framework/arduino.html>


## Contributions

Contributions are welcome.  Please use GitHub Issues for discussing and Pull Request.  Need more -- just ask.


## The Examples

The examples that come with this library demonstrate on how to use gfxdraw in several situations and project setup.

* using Adafruit GFX
* using the GFX Library for Arduino
* draw primitives
* path drawing and transformation functions

<!-- ### Adafruit GFX Example

This example demonstrates how to use gfxDraw with the
[Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library) that is on of the most often used GFX libraries
for Arduino. -->


### Moon GFX Example

This example demonstrates how to use gfxDraw with the
[GFX Library for Arduino](https://github.com/moononournation/Arduino_GFX) that has some excellent support for devices
based on the ESP32 chips and graphics displays.

The example shows various drawing outputs in a loop.


### VSCode PNG Example

In the `examples/png` folder you can find a implementation for using the library by a windows executable to produce
several png files with test images.

This example is especially helpful while engineering the library with fast turn-around cycles and debugging
capabilities.


<!-- ### gfxprimitives Example

To draw the paths the primitive functions for drawing lines, arc and curves are part of the library and also can be used directly. -->


<!-- ### gfxsegments Example

After parsing the path syntax a list (vector) of segments is created than can be used for transformations and drawing.

This example shows how to draw using these functions. They are also used by the gfxDrawWidget implementation. -->

## Fonts and Text output

The fonts provided with this library can be loaded from program memory.

<!-- or as files from a filesystem.  -->

The drawing functionality for fonts was re-implemented to ensure that all characters of the same font have the same
baseline and total height.  Positioning of the text is done by providing the upper left pixel as a starting point.

Drawing text is not supporting a background color.


## Copyright for Adafruit_GFX

This library uses the font defintions from the Adafruit_GFX library that can be found at
<https://github.com/adafruit/Adafruit-GFX-Library>.  Therefore the compatible fonts can be used.

The Adafruit_GFX library was publised by also using the BSD license:

> Software License Agreement (BSD License)
>
> Copyright (c) 2012 Adafruit Industries.  All rights reserved.
>
> Redistribution and use in source and binary forms, with or without
> modification, are permitted provided that the following conditions are met:
>
> * Redistributions of source code must retain the above copyright notice,
>   this list of conditions and the following disclaimer.
> * Redistributions in binary form must reproduce the above copyright notice,
>   this list of conditions and the following disclaimer in the documentation
>   and/or other materials provided with the distribution.
>
> THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
> AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
> IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
> ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
> LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
> CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
> SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
> INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
> CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
> ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
> POSSIBILITY OF SUCH DAMAGE.


## See also

* [Bresenham efficient drawing functions](http://members.chello.at/easyfilter/bresenham.html)
* [Wikipedia Bresenham Algorithm](https://de.wikipedia.org/wiki/Bresenham-Algorithmus)
* [Wikipedia Scanline Rendering](https://en.wikipedia.org/wiki/Scanline_rendering)
* [Math background for using transformation matrixes in 2D drawings](https://www.matheretter.de/wiki/homogene-koordinaten)
* [SVG Game Icons](https://game-icons.net/)
* [SVG IoT Icons](https://github.com/HomeDing/WebFiles/tree/master/i)
