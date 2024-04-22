#ifndef TYPES_H
#define TYPES_H

#define WIFI_MAX_SSID_LENGTH 40
#define WIFI_MAX_PASSWORD_LENGTH 40

struct WifiCredentials
{
  char ssid[WIFI_MAX_SSID_LENGTH];
  char password[WIFI_MAX_PASSWORD_LENGTH];
};

typedef enum
{
  MODE_DEFAULT,
  MODE_BLE,
} MODE;

enum bmp_err_e
{
    BMP_NO_ERR,
    BMP_NOT_BMP,
    BMP_BAD_SIZE,
    BMP_COLOR_SCHEME_FAILED,
    BMP_INVALID_OFFSET,
};

#endif