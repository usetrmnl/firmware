#include <GUI_Paint.h>
#include <bmp.h>
#include <display.h>
#include <filesystem.h>
#include <image.h>
#include <string.h>
#include <trmnl_log.h>

#pragma pack(push, 1)
// 40
typedef struct {
  uint32_t biSize;
  int32_t width;
  int32_t height;
  uint16_t planes;
  uint16_t bitsperpixel;
  uint32_t compression;
  uint32_t imagesize;
  uint32_t xresolution;
  uint32_t yresolution;
  uint32_t clrUsed;
  uint32_t clrImportant;
} BitmapInfoHeaderV1;

// 52
typedef struct {
  BitmapInfoHeaderV1 bihV1;
  uint32_t biRedMask;
  uint32_t biGreenMask;
  uint32_t biBlueMask;
} BitmapInfoHeaderV2;

// 56
typedef struct {
  BitmapInfoHeaderV2 bihV2;
  uint32_t biAlphaMask;
} BitmapInfoHeaderV3;

typedef struct {
  uint8_t sig[2];
  uint32_t disksize;
  uint16_t reserved1;
  uint16_t reserved2;
  uint32_t offset;
} BitmapFileHeader;

#pragma pack(pop)

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

static char dbgbuf[256];
trmnl_bitmap *decodeBMPFromMemory(const uint8_t *bufferin,
                                  uint32_t bufferin_size, trmnl_error *error) {
  trmnl_error ierror = NO_ERROR;
  if (bufferin_size < sizeof(BitmapFileHeader)) {
    *error = SIZE_TOO_SMALL;
    return nullptr;
  }
  BitmapFileHeader *header = (BitmapFileHeader *)bufferin;
  if (header->sig[0] != 'B' || header->sig[1] != 'M') {
    *error = IMAGE_FORMAT_UNEXPECTED_SIGNATURE;
    return nullptr;
  }

  BitmapInfoHeaderV1 *bih =
      (BitmapInfoHeaderV1 *)(bufferin + sizeof(BitmapFileHeader));
  int32_t width = bih->width;
  int32_t height = bih->height;
  boolean is_supported_version =
      (bih->biSize == 40) || (bih->biSize == 52) | (bih->biSize == 56);
  if (!is_supported_version || (bih->planes != 1) || (bih->bitsperpixel > 8) ||
      (bih->compression != 0)) {
    *error = IMAGE_FORMAT_UNSUPPORTED;
    return nullptr;
  }

  uint32_t srcstride = stride_32(width, bih->bitsperpixel);
  uint32_t dststride = stride_8(width, bih->bitsperpixel);
  trmnl_bitmap *bitmap =
      bitmap_create(width, abs(height), bih->bitsperpixel, dststride, WHITE);
  if (bitmap == nullptr) {
    ierror = MALLOC_FAILED;
    return nullptr;
    *error = ierror;
    return bitmap;
  }
  // copy palette
  uint8_t num_colors = 1 << bih->bitsperpixel;
  const uint32_t palette_offset = sizeof(BitmapFileHeader) + bih->biSize;
  const uint8_t *bmppalette = bufferin + palette_offset;
  for (uint8_t i = 0; i < num_colors; ++i) {
    bitmap_set_palette_entry(bitmap, i, bmppalette[i * 4 + 2],
                             bmppalette[i * 4 + 1], bmppalette[i * 4 + 0]);
  }
  // copy data, flipping if necessary
  // note the negative stride

  sprintf(dbgbuf,
          "%s [%d]: BMP Header Information:\nWidth: %d\nHeight: "
          "%d\nBits per Pixel: %d\nCompression Method: %d\nImage Data "
          "Size: %d\nColor Table Entries: %d\nData offset: %d\nBIH type: %d\n",
          __FILE__, __LINE__, width, height, bih->bitsperpixel,
          bih->compression, bih->imagesize, 1 << bih->bitsperpixel,
          header->offset, bih->biSize);
  //debug_text((const uint8_t *)dbgbuf);

  // 0 is black 1 is white
  // bitmaps are topdown
  bool needsflip = height > 0;
  bool invert = bitmap->palette[0].red > 10;
  sprintf(dbgbuf,
          "color index 0 is %d %d %d\ncolor index 1 is %d %d %d\n -=> invert %s, flip %s\n",
          bitmap->palette[0].red, bitmap->palette[0].green,
          bitmap->palette[0].blue, bitmap->palette[1].red,
          bitmap->palette[1].green, bitmap->palette[1].blue,
          invert ? "yes" : "no", needsflip ? "yes" : "no");
  debug_text((const uint8_t *)dbgbuf);

  for (uint32_t h = 0; h < bitmap->height; ++h) {
    uint8_t *dstdata =
        needsflip ? (bitmap->data + (dststride * (bitmap->height - h - 1)))
                                    : bitmap->data;
    uint8_t *srcdata = (uint8_t *)bufferin + header->offset + (srcstride * h);
    if (false && invert) {
      for (uint32_t xx = 0; xx < dststride; ++xx) {
        dstdata[xx] = ~srcdata[xx];
      }
    } else {
      memcpy(dstdata, (const char *)srcdata, bitmap->stride);
    }
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