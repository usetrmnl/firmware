#include <image.h>
#include <trmnl_log.h>
#include <cstring>
#include <GUI_Paint.h>

trmnl_bitmap *bitmap_create(uint32_t width, uint32_t height, uint8_t bpp,
                            uint32_t stride, uint8_t fillcolor) {
  if (bpp == 0 || bpp > 8) {
    Log.error("%s [%d]: bpp must be between 1 and 8\r\n", __FILE__, __LINE__);
    return nullptr;
  }
  if ((width > IMAGE_WIDTH_LIMIT) ||
      (height > IMAGE_HEIGHT_LIMIT)) {
    Log.error("%s [%d]: image size too big: %dx%d\r\n", __FILE__, __LINE__,
              width, height);
    return nullptr;
  }
  uint32_t datasize = stride * height;
  uint8_t palettesize = 1 << bpp;
  //
  trmnl_bitmap *bitmap = (trmnl_bitmap *)malloc(sizeof(trmnl_bitmap));
  bitmap->width = width;
  bitmap->height = height;
  bitmap->bitsperpixel = bpp;
  bitmap->numcolors = palettesize;
  bitmap->stride = stride;
  bitmap->own = true;
  bitmap->palette = (trmnl_color *)malloc(palettesize * sizeof(trmnl_color));
  bitmap->palette[0].red = 0x00;
  bitmap->palette[0].green = 0x00;
  bitmap->palette[0].blue = 0x00;
  bitmap->palette[1].red = 0xff;
  bitmap->palette[1].green = 0xff;
  bitmap->palette[1].blue = 0xff;

  bitmap->data = (uint8_t *)malloc(datasize);
  if (bitmap->data == nullptr) {
    printf("ca delire\r\n");
    Log.error("%s [%d]: image data malloc failed: %dx%dx%d (%d bytes)\r\n", __FILE__,
              __LINE__, width, height, bpp, datasize);
    free(bitmap->palette);
    free(bitmap);
    return nullptr;
  }
  // memset(bitmap->data, 0x9a, datasize); // put a pattern
  uint8_t fillvalue = fillcolor == WHITE  ? 0xff: 0x00;
  memset(bitmap->data, fillvalue, bitmap->stride * height);
  return bitmap;
}

trmnl_bitmap *bitmap_create_from_memory(const int8_t *data, uint32_t width,
                                        uint32_t height, uint8_t bpp,
                                        uint32_t stride) {
  if (bpp == 0 || bpp > 8) {
    Log.error("%s [%d]: bpp must be between 1 and 8\r\n", __FILE__, __LINE__);
    return nullptr;
  }
  uint32_t datasize = stride * height;
  uint8_t palettesize = 1 << bpp;
  trmnl_bitmap *bitmap =
      (trmnl_bitmap *)malloc(sizeof(trmnl_bitmap) + palettesize + datasize);
  bitmap->width = width;
  bitmap->height = height;
  bitmap->bitsperpixel = bpp;
  bitmap->numcolors = palettesize;
  bitmap->stride = stride;
  bitmap->own = false;
  bitmap->palette = (trmnl_color *)bitmap + sizeof(trmnl_bitmap);
  bitmap->data =
      (uint8_t *)bitmap->palette + bitmap->numcolors * sizeof(trmnl_color);
  return bitmap;
}

void bitmap_delete(trmnl_bitmap *bitmap) {
  if ((bitmap != nullptr) && bitmap->own) {
    free(bitmap->data);
    free(bitmap->palette);
    free(bitmap);
  }
}

void bitmap_set_palette_entry(trmnl_bitmap *bitmap, int8_t index, uint8_t r,
                              uint8_t g, uint8_t b) {
  trmnl_color *paletteentry = &bitmap->palette[index];
  paletteentry->red = r;
  paletteentry->green = g;
  paletteentry->red = b;
}

uint32_t stride_32(uint32_t pixel_width, uint8_t bpp) { return(((pixel_width * bpp) + 31) & ~31) >> 3; }
uint32_t stride_8(uint32_t pixel_width, uint8_t bpp) { return ((( pixel_width * bpp) + 7) & ~7) >> 3; }

