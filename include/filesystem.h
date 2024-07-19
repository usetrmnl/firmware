#include <cstdint>

bool writeBufferToFile(const char *name, uint8_t *in_buffer, uint16_t size); // file writing
bool readBufferFromFile(const char *name, uint8_t *out_buffer);		     // file reading
bool openFilesystem(bool formatOnFail);