#include "png.h"
#include <PNGdec.h>
#include <SPIFFS.h>
#include <esp_mac.h>
#include <png_file.h>
#include <trmnl_log.h>
#include <stdio.h>

/**
 * @brief Function to decode png file
 * @param szFilename PNG file location
 * @param decoded_buffer Buffer where decoded PNG bitmap save
 * @return image_err_e error code
 */
image_err_e decodePNG(const char *szFilename, uint8_t *&decoded_buffer) {
  PNG *png = new PNG();

  if (!png)
    return PNG_MALLOC_FAILED;

  File f = SPIFFS.open(szFilename);
  int32_t size = f.size();
  uint8_t *buffer_png = (uint8_t *)malloc(size);
  f.readBytes((char*)buffer_png, size);
  f.close();
  image_err_e result = processPNG(png, buffer_png, decoded_buffer);
  delete png;
  free(buffer_png);
  return result;
}