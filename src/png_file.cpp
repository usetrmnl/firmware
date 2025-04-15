#include <png_file.h>
#include <trmnl_log.h>
#include <PNGdec.h>
#include <esp_mac.h>
#include <SPIFFS.h>

File pngfile; // Global file handle

void *pngOpen(const char *filename, int32_t *size)
{
  Serial.printf("Attempting to open %s from SPIFFS\n", filename);
  pngfile = SPIFFS.open(filename, "r");

  if (!pngfile)
  {
    Serial.println("Failed to open file!");
    *size = 0;
    return nullptr;
  }

  *size = pngfile.size();
  return &pngfile;
}

void pngClose(void *handle)
{
  if (pngfile)
  {
    pngfile.close();
  }
}

int32_t pngRead(PNGFILE *page, uint8_t *buffer, int32_t length)
{
  if (!pngfile)
    return 0;
  (void)page; // suppress unused warning
  return pngfile.read(buffer, length);
}

int32_t pngSeek(PNGFILE *page, int32_t position)
{
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

image_err_e decodePNG(const char *szFilename, uint8_t *&decoded_buffer)
{
  PNG *png = new PNG();

  if (!png)
    return PNG_MALLOC_FAILED;

  int rc = png->open(szFilename, pngOpen, pngClose, pngRead, pngSeek, nullptr);

  if (rc == PNG_INVALID_FILE)
  {
    Log.error("WRONG_FILE\n\r");
    delete png;
    return PNG_WRONG_FORMAT;
  }

  if (!(decoded_buffer = (uint8_t *)malloc(48000)))
  {
    Log.error("PNG MALLOC FAILED\n");
    delete png;
    return PNG_MALLOC_FAILED;
  }
  png->setBuffer(decoded_buffer);

  uint32_t width = png->getWidth();
  uint32_t height = png->getHeight();
  uint32_t bpp = png->getBpp();

  if (width != 800 || height != 480 || bpp != 1)
  {
    Log.error("PNG_BAD_SIZE\n");
    delete png;
    return PNG_BAD_SIZE;
  }

  if (!(png->decode(nullptr, 0)))
  {
    Log.error("PNG_SUCCESS\n");
    delete png;
    return PNG_NO_ERR;
  }
  Log.error("PNG_DECODE_ERR\n");
  delete png;
  return PNG_DECODE_ERR;
}

/**
 * @brief Function to decode png from buffer
 * @param buffer pointer to the buffer
 * @param decodded_buffer Buffer where decoded PNG bitmap save
 * @return image_err_e error code
 */
image_err_e decodePNG(uint8_t *buffer, uint8_t *&decoded_buffer)
{
  PNG *png = new PNG();

  if (!png)
    return PNG_MALLOC_FAILED;

  int rc = png->openRAM(buffer, 48000, nullptr);

  if (rc == PNG_INVALID_FILE)
  {
    delete png;
    return PNG_WRONG_FORMAT;
  }

  if (!(decoded_buffer = (uint8_t *)malloc(48000)))
  {
    Log.error("PNG MALLOC FAILED\n");
    delete png;
    return PNG_MALLOC_FAILED;
  }
  png->setBuffer(decoded_buffer);

  uint32_t width = png->getWidth();
  uint32_t height = png->getHeight();
  uint32_t bpp = png->getBpp();

  if (width != 800 || height != 480 || bpp != 1)
  {
    Log.error("PNG_BAD_SIZE\n");
    delete png;
    return PNG_BAD_SIZE;
  }

  if (!(png->decode(nullptr, 0)))
  {
    Log.error("PNG_SUCCESS\n");
    delete png;
    return PNG_NO_ERR;
  }
  Log.error("PNG_DECODE_ERR\n");
  delete png;
  return PNG_DECODE_ERR;
}