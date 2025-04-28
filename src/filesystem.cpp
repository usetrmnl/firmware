#include <Arduino.h>
#include <ArduinoLog.h>
#include <SPIFFS.h>
#include <filesystem.h>

/**
 * @brief Function to init the filesystem
 * @param none
 * @return bool result
 */
bool filesystem_init(void) {
  if (!SPIFFS.begin(true)) {
    Log.fatal("%s [%d]: Failed to mount SPIFFS\r\n", __FILE__, __LINE__);
    ESP.restart();
    return false;
  } else {
    Log.info("%s [%d]: SPIFFS mounted\r\n", __FILE__, __LINE__);
    return true;
  }
}

/**
 * @brief Function to de-init the filesystem
 * @param none
 * @return none
 */
void filesystem_deinit(void) { SPIFFS.end(); }

// reads a complete file, allocating data
uint8_t *filesystem_read_file(const char *name, uint32_t *size,
                              trmnl_error *error) {
  *size = 0;
  if (!SPIFFS.exists(name)) {
    Log.error("%s [%d]: file %s not exists\r\n", __FILE__, __LINE__, name);
    *error = FILE_NOT_FOUND;
    return nullptr;
  }
  File file = SPIFFS.open(name, FILE_READ);
  if (!file) {
    Log.error("%s [%d]: file %s open error\r\n", __FILE__, __LINE__, name);
    *error = FS_FAIL;
    return nullptr;
  }
  uint32_t filesize = file.size();

  uint8_t *buffer = (uint8_t *)malloc(filesize);
  if (buffer == nullptr) {
    Log.error("%s [%d]: malloc failed %d\r\n", __FILE__, __LINE__, filesize);
    file.close();
    *error = MALLOC_FAILED;
    return nullptr;
  }
  size_t rb = file.readBytes((char *)buffer, filesize);
  if (rb != filesize) {
    Log.error("%s [%d]: read file error %d expected %d\r\n", __FILE__, __LINE__,
              rb, filesize);
    free(buffer);
    file.close();
    *error = FS_FAIL;
    return nullptr;
  }
  file.close();
  Log.info("%s [%d]: file %s read success - %d bytes\r\n", __FILE__, __LINE__,
           name, filesize);
  *size = filesize;
  *error = NO_ERROR;
  return buffer;
}

// writes a complete file
trmnl_error filesystem_write_file(const char *name, const uint8_t *in_buffer,
                                  size_t size) {
  uint32_t SPIFFS_freeBytes = (SPIFFS.totalBytes() - SPIFFS.usedBytes());
  Log.info("%s [%d]: SPIFFS freee space - %d, total -%d\r\n", __FILE__,
           __LINE__, SPIFFS_freeBytes, SPIFFS.totalBytes());
  if (SPIFFS.exists(name)) {
    Log.info("%s [%d]: file %s exists. Deleting...\r\n", __FILE__, __LINE__,
             name);
    if (SPIFFS.remove(name))
      Log.info("%s [%d]: file %s deleted\r\n", __FILE__, __LINE__, name);
    else
      Log.info("%s [%d]: file %s deleting failed\r\n", __FILE__, __LINE__,
               name);
  } else {
    Log.info("%s [%d]: file %s not exists.\r\n", __FILE__, __LINE__, name);
  }
  delay(100);
  File file = SPIFFS.open(name, FILE_WRITE);
  if (file) {
    // Write the buffer in chunks
    size_t bytesWritten = 0;
    while (bytesWritten < size) {

      size_t diff = size - bytesWritten;
      size_t chunkSize = _min(4096, diff);
      uint16_t res = file.write(in_buffer + bytesWritten, chunkSize);
      if (res != chunkSize) {
        file.close();

        Log.info("%s [%d]: Erasing SPIFFS...\r\n", __FILE__, __LINE__);
        if (SPIFFS.format()) {
          Log.info("%s [%d]: SPIFFS erased successfully.\r\n", __FILE__,
                   __LINE__);
        } else {
          Log.error("%s [%d]: Error erasing SPIFFS.\r\n", __FILE__, __LINE__);
        }
        return FS_FAIL;
      }
      bytesWritten += chunkSize;
    }
    Log.info("%s [%d]: file %s writing success - %d bytes\r\n", __FILE__,
             __LINE__, name, bytesWritten);
    file.close();
    return NO_ERROR;
  } else {
    return FS_FAIL;
  }
}

/**
 * @brief Function to check if file exists
 * @param name filename
 * @return result - true if exists; false - if not exists
 */
bool filesystem_file_exists(const char *name, uint32_t *size) {
  if (!SPIFFS.exists(name)) {
    Log.error("%s [%d]: file %s not exists.\r\n", __FILE__, __LINE__, name);
    return false;
  }
  File file = SPIFFS.open(name, FILE_READ);
  Log.info("%s [%d]: file %s exists\r\n", __FILE__, __LINE__, name);
  if (size != nullptr) {
    *size = file.size();
    Log.info("%s [%d]:size %d bytes.\r\n", __FILE__, __LINE__, *size);
    file.close();
  }
  return true;
}

/**
 * @brief Function to delete the file
 * @param name filename
 * @return result - true if success; false - if failed
 */
bool filesystem_file_delete(const char *name) {
  if (SPIFFS.exists(name)) {
    if (SPIFFS.remove(name)) {
      Log.info("%s [%d]: file %s deleted\r\n", __FILE__, __LINE__, name);
      return true;
    } else {
      Log.error("%s [%d]: file %s deleting failed\r\n", __FILE__, __LINE__,
                name);
      return false;
    }
  } else {
    Log.info("%s [%d]: file %s doesn't exist\r\n", __FILE__, __LINE__, name);
    return true;
  }
}

/**
 * @brief Function to rename the file
 * @param old_name old filename
 * @param new_name new filename
 * @return result - true if success; false - if failed
 */
bool filesystem_file_rename(const char *old_name, const char *new_name) {
  if (SPIFFS.exists(old_name)) {
    Log.info("%s [%d]: file %s exists.\r\n", __FILE__, __LINE__, old_name);
    bool res = SPIFFS.rename(old_name, new_name);
    if (res) {
      Log.info("%s [%d]: file %s renamed to %s.\r\n", __FILE__, __LINE__,
               old_name, new_name);
      return true;
    } else
      Log.error("%s [%d]: file %s wasn't renamed.\r\n", __FILE__, __LINE__,
                old_name);
    return false;
  } else {
    Log.error("%s [%d]: file %s not exists.\r\n", __FILE__, __LINE__, old_name);
    return false;
  }
}

static File current_write_file;
static bool current_write_file_opened = false;
static File current_read_file;
static bool current_read_file_opened = false;

bool filesystem_open_write_file(const char *name) {
  if (current_write_file_opened) {
    Log.error("%s [%d]: a File is already opened for writing\r\n", __FILE__,
              __LINE__);
    return false;
  }
  uint32_t SPIFFS_freeBytes = (SPIFFS.totalBytes() - SPIFFS.usedBytes());
  Log.info("%s [%d]: SPIFFS freee space - %d, total -%d\r\n", __FILE__,
           __LINE__, SPIFFS_freeBytes, SPIFFS.totalBytes());
  if (SPIFFS.exists(name)) {
    Log.info("%s [%d]: file %s exists. Deleting...\r\n", __FILE__, __LINE__,
             name);
    if (SPIFFS.remove(name))
      Log.info("%s [%d]: file %s deleted\r\n", __FILE__, __LINE__, name);
    else
      Log.info("%s [%d]: file %s deleting failed\r\n", __FILE__, __LINE__,
               name);
  } else {
    Log.info("%s [%d]: file %s not exists.\r\n", __FILE__, __LINE__, name);
  }

  if (SPIFFS.exists(name)) {
    Log.info("%s [%d]: file %s exists\r\n", __FILE__, __LINE__, name);
    current_write_file = SPIFFS.open(name, FILE_WRITE);
    if (current_write_file) {
      current_write_file_opened = true;
      return true;
    } else {
      Log.error("%s [%d]: File %s open error\r\n", __FILE__, __LINE__, name);
      return false;
    }
  } else {
    Log.error("%s [%d]: file %s doesn\'t exists\r\n", __FILE__, __LINE__, name);
    return false;
  }
}

bool filesystem_open_read_file(const char *name) {
  if (current_read_file_opened) {
    Log.error("%s [%d]: a File is already opened for reading\r\n", __FILE__,
              __LINE__);
    return false;
  }
  if (SPIFFS.exists(name)) {
    Log.info("%s [%d]: file %s exists\r\n", __FILE__, __LINE__, name);
    current_read_file = SPIFFS.open(name, FILE_READ);
    if (current_read_file != 0) {
      current_write_file_opened = true;
      return true;
    } else {
      Log.error("%s [%d]: File %s open error\r\n", __FILE__, __LINE__, name);
      return false;
    }
  } else {
    Log.error("%s [%d]: file %s doesn\'t exists\r\n", __FILE__, __LINE__, name);
    return false;
  }
}

bool filesystem_write_to_file(const uint8_t *in_buffer, uint32_t *size) {
  if (current_write_file_opened) {
    // Write the buffer in chunks
    uint32_t bytesWritten = 0;
    uint32_t dsize = *size;
    while (bytesWritten < dsize) {
      uint32_t diff = dsize - bytesWritten;
      uint32_t chunkSize = _min(4096, diff);
      uint16_t res =
          current_write_file.write(in_buffer + bytesWritten, chunkSize);
      bytesWritten += chunkSize;
    }
    Log.info("%s [%d]: partial write success - %d bytes\r\n", __FILE__,
             __LINE__, bytesWritten);
    *size = bytesWritten;
    return true;
  } else {
    *size = 0;
    Log.error("%s [%d]: No file opened for writing\r\n", __FILE__, __LINE__);
    return false;
  }
}

bool filesystem_read_from_file(uint8_t *out_buffer, size_t *size) {
  if (current_read_file_opened) {
    uint32_t readBytes = current_read_file.readBytes((char *)out_buffer, *size);
    if (readBytes != *size) {
      Log.error("%s [%d]: read file error %d expected %d\r\n", __FILE__,
                __LINE__, readBytes, *size);
      return false;
    } else {
      Log.info("%s [%d]: read file success - %d bytes\r\n", __FILE__, __LINE__,
               readBytes);
      *size = readBytes;
      return true;
    }
  } else {
    Log.error("%s [%d]: No file opened for reading\r\n", __FILE__, __LINE__);
    return 0;
  }
}

bool filesystem_close_write_file() {
  if (current_write_file_opened) {
    current_write_file.close();
    current_write_file_opened = false;
    return true;
  } else {
    Log.error("%s [%d]: No file opened for writing\r\n", __FILE__, __LINE__);
    return false;
  }
}

bool filesystem_close_read_file() {
  if (current_read_file_opened) {
    current_read_file.close();
    current_read_file_opened = false;
    return true;
  } else {
    Log.error("%s [%d]: No file opened for reading\r\n", __FILE__, __LINE__);
    return false;
  }
}
