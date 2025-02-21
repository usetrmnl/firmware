#include "onebit.h"

enum bitmap_err_e {
  BMP_NO_ERR,
  PNG_NO_ERR,
  BMP_NOT_BMP,
  PNG_NOT_PNG,
  BMP_BAD_SIZE,
  BMP_COLOR_SCHEME_FAILED,
  BMP_INVALID_OFFSET,
  PNG_BAD_SIZE,
  PNG_COLOR_SCHEME_FAILED,
  INSUFFICIENT_DATA,
  TRMNL_INCOMPATIBLE
};

bitmap_err_e onebit_trmnl_decode_mem_bmp1(uint8_t *buffer, uint8_t *data,
                                          int data_length, int expectedWidth,
                                          int expectedHeight) {
  if (data_length < 62)
    return INSUFFICIENT_DATA;
  uint8_t *ptr = data;
  if (!match_bmp(data, data_length))
    return BMP_NOT_BMP;
  ptr += 2;
  ptr += 4;
  ptr += 2;
  ptr += 2;
  uint32_t offset = readMemLittle32(ptr);
  ptr += 4;
  uint32_t bih_size = readMemLittle32(ptr);
  ptr += 4;

  int32_t width = readMemLittle32(ptr);
  ptr += 4;
  int32_t height = readMemLittle32(ptr);

  if (width != expectedWidth || abs(height) != expectedHeight)
    return BMP_BAD_SIZE;

  ptr += 4;
  uint16_t color_planes = readMemLittle16(ptr);
  ptr += 2;
  uint16_t bits_per_pixel = readMemLittle16(ptr);
  if (bits_per_pixel != 1)
    return BMP_COLOR_SCHEME_FAILED;
  ptr += 2;
  uint32_t compression = readMemLittle32(ptr);
  ptr += 4;
  uint32_t raw_image_size = readMemLittle32(ptr);
  ptr += 4;

  ptr = data + offset;
  int stride = onebit_bmp_stride(width);

  if (buffer == nullptr)
    buffer = (uint8_t *)malloc(abs(height) * stride);
  // TODO image might be upside-down...
  memcpy(buffer, ptr, stride * abs(height));
  return BMP_NO_ERR;
}

bitmap_err_e onebit_trmnl_decode_mem_png1(uint8_t *buffer, uint8_t *data,
                                          int data_length, int expectedWidth,
                                          int expectedHeight) {

  int width = 0;
  int height = 0;
  uint8_t *ptr = data;
  if (!match_png(ptr, data_length)) {
    return PNG_NOT_PNG;
  }
  ptr += png_signature_length;
  // read chunks
  bool reading_chunks = true;
  int compressed_data_size = 0;
  uint8_t *compressed_data = nullptr;
  int compressed_data_pos = 0;

  do {
    int chunk_length = readMemBig32(ptr);
    ptr += 4;
    uint8_t *save_ptr = ptr;
    if (match_known(ptr, chunk_IHDR, 4)) {
      ptr += 4;
      width = readMemBig32(ptr);
      ptr += 4;
      height = readMemBig32(ptr);
      ptr += 4; // we will invert later
      if (width != expectedWidth || height != expectedHeight) return PNG_BAD_SIZE;
      int bits_per_pixel = *ptr++; // 
      if (bits_per_pixel != 1) return PNG_COLOR_SCHEME_FAILED;
      int color_type = *ptr++;     // 0 grayscale
      int compression = *ptr++;
      int filter_method = *ptr++;
      int interlacing = *ptr++;
    } else if (match_known(ptr, chunk_IDAT, 4)) {
      ptr += 4;
      compressed_data_size += chunk_length;
      if (compressed_data == nullptr) {
        // TODO check if miniz allows to decode as we read instead of reading
        // the whole compressed contents
        compressed_data = (uint8_t *)malloc(compressed_data_size);
      } else {
        compressed_data =
            (uint8_t *)realloc(compressed_data, compressed_data_size);
      }
      memcpy(compressed_data + compressed_data_pos, ptr, chunk_length);
      compressed_data_pos += chunk_length;
    } else if (match_known(ptr, chunk_IEND, 4)) {
      ptr += 4;
      reading_chunks = false;
    }
    ptr = save_ptr + chunk_length + 4;
    unsigned int crc = readMemBig32(ptr);
    ptr += 4;
  } while (reading_chunks);
  int bmp_stride = onebit_bmp_stride(width);
  int pngstride = onebit_png_stride(width);
  unsigned long srclen = compressed_data_size;
  unsigned long dstlen = (pngstride + 1) * height;

  // we use payload for destination decompression (allocated a bit larger than buffer)
  // then copy to buffer
  // that makes more allocations that necessary but it should be fine on the esp32
  // we have
  // - dynamically allocated compressed_data (usually much more smaller than 48000)
  //   - buffer (100 * 480 + 62) that will hold the final 1 bit BMP
  //   - payload (101 * 480) (data) that holds the decompressed data
  int status = uncompress2((unsigned uint8_t *)data, &dstlen,
                           (const unsigned uint8_t *)compressed_data, &srclen);
  free(compressed_data);
  // we now copy to buffer and invert
  for (int y = 0; y < height; ++y) {
    memcpy(buffer + (height - y - 1) * bmp_stride,
           data + (y * (pngstride + 1)) + 1, pngstride);
  }
#if defined(DEBUG_IDAT)
  onebit_write_file_bmp1("pngdecoded.bmp", width, height, data);
#endif
  return PNG_NO_ERR;
}

bitmap_err_e onebit_decode_to_trmnl(uint8_t *buffer, uint8_t *payload,
                                    int payloadSize, int expectedWidth,
                                    int expectedHeight) {
  // try BMP
  bitmap_err_e res =
      onebit_trmnl_decode_mem_bmp1(buffer, payload, payloadSize, 800, 480);
  if (res == BMP_NO_ERR || res != BMP_NOT_BMP) // success or some BMP error
    return res;
  // try PNG
  res = onebit_trmnl_decode_mem_png1(buffer, payload, payloadSize, 800, 480);
  if (res == PNG_NO_ERR || res != PNG_NOT_PNG) // success or some PNG error
    return res;
  return TRMNL_INCOMPATIBLE;
}
