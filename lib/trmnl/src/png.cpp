#include "png.h"
#include <PNGdec.h>
#include <trmnl_log.h>

// raw chunk (type size data chcksum)
typedef uint8_t(callback)(uint32_t chunk, uint32_t size, uint8_t *data);

uint8_t chunk_IEND[4] = {'I', 'E', 'N', 'D'};
uint8_t chunk_bKGD[4] = {'b', 'K', 'G', 'D'};
uint8_t chunk_oFFs[4] = {'o', 'F', 'F', 's'};

uint32_t PNGStride(uint32_t width) { return ((width + 7) / 8); }

void handleKnownChunks(uint8_t *data_chunk, int32_t *xoffset,
                       int32_t *yoffset) {
  bool finished = false;
  do {

    uint32_t chunk_name = *(uint32_t *)&data_chunk[0];
    uint32_t chunk_length = *(uint32_t *)&data_chunk[4];
    if (chunk_name == *((uint32_t *)chunk_bKGD)) {
      uint16_t background = *((uint16_t *)data_chunk);
      int8_t background_fill = background & 1 ? 0xff : 0;
    } else if (chunk_name == *((uint32_t *)chunk_oFFs)) {
      *xoffset = *((uint16_t *)data_chunk);
      *yoffset = *((uint16_t *)(data_chunk + 4));
    }
    data_chunk += chunk_length + 12;
    finished = chunk_name == *((uint32_t *)chunk_IEND);
  } while (!finished);
}

image_err_e processPNG(PNG *png, uint8_t *png_buffer, uint8_t *decoded_buffer) {

  uint32_t width = png->getWidth();
  uint32_t height = png->getHeight();
  uint32_t bpp = png->getBpp();

  if (width > 800 || height > 480 || bpp != 1) {
    Log_error("PNG_BAD_SIZE");
    return PNG_BAD_SIZE;
  }

  if (width != 800 || height != 480) {
    int32_t xoffset = 0;
    int32_t yoffset = 0;
    handleKnownChunks(png_buffer, &xoffset, &yoffset);
    // decode to local buffer
    uint8_t *tmpbuf = (uint8_t *)malloc(PNGStride(width) * height);
    png->setBuffer(tmpbuf);
    png->decode(nullptr, 0);
    // copy to image
    int32_t x_in_bytes = xoffset / 8;
    for (int32_t yy = 0; yy < height; ++yy) {
      if (((yoffset + yy) > 0 && (yoffset + yy) < 480)) {
        uint8_t *startrow =
            decoded_buffer + ((yoffset + yy) * PNGStride(width));
        uint8_t *startsrc = tmpbuf + yy * x_in_bytes;
        for (int32_t xx = 0; xx < PNGStride(width); ++xx) {
          // last incomplete byte depends on width % 8
          // TODO replace by memcpy
          if (((x_in_bytes + xx) > 0 && (x_in_bytes + xx) < 800)) {
            startrow[xx + x_in_bytes] = startsrc[xx];
          }
        }
      }
    }
    free(tmpbuf);
    return PNG_NO_ERR;
  } else {

    if (!(png->decode(nullptr, 0))) {
      Log_error("PNG_SUCCESS");
      return PNG_NO_ERR;
    }
  }
  Log_error("PNG_DECODE_ERR");
  return PNG_DECODE_ERR;
}

/**
 * @brief Function to decode png from buffer
 * @param buffer pointer to the buffer
 * @param decodded_buffer Buffer where decoded PNG bitmap save
 * @return image_err_e error code
 */
image_err_e decodePNG(uint8_t *buffer, uint8_t *&decoded_buffer) {
  PNG *png = new PNG();

  if (!png)
    return PNG_MALLOC_FAILED;

  int rc = png->openRAM(buffer, 48000, nullptr);

  if (rc == PNG_INVALID_FILE) {
    delete png;
    return PNG_WRONG_FORMAT;
  }

  image_err_e result = processPNG(png, buffer, decoded_buffer);
  delete png;
  return result;
}