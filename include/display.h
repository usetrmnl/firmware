#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <image.h>

enum MSG {
  NONE,
  FRIENDLY_ID,
  WIFI_CONNECT,
  WIFI_FAILED,
  WIFI_WEAK,
  WIFI_INTERNAL_ERROR,
  API_ERROR,
  API_SIZE_ERROR,
  FW_UPDATE,
  FW_UPDATE_FAILED,
  FW_UPDATE_SUCCESS,
  BMP_FORMAT_ERROR,
  MAC_NOT_REGISTERED,
  TEST,
};

/**
 * @brief Function to init the display
 * @param none
 * @return none
 */
void display_init(void);

/**
 * @brief Function to reset the display
 * @param none
 * @return none
 */
void display_reset(void);

/**
 * @brief Function to read the display height
 * @return uint16_t - height of display in pixels
 */
uint16_t display_height();

/**
 * @brief Function to read the display width
 * @return uint16_t - width of display in pixels
 */
uint16_t display_width();

void display_show_bitmap(trmnl_bitmap* bitmap, bool flip = false, bool reverse = false);

// shows image with overlay text
void display_show_debug(const uint8_t *text);
void debug_clear_display(uint8_t color);
void debug_free_resources();

/**
 * @brief Function to show the image with message on the display
 * @param bitmap pointer to the bitmap
 * @param message_type type of message that will show on the screen
 * @return none
 */
void display_show_msg(trmnl_bitmap* bitmap, MSG message_type);

/**
 * @brief Function to show the image with message on the display
 * @param bitmap pointer to the bitmap
 * @param message_type type of message that will show on the screen
 * @param friendly_id device friendly ID
 * @param id shows if ID exists
 * @param fw_version version of the firmaware
 * @param message additional message
 * @return none
 */
void display_show_msg(trmnl_bitmap* bitmap, MSG message_type, String friendly_id, bool id, const char * fw_version, String message);

/**
 * @brief Function to got the display to the sleep
 * @param none
 * @return none
 */
void display_sleep(void);

#endif