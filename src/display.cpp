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

#if defined(DEBUG_TEXT)
void display_debug_text();
#endif

uint16_t logofullheight = 80;
uint16_t fw_offset = 0;
uint16_t logoheight = 40;
uint16_t logowidth = 384;
int16_t marginx = 20;
int16_t marginy = 20;
int16_t marginbottom = 20; // some pixels are hidden
uint16_t qrcodesize = 130;

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

#if defined(DEBUG_TEXT)
  display_debug_text();
#endif
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
 * @param is_center_aligned If tru// the text; if false, left-align
 * @return none
 */
void Paint_DrawMultilineTextOld(UWORD x_start, UWORD y_start,
                                const char *message, uint16_t max_width,
                                uint16_t font_width, UWORD color_fg,
                                UWORD color_bg, sFONT *font,
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
  UWORD Imagesize = ((width % 8 == 0) ? (width / 8) : (width / 8 + 1)) * height;

  Log_error("free heap - %d", ESP.getFreeHeap());
  Log_error("free alloc heap - %d", ESP.getMaxAllocHeap());
  if ((BlackImage = (UBYTE *)malloc(Imagesize)) == NULL) {
    Log_fatal("Failed to apply for black memory...");
    ESP.restart();
  }
  Log_info("Paint_NewImage %d", reverse);

  Paint_NewImage(BlackImage, width, height, 0, WHITE);

  Log_info("show image for array");
  Paint_SelectImage(BlackImage);
  Paint_Clear(WHITE);
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
  EPD_7IN5_V2_Display(BlackImage);
  Log_info("Display done");

  free(BlackImage);
  BlackImage = NULL;
}

static char fw_version[20] = {0};
const char *GetFirmwareVersionString() {
  if (fw_version[0] == 0) {
    sprintf((char *)fw_version, "%d.%d.%d", FW_MAJOR_VERSION, FW_MINOR_VERSION,
            FW_PATCH_VERSION);
  }
  Log_info("GetFirmwareVersionString %s", fw_version);
  return fw_version;
}

static void Paint_FirmwareVersion() {
  // place firmware on logo
  const char *fw_text = GetFirmwareVersionString();
  uint16_t fw_width = 0;
  uint16_t fw_height = 0;
  uint16_t lineheight = 0;
  uint16_t fw_lines = 1;
  Paint_GetTextInfo(fw_text, &fw_lines, &fw_width, &fw_height, &lineheight);

  int16_t fw_centerx =
      logowidth + (display_width() - logowidth) / 2 - fw_width / 2;
  int16_t fw_centery =
      (display_height() / 2) + logoheight / 2 + fw_offset + fw_height / 2;

#if defined(DEBUG_TEXT) && defined(DEBUG_TEXT_LAYOUT)
  Paint_DrawLine(0, display_height() / 2, display_width(), display_height() / 2,
                 BLACK, DOT_PIXEL_DFT, LINE_STYLE_SOLID);
  Paint_DrawLine(0, fw_centery, display_width(), fw_centery, BLACK,
                 DOT_PIXEL_DFT, LINE_STYLE_DOTTED);
  Paint_DrawLine(fw_centerx, 0, fw_centerx, display_height(), BLACK,
                 DOT_PIXEL_DFT, LINE_STYLE_DOTTED);
#endif
  Paint_DrawTextAt(fw_text, fw_centerx, fw_centery, BLACK);
}

typedef struct UserMessage {
  MSG msg; // the MSG
  const char *text;
} UserMessage;

// order does not matter we search for the MSG..
UserMessage messages[] = {
    {WIFI_CONNECT, "Connect phone or computer to\n\"TRMNL\" WiFi network},\n"
                   "or scan QR code for help."},
    {WIFI_FAILED, "Can't establish WiFi connection.\n Hold button on "
                  " the back to reset WiFi\nor scan QR Code for help."},
    {WIFI_WEAK, "WiFi connected but signal is weak"},
    {WIFI_INTERNAL_ERROR,
     "WiFi connected, but API connection cannot be established.\n"
     "Try to refresh or scan QR Code for help."},
    {API_ERROR, "WiFi connected, but TRMNL not responding.\n"
                "Short click the button on back, "
                "otherwise check your internet."},
    {API_SIZE_ERROR, "WiFi connected, TRMNL content malformed."},
    {FW_UPDATE, "Firmware update !\nStarting now..."},
    {FW_UPDATE_FAILED, "Firmware update failed.\nDevice will restart..."},
    {FW_UPDATE_SUCCESS, "Firmware update success.\n Device will restart.."},
    {BMP_FORMAT_ERROR, "The image format is incorrect"}};

const char *GetUserMessage(MSG msg) {
  uint8_t nmsg = sizeof(messages) / sizeof(UserMessage);
  for (uint8_t m = 0; m < nmsg; ++m) {
    if (messages[m].msg == msg) {
      return messages[m].text;
    }
  }
  return nullptr;
}

// centered below logo but has space for the optional qrcode on the right
// all displayed messages are centered in the same bbox
void GetTextBBox(int16_t *left, int16_t *top, uint16_t *width,
                 uint16_t *height) {
  uint16_t dwidth = (uint16_t)display_width();
  uint16_t dheight = (uint16_t)display_height();
  *width = dwidth - (qrcodesize * 2 + marginx) * 4;
  *height = (dheight - logofullheight) / 2 - (2 * marginy) - marginbottom;
  *left = (dwidth - *width) / 2;
  *top = ((dheight + logofullheight) / 2) + marginy - marginbottom;
#if defined(DEBUG_TEXT) && defined(DEBUG_TEXT_LAYOUT)
  Paint_DrawRectangle(*left, *top, *left + *width, *top + *height, BLACK,
                      DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
  Paint_DrawLine(dwidth / 2, 0, dwidth / 2, dheight, BLACK, DOT_PIXEL_DFT,
                 LINE_STYLE_DOTTED);

#endif
}

void GetQrCodeCenter(int16_t *centerx, int16_t *centery) {
  // aligned with bbox center in y, so same value
   uint16_t dwidth = (uint16_t)display_width();
  uint16_t dheight = (uint16_t)display_height();
  uint16_t height = (dheight - logofullheight) / 2 - (2 * marginy) - marginbottom;
  int16_t top = ((dheight + logofullheight) / 2) + marginy - marginbottom;
  *centery = top + height / 2;
  *centerx = dwidth - marginx - qrcodesize / 2;
#if defined(DEBUG_TEXT) && defined(DEBUG_TEXT_LAYOUT)
  Paint_DrawLine(0, *centery, dwidth, *centery, BLACK, DOT_PIXEL_2X2,
                 LINE_STYLE_DOTTED);
#endif
}

/**
 * @brief Function to show the image with message on the display
 * @param image_buffer pointer to the uint8_t image buffer
 * @param message_type type of message that will show on the screen
 * @return none
 */
void display_show_msg(uint8_t *image_buffer, MSG message_type) {
  Log_info("display_show_msg %d", (uint16_t)message_type);
  auto dwidth = display_width();
  auto dheight = display_height();
  UBYTE *BlackImage;
  UWORD Imagesize =
      ((dwidth % 8 == 0) ? (dwidth / 8) : (dwidth / 8 + 1)) * dheight;
  Log_error("free heap - %d", ESP.getFreeHeap());
  Log_error("free alloc heap - %d", ESP.getMaxAllocHeap());
  if ((BlackImage = (UBYTE *)malloc(Imagesize)) == NULL) {
    Log_fatal("Failed to apply for black memory...");
    ESP.restart();
  }

  Log_info("Paint_NewImage");
  Paint_NewImage(BlackImage, dwidth, dheight, 0, WHITE);

  Log_info("show image for array");
  Paint_SelectImage(BlackImage);
  Paint_Clear(WHITE);
  Paint_DrawBitMap(image_buffer + 62);
  Paint_FirmwareVersion();

  // text is  below the logo
  // default box to show text in
  int16_t left, top = 0; 
  uint16_t width, height = 0;
  // center of qrcode
  int16_t qrcenterx, qrcentery = 0;

  GetTextBBox(&left, &top, &width, &height);
  GetQrCodeCenter(&qrcenterx, &qrcentery);

  const char *text = GetUserMessage(message_type);
  if (text) {
    Paint_DrawTextRect(text, left, top, width, height, BLACK);
  }
  if ((message_type == WIFI_FAILED) || (message_type == WIFI_INTERNAL_ERROR)) {
    Paint_DrawImage(wifi_failed_qr, qrcenterx - (qrcodesize / 2),
                    qrcentery - (qrcodesize / 2), qrcodesize, qrcodesize);
  } else if (message_type == TEST) {
    Demo_Charset();
  }
  EPD_7IN5_V2_Display(BlackImage);
  Log_info("display");
  free(BlackImage);
  BlackImage = NULL;
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
void display_show_msg(uint8_t *image_buffer, MSG message_type,
                      String friendly_id, bool id, const char *fw_version,
                      String message) {
  Log_info("display_show_msg  with FW %d", (uint16_t)message_type);
  if (message_type == WIFI_CONNECT) {
    Log_info("Display set to white");
    EPD_7IN5_V2_ClearWhite();
    delay(1000);
  }

  auto dwidth = display_width();
  auto dheight = display_height();
  UBYTE *BlackImage;
  /* you have to edit the startup_stm32fxxx.s file and set a big enough heap
   * size */
  UWORD Imagesize =
      ((dwidth % 8 == 0) ? (dwidth / 8) : (dwidth / 8 + 1)) * dheight;
  Log_error("free heap - %d", ESP.getFreeHeap());
  Log_error("free alloc heap - %d", ESP.getMaxAllocHeap());
  if ((BlackImage = (UBYTE *)malloc(Imagesize)) == NULL) {
    Log_fatal("Failed to apply for black memory...");
    ESP.restart();
  }

  Log_info("Paint_NewImage");
  Paint_NewImage(BlackImage, dwidth, dheight, 0, WHITE);

  Paint_SelectImage(BlackImage);
  Paint_Clear(WHITE);
  Paint_DrawBitMap(image_buffer + 62);

  Paint_FirmwareVersion();
  // text is  below the logo
  // default box to show text in
  int16_t left, top = 0;
  uint16_t width, height = 0;
  // center of qrcode
  int16_t qrcenterx, qrcentery = 0;

  GetTextBBox(&left, &top, &width, &height);
  GetQrCodeCenter(&qrcenterx, &qrcentery);

  const char *text = GetUserMessage(message_type);
  if (text) {
    Paint_DrawTextRect(text, left, top, width, height, BLACK);
  }
  if (message_type == FRIENDLY_ID) {
    Log_info("friendly id case");
    String message("Please sign up at usetrmnl.com/signup\nwith Friendly ID ");
    message += id ? friendly_id : "";
    message += "\nto finish setup";
    Paint_DrawTextRect(message.c_str(), left, top, width, height, BLACK);
  } else if (message_type == MAC_NOT_REGISTERED) {
    Log_info("mac_not_registered case");
    Paint_DrawTextRect(message.c_str(), left, top, width, height, BLACK);
  } else if (message_type == WIFI_CONNECT) {
       Paint_DrawImage(wifi_connect_qr, qrcenterx - (qrcodesize / 2),
                    qrcentery - (qrcodesize / 2), qrcodesize, qrcodesize);

  }
  Log_info("Start drawing...");
  EPD_7IN5_V2_Display(BlackImage);
  Log_info("Done");
  free(BlackImage);
  BlackImage = NULL;
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

#if defined(DEBUG_TEXT)
const char loremipsum[] =
    "Lorem ipsum dolor sit amet, consectetur adipiscing elit,\nsed do eiusmod "
    "tempor incididunt ut labore et dolore magna aliqua.\nUt enim ad minim "
    "veniam,\nquis nostrud exercitation ullamco laboris nisi ut aliquip ex ea "
    "commodo consequat. ";

void display_debug_text() {
  Log_info("Starting display messages visual test");
  uint8_t *background = (uint8_t *)default_icon;
  display_show_msg(background, WIFI_FAILED);
  delay(1000 * 10);
  display_show_msg(background, WIFI_WEAK);
  delay(1000 * 10);
  display_show_msg(background, WIFI_INTERNAL_ERROR);
  delay(1000 * 10);
  display_show_msg(background, API_ERROR);
  delay(1000 * 10);
  display_show_msg(background, API_SIZE_ERROR);
  delay(1000 * 10);
  display_show_msg(background, FW_UPDATE);
  delay(1000 * 10);
  display_show_msg(background, FW_UPDATE_FAILED);
  delay(1000 * 10);
  display_show_msg(background, FW_UPDATE_SUCCESS);
  delay(1000 * 10);
  display_show_msg(background, BMP_FORMAT_ERROR);
  delay(1000 * 10);
  display_show_msg(background, TEST);
  delay(1000 * 10);

  display_show_msg(background, FRIENDLY_ID, "123456", true, "1.5.6 ßétà", "");
  delay(1000 * 10);
  display_show_msg(background, MAC_NOT_REGISTERED, "123456", true,
                   "1.5.6 ßétà2", loremipsum);
  delay(1000 * 10);

  Log_info("Done display messages test");
}
#endif