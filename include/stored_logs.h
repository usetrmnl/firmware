#include <Preferences.h>

void store_log(const char *log_buffer, size_t size, Preferences &preferences);

void gather_stored_logs(String &log, Preferences &preferences);

void clear_stored_logs(Preferences &preferences);
