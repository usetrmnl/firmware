#include <bmp.h>
#include <trmnl_log.h>
#include <cstdlib>
#include <cstring>

// currently same code than in png_flip
// TODO: better
void flip_vertically(unsigned char *buffer, int width, int height) {
    int row_bytes = width / 8;
    unsigned char *temp_row = (uint8_t *)malloc(row_bytes);

    for (int y = 0; y < height / 2; y++) {
        unsigned char *top_row = buffer + y * row_bytes;
        unsigned char *bottom_row = buffer + (height - y - 1) * row_bytes;

        // Swap top and bottom rows
        memcpy(temp_row, top_row, row_bytes);
        memcpy(top_row, bottom_row, row_bytes);
        memcpy(bottom_row, temp_row, row_bytes);
    }
    free(temp_row);
  }

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
    Log_fatal("It is not a BMP file");
    return BMP_NOT_BMP;
  }
  // Get width and height from the header
  uint32_t width = *(uint32_t *)&data[18];
  int32_t fheight = *(int32_t *)&data[22];
  uint16_t bitsPerPixel = *(uint16_t *)&data[28];
  uint32_t compressionMethod = *(uint32_t *)&data[30];
  uint32_t imageDataSize = *(uint32_t *)&data[34];
  uint32_t colorTableEntries = *(uint32_t *)&data[46];

  uint32_t height = fheight >= 0 ? fheight : -fheight;
  if (width != 800 || height != 480 || bitsPerPixel != 1 || imageDataSize != 48000 || colorTableEntries != 2)
    return BMP_BAD_SIZE;
  // Get the offset of the pixel data
  uint32_t dataOffset = *(uint32_t *)&data[10];

  // Display BMP information
  Log_info("BMP Header Information:\r\nWidth: %d\r\nHeight: %d\r\nBits per Pixel: %d\r\nCompression Method: %d\r\nImage Data Size: %d\r\nColor Table Entries: %d\r\nData offset: %d", width, height, bitsPerPixel, compressionMethod, imageDataSize, colorTableEntries, dataOffset);
  Log_info("BMP Header Information:\r\nFHeight: %d\n", fheight);
  // Check if there's a color table
  if (dataOffset > 54)
  {
    // Read color table entries
    uint32_t colorTableSize = colorTableEntries * 4; // Each color entry is 4 bytes

    // Display color table
    Log_info("Color table");
    for (uint32_t i = 0; i < colorTableSize; i += 4)
    {
      Log_info("Color %d: B-%d, R-%d, G-%d, A-%d", i / 4 + 1, data[54 + i], data[55 + i], data[56 + i], data[57 + i]);
    }

    if (data[54] == 0 && data[55] == 0 && data[56] == 0 && data[57] == 0 && data[58] == 255 && data[59] == 255 && data[60] == 255 && data[61] == 0)
    {
      Log_info("Color scheme standard");
      reversed = false;
    }
    else if (data[54] == 255 && data[55] == 255 && data[56] == 255 && data[57] == 0 && data[58] == 0 && data[59] == 0 && data[60] == 0 && data[61] == 0)
    {
      Log_info("Color scheme reversed");
      reversed = true;
    }
    else
    {
      Log_info("Color scheme demaged");
      return BMP_COLOR_SCHEME_FAILED;
    }
    // flip y
    if (fheight > 0) {
      Log_info("Needs flip");
     flip_vertically(data + dataOffset, width, height);
      Log_info("Done vertical flip");
    }
    return BMP_NO_ERR;
  }
  else
  {
    return BMP_INVALID_OFFSET;
  }
}