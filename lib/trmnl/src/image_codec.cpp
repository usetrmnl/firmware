#include <image_codec.h>
#include <trmnl_log.h>
#include <PNGdec.h>
#include <esp_mac.h>
#include <SPIFFS.h>

/**
 * @brief Function to parse .bmp file header
 * @param data pointer to the buffer
 * @param reserved variable address to store parsed color schematic
 * @return bmp_err_e error code
 */


image_err_e decodePNG(uint8_t* buffer, uint8_t* &decoded_buffer){
    PNG* png = new PNG();

    if(!png) return PNG_MALLOC_FAILED;

    int rc = png->openRAM(buffer, 48000, nullptr);

    if(rc == PNG_INVALID_FILE)
    {
        delete png;
        return IMAGE_WRONG_FORMAT;
    }

    if(!(decoded_buffer = (uint8_t *)malloc(48000)))
    {
      Log.error("PNG MALLOC FAILED\n");
      delete png;
      return PNG_MALLOC_FAILED;
    }
    png->setBuffer(decoded_buffer);
    
    uint32_t width = png->getWidth();
    uint32_t height = png->getHeight();
    uint32_t bpp = png->getBpp();

    if (width != 800 || height != 480 || bpp != 1){
        Log.error("PNG_BAD_SIZE\n");
        delete png;
        return IMAGE_BAD_SIZE;
    }

    if(!(png->decode(nullptr, 0))){
      Log.error("PNG_SUCCESS\n");
      delete png;
      return IMAGE_NO_ERR;
    }
    Log.error("PNG_DECODE_ERR\n");
    delete png;
    return PNG_DECODE_ERR;
}


image_err_e parseBMPHeader(uint8_t *data, bool &reversed)
{
  // Get width and height from the header
  uint32_t width = *(uint32_t *)&data[18];
  uint32_t height = *(uint32_t *)&data[22];
  uint16_t bitsPerPixel = *(uint16_t *)&data[28];
  uint32_t compressionMethod = *(uint32_t *)&data[30];
  uint32_t imageDataSize = *(uint32_t *)&data[34];
  uint32_t colorTableEntries = *(uint32_t *)&data[46];

  if (width != 800 || height != 480 || bitsPerPixel != 1 || imageDataSize != 48000 || colorTableEntries != 2)
    return IMAGE_BAD_SIZE;
  // Get the offset of the pixel data
  uint32_t dataOffset = *(uint32_t *)&data[10];

  // Display BMP information
  Log.info("%s [%d]: BMP Header Information:\r\nWidth: %d\r\nHeight: %d\r\nBits per Pixel: %d\r\nCompression Method: %d\r\nImage Data Size: %d\r\nColor Table Entries: %d\r\nData offset: %d\r\n", __FILE__, __LINE__, width, height, bitsPerPixel, compressionMethod, imageDataSize, colorTableEntries, dataOffset);

  // Check if there's a color table
  if (dataOffset > 54)
  {
    // Read color table entries
    uint32_t colorTableSize = colorTableEntries * 4; // Each color entry is 4 bytes

    // Display color table
    Log.info("%s [%d]: Color table\r\n", __FILE__, __LINE__);
    for (uint32_t i = 0; i < colorTableSize; i += 4)
    {
      Log.info("%s [%d]: Color %d: B-%d, R-%d, G-%d\r\n", __FILE__, __LINE__, i / 4 + 1, data[54 + 4 * i], data[55 + 4 * i], data[56 + 4 * i], data[57 + 4 * i]);
    }

    if (data[54] == 0 && data[55] == 0 && data[56] == 0 && data[57] == 0 && data[58] == 255 && data[59] == 255 && data[60] == 255 && data[61] == 0)
    {
      Log.info("%s [%d]: Color scheme standart\r\n", __FILE__, __LINE__);
      reversed = false;
    }
    else if (data[54] == 255 && data[55] == 255 && data[56] == 255 && data[57] == 0 && data[58] == 0 && data[59] == 0 && data[60] == 0 && data[61] == 0)
    {
      Log.info("%s [%d]: Color scheme reversed\r\n", __FILE__, __LINE__);
      reversed = true;
    }
    else
    {
      Log.info("%s [%d]: Color scheme demaged\r\n", __FILE__, __LINE__);
      return BMP_COLOR_SCHEME_FAILED;
    }
    return IMAGE_NO_ERR;
  }
  else
  {
    return BMP_INVALID_OFFSET;
  }
}