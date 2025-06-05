#include "DEV_Config.h"
#include "EPD.h"
#include "GUI_Paint.h"
#include "png_flip.h"
#include <Arduino.h>
#include <ImageData.h>
#include <config.h>
#include <display.h>
#include <drawfont.h>
#include <fonts.h>
#include <trmnl_log.h>

/**
 * @brief Function to init the display
 * @param none
 * @return none
 */
void display_init(void) {
  Log_info("dev module start");
  DEV_Module_Init();
  Log_info("dev module end");

  Log_info("screen hw start");
  EPD_7IN5_V2_Init_New();
  Log_info("screen hw end");
}

/**
 * @brief Function to reset the display
 * @param none
 * @return none
 */
void display_reset(void) {
  Log_info("e-Paper Clear start");
  EPD_7IN5_V2_Clear();
  Log_info("e-Paper Clear end");
  // DEV_Delay_ms(500);
}

/**
 * @brief Function to read the display height
 * @return uint16_t - height of display in pixels
 */
uint16_t display_height() { return EPD_7IN5_V2_HEIGHT; }

/**
 * @brief Function to read the display width
 * @return uint16_t - width of display in pixels
 */
uint16_t display_width() { return EPD_7IN5_V2_WIDTH; }

/**
 * @brief Function to draw multi-line text onto the display
 * @param x_start X coordinate to start drawing
 * @param y_start Y coordinate to start drawing
 * @param message Text message to draw
 * @param max_width Maximum width in pixels for each line
 * @param font_width Width of a single character in pixels
 * @param color_fg Foreground color
 * @param color_bg Background color
 * @param font Font to use
 * @param is_center_aligned If true, center the text; if false, left-align
 * @return none
 */
void Paint_DrawMultilineText(UWORD x_start, UWORD y_start, const char *message,
                             uint16_t max_width, uint16_t font_width,
                             UWORD color_fg, UWORD color_bg, sFONT *font,
                             bool is_center_aligned) {
  uint16_t display_width_pixels = max_width;
  int max_chars_per_line = display_width_pixels / font_width;

  uint8_t MAX_LINES = 4;

  char lines[MAX_LINES][max_chars_per_line + 1] = {0};
  uint16_t line_count = 0;

  int text_len = strlen(message);
  int current_width = 0;
  int line_index = 0;
  int line_pos = 0;
  int word_start = 0;
  int i = 0;
  char word_buffer[max_chars_per_line + 1] = {0};
  int word_length = 0;

  while (i <= text_len && line_index < MAX_LINES) {
    word_length = 0;
    word_start = i;

    // Skip leading spaces
    while (i < text_len && message[i] == ' ') {
      i++;
    }
    word_start = i;

    // Find end of word or end of text
    while (i < text_len && message[i] != ' ') {
      i++;
    }

    word_length = i - word_start;
    if (word_length > max_chars_per_line) {
      word_length = max_chars_per_line; // Truncate if word is too long
    }

    if (word_length > 0) {
      strncpy(word_buffer, message + word_start, word_length);
      word_buffer[word_length] = '\0';
    } else {
      i++;
      continue;
    }

    int word_width = word_length * font_width;

    // Check if adding the word exceeds max_width
    if (current_width + word_width + (current_width > 0 ? font_width : 0) <=
        display_width_pixels) {
      // Add space before word if not the first word in the line
      if (current_width > 0 && line_pos < max_chars_per_line - 1) {
        lines[line_index][line_pos++] = ' ';
        current_width += font_width;
      }

      // Add word to current line
      if (line_pos + word_length <= max_chars_per_line) {
        strcpy(&lines[line_index][line_pos], word_buffer);
        line_pos += word_length;
        current_width += word_width;
      }
    } else {
      // Current line is full, draw it
      if (line_pos > 0) {
        lines[line_index][line_pos] = '\0'; // Null-terminate the current line
        line_index++;
        line_count++;

        if (line_index >= MAX_LINES) {
          break;
        }

        // Start new line with this word
        strncpy(lines[line_index], word_buffer, word_length);
        line_pos = word_length;
        current_width = word_width;
      } else {
        // Single long word case
        strncpy(lines[line_index], word_buffer, max_chars_per_line);
        lines[line_index][max_chars_per_line] = '\0';
        line_index++;
        line_count++;
        line_pos = 0;
        current_width = 0;
      }
    }

    // Move to next word
    if (message[i] == ' ') {
      i++;
    }
  }

  // Store the last line if any
  if (line_pos > 0 && line_index < MAX_LINES) {
    lines[line_index][line_pos] = '\0';
    line_count++;
  }

  // Draw the lines
  for (int j = 0; j < line_count; j++) {
    uint16_t line_width = strlen(lines[j]) * font_width;
    uint16_t draw_x = x_start;

    if (is_center_aligned) {
      if (line_width < max_width) {
        draw_x = x_start + (max_width - line_width) / 2;
      }
    }

    Paint_DrawString_EN(draw_x, y_start + j * (font->Height + 5), lines[j],
                        font, color_fg, color_bg);
  }
}

/**
 * @brief Function to show the image on the display
 * @param image_buffer pointer to the uint8_t image buffer
 * @param reverse shows if the color scheme is reverse
 * @return none
 */
void display_show_image(uint8_t *image_buffer, bool reverse, bool isPNG) {
  auto width = display_width();
  auto height = display_height();
  //  Create a new image cache
  UBYTE *BlackImage;
  /* you have to edit the startup_stm32fxxx.s file and set a big enough heap
   * size */
  UWORD Imagesize = ((width % 8 == 0) ? (width / 8) : (width / 8 + 1)) * height;

  Log_error("free heap - %d", ESP.getFreeHeap());
  Log_error("free alloc heap - %d", ESP.getMaxAllocHeap());
  Paint_NewImage(image_buffer, width, height, 0, WHITE);

  Log_info("show image for array");
  if (reverse) {
    Log_info("inverse the image");
    for (size_t i = 0; i < DISPLAY_BMP_IMAGE_SIZE; i++) {
      image_buffer[i] = ~image_buffer[i];
    }
  }
  if (isPNG == true) {
    Log_info("Drawing PNG");
    flip_image(image_buffer, width, height);
    horizontal_mirror(image_buffer, width, height);
    Paint_DrawBitMap(image_buffer);
  } else {
    Paint_DrawBitMap(image_buffer + 62);
  }
  EPD_7IN5_V2_Display(image_buffer);
  Log_info("display");
}

// utility function to display text in a rectangle
void DisplayShowMessage(int16_t x, int16_t y, uint16_t width, uint16_t height,
                        const char *message[], uint16_t numtext,
                        JUSTIFICATION justification) {
  const GFXfont *font = Paint_GetFont();
#if defined(DEBUG_TEXT_LAYOUT)
  Log_info(" %d %d %dx%d", x, y, width, height);
  Paint_DrawRectangle(x, y, x + width, y + height, BLACK, DOT_PIXEL_2X2,
                      DRAW_FILL_EMPTY);
  Paint_DrawLine(x, y, x + width, y + height, BLACK, DOT_PIXEL_DFT,
                 LINE_STYLE_DOTTED);

  Paint_DrawLine(x, y + height, x + width, y, BLACK, DOT_PIXEL_DFT,
                 LINE_STYLE_DOTTED);
#endif
  uint16_t textheight = numtext * font->yAdvance;
  // find max width
  uint16_t textwidth = 0;
  for (uint16_t t = 0; t < numtext; ++t) {
    uint16_t twidth = 0, theight = 0;
    Paint_GetTextBounds((const uint8_t *)message[t], &twidth, &theight);
    if (twidth > textwidth)
      textwidth = twidth;
  }
  // center in the whole box
  int16_t newx = x + (width - textwidth) / 2;
  int16_t newy = y + (height - textheight) / 2;
  uint16_t newwidth = textwidth;
  uint16_t newheight = textheight;
#if defined(DEBUG_TEXT_LAYOUT)
  Paint_DrawRectangle(newx, newy, newx + newwidth, newy + newheight, BLACK,
                      DOT_PIXEL_DFT, DRAW_FILL_EMPTY);

#endif

  int16_t ypos = newy;
  for (uint8_t t = 0; t < numtext; ++t) {
    int16_t xpos = 0;
    uint16_t twidth, theight;
    Paint_GetTextBounds((const uint8_t *)message[t], &twidth, &theight);
    switch (justification) {
    case LEFT:
      xpos = newx;
      break;
    case RIGHT:
      xpos = newx + textwidth - twidth;
      break;
    case CENTER:
      xpos = newx + ((textwidth - twidth) / 2);
      break;
    }
#if defined(DEBUG_TEXT_LAYOUT)
    Paint_DrawPoint(xpos, ypos, BLACK, DOT_PIXEL_3X3, DOT_FILL_RIGHTUP);
    Paint_DrawRectangle(xpos, ypos, xpos + twidth, ypos + theight, BLACK,
                        DOT_PIXEL_DFT, DRAW_FILL_EMPTY);
#endif
    ypos +=
        Paint_DrawText(xpos, ypos, (const uint8_t *)message[t], BLACK, LEFT);
  }
}

const char *hex = "0123456789ABCDEF";
void BMPToCustomSerial(const char *name, const uint8_t *data) {
  Serial.write("BEG-");
  Serial.write(name);
  Serial.write("-");
  for (uint32_t i = 0; i < 48000; ++i) {
    uint8_t byte = data[i];
    Serial.write(hex[(byte >> 4) & 0xF]);
    Serial.write(hex[byte & 0xF]);
  }
  Serial.write("-END");
}

/**
 * @brief Function to show the image with message on the display
 * @param image_buffer pointer to the uint8_t image buffer
 * @param message_type type of message that will show on the screen
 * @return none
 */
void display_show_msg(uint8_t *image_buffer, MSG message_type) {
  auto width = display_width();
  auto height = display_height();

  // uss image_buffer
  Log_info("Paint_NewImage");
  Paint_NewImage(image_buffer + 62, width, height, 0, WHITE);

  uint16_t logoheight = 100;
  uint16_t margin = 20;
  // message bbox is computed there
  // center text horizontally and start after the logo
  int16_t xpos = margin;
  uint16_t availablewidth = display_width() - (2 * margin);
  int16_t ypos = (display_height() / 2) + (logoheight / 2) + margin;
  uint16_t availableheight = (display_height() - logoheight) / 2 - (2 * margin);

  switch (message_type) {
  case WIFI_CONNECT: {
    const char *strings[] = {"Connect to TRMNL WiFi",
                             "on your phone or computer"};
    uint8_t numtext = sizeof(strings) / sizeof(const char *);
    DisplayShowMessage(xpos, ypos, availablewidth, availableheight, strings,
                       numtext, CENTER);
  } break;
  case WIFI_FAILED: {
    const char *strings[] = {
        "can't establish WiFi", "connection. Hold button on",
        "the back to reset WiFi", "or scan QR Code for help."};
    uint8_t numtext = sizeof(strings) / sizeof(const char *);
    uint16_t qrcodesize = 130;
    availablewidth = display_width() - qrcodesize - 3 * margin;
    DisplayShowMessage(xpos, ypos, availablewidth, availableheight, strings,
                       numtext, CENTER);
    // nicoclean32
    // DisplayShowMessage(margin, margin, display_width() - 2 * margin,
    // display_height() - 2 * margin, strings, numtext, CENTER);
    int16_t qrcodex = xpos + availablewidth + margin;
    int16_t qrcodey = ypos + (availableheight / 2) - (qrcodesize / 2);
    Paint_DrawImage(wifi_failed_qr, qrcodex, qrcodey, qrcodesize, qrcodesize);
  } break;
  case WIFI_INTERNAL_ERROR: {
    const char *strings[] = {"WiFi connected, but", "API connection cannot be",
                       "established. Try to refresh,"
                       "or scan QR Code for help."};
    DisplayShowMessage(xpos, ypos, availablewidth, availableheight, strings,
                       sizeof(strings) / sizeof(char *), CENTER);
    Paint_DrawImage(wifi_failed_qr, 640, 337, 130, 130);
  } break;
  case WIFI_WEAK: {
    const char *strings[] = {"WiFi connected but signal is weak"};
    DisplayShowMessage(xpos, ypos, availablewidth, availableheight, strings,
                       sizeof(strings) / sizeof(char *), CENTER);
    } break;
  case API_ERROR: {
    const char *strings[] = {"WiFi connected, TRMNL not responding.","Short click the button on back,", "otherwise check your internet."};
    DisplayShowMessage(xpos, ypos, availablewidth, availableheight, strings,
                       sizeof(strings) / sizeof(char *), CENTER);
  } break;
  case API_SIZE_ERROR: {
    const char *strings[] = {
      "WiFi connected, TRMNL content malformed.", "Wait or reset by holding button on back." };
    DisplayShowMessage(xpos, ypos, availablewidth, availableheight, strings,
                       sizeof(strings) / sizeof(char *), CENTER);
  } break;
  case FW_UPDATE: {
    const char *strings[] = {"Firmware update available! Starting now..."};
    DisplayShowMessage(xpos, ypos, availablewidth, availableheight, strings,
                       sizeof(strings) / sizeof(char *), CENTER);
  } break;
  case FW_UPDATE_FAILED: {
    const char *strings[] = {"Firmware update failed. Device will restart..."};
    DisplayShowMessage(xpos, ypos, availablewidth, availableheight, strings,
                       sizeof(strings) / sizeof(char *), CENTER);
  } break;
  case FW_UPDATE_SUCCESS: {
    const char *strings[] = {"Firmware update success. Device will restart.."};
    DisplayShowMessage(xpos, ypos, availablewidth, availableheight, strings,
                       sizeof(strings) / sizeof(char *), CENTER);
  } break;
  case BMP_FORMAT_ERROR: {
    const char *strings[] = {"The image format is incorrect"};
    DisplayShowMessage(xpos, ypos, availablewidth, availableheight, strings,
                       sizeof(strings) / sizeof(char *), CENTER);
  } break;
  case TEST: {
    displayCharset();
  } break;
  default:
    break;
  }
#if defined(DEBUG_TEXT)
    BMPToCustomSerial("show_msg.bmp", image_buffer);
#endif
    EPD_7IN5_V2_Display(image_buffer);
    Log_info("display");
  }

  /**
   * @brief Function to show the image with message on the display
   * @param image_buffer pointer to the uint8_t image buffer
   * @param message_type type of message that will show on the screen
   * @param friendly_id device friendly ID
   * @param id shows if ID exists
   * @param fw_version version of the firmware
   * @param message additional message
   * @return none
   */
  void display_show_msg(uint8_t * image_buffer, MSG message_type,
                        String friendly_id, bool id, const char *fw_version,
                        String message) {
    if (message_type == WIFI_CONNECT) {
      Log_info("Display set to white");
      EPD_7IN5_V2_ClearWhite();
      delay(1000);
    }

    auto width = display_width();
    auto height = display_height();
    Log_info("Paint_NewImage");
    Paint_NewImage(image_buffer, width, height, 0, WHITE);

 uint16_t logoheight = 100;
  uint16_t margin = 20;
  // message bbox is computed there
  // center text horizontally and start after the logo
  int16_t xpos = margin;
  uint16_t availablewidth = display_width() - (2 * margin);
  int16_t ypos = (display_height() / 2) + (logoheight / 2) + margin;
  uint16_t availableheight = (display_height() - logoheight) / 2 - (2 * margin);

    Log_info("show image for array");
    switch (message_type) {
    case FRIENDLY_ID: {
      Log_info("friendly id case");
       String string2 = "with Friendly ID ";
      if (id) {
        string2 += friendly_id;
      }
      const char* id = friendly_id.c_str();
      const char* strings[] = { "Please sign up at usetrmnl.com/signup", "with Friendly ID ", id, " to finih setup" };
      DisplayShowMessage(xpos, ypos, availablewidth, availableheight, strings,
                       sizeof(strings) / sizeof(char *), CENTER);
      } break;
    case WIFI_CONNECT: {
      Log_info("wifi connect case");
      String string1 = String("FW: ") + fw_version;
      const char* strings[] = {  string1.c_str(), "Connect phone or computer", "to \"TRMNL\" WiFi network", "or scan QR code for help." };
      DisplayShowMessage(xpos, ypos, availablewidth, availableheight, strings,
                       sizeof(strings) / sizeof(char *), CENTER);
      Paint_DrawImage(wifi_connect_qr, 640, 337, 130, 130);
    } break;
    case MAC_NOT_REGISTERED: {
      UWORD y_start = 340;
      Paint_DrawMultilineText(0, y_start, message.c_str(), width, Font24.Width,
                              WHITE, BLACK, &Font24, true);
    } break;
    default:
      break;
    }
    Log_info("Start drawing...");
    EPD_7IN5_V2_Display(image_buffer);
    Log_info("display");
  }

  /**
   * @brief Function to got the display to the sleep
   * @param none
   * @return none
   */
  void display_sleep(void) {
    Log_info("Goto Sleep...");
    EPD_7IN5B_V2_Sleep();
  }
