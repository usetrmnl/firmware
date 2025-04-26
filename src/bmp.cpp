#include <bmp.h>
#include <filesystem.h>
#include <image.h>
#include <display.h>
#include <string.h>
#include <trmnl_log.h>
#include <GUI_Paint.h>

typedef struct {
  uint32_t size; // expect 40
  int32_t width;
  int32_t height;
  uint16_t planes;
  uint16_t bitsperpixel;
  uint32_t compression;
  uint32_t imagesize;
  uint32_t xresolution;
  uint32_t yresolution;
  uint32_t ncolors;
} bitmapinfoheader;

typedef struct {
  uint8_t sig[2];
  uint8_t disksize;
  uint32_t reserved;
  uint32_t offset;
  bitmapinfoheader bih;
} bitmapfileheader;

const uint32_t bmp_bytes_per_color = 4; // 32 bit color

/// current uses memory version
trmnl_bitmap *decodeBMPFromFile(const char *szFilename, trmnl_error *error) {
  uint32_t filesize = 0;
  if (!filesystem_file_exists(szFilename, &filesize)) {
    Log.error("%s [%d]: File %s not exists\r\n", __FILE__, __LINE__,
              szFilename);
    *error = FILE_NOT_FOUND;
    return nullptr;
  }
  uint32_t size = 0;
  uint8_t *data = filesystem_read_file(szFilename, &size, error);
  if (data && (*error == NO_ERROR)) {
    trmnl_bitmap *bitmap = decodeBMPFromMemory(data, size, error);
    free(data);
    return bitmap;
  }
  return nullptr;
}

trmnl_bitmap *decodeBMPFromMemory(const uint8_t *bufferin, uint32_t bufferin_size,
                                  trmnl_error *error) {
  trmnl_error ierror = NO_ERROR;
 if (bufferin_size < sizeof(bitmapfileheader)) {
  *error = SIZE_TOO_SMALL;
    return nullptr;
 }
    bitmapfileheader *header = (bitmapfileheader *)bufferin;
  if (header->sig[0] != 'B' || header->sig[1] != 'M') {
    *error = IMAGE_FORMAT_UNEXPECTED_SIGNATURE;
  }
  display_show_debug((const uint8_t*)"pass here");
  if (header->offset + 12 > bufferin_size)
    *error = IMAGE_STRUCTURE_CORRUPTED;

  if (error) {
    Log.error("%s [%d]: BMP header error: %d\r\n", __FILE__, __LINE__, ierror);
    return nullptr;
  }

  bitmapinfoheader bih = header->bih;
  int32_t width = bih.width;
  int32_t height = bih.height;
  Log.info("%s [%d]: BMP Header Information:\r\nWidth: %d\r\nHeight: "
           "%d\r\nBits per Pixel: %d\r\nCompression Method: %d\r\nImage Data "
           "Size: %d\r\nColor Table Entries: %d\r\nData offset: %d\r\n",
           __FILE__, __LINE__, width, height, bih.bitsperpixel, bih.compression,
           bih.imagesize, 1 << bih.bitsperpixel, header->offset);
  if ((bih.size != 40) || (bih.planes > 1) || (bih.bitsperpixel > 8) ||
      (bih.compression != 0)) {
    *error = IMAGE_INCOMPATIBLE;
    return nullptr;
  }

  uint32_t srcstride = stride_32(width, bih.bitsperpixel);
  trmnl_bitmap *bitmap =
      bitmap_create(width, abs(height), bih.bitsperpixel, stride_8(width, bih.bitsperpixel), WHITE);
  if (bitmap == nullptr) {
    ierror = MALLOC_FAILED;
    return nullptr;
    *error = ierror;
    return bitmap;
  }
  // copy palette
  const uint8_t *bmppalette = bufferin + sizeof(bitmapfileheader);
  for (uint8_t i = 0; i < bitmap->numcolors; ++i) {
    bitmap_set_palette_entry(bitmap, i, bmppalette[i * 4 + 2],
                             bmppalette[i * 4 + 1], bmppalette[i * 4 + 0]);
  }
  // copy data, flipping if necessary
  // note the negative stride
  bool istopdown = height < 0;
  // when fliping start from latest row and go backwrds (negative stride )
  // the loop stays the same..
  const uint32_t normalstride = stride_8(width, bih.bitsperpixel);
  const int32_t dststride = istopdown ? normalstride  : -normalstride;
  uint8_t *dstdata = istopdown
                         ? bitmap->data
                         : bitmap->data + bitmap->stride * (bitmap->height - 1);
  uint8_t *srcdata = (uint8_t *)bufferin + header->offset;

  uint8_t *srcstart =
      bitmap->data + (istopdown ? 0 : bitmap->stride * (bitmap->height - 1));
  for (uint32_t h = 0; h < abs(height); ++h) {
    memcpy(dstdata, (const char *)srcdata, bitmap->stride);
    dstdata += dststride;
    srcdata += srcstride;
  }
  return bitmap;
}

uint8_t *encodeBMP(trmnl_bitmap *bitmap, uint32_t *size) {
  *size = 0;
  return nullptr;
}
uint32_t encodeBMP(trmnl_bitmap *bitmap, const char *filename) { return 0; }