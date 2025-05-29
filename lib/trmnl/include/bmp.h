#include <cstdint>

enum bmp_err_e
{
  BMP_NO_ERR,
  BMP_NOT_BMP,
  BMP_BAD_SIZE,
  BMP_COLOR_SCHEME_FAILED,
  BMP_INVALID_OFFSET,
};

bmp_err_e parseBMPHeader(uint8_t *data, bool &reserved);
