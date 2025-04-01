#include <cstdint>

enum image_err_e
{
  IMAGE_BAD_SIZE,
  IMAGE_WRONG_FORMAT,
  IMAGE_NO_ERR,
  PNG_DECODE_ERR,
  PNG_MALLOC_FAILED,
  BMP_COLOR_SCHEME_FAILED,
  BMP_INVALID_OFFSET,
  IMAGE_FS_ERROR,
  IMAGE_FILE_NOT_FOUND
};

image_err_e decodePNG(const char *szFilename, uint8_t* &decoded_buffer);

image_err_e decodePNG(uint8_t* buffer, uint8_t* &decoded_buffer);

image_err_e parseBMPHeader(uint8_t *data, bool &reserved);