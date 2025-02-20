#ifndef INCLUDE_ONEBIT_IMAGE_H
#define INCLUDE_ONEBIT_IMAGE_H

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" {
#include "miniz.h"
}

#include <stdlib.h>

// clang -g -I. -Iminiz-3.0.2 onebit_image_demo.cpp miniz-3.0.2/miniz.c puff.c
// -o onebit_image_dem

// if STB_IMAGE_WRITE_STATIC causes problems, try defining STBIWDEF to 'inline'
// or 'static inline'
#ifndef ONEBITWDEF
#ifdef ONEBIT_IMAGE_STATIC
#define ONEBITWDEF static
#else
#ifdef __cplusplus
#define ONEBITWDEF extern "C"
#else
#define ONEBITWDEF extern
#endif
#endif
#endif

const int bmp_signature_length = 2;
char bmp_signature[bmp_signature_length] = {'B', 'M'};

const int png_signature_length = 8;
char png_signature[png_signature_length] = {137, 80, 78, 71,
                                                     13,  10, 26, 10};

char *onebit_read_mem_bmp1(char *data, int data_length,
                                    int *w, int *h, int *stride);
char *onebit_read_mem_png1(char *data, int data_length,
                                    int *w, int *h, int *stride);
char *onebit_write_mem_bmp1(int w, int h, char *data,
                                     int *size);
char *onebit_write_mem_png1(int w, int h, char *data,
                                     int *size);

char *onebit_read_file_bmp1(const char *filename, int *w, int *h);
char *onebit_read_file_png1(const char *filename, int *w, int *h);
int onebit_write_file_bmp1(const char *filename, int w, int h,
                           const char *data);
int onebit_write_file_png1(const char *filename, int w, int h,
                           const char *data);

// checks signature
char *onebit_read_file(const char *filename, int *w, int *h);

bool match_png(char *data, int data_size) {
  return (data_size >= png_signature_length) &&
         !memcmp(data, png_signature, png_signature_length);
}

bool match_bmp(char *data, int data_size) {
  return (data_size >= bmp_signature_length) &&
         !memcmp(data, bmp_signature, bmp_signature_length);
}

#endif // INCLUDE_ONEBIT_IMAGE_H

#define ONEBIT_IMAGE_IMPLEMENTATION

#ifdef ONEBIT_IMAGE_IMPLEMENTATION

#ifdef _WIN32
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#ifndef _CRT_NONSTDC_NO_DEPRECATE
#define _CRT_NONSTDC_NO_DEPRECATE
#endif
#endif

#ifndef STBI_WRITE_NO_STDIO
#include <stdio.h>
#endif // STBI_WRITE_NO_STDIO
#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#if defined(STBIW_MALLOC) && defined(STBIW_FREE) &&                            \
    (defined(STBIW_REALLOC) || defined(STBIW_REALLOC_SIZED))
// ok
#elif !defined(STBIW_MALLOC) && !defined(STBIW_FREE) &&                        \
    !defined(STBIW_REALLOC) && !defined(STBIW_REALLOC_SIZED)
// ok
#else
#error                                                                         \
    "Must define all or none of STBIW_MALLOC, STBIW_FREE, and STBIW_REALLOC (or STBIW_REALLOC_SIZED)."
#endif

#ifndef STBIW_MALLOC
#define STBIW_MALLOC(sz) malloc(sz)
#define STBIW_REALLOC(p, newsz) realloc(p, newsz)
#define STBIW_FREE(p) free(p)
#endif

#ifndef STBIW_REALLOC_SIZED
#define STBIW_REALLOC_SIZED(p, oldsz, newsz) STBIW_REALLOC(p, newsz)
#endif

#ifndef STBIW_MEMMOVE
#define STBIW_MEMMOVE(a, b, sz) memmove(a, b, sz)
#endif

typedef struct {
  int w;
  int h;
  int stride;
  char *data;
} one_bit_image;

// if system is little endian  ? ...

void writeLittle16(FILE *fp, uint16_t value) {
  fputc(value & 0xff, fp);
  fputc(value >> 8, fp);
}

uint16_t readLittle16(FILE *fp) { return fgetc(fp) + fgetc(fp) * 256; }

uint16_t readMemLittle16(char *ptr) {
  return ((uint16_t)ptr[0]) | (uint16_t)ptr[1] * 256;
}

void write0_16(FILE *fp) {
  fputc(0, fp);
  fputc(0, fp);
}
void write0_32(FILE *fp) {
  fputc(0, fp);
  fputc(0, fp);
  fputc(0, fp);
  fputc(0, fp);
}

void writeLittle32(FILE *fp, uint32_t value) {
  char c;
  c = value & 0xff;
  fputc(c, fp);
  c = (value >> 8) & 0xff;
  fputc(c & 0xff, fp);
  c = (value >> 16) & 0xff;
  fputc(c, fp);

  c = (value >> 24) & 0xff;
  fputc(c, fp);
}

uint32_t readLittle32(FILE *fp) {
  return fgetc(fp) | ((uint32_t)fgetc(fp) << 8) | ((uint32_t)fgetc(fp) << 16) |
         ((uint32_t)fgetc(fp) << 24);
}

uint32_t readMemLittle32(char *ptr) {
  return ((uint32_t)ptr[0]) | (((uint32_t)ptr[1]) * 256) |
         (((uint32_t)ptr[2]) << 16) | ((uint32_t)ptr[3] << 24);
}

uint32_t readMemBig32(char *ptr) {
  return ((uint32_t)ptr[0]) << 24 | (((uint32_t)ptr[1]) << 16) |
         (((uint32_t)ptr[2]) << 8) | ((uint32_t)ptr[3]);
}

uint32_t readMemBig16(char *ptr) {
  return ((uint32_t)ptr[0]) << 8 | ((uint32_t)ptr[1]);
}

void writeBig16(FILE *fp, uint16_t value) {
  fputc(value >> 8, fp);
  fputc(value & 0xff, fp);
  fflush(fp);
}

uint16_t readBig16(FILE *fp) { return fgetc(fp) * 256 + fgetc(fp); }

void writeBig32(FILE *fp, uint32_t value) {
  char c;
  c = (value >> 24) & 0xff;
  fputc(c, fp);
  c = (value >> 16) & 0xff;
  fputc(c, fp);
  c = (value >> 8) & 0xff;
  fputc(c & 0xff, fp);
  c = value & 0xff;
  fputc(c, fp);
  fflush(fp);
}

void little32(char *ptr, int32_t value) {
  uint32_t *addr = (uint32_t *)ptr;
  *addr = value;
}

void little16(char *ptr, uint16_t value) {
  uint16_t *addr = (uint16_t *)ptr;
  *addr = value;
}

void writeBig32Mem(char *data, uint32_t value) {
  data[0] = (value >> 24) & 0xff;
  data[1] = (value >> 16) & 0xff;
  data[2] = (value >> 8) & 0xff;
  data[3] = value & 0xff;
}

uint32_t readBig32(FILE *fp) {
  return ((uint32_t)fgetc(fp) << 24) | ((uint32_t)fgetc(fp) << 16) |
         ((uint32_t)fgetc(fp) << 8) | fgetc(fp);
}

void writeN(FILE *fp, const char *data, int n) {
  fwrite(data, 1, n, fp);
}
void writeNMem(char *dataout, const char *datain, int n) {
  memcpy(dataout, datain, n);
}

void write1(FILE *fp, char value) { fputc(value, fp); }
void write1Mem(char *dataout, char value) {
  dataout[0] = value;
}

void skipN(FILE *fp, int n) {
  for (int i = 0; i < n; ++n)
    fgetc(fp);
}

inline void skip1(FILE *fp) { fgetc(fp); }

inline void skip2(FILE *fp) {
  fgetc(fp);
  fgetc(fp);
}

inline void skip4(FILE *fp) {
  fgetc(fp);
  fgetc(fp);
  fgetc(fp);
  fgetc(fp);
}

int readN(FILE *fp, char *data, int n) {
  return fread(data, 1, n, fp);
}

char read1(FILE *fp) { return fgetc(fp); }

constexpr int onebit_bmp_stride(int width) { return ((width + 31) & ~31) >> 3; }

constexpr int onebit_png_stride(int width) { return (width + 7) >> 3; }

#if 0
//  BMP V1 
struct BitmapInfoHeaderV1 {
  uint32_t biSize;
  int32_t biWidth;
  int32_t biHeight;
  uint16_t biPlanes;
  uint16_t biBitCount;
  uint32_t biCompression;
  uint32_t biSizeImage;
  int32_t biXPelsPerMeter;
  int32_t biYPelsPerMeter;
  uint32_t biClrUsed;
  uint32_t biClrImportant;
};

struct BitmapFileHeader {
  char bfType[2];
  int32_t bfSize;
  int16_t bfReserved1;
  int16_t bfReserved2;
  int32_t bfOffBits;
};

struct BMPHeader {
  struct BitmapFileHeader bfh;
  struct BitmapInfoHeaderV1 bih;
};

#endif

typedef struct BGRA {
  Byte b;
  Byte g;
  Byte r;
  Byte a;
} BGRA;

// TODO: check data size
char *onebit_read_mem_bmp1(char *data, int data_length,
                                    int *w, int *h, int *stride) {
  if (data_length < 62)
    return data;
  char *ptr = data;
  if (match_bmp(data, 2))
    return nullptr;
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

  ptr += 4;
  uint16_t color_planes = readMemLittle16(ptr);
  ptr += 2;
  uint16_t bits_per_pixel = readMemLittle16(ptr);
  ptr += 2;
  uint32_t compression = readMemLittle32(ptr);
  ptr += 4;
  uint32_t raw_image_size = readMemLittle32(ptr);
  ptr += 4;

  ptr = data + offset;
  *w = width;
  *h = abs(height);
  *stride = onebit_bmp_stride(width);

  char *dataout = (char *)malloc(abs(height) * (*stride));
  memcpy(dataout, ptr, *stride * abs(height));
  return dataout;
}

char *onebit_read_file_bmp1(const char *filename, int *w, int *h) {
  *w = 0;
  *h = 0;
  FILE *fp = fopen(filename, "rb");
  if (fp == nullptr) {
    *w = 0;
    *h = 0;
    return nullptr;
  }
  char *data = nullptr;
  char bb[2];
  bb[0] = read1(fp);
  bb[1] = read1(fp);
  if (!match_bmp(bb, 2)) {
    *w = 0;
    *h = 0;
    return data;
  }
  skip4(fp); // filesize
  skip2(fp);
  skip2(fp); // reserved
  uint32_t offset = readLittle32(fp);

  uint32_t bih_size = readLittle32(fp);
  // 108 bitmapv4header
  // if (bih_size != 40) return data;
  int32_t width = readLittle32(fp);
  int32_t height = readLittle32(fp);
  uint16_t color_planes = readLittle16(fp);
  uint16_t bits_per_pixel = readLittle16(fp);
  uint32_t compression = readLittle32(fp);
  uint32_t raw_image_size = readLittle32(fp);
  // uint32_t vres = readLittle32(fp);
  // uint32_t hres = readLittle32(fp);
  // uint32_t num_colors = readLittle32(fp);
  // if (num_colors == 0)
  //   num_colors = 1 << bits_per_pixel;
  // uint32_t important_colors = readLittle32(fp);

  // for (int c = 0; c < num_colors; ++c) {
  //   skip4(fp);
  // }

  fseek(fp, offset, SEEK_SET);
  uint32_t stride = onebit_bmp_stride(width);
  data = (char *)malloc(stride * height);
  readN(fp, data, stride * height);
  *w = width;
  *h = height;
  return data;
}

const int full_chunk_IHDR_length = 25;

char chunk_IHDR[4] = {'I', 'H', 'D', 'R'};
char chunk_IDAT[4] = {'I', 'D', 'A', 'T'};
char chunk_IEND[4] = {'I', 'E', 'N', 'D'};

const int full_chunk_IEND_length = 12;
char full_chunk_IEND[full_chunk_IEND_length] = {
    0x00, 0x00, 0x00, 0x0, 0x49, 0x45, 0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82};

inline bool match_known(char *bytes, char *known, int len) {
  return !memcmp(bytes, known, len);
}

void writeToFile(const char *filename, char *data, int size) {
  FILE *fp = fopen(filename, "wb");
  if (fp != nullptr)
    fwrite(data, 1, size, fp);
  fclose(fp);
}

char *onebit_write_mem_png1(int w, int h, const char *data,
                                     int *size) {
  int ihdr_contents_size = 13;
  int chunk_overhead = 12; // count 4 chunktypr 4 crc 4
  int ihdr_size = ihdr_contents_size +
                  chunk_overhead; // size + chunk name * chunkdata + crc = 25
  char *signature_ihdr =
      (char *)malloc(png_signature_length + ihdr_size);

  // first compress to get final size
  int bmpstride = onebit_bmp_stride(w);
  int pngstride = onebit_png_stride(w);
  // FILTER adds  byte at the begining !!!
  mz_ulong data_len = (pngstride + 1) * h;
  char *newdata = (char *)malloc(data_len);
  // invert a the same time

  for (int y = 0; y < abs(h); y++) {
    newdata[y * (pngstride + 1)] = 0;
    memcpy(newdata + y * (pngstride + 1) + 1,
           h <= 0 ? data + y * bmpstride : data + (abs(h) - y - 1) * bmpstride,
           pngstride);
  }

  mz_ulong compressed_data_length = mz_compressBound(data_len);
  char *compressed_data =
      (char *)malloc(compressed_data_length); // same as uncompress

  int status =
      compress2((unsigned char*)compressed_data, &compressed_data_length, (const unsigned char*)newdata, data_len, 7);
  int cdata_len = compressed_data_length;
  free(newdata);
  int idat_size = chunk_overhead + cdata_len;

  // now build the final contents
  // signature
  int final_size =
      png_signature_length + ihdr_size + idat_size + full_chunk_IEND_length;
  char *final_data = (char *)malloc(final_size);
  memcpy(final_data, png_signature, png_signature_length);

  // ihdr
  char *ihdr_data = final_data + png_signature_length;
  memset(ihdr_data, 0, ihdr_size);
  writeBig32Mem(ihdr_data, 13); // chunk contents size
  memcpy(ihdr_data + 4, chunk_IHDR, 4);
  writeBig32Mem(ihdr_data + 8, w);
  writeBig32Mem(ihdr_data + 12, abs(h));
  ihdr_data[16] = 1;
  int crc = mz_crc32(MZ_CRC32_INIT, (const unsigned char*)ihdr_data + 4, ihdr_contents_size + 4);
  writeBig32Mem(ihdr_data + ihdr_size - 4, crc);

  // idat
  char *idat_data = ihdr_data + ihdr_size;
  writeBig32Mem(idat_data, compressed_data_length);
  memcpy(idat_data + 4, chunk_IDAT, 4);
  memcpy(idat_data + 8, compressed_data, compressed_data_length);
  crc = mz_crc32(MZ_CRC32_INIT, (const unsigned char*)idat_data + 4, compressed_data_length + 4);
  writeBig32Mem(idat_data + idat_size - 4, crc);
  free(compressed_data);

  // iend
  memcpy(final_data + png_signature_length + ihdr_size + idat_size,
         full_chunk_IEND, full_chunk_IEND_length);

  *size = final_size;
  return final_data;
}

int onebit_write_file_png1(const char *filename, int w, int h,
                           const char *data) {
  FILE *fp = fopen(filename, "wb");
  if (fp == nullptr)
    return 1;
  writeN(fp, png_signature, png_signature_length);
  // ihdr
  writeBig32(fp, 13);
  char ihdr[17];
  char *ptr = ihdr;

  writeNMem(ptr, chunk_IHDR, 4);
  ptr += 4;
  writeBig32Mem(ptr, w);
  ptr += 4;
  writeBig32Mem(ptr, abs(h));
  ptr += 4;
  write1Mem(ptr, 1);
  ptr++; // bits per pixel
  write1Mem(ptr, 0);
  ptr++; // colortype (greyscale)
  write1Mem(ptr, 0);
  ptr++; // compression
  write1Mem(ptr, 0);
  ptr++; // filter
  write1Mem(ptr, 0);
  ptr++; // interlace

  unsigned int ihdr_crc = mz_crc32(MZ_CRC32_INIT, (const unsigned char*)ihdr, 17);
  writeN(fp, ihdr, 17);
  writeBig32(fp, ihdr_crc);

  // idat
  int bmpstride = onebit_bmp_stride(w);
  int pngstride = onebit_png_stride(w);
  // FILTER adds  byte at the begining !!!
  mz_ulong data_len = (pngstride + 1) * h;
  char *newdata = (char *)malloc(data_len);
  // invert a the same time

  for (int y = 0; y < abs(h); y++) {
    newdata[y * (pngstride + 1)] = 0;
    memcpy(newdata + y * (pngstride + 1) + 1,
           h <= 0 ? data + y * bmpstride : data + (abs(h) - y - 1) * bmpstride,
           pngstride);
  }

  mz_ulong compressed_data_len = mz_compressBound(data_len);
  char *compressed_data =
      (char *)malloc(compressed_data_len); // same as uncompress

  int status =
      compress2((unsigned char*)compressed_data, &compressed_data_len, (const unsigned char*)newdata, data_len, 7);
  int cdata_len = compressed_data_len;

  uint32_t scdata_len = cdata_len;
  free(newdata);

  writeBig32(fp, cdata_len);
  writeN(fp, chunk_IDAT, 4);
  writeN(fp, compressed_data, cdata_len);
  unsigned int idat_crc = mz_crc32(MZ_CRC32_INIT, (const unsigned char*)chunk_IDAT, 4);
  idat_crc = mz_crc32(idat_crc, (const unsigned char*)compressed_data, cdata_len);

  writeBig32(fp, idat_crc);

  // iend
  writeN(fp, full_chunk_IEND, full_chunk_IEND_length);

#if defined(DEBUG_IDAT)
  FILE *rawdata = fopen("rawdata.zs", "wb");
  fwrite(rawdata, 1, cdata_len, rawdata);
  fclose(rawdata);
#endif
  free(compressed_data);
  fclose(fp);
  return 1;
}

char *onebit_read_mem_png1(char *data_mem, int data_length,
                                    int *w, int *h, int *stride) {
  int width = 0;
  int height = 0;
  char *ptr = data_mem;
  if (!match_known(ptr, png_signature, png_signature_length)) {
    return nullptr;
  }
  ptr += png_signature_length;
  // read chunks
  bool reading_chunks = true;
  int compressed_data_size = 0;
  char *data = nullptr;
  char *compressed_data = nullptr;
  int compressed_data_pos = 0;

  do {
    int chunk_length = readMemBig32(ptr);
    ptr += 4;
    char *save_ptr = ptr;
    if (match_known(ptr, chunk_IHDR, 4)) {
      ptr += 4;
      width = readMemBig32(ptr);
      ptr += 4;
      height = readMemBig32(ptr);
      ptr += 4; // we will invert later

      int bits_per_pixel = *ptr++; // 1
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
        compressed_data = (char *)malloc(compressed_data_size);
      } else {
        compressed_data =
            (char *)realloc(compressed_data, compressed_data_size);
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
  int tmp_stride = onebit_bmp_stride(width);
  *stride = tmp_stride;
  *w = width;
  *h = height;
  int pngstride = onebit_png_stride(width);
  unsigned long srclen = compressed_data_size;
  unsigned long dstlen = (pngstride + 1) * height;
  // we allocate for final data. the real size + one columnn for the filter
  // to prevent doing two allocations and copying from decompressed to final
  // size this wastes some memory. uncompressing to final data might be possible
  // with a streaming API for deflate, but we currently retrieve the full
  // uncompressed data... we have to layout the data "in-place",
  char *tmp_data = (char *)malloc(srclen);
  int status = uncompress2((unsigned char*)tmp_data, &dstlen, (const unsigned char*)compressed_data, &srclen);
  free(compressed_data);
#if 1
  data = (char *)malloc(tmp_stride * height);
  // invert
  for (int y = 0; y < height; ++y) {
    memcpy(data + (height - y - 1) * tmp_stride,
           tmp_data + (y * (pngstride + 1)) + 1, pngstride);
  }
  free(tmp_data);
#else
// TODO
#endif
#if defined(DEBUG_IDAT)
  onebit_write_file_bmp1("pngdecoded.bmp", width, height, data);
#endif

  return data;
}

char *onebit_read_file_png1(const char *filename, int *w, int *h) {

  char *data = nullptr;
  FILE *fp = fopen(filename, "rb");
  if (fp == nullptr) {
    *w = 0;
    *h = 0;
    return nullptr;
  }
  char signature[png_signature_length] = {0};

  char *compressed_data = nullptr;
  int compressed_data_pos = 0;
  int compressed_data_size = 0;
  int width = 0;
  int height = 0;
  int stride = 0;
  int num_read = readN(fp, signature, png_signature_length);
  if ((num_read < png_signature_length) ||
      !match_known(signature, png_signature, png_signature_length)) {
    return data;
  }
  // read chunks
  bool reading_chunks = true;
  do {
    char chunk[4] = {0};
    int chunk_length = readBig32(fp);
    readN(fp, chunk, 4);
    int pos = ftell(fp);
    if (match_known(chunk, chunk_IHDR, 4)) {
      width = readBig32(fp);
      height = readBig32(fp); // we will invert later

      int bits_per_pixel = read1(fp); // 1
      int color_type = read1(fp);     // 0 grayscale
      int compression = read1(fp);    // 0 zlib
      int filter_method = read1(fp);  // 0 none
      int interlacing = read1(fp);    // 0 none 1 adam7

      stride = onebit_bmp_stride(width);
      *w = width;
      *h = height;
    } else if (match_known(chunk, chunk_IDAT, 4)) {
      compressed_data_size += chunk_length;
      if (compressed_data == nullptr) {
        // TODO check if miniz allows to decode as we read instead of reading
        // the whole compressed contents
        compressed_data = (char *)malloc(compressed_data_size);
      } else {
        compressed_data =
            (char *)realloc(compressed_data, compressed_data_size);
      }
      readN(fp, compressed_data + compressed_data_pos, chunk_length);
      compressed_data_pos += chunk_length;
    } else if (match_known(chunk, chunk_IEND, 4)) {
      reading_chunks = false;
    }
    fseek(fp, pos + chunk_length, SEEK_SET);
    unsigned int crc = readBig32(fp);
  } while (reading_chunks);
  fclose(fp);
  unsigned long srclen = compressed_data_size;
  int pngstride = onebit_png_stride(width);
  unsigned long dstlen = (pngstride + 1) * height;
  // these two allocations can be reduced to the largest one,
  // the returned data would have height "wasted" bytes ate the end
  // second pass would move data to correct location "in place"
  char *tmp_data = (char *)malloc(dstlen);
  int status = uncompress2((unsigned char*)tmp_data, &dstlen, (const unsigned char*)compressed_data, &srclen);
  free(compressed_data);
  data = (char *)malloc(stride * height);
  // invert
  for (int y = 0; y < height; ++y) {
    memcpy(data + ((height - y - 1) * stride),
           tmp_data + (y * (pngstride + 1)) + 1, pngstride);
  }
  free(tmp_data);
#if defined(DEBUG_IDAT)
  onebit_write_file_bmp1("pngdecoded.bmp", width, height, data);
#endif
  return data;
}

// TODO: check on TRML device is 0 is black or white
char black[4] = {0x00, 0x00, 0x00, 0x00};
char white[4] = {0xff, 0xff, 0xff, 0x00};

char *onebit_write_mem_bmp1(int w, int h, char *data,
                                     int *size) {
  *size = 0;
  int ncolors = 2;
  unsigned int stride = onebit_bmp_stride(w);
  unsigned int absolute_height = abs(h);
  bool is_top_down = h < 0;
  int header_size = 54;
  int color_size = ncolors * 4;
  int offset = header_size + color_size;
  int data_size = stride * absolute_height;
  int save_size = header_size + color_size + data_size;
  char *dataout = (char *)malloc(save_size);
  memset(dataout, 0, header_size);
  memcpy(dataout, bmp_signature, bmp_signature_length);
  little32(dataout + 2, save_size);
  little32(dataout + 10, offset); // offset

  // bih8
  little32(dataout + 14, 40);
  little32(dataout + 18, w);
  little32(dataout + 22, h);
  dataout[26] = 1;
  dataout[28] = 1; // bits per pixel

  dataout[46] = 2;
  dataout[50] = 2;
  memcpy(dataout + header_size, black, 4);
  memcpy(dataout + header_size + 4, white, 4);

  memcpy(dataout + offset, data, stride * absolute_height);
  *size = save_size;
  return dataout;
}

int onebit_write_file_bmp1(const char *filename, int w, int h, char *data) {
  FILE *fp = fopen(filename, "wb");
  if (fp == nullptr) {
    return 1;
  }

  int ncolors = 2;
  int stride = onebit_bmp_stride(w);
  unsigned int absolute_height = abs(h);
  bool is_top_down = h < 0;
  writeN(fp, bmp_signature, bmp_signature_length);
  writeLittle32(fp, 54 + ncolors * 4 + stride * h); // filesize
  write0_16(fp);
  write0_16(fp);
  writeLittle32(fp, 62);

  writeLittle32(fp, 40);
  writeLittle32(fp, w);
  writeLittle32(fp, h);
  writeLittle16(fp, 1);
  writeLittle16(fp, 1);
  write0_32(fp); // compression
  writeLittle32(fp, stride * abs(h));
  write0_32(fp);        // h res
  write0_32(fp);        // v res
  writeLittle32(fp, 2); //
  writeLittle32(fp, 2); //

  writeN(fp, black, 4);
  writeN(fp, white, 4);
  writeN(fp, data, stride * abs(h));
  fclose(fp);
  return 1;
}

char *onebit_read_file(const char *filename, int *w, int *h) {
  *w = 0;
  *h = 0;
  char *data;
  FILE *fp = fopen(filename, "rb");
  if (fp == nullptr)
    return data;
  // try to read 8 bytes
  const int read_asked_size = png_signature_length;
  char read_data[read_asked_size];
  int read = fread(read_data, 1, read_asked_size, fp);
  if (match_bmp(read_data, read))
    data = onebit_read_file_bmp1(filename, w, h);
  else if (match_png(read_data, read))
    data = onebit_read_file_png1(filename, w, h);
  fclose(fp);
  return data;
}

#endif