#include <PNGdec.h>
#include <SPIFFS.h>
#include <png.h>
#include <trmnl_log.h>
#include <GUI_Paint.h>


File pngfile; // Global file handle

void *pngOpen(const char *filename, int32_t *size) {
  Serial.printf("Attempting to open %s from SPIFFS\n", filename);
  pngfile = SPIFFS.open(filename, "r");

  if (!pngfile) {
    Serial.println("Failed to open file!");
    *size = 0;
    return nullptr;
  }

  *size = pngfile.size();
  return &pngfile;
}

void pngClose(void *handle) {
  if (pngfile) {
    pngfile.close();
  }
}

int32_t pngRead(PNGFILE *page, uint8_t *buffer, int32_t length) {
  if (!pngfile)
    return 0;
  (void)page; // suppress unused warning
  return pngfile.read(buffer, length);
}

int32_t pngSeek(PNGFILE *page, int32_t position) {
  if (!pngfile)
    return 0;
  (void)page;
  return pngfile.seek(position);
}
/**
 * @brief Function to decode png file
 * @param szFilename PNG file location
 * @param decodded_buffer Buffer where decoded PNG bitmap save
 * @return image_err_e error code
 */

 trmnl_bitmap* decodePNGFromFile(const char *szFilename, trmnl_error* error) {
  PNG png;

  int rc = png.open(szFilename, pngOpen, pngClose, pngRead, pngSeek, nullptr);
  if (rc == PNG_INVALID_FILE) {
    *error = IMAGE_FORMAT_UNEXPECTED_SIGNATURE;
    return nullptr;
  }

  uint32_t width = png.getWidth();
  uint32_t height = png.getHeight();
  uint32_t bpp = png.getBpp();

  trmnl_bitmap *bitmap = bitmap_create(width, height, bpp, stride_8(width, bpp), WHITE);
  if (bitmap == nullptr) { 
    *error = IMAGE_CREATION_FAILED;
    return nullptr;
  }             
  png.setBuffer(bitmap->data);

  if (!(png.decode(nullptr, 0))) {
    Log.error("PNG_SUCCESS\n");
    *error = NO_ERROR;
    return bitmap;
  }
  Log.error("PNG_DECODE_ERR\n");
  *error = IMAGE_DECODE_FAIL;
  return nullptr;
}

/** 
 */
trmnl_bitmap* decodePNGFromMemory(const uint8_t *bufferin, uint32_t bufferin_size, trmnl_error* error)  {
  PNG png;

  int rc = png.openRAM((uint8_t*)bufferin, bufferin_size, nullptr);

  if (rc == PNG_INVALID_FILE) {
    *error = IMAGE_FORMAT_UNEXPECTED_SIGNATURE;
    return nullptr;
  }

  uint32_t width = png.getWidth();
  uint32_t height = png.getHeight();
  uint32_t bpp = png.getBpp();

  trmnl_bitmap *bitmap = bitmap_create(width, height, bpp, stride_8(width, bpp), WHITE);
  if (bitmap == nullptr) {
    *error = IMAGE_CREATION_FAILED;
    return nullptr;
  }
  png.setBuffer(bitmap->data);

  if (!(png.decode(nullptr, 0))) {
    Log.error("PNG_SUCCESS\n");
    *error = NO_ERROR;
    return bitmap;
  }
  Log.error("PNG_DECODE_ERR\n");
  *error = IMAGE_DECODE_FAIL;
  return nullptr; 
  }

uint8_t *encodePNG(trmnl_bitmap *bitmap, uint32_t *size) { *size = 0; return nullptr; }
uint32_t encodePNG(trmnl_bitmap *bitmap, const char *filename) { return 0; }
