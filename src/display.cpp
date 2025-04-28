#include "DEV_Config.h"
#include "GUI_Paint.h"
#include "png_flip.h"
#include "utility/Debug.h"
#include "utility/EPD_7in5_V2.h"
#include <Arduino.h>
#include <ArduinoLog.h>
#include <ImageData.h>
#include <cctype>
#include <config.h>
#include <display.h>
#include <image.h>

#include <rover_bmp.h>
#include <trmnl_logo_bmp.h>
#include <trmnl_logo_png.h>

void EPD_7IN5_V2_Display_Mirror_Invert(const UBYTE *blackimage, bool mirror,
                                       bool invert);

/**
 * @brief Function to init the display
 * @param none
 * @return none
 */
void display_init(void) {
  Serial.println("display init");
  Log.info("%s [%d]: dev module start\r\n", __FILE__, __LINE__);
  DEV_Module_Init();
  Log.info("%s [%d]: dev module end\r\n", __FILE__, __LINE__);

  Log.info("%s [%d]: screen hw start\r\n", __FILE__, __LINE__);
  EPD_7IN5_V2_Init();
  // EPD_7IN5_V2_Clear();
  Log.info("%s [%d]: screen hw end\r\n", __FILE__, __LINE__);
}

/**
 * @brief Function to reset the display
 * @param none
 * @return none
 */
void display_reset(void) {
  Log.info("%s [%d]: e-Paper Clear start\r\n", __FILE__, __LINE__);
  EPD_7IN5_V2_Clear();
  Log.info("%s [%d]:  e-Paper Clear end\r\n", __FILE__, __LINE__);
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

void display_show_bitmap(trmnl_bitmap *bitmap, bool flip, bool reverse) {
  // this is only used to display a full image so no need to allocate, we use
  // a direct call to a modified driver
  // EPD_7IN5_V2_Display(bitmap->data);
  EPD_7IN5_V2_Display_Mirror_Invert(bitmap->data, flip, reverse);

  Log.info("%s [%d]: display\r\n", __FILE__, __LINE__);
}

// font 16
const uint16_t charwidth = 7;
const uint16_t charheight = 12;
const uint16_t lineheight = charheight + 5;

static uint16_t ystart = 10 + lineheight;
static uint16_t xstart = 10;
static uint16_t xpos = xstart;
static uint16_t ypos = ystart;
static trmnl_bitmap *dbgscreen = nullptr;

void debug_text(const uint8_t *text, trmnl_error error) {
  if (dbgscreen == nullptr) {
    dbgscreen = bitmap_create(display_width(), display_height(), 1, 100, WHITE);
  }
  Paint_NewImage(dbgscreen->data, display_width(), display_height(), false,
                 false);

#if 1
  uint8_t *ptr = (uint8_t *)text;
  do {
    char cchar = (char)*ptr++;
    if (cchar == '\n') {
      ypos += lineheight;
      xpos = xstart;
    } else if (!iscntrl(cchar)) {
      Paint_DrawChar(xpos, ypos, cchar, &Font12, BLACK, WHITE);
      xpos += charwidth;
    }
    if (ypos > (display_height() - 10 - lineheight)) {
      Paint_Clear(WHITE);
      ypos = 10 + lineheight;
      xpos = xstart;
    }
  } while (*ptr != 0);
#else
  Paint_DrawString_EN(xpos, ypos, (const char *)text, &Font16, WHITE, BLACK);
#endif
  if (error != NO_ERROR)
    Paint_DrawString_EN(xpos, ypos, error_get_text(error), &Font12, WHITE,
                        BLACK);
  ypos += lineheight;
  display_show_bitmap(dbgscreen, true, false);
  delay(4000);
}

void debug_clear(uint8_t color) {
  if (dbgscreen == nullptr)
    return;
  ypos = ystart;
  xpos = xstart;
  memset(dbgscreen->data, color == WHITE ? 0xff : 0x00,
         dbgscreen->stride * dbgscreen->height);
}

void debug_heap() {
  char buf[64];
  sprintf(buf, "FreeHeap() = %d\nMaxAllocHeap() = %d\n", ESP.getFreeHeap(),
          ESP.getMaxAllocHeap());
  debug_text((const uint8_t *)buf);
}

void debug_clean() {
  bitmap_delete(dbgscreen);
  dbgscreen = nullptr;
}

void display_send_both(trmnl_bitmap *oldbitmap, trmnl_bitmap *newbitmap) {
  EPD_Both(oldbitmap->data, newbitmap->data);
}

void display_show_msg(trmnl_bitmap *bitmap, MSG message_type) {
  auto width = display_width();
  auto height = display_height();
  Log.info("%s [%d]: Paint_NewImage\r\n", __FILE__, __LINE__);
  Paint_NewImage(bitmap->data, width, height, 0, WHITE);

  switch (message_type) {
  case WIFI_CONNECT: {
    char string1[] = "Connect to TRMNL WiFi";
    Paint_DrawString_EN((800 - sizeof(string1) * 17 > 9)
                            ? (800 - sizeof(string1) * 17) / 2 + 9
                            : 0,
                        400, string1, &Font24, WHITE, BLACK);
    char string2[] = "on your phone or computer";
    Paint_DrawString_EN((800 - sizeof(string2) * 17 > 9)
                            ? (800 - sizeof(string2) * 17) / 2 + 9
                            : 0,
                        430, string2, &Font24, WHITE, BLACK);
  } break;
  case WIFI_FAILED: {
    char string1[] = "Can't establish WiFi";
    Paint_DrawString_EN((800 - sizeof(string1) * 17 > 9)
                            ? (800 - sizeof(string1) * 17) / 2 + 9
                            : 0,
                        340, string1, &Font24, WHITE, BLACK);
    char string2[] = "connection. Hold button on";
    Paint_DrawString_EN((800 - sizeof(string2) * 17 > 9)
                            ? (800 - sizeof(string2) * 17) / 2 + 9
                            : 0,
                        370, string2, &Font24, WHITE, BLACK);
    char string3[] = "the back to reset WiFi";
    Paint_DrawString_EN((800 - sizeof(string3) * 17 > 9)
                            ? (800 - sizeof(string3) * 17) / 2 + 9
                            : 0,
                        400, string3, &Font24, WHITE, BLACK);
    char string4[] = "or scan QR Code for help.";
    Paint_DrawString_EN((800 - sizeof(string4) * 17 > 9)
                            ? (800 - sizeof(string4) * 17) / 2 + 9
                            : 0,
                        430, string4, &Font24, WHITE, BLACK);

    Paint_DrawImage(wifi_failed_qr, 640, 337, 130, 130);
  } break;
  case WIFI_INTERNAL_ERROR: {
    char string1[] = "WiFi connected, but";
    Paint_DrawString_EN((800 - sizeof(string1) > 9) * 17
                            ? (800 - sizeof(string1) * 17) / 2 + 9
                            : 0,
                        340, string1, &Font24, WHITE, BLACK);
    char string2[] = "API connection cannot be";
    Paint_DrawString_EN((800 - sizeof(string2) > 9) * 17
                            ? (800 - sizeof(string2) * 17) / 2 + 9
                            : 0,
                        370, string2, &Font24, WHITE, BLACK);
    char string3[] = "established. Try to refresh,";
    Paint_DrawString_EN((800 - sizeof(string3) > 9) * 17
                            ? (800 - sizeof(string3) * 17) / 2 + 9
                            : 0,
                        400, string3, &Font24, WHITE, BLACK);
    char string4[] = "or scan QR Code for help.";
    Paint_DrawString_EN((800 - sizeof(string4) > 9) * 17
                            ? (800 - sizeof(string4) * 17) / 2 + 9
                            : 0,
                        430, string4, &Font24, WHITE, BLACK);

    Paint_DrawImage(wifi_failed_qr, 640, 337, 130, 130);
  } break;
  case WIFI_WEAK: {
    char string1[] = "WiFi connected but signal is weak";
    Paint_DrawString_EN((800 - sizeof(string1) > 9) * 17
                            ? (800 - sizeof(string1) * 17) / 2 + 9
                            : 0,
                        400, string1, &Font24, WHITE, BLACK);
  } break;
  case API_ERROR: {
    char string1[] = "WiFi connected, TRMNL not responding.";
    Paint_DrawString_EN((800 - sizeof(string1) > 9) * 17
                            ? (800 - sizeof(string1) * 17) / 2 + 9
                            : 0,
                        340, string1, &Font24, WHITE, BLACK);
    char string2[] = "Short click the button on back,";
    Paint_DrawString_EN((800 - sizeof(string2) > 9) * 17
                            ? (800 - sizeof(string2) * 17) / 2 + 9
                            : 0,
                        400, string2, &Font24, WHITE, BLACK);
    char string3[] = "otherwise check your internet.";
    Paint_DrawString_EN((800 - sizeof(string3) > 9) * 17
                            ? (800 - sizeof(string3) * 17) / 2 + 9
                            : 0,
                        430, string3, &Font24, WHITE, BLACK);
  } break;
  case API_SIZE_ERROR: {
    char string1[] = "WiFi connected, TRMNL content malformed.";
    Paint_DrawString_EN((800 - sizeof(string1) > 9) * 17
                            ? (800 - sizeof(string1) * 17) / 2 + 9
                            : 0,
                        400, string1, &Font24, WHITE, BLACK);
    char string2[] = "Wait or reset by holding button on back.";
    Paint_DrawString_EN((800 - sizeof(string2) > 9) * 17
                            ? (800 - sizeof(string2) * 17) / 2 + 9
                            : 0,
                        430, string2, &Font24, WHITE, BLACK);
  } break;
  case FW_UPDATE: {
    char string1[] = "Firmware update available! Starting now...";
    Paint_DrawString_EN((800 - sizeof(string1) > 9) * 17
                            ? (800 - sizeof(string1) * 17) / 2 + 9
                            : 0,
                        400, string1, &Font24, WHITE, BLACK);
  } break;
  case FW_UPDATE_FAILED: {
    char string1[] = "Firmware update failed. Device will restart...";
    Paint_DrawString_EN((800 - sizeof(string1) > 9) * 17
                            ? (800 - sizeof(string1) * 17) / 2 + 9
                            : 0,
                        400, string1, &Font24, WHITE, BLACK);
  } break;
  case FW_UPDATE_SUCCESS: {
    char string1[] = "Firmware update success. Device will restart..";
    Paint_DrawString_EN((800 - sizeof(string1) > 9) * 17
                            ? (800 - sizeof(string1) * 17) / 2 + 9
                            : 0,
                        400, string1, &Font24, WHITE, BLACK);
  } break;
  case BMP_FORMAT_ERROR: {
    char string1[] = "The image format is incorrect";
    Paint_DrawString_EN((800 - sizeof(string1) > 9) * 17
                            ? (800 - sizeof(string1) * 17) / 2 + 9
                            : 0,
                        400, string1, &Font24, WHITE, BLACK);
  } break;
  case MAC_NOT_REGISTERED: {
    char string1[] = "MAC Address is not registered,";
    Paint_DrawString_EN((800 - sizeof(string1) * 17 > 9)
                            ? (800 - sizeof(string1) * 17) / 2 + 9
                            : 0,
                        370, string1, &Font24, WHITE, BLACK);
    char string2[] = "so API is not available.";
    Paint_DrawString_EN((800 - sizeof(string2) * 17 > 9)
                            ? (800 - sizeof(string2) * 17) / 2 + 9
                            : 0,
                        400, string2, &Font24, WHITE, BLACK);
    char string3[] = "Contact support for details.";
    Paint_DrawString_EN((800 - sizeof(string3) * 17 > 9)
                            ? (800 - sizeof(string3) * 17) / 2 + 9
                            : 0,
                        430, string3, &Font24, WHITE, BLACK);
    break;
  }
  case TEST: {
    Paint_DrawString_EN(0, 0,
                        "ABCDEFGHIYABCDEFGHIYABCDEFGHIYABCDEFGHIYABCDEFGHIY",
                        &Font24, WHITE, BLACK);
    Paint_DrawString_EN(0, 40,
                        "abcdefghiyabcdefghiyabcdefghiyabcdefghiyabcdefghiy",
                        &Font24, WHITE, BLACK);
    Paint_DrawString_EN(0, 80,
                        "A B C D E F G H I Y A B C D E F G H I Y A B C D E",
                        &Font24, WHITE, BLACK);
    Paint_DrawString_EN(0, 120,
                        "a b c d e f g h i y a b c d e f g h i y a b c d e",
                        &Font24, WHITE, BLACK);
  } break;
  default:
    break;
  }

  EPD_7IN5_V2_Display(bitmap->data);
  Log.info("%s [%d]: display\r\n", __FILE__, __LINE__);
}

void display_show_msg(trmnl_bitmap *bitmap, MSG message_type,
                      String friendly_id, bool id, const char *fw_version,
                      String message) {
  if (message_type == WIFI_CONNECT) {
    Log.info("%s [%d]: Display set to white\r\n", __FILE__, __LINE__);
    EPD_7IN5_V2_Clear();
    delay(1000);
  }

  auto width = display_width();
  auto height = display_height();

  Log.info("%s [%d]: Paint_NewImage\r\n", __FILE__, __LINE__);
  Paint_NewImage(bitmap->data, width, height, 0, WHITE);

  switch (message_type) {
  case FRIENDLY_ID: {
    Log.info("%s [%d]: friendly id case\r\n", __FILE__, __LINE__);
    char string1[] = "Please sign up at usetrmnl.com/signup";
    Paint_DrawString_EN((800 - sizeof(string1) * 17 > 9)
                            ? (800 - sizeof(string1) * 17) / 2 + 9
                            : 0,
                        400, string1, &Font24, WHITE, BLACK);

    String string2 = "with Friendly ID ";
    if (id) {
      string2 += friendly_id;
    }
    string2 += " to finish setup";
    Paint_DrawString_EN((800 - string2.length() * 17 > 9)
                            ? (800 - string2.length() * 17) / 2 + 9
                            : 0,
                        430, string2.c_str(), &Font24, WHITE, BLACK);
  } break;
  case WIFI_CONNECT: {
    Log.info("%s [%d]: wifi connect case\r\n", __FILE__, __LINE__);

    String string1 = "FW: ";
    string1 += fw_version;
    Paint_DrawString_EN((800 - string1.length() * 17 > 9)
                            ? (800 - string1.length() * 17) / 2 + 9
                            : 0,
                        340, string1.c_str(), &Font24, WHITE, BLACK);
    char string2[] = "Connect phone or computer";
    Paint_DrawString_EN((800 - sizeof(string2) * 17 > 9)
                            ? (800 - sizeof(string2) * 17) / 2 + 9
                            : 0,
                        370, string2, &Font24, WHITE, BLACK);
    char string3[] = "to \"TRMNL\" WiFi network";
    Paint_DrawString_EN((800 - sizeof(string3) * 17 > 9)
                            ? (800 - sizeof(string3) * 17) / 2 + 9
                            : 0,
                        400, string3, &Font24, WHITE, BLACK);
    char string4[] = "or scan QR code for help.";
    Paint_DrawString_EN((800 - sizeof(string4) * 17 > 9)
                            ? (800 - sizeof(string4) * 17) / 2 + 9
                            : 0,
                        430, string4, &Font24, WHITE, BLACK);

    Paint_DrawImage(wifi_connect_qr, 640, 337, 130, 130);
  } break;
  default:
    break;
  }
  Log.info("%s [%d]: Start drawing...\r\n", __FILE__, __LINE__);
  EPD_7IN5_V2_Display(bitmap->data);
  Log.info("%s [%d]: display\r\n", __FILE__, __LINE__);
}

/**
 * @brief Function to got the display to the sleep
 * @param none
 * @return none
 */
void display_sleep(void) {
  Log.info("%s [%d]: Goto Sleep...\r\n", __FILE__, __LINE__);
  EPD_7IN5_V2_Sleep();
}
