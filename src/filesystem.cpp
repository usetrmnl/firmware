#include <filesystem.h>

#include <ArduinoLog.h>
#include <SPIFFS.h>
#include <config.h>

/**
 * @brief Function to reading image buffer from file
 * @param name buffer pointer
 * @return 1 - if success; 0 - if failed
 */
bool writeBufferToFile(const char *name, uint8_t *in_buffer, uint16_t size)
{
  if (SPIFFS.exists(name))
  {
    Log.info("%s [%d]: file %s exists. Deleting...\r\n", __FILE__, __LINE__, name);
    if (SPIFFS.remove(name))
      Log.info("%s [%d]: file %s deleted\r\n", __FILE__, __LINE__, name);
    else
      Log.info("%s [%d]: file %s deleting failed\r\n", __FILE__, __LINE__, name);
  }
  else
  {
    Log.info("%s [%d]: file %s not exists.\r\n", __FILE__, __LINE__, name);
  }
  File file = SPIFFS.open(name, FILE_APPEND);
  Serial.println(file);
  if (file)
  {
    size_t res = file.write(in_buffer, size);
    file.close();
    if (res)
    {
      Log.info("%s [%d]: file %s writing success\r\n", __FILE__, __LINE__, name);
      return true;
    }
    else
    {
      Log.error("%s [%d]: file %s writing error\r\n", __FILE__, __LINE__, name);
      return false;
    }
  }
  else
  {
    Log.error("%s [%d]: File open ERROR\r\n", __FILE__, __LINE__);
    return false;
  }
}

/**
 * @brief Function to reading image buffer from file
 * @param out_buffer buffer pointer
 * @return 1 - if success; 0 - if failed
 */
bool readBufferFromFile(const char *name, uint8_t *out_buffer)
{
  if (SPIFFS.exists(name))
  {
    Log.info("%s [%d]: icon exists\r\n", __FILE__, __LINE__);
    File file = SPIFFS.open("name", FILE_READ);
    if (file)
    {
      if (file.size() == DISPLAY_BMP_IMAGE_SIZE)
      {
        Log.info("%s [%d]: the size is the same\r\n", __FILE__, __LINE__);
        file.readBytes((char *)out_buffer, DISPLAY_BMP_IMAGE_SIZE);
      }
      else
      {
        Log.error("%s [%d]: the size is NOT the same %d\r\n", __FILE__, __LINE__, file.size());
        return false;
      }
      file.close();
      return true;
    }
    else
    {
      Log.error("%s [%d]: File open ERROR\r\n", __FILE__, __LINE__);
      return false;
    }
  }
  else
  {
    Log.error("%s [%d]: icon DOESN\'T exists\r\n", __FILE__, __LINE__);
    return false;
  }
}

bool openFilesystem(bool formatOnFail)
{
  return SPIFFS.begin(formatOnFail);
}