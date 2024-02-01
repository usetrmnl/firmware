#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>

enum MSG {
  WIFI_CONNECT,
  WIFI_FAILED,
  API_ERROR,
  API_SIZE_ERROR,
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
void display_show_image(uint8_t * image_buffer);

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
void display_show_msg(uint8_t * image_buffer, MSG message_type, String friendly_id);

/**
 * @brief Function to got the display to the sleep
 * @param none
 * @return none
 */
void display_sleep(void);

#endif