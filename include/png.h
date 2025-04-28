#include <cstdint>
#include <image.h>
#include <trmnl_errors.h>

trmnl_bitmap* decodePNGFromFile(const char *szFilename, trmnl_error* error);

trmnl_bitmap* decodePNGFromMemory(const uint8_t *bufferin, uint32_t bufferin_size,
  trmnl_error* error);

uint8_t *encodePNG(trmnl_bitmap *bitmap, uint32_t *size);
uint32_t encodePNG(trmnl_bitmap *bitmap, const char *filename);