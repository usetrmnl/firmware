#include "stored_logs.h"
#include "config.h"
#include "log.h"

void store_log(const char *log_buffer, size_t size, Preferences &preferences)
{
  bool result = false;

  for (uint8_t i = 0; i < LOG_MAX_NOTES_NUMBER; i++)
  {
    String key = PREFERENCES_LOG_KEY + String(i);
    if (preferences.isKey(key.c_str()))
    {
      Log.info("%s [%d]: key %s exists\r\n", __FILE__, __LINE__, key.c_str());
      result = false;
    }
    else
    {
      Log.info("%s [%d]: key %s not exists\r\n", __FILE__, __LINE__, key.c_str());
      size_t res = preferences.putString(key.c_str(), log_buffer);
      Log.info("%s [%d]: Initial size %d. Received size - %d\r\n", __FILE__, __LINE__, size, res);
      if (res == size)
      {
        Log.info("%s [%d]: log note written success\r\n", __FILE__, __LINE__);
      }
      else
      {
        Log.info("%s [%d]: log note writing failed\r\n", __FILE__, __LINE__);
      }
      result = true;
      break;
    }
  }
  if (!result)
  {
    uint8_t head = 0;
    if (preferences.isKey(PREFERENCES_LOG_BUFFER_HEAD_KEY))
    {
      Log.info("%s [%d]: head exists\r\n", __FILE__, __LINE__);
      head = preferences.getUChar(PREFERENCES_LOG_BUFFER_HEAD_KEY, 0);
    }
    else
    {
      Log.info("%s [%d]: head NOT exists\r\n", __FILE__, __LINE__);
    }

    String key = PREFERENCES_LOG_KEY + String(head);
    size_t res = preferences.putString(key.c_str(), log_buffer);
    if (res == size)
    {
      Log.info("%s [%d]: log note written success\r\n", __FILE__, __LINE__);
    }
    else
    {
      Log.info("%s [%d]: log note writing failed\r\n", __FILE__, __LINE__);
    }

    head += 1;
    if (head == LOG_MAX_NOTES_NUMBER)
    {
      head = 0;
    }

    uint8_t result_write = preferences.putUChar(PREFERENCES_LOG_BUFFER_HEAD_KEY, head);
    if (result_write)
      Log.info("%s [%d]: head written success\r\n", __FILE__, __LINE__);
    else
      Log.info("%s [%d]: head note writing failed\r\n", __FILE__, __LINE__);
  }
}

void gather_stored_logs(String &log, Preferences &preferences)
{

  for (uint8_t i = 0; i < LOG_MAX_NOTES_NUMBER; i++)
  {
    String key = PREFERENCES_LOG_KEY + String(i);
    if (preferences.isKey(key.c_str()))
    {
      Log_info("log note exists");
      String note = preferences.getString(key.c_str(), "");
      if (note.length() > 0)
      {
        log += note;
        log += ",";
      }
    }
  }
}

void clear_stored_logs(Preferences &preferences)
{

  for (uint8_t i = 0; i < LOG_MAX_NOTES_NUMBER; i++)
  {
    String key = PREFERENCES_LOG_KEY + String(i);
    if (preferences.isKey(key.c_str()))
    {
      Log.info("%s [%d]: log note exists\r\n", __FILE__, __LINE__);
      bool note_del = preferences.remove(key.c_str());
      if (note_del)
        Log.info("%s [%d]: log note deleted\r\n", __FILE__, __LINE__);
      else
        Log.info("%s [%d]: log note not deleted\r\n", __FILE__, __LINE__);
    }
  }
}