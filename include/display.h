#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>

enum MSG {
  NONE,
  WIFI_CONNECT,
  WIFI_FAILED,
  WIFI_INTERNAL_ERROR,
  API_ERROR,
  API_SIZE_ERROR,
  FW_UPDATE,
  FW_UPDATE_FAILED,
  FW_UPDATE_SUCCESS,
  BMP_FORMAT_ERROR,
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
 * @brief Function to show the image on the display
 * @param image_buffer - pointer to the uint8_t image buffer
 * @return none
 */
void display_show_image(uint8_t * image_buffer, bool reverse);

/**
 * @brief Function to show the image with message on the display
 * @param image_buffer - pointer to the uint8_t image buffer
 * @return none
 */
void display_show_msg(uint8_t * image_buffer, MSG message_type);

/**
 * @brief Function to show the image with message on the display
 * @param image_buffer - pointer to the uint8_t image buffer
 * @return none
 */
void display_show_msg(uint8_t * image_buffer, MSG message_type, String friendly_id, const char * fw_version);

/**
 * @brief Function to got the display to the sleep
 * @param none
 * @return none
 */
void display_sleep(void);

#endif