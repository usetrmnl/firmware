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
  uint32_t biSize = *(uint32_t *)&data[14];

  if (width != 800 || height != 480 || bitsPerPixel != 1 || imageDataSize != 48000 || colorTableEntries != 2)
    return BMP_BAD_SIZE;

    if (biSize != 40 || biSize != 108 || biSize != 124)
    return BMP_INVALID_TYPE;
  // Get the offset of the pixel data
  uint32_t dataOffset = *(uint32_t *)&data[10];

  // Display BMP information
  Log.info("%s [%d]: BMP Header Information:\r\nbiSize:%d\r\nWidth: %d\r\nHeight: %d\r\nBits per Pixel: %d\r\nCompression Method: %d\r\nImage Data Size: %d\r\nColor Table Entries: %d\r\nData offset: %d\r\n", __FILE__, __LINE__, biSize, width, height, bitsPerPixel, compressionMethod, imageDataSize, colorTableEntries, dataOffset);

  // Check if there's a color table
  if (dataOffset >= (biSize + 14))
  {
    // Read color table entries
    uint32_t colorTableSize = colorTableEntries * 4; // Each color entry is 4 bytes
    uint8_t* colorTable = data + dataOffset - colorTableSize; // = data + biSize + 14 (40 + 14 == 62 - 8)

    // Display color table
    Log.info("%s [%d]: Color table\r\n", __FILE__, __LINE__);
    for (uint32_t i = 0; i < colorTableSize; i += 4)
    {
      Log.info("%s [%d]: Color %d: B-%d, R-%d, G-%d\r\n", __FILE__, __LINE__, i / 4 + 1, colorTable[4 * i + 0], colorTable[4 * i + 1], colorTable[4 * i + 2], colorTable[4 * i + 3]);
    }

    if (colorTable[0] == 0 && colorTable[1] == 0 && colorTable[2] == 0 && colorTable[4] == 255 && colorTable[5] == 255 && colorTable[6] == 255)
    {
      Log.info("%s [%d]: Color scheme standart\r\n", __FILE__, __LINE__);
      reversed = false;
    }
    else if (colorTable[0] == 255 && colorTable[1] == 255 && colorTable[2] == 255 && colorTable[4] == 0 && colorTable[5] == 0 && colorTable[6] == 0)
    {
      Log.info("%s [%d]: Color scheme reversed\r\n", __FILE__, __LINE__);
      reversed = true;
    }
    else
    {
      Log.info("%s [%d]: Color scheme damaged\r\n", __FILE__, __LINE__);
      return BMP_COLOR_SCHEME_FAILED;
    }
    return BMP_NO_ERR;
  }
  else
  {
    return BMP_INVALID_OFFSET;
  }
}