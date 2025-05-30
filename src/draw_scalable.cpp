#include <GUI_Paint.h>
#include <gfxDrawPath.h>

#include <string>

void draw_scalable(const char *svgPath, int16_t pathX, int16_t pathY,
                   int16_t pathWidth, uint16_t pathHeight, int16_t centerX,
                   int16_t centerY, int16_t width,  UWORD fillColor, UWORD strokeColor, float rotation) {
  const float scale = width / (float)pathWidth;
  std::vector<gfxDraw::Segment> drawing = gfxDraw::parsePath(svgPath);
  int16_t pathCenterX = pathX + (pathWidth / 2.0);
  int16_t pathCenterY = pathY + (pathHeight / 2.0);
  gfxDraw::moveSegments(drawing, -pathCenterX, -pathCenterY);
  gfxDraw::scaleSegments(drawing, scale * 100);
  gfxDraw::rotateSegments(drawing, rotation);
  gfxDraw::moveSegments(drawing, centerX, centerY);
  gfxDraw::fillSegments(
      drawing, [&](int16_t x, int16_t y) { Paint_SetPixel(x, y, strokeColor); },
      [&](int16_t x, int16_t y) { Paint_SetPixel(x, y, fillColor); });
}

// from official svg viewbbox 0 0 519 107
const int16_t total_width = 519;
const int16_t total_height = 107;
const int16_t logo_width = 130;
const char *logo =
    "M3 45l8 13L37 42L29 30L3 45Z M17 88l14 2l4-30l-14-2l-4 30Z m42 17L70 94 "
    "L49 73L38 83l21 22Z M98 82L97 67L67 70l1 15l30-3Z m7-45l-13-8L76 54l12 8 "
    "l17-25Z M74 4L60 9l9 28l14-4L74 4Z M29 8L23 21L52 32l5-14L29 8Z";

const int16_t trmnltext_width = 395;
const char *trmnltext =
    "M125 37V26h64V37H163V82H151V37H125Z "
    "M202 26V82H214V62H247C249 62 251 63 252 64 C 253 65 253 66 253 "
    "68V82H265V67C265 63 264 60 262 59 C 261 57 259 57 258 56 C 260 55 261 54 "
    "263 52 C 265 50 266 46 266 43 C 266 37 264 33 260 30 C 257 27 251 26 244 "
    "26H202Z M242 52H214V36H242C246 36 249 37 251 38 C 252 39 253 41 253 44 C "
    "253 47 252 49 250 50 C 249 51 246 52 242 52Z "
    "M281 82V26H298L322 68 347 26H364V82H351V41L328 82H317L293 41V82H281Z "
    "M397 26H380V82H393V39L432 82H448V26H436V69L397 26Z "
    "M477 26H465V82H517V71H477V26Z";

void drawLogo(int16_t centerX, int16_t centerY, int16_t width, float rotation) {
  draw_scalable(logo, 0, 0, logo_width, total_height, centerX, centerY,
                        width, BLACK, BLACK, rotation);
}

void drawTRMNL(int16_t centerX, int16_t centerY, int16_t width, 
               float rotation) {
  draw_scalable(trmnltext, 125, 0, trmnltext_width, total_height, centerX,
                centerY, width, BLACK, BLACK, rotation);
}

void drawFullLogo(int16_t centerX, int16_t centerY, int16_t width,
                  float rotation) {
  String trmnlfulllogo = String(logo) + String(trmnltext);
  draw_scalable(trmnlfulllogo.c_str(), 0, 0, total_width, total_height, centerX,
                centerY, width, BLACK, BLACK, rotation);
}