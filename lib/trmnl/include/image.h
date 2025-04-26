#if !defined(TRMNL_IMAGE)
#define TRMNL_IMAGE

#include <stdint.h>

typedef struct {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
} trmnl_color;

typedef struct {
  uint32_t width;
  uint32_t height;
  uint8_t bitsperpixel;
  uint8_t numcolors;
  uint32_t stride; // byte width
  bool own;
  trmnl_color *palette;
  uint8_t *data;
} trmnl_bitmap;

#define IMAGE_WIDTH_LIMIT 800
#define IMAGE_HEIGHT_LIMIT 480

trmnl_bitmap *bitmap_create(uint32_t width, uint32_t height, uint8_t bpp,
                            uint32_t stride, uint8_t fillcolor);
trmnl_bitmap *bitmap_create_from_data(const int8_t *data, uint32_t width,
                                uint32_t height, uint8_t bpp, uint32_t stride);
void bitmap_delete(trmnl_bitmap *bitmap);
void bitmap_set_palette_entry(trmnl_bitmap *bitmap, int8_t index, uint8_t r,
                              uint8_t g, uint8_t b);
trmnl_bitmap *bitmap_from_data(const uint8_t *data, uint32_t datasize);

uint32_t stride_32(uint32_t pixel_width, uint8_t bpp);
uint32_t stride_8(uint32_t pixel_width, uint8_t bpp);;


#endif