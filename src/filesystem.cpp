#include <filesystem.h>
#include <Arduino.h>
#include <SPIFFS.h>
#include <ArduinoLog.h>

/**
 * @brief Function to init the filesystem
 * @param none
 * @return bool result
 */
bool filesystem_init(void)
{
    if (!SPIFFS.begin(true))
    {
        Log.fatal("%s [%d]: Failed to mount SPIFFS\r\n", __FILE__, __LINE__);
        ESP.restart();
        return false;
    }
    else
    {
        Log.info("%s [%d]: SPIFFS mounted\r\n", __FILE__, __LINE__);
        return true;
    }
}

/**
 * @brief Function to de-init the filesystem
 * @param none
 * @return none
 */
void filesystem_deinit(void)
{
    SPIFFS.end();
}

/**
 * @brief Function to read data from file
 * @param name filename
 * @param out_buffer pointer to output buffer
 * @return result - true if success; false - if failed
 */
bool filesystem_read_from_file(const char *name, uint8_t *out_buffer, size_t size)
{
    if (SPIFFS.exists(name))
    {
        Log.info("%s [%d]: file %s exists\r\n", __FILE__, __LINE__, name);
        File file = SPIFFS.open(name, FILE_READ);
        if (file)
        {
            file.readBytes((char *)out_buffer, size);
            return true;
        }
        else
        {
            Log.error("%s [%d]: File %s open error\r\n", __FILE__, __LINE__, name);
            return false;
        }
    }
    else
    {
        Log.error("%s [%d]: file %s doesn\'t exists\r\n", __FILE__, __LINE__, name);
        return false;
    }
}

/**
 * @brief Function to write data to file
 * @param name filename
 * @param in_buffer pointer to input buffer
 * @param size size of the input buffer
 * @return size of written bytes
 */
size_t filesystem_write_to_file(const char *name, uint8_t *in_buffer, size_t size)
{
    uint32_t SPIFFS_freeBytes = (SPIFFS.totalBytes() - SPIFFS.usedBytes());
    Log.info("%s [%d]: SPIFFS freee space - %d\r\n", __FILE__, __LINE__, SPIFFS_freeBytes);
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
    delay(100);
    Log.error("%s [%d]: free heap - %d\r\n", __FILE__, __LINE__, ESP.getFreeHeap());
    Log.error("%s [%d]: free alloc heap - %d\r\n", __FILE__, __LINE__, ESP.getMaxAllocHeap());
    File file = SPIFFS.open(name, FILE_WRITE);
    if (file)
    {
        Log.error("%s [%d]: free heap - %d\r\n", __FILE__, __LINE__, ESP.getFreeHeap());
        Log.error("%s [%d]: free alloc heap - %d\r\n", __FILE__, __LINE__, ESP.getMaxAllocHeap());

        // Write the buffer in chunks
        size_t bytesWritten = 0;
        while (bytesWritten < size)
        {

            size_t diff = size - bytesWritten;
            size_t chunkSize = _min(4096, diff);
            // Log.info("%s [%d]: chunksize - %d\r\n", __FILE__, __LINE__, chunkSize);
            // delay(10);
            uint16_t res = file.write(in_buffer + bytesWritten, chunkSize);
            if (res != chunkSize)
            {
                file.close();

                Log.info("%s [%d]: Erasing SPIFFS...\r\n", __FILE__, __LINE__);
                if (SPIFFS.format())
                {
                    Log.info("%s [%d]: SPIFFS erased successfully.\r\n", __FILE__, __LINE__);
                }
                else
                {
                    Log.error("%s [%d]: Error erasing SPIFFS.\r\n", __FILE__, __LINE__);
                }

                return bytesWritten;
            }
            bytesWritten += chunkSize;
        }
        Log.info("%s [%d]: file %s writing success - %d bytes\r\n", __FILE__, __LINE__, name, bytesWritten);
        file.close();
        return bytesWritten;
    }
    else
    {
        Log.error("%s [%d]: File open ERROR\r\n", __FILE__, __LINE__);
        return 0;
    }
}

/**
 * @brief Function to check if file exists
 * @param name filename
 * @return result - true if exists; false - if not exists
 */
bool filesystem_file_exists(const char *name)
{
    if (SPIFFS.exists(name))
    {
        Log.info("%s [%d]: file %s exists.\r\n", __FILE__, __LINE__, name);
        return true;
    }
    else
    {
        Log.error("%s [%d]: file %s not exists.\r\n", __FILE__, __LINE__, name);
        return false;
    }
}

/**
 * @brief Function to delete the file
 * @param name filename
 * @return result - true if success; false - if failed
 */
bool filesystem_file_delete(const char *name)
{
    if (SPIFFS.exists(name))
    {
        if (SPIFFS.remove(name))
        {
            Log.info("%s [%d]: file %s deleted\r\n", __FILE__, __LINE__, name);
            return true;
        }
        else
        {
            Log.error("%s [%d]: file %s deleting failed\r\n", __FILE__, __LINE__, name);
            return false;
        }
    }
    else
    {
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
bool filesystem_file_rename(const char *old_name, const char *new_name)
{
    if (SPIFFS.exists(old_name))
    {
        Log.info("%s [%d]: file %s exists.\r\n", __FILE__, __LINE__, old_name);
        bool res = SPIFFS.rename(old_name, new_name);
        if (res)
        {
            Log.info("%s [%d]: file %s renamed to %s.\r\n", __FILE__, __LINE__, old_name, new_name);
            return true;
        }
        else
            Log.error("%s [%d]: file %s wasn't renamed.\r\n", __FILE__, __LINE__, old_name);
    }
    else
    {
        Log.error("%s [%d]: file %s not exists.\r\n", __FILE__, __LINE__, old_name);
        return false;
    }
}