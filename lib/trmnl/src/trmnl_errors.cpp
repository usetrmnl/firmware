#include <trmnl_errors.h>

const char *error_get_text(trmnl_error error) {
  switch (error) {
  case NO_ERROR:
    return "No error";
  case MALLOC_FAILED:
    return "Malloc failed";
  case FS_FAIL:
    return "File system failed";
  case FILE_NOT_FOUND:
    return "File not found";
  case SIZE_TOO_LARGE:
    return "Size too large";
  case SIZE_TOO_SMALL:
    return "Size too small";
  case IMAGE_CREATION_FAILED:
    return "image creation failed";
  case IMAGE_STRUCTURE_CORRUPTED:
    return "image structure corrupted";
  case IMAGE_FORMAT_UKNOWN:
    return "Unknown format";
  case IMAGE_FORMAT_UNEXPECTED_SIGNATURE:
    return "Signature is not what is expected";
  case IMAGE_FORMAT_UNSUPPORTED:
    return "Unsupported format";
  case IMAGE_INCOMPATIBLE:
    return "Incompatible image";
  case IMAGE_COLORS_INVALID:
    return "Invalid colors";
  case IMAGE_HEADER_INVALID:
    return "Invalid header";
  case IMAGE_DECODE_FAIL:
    return "Image decoding failed"; 
default:
    return "Unknown error";
  }
}
