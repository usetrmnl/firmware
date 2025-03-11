#include <bmp.h>
#include <trmnl_log.h>

/**
 * @brief Function to parse .bmp file header
 * @param data pointer to the buffer
 * @param reserved variable address to store parsed color schematic
 * @return bmp_err_e error code
 */
bmp_err_e parseBMPHeader(uint8_t *data, bool &reversed)
{

  // Check if the file is a BMP image
  if (data[0] != 'B' || data[1] != 'M')
  {
    Log.fatal("%s [%d]: It is not a BMP file\r\n", __FILE__, __LINE__);
    return BMP_NOT_BMP;
  }

  // Get width and height from the header
  uint32_t width = *(uint32_t *)&data[18];
  uint32_t height = *(uint32_t *)&data[22];
  uint16_t bitsPerPixel = *(uint16_t *)&data[28];
  uint32_t compressionMethod = *(uint32_t *)&data[30];
  uint32_t imageDataSize = *(uint32_t *)&data[34];
  uint32_t colorTableEntries = *(uint32_t *)&data[46];

#ifndef EPDIY
  if (width != 800 || height != 480 || bitsPerPixel != 1 || imageDataSize != 48000 || colorTableEntries != 2)
    return BMP_BAD_SIZE;
#endif
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
    return BMP_NO_ERR;
  }
  else
  {
    return BMP_INVALID_OFFSET;
  }
}