#pragma once

#include "GUI_Paint.h"

/**
 * @brief Draws a scalable SVG path onto a graphical canvas.
 * 
 * This function parses an SVG path, applies transformations such as scaling,
 * rotation, and translation, and then renders the path using specified fill
 * and stroke colors.
 * 
 * @param svgPath      The SVG path string to be drawn.
 * @param pathX        The X-coordinate of the top-left corner of the SVG path's bounding box.
 * @param pathY        The Y-coordinate of the top-left corner of the SVG path's bounding box.
 * @param pathWidth    The width of the SVG path's bounding box.
 * @param pathHeight   The height of the SVG path's bounding box.
 * @param centerX      The X-coordinate of the center point where the path will be drawn.
 * @param centerY      The Y-coordinate of the center point where the path will be drawn.
 * @param width        The desired width of the scaled path.
 * @param fillColor    The color used to fill the path.
 * @param strokeColor  The color used to stroke the path.
 * @param rotation     The rotation angle (in degrees) to apply to the path.
 */

void draw_scalable(const char *svgPath, int16_t pathX, int16_t pathY,
                   int16_t pathWidth, uint16_t pathHeight, int16_t centerX,
                   int16_t centerY, int16_t width, UWORD fillColor = BLACK, UWORD strokeColor = WHITE, float rotation = 0.0);

/**
 * @brief Draws a TRMNL logo image at the specified position with optional rotation.
 * 
 * @param centerX The x-coordinate of the center of the logo.
 * @param centerY The y-coordinate of the center of the logo.
 * @param width The width of the logo.
 * @param rotation The rotation angle of the logo in degrees (default is 0.0).
 */
void drawLogo(int16_t centerX, int16_t centerY, int16_t width, float rotation = 0.0);

/**
 * @brief Draws a TRMNL logo text at the specified position with optional rotation.
 * 
 * @param centerX The x-coordinate of the center of the logo.
 * @param centerY The y-coordinate of the center of the logo.
 * @param width The width of the logo.
 * @param rotation The rotation angle of the logo in degrees (default is 0.0).
 */
 void drawTRMNL(int16_t centerX, int16_t centerY, int16_t width, float rotation = 0.0);

 /**
 * @brief Draws a full TRMNL logo at the specified position with optional rotation.
 * 
 * @param centerX The x-coordinate of the center of the logo.
 * @param centerY The y-coordinate of the center of the logo.
 * @param width The width of the logo.
 * @param rotation The rotation angle of the logo in degrees (default is 0.0).
 */
void drawFullLogo(int16_t centerX, int16_t centerY, int16_t width, float rotation = 0.0);