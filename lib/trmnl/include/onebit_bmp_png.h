#include "miniz.h"
#include "onebit.h"

enum bitmap_err_e {
  BMP_NO_ERR,
  PNG_NO_ERR,
  BMP_NOT_BMP,
  PNG_NOT_PNG,
  PNG_INFLATE_ERROR,
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

// buffer and data are preallocated to ther expected sizes
// no more allocation
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
  int bmp_stride = 0;
  int png_stride = 0;
  z_stream stream;
  memset(&stream, 0, sizeof(stream));
  int miniz_status = 0;
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
      if (width != expectedWidth || height != expectedHeight)
        return PNG_BAD_SIZE;
      bmp_stride = onebit_bmp_stride(width);
      png_stride = onebit_png_stride(width);
      stream.next_out = data;
      stream.avail_out = data_length;
      mz_inflateInit(&stream);
      int bits_per_pixel = *ptr++; //
      if (bits_per_pixel != 1)
        return PNG_COLOR_SCHEME_FAILED;
      int color_type = *ptr++; // 0 grayscale
      int compression = *ptr++;
      int filter_method = *ptr++;
      int interlacing = *ptr++;
    } else if (match_known(ptr, chunk_IDAT, 4)) {
      // setup miniz, inflate whole chunk
      stream.next_in =  ptr + 4;
      stream.avail_in = chunk_length;
      miniz_status = mz_inflate(&stream, Z_NO_FLUSH);
      if (miniz_status != MZ_OK) {
        fprintf(stderr, "Error %s\n", mz_error(miniz_status));
        return PNG_INFLATE_ERROR;
      }
    } else if (match_known(ptr, chunk_IEND, 4)) {
      ptr += 4;
      reading_chunks = false;
    }
    ptr = save_ptr + chunk_length + 4;
    unsigned int crc = readMemBig32(ptr);
    ptr += 4;
  } while (reading_chunks);
  // done reading
  miniz_status = mz_inflateEnd(&stream);
  if (miniz_status != MZ_OK) {
  fprintf(stderr, "Error %s\n", mz_error(miniz_status));
  return PNG_INFLATE_ERROR;
  }
  
  // we now copy to buffer (with different potential strides) and invert
  for (int y = 0; y < height; ++y) {
    memcpy(buffer + (height - y - 1) * bmp_stride,
           data + (y * (png_stride + 1 )) + 1, png_stride);
  }
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
