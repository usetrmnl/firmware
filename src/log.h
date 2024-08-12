#include <ArduinoLog.h>

#define Log_info(format, ...) Log.info("%s [%d]: " format "\r\n", __FILE__, __LINE__, ##__VA_ARGS__)

#define Log_error(format, ...) Log.error("%s [%d]: " format "\r\n", __FILE__, __LINE__, ##__VA_ARGS__)

#define Log_fatal(format, ...) Log.fatal("%s [%d]: " format "\r\n", __FILE__, __LINE__, ##__VA_ARGS__)
