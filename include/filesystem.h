#pragma once

#include <Arduino.h>
#include <stdint.h>
#include <trmnl_errors.h>

/**
 * @brief Function to init the filesystem
 * @param none
 * @return bool result
 */
bool filesystem_init(void);

/**
 * @brief Function to de-init the filesystem
 * @param none
 * @return none
 */
void filesystem_deinit(void);

uint8_t* filesystem_read_file(const char *name, uint32_t *size, trmnl_error* error);

// writes the whole buffer in  file
trmnl_error filesystem_write_file(const char *name, const uint8_t *in_buffer, size_t size);

/**
 * @brief Function to check if file exists
 * @param name filename
 * @return result - true if exists; false - if not exists
 */
bool filesystem_file_exists(const char *name, uint32_t* size = nullptr);

/**
 * @brief Function to delete the file
 * @param name filename
 * @return result - true if success; false - if failed
 */
bool filesystem_file_delete(const char *name);

/**
 * @brief Function to rename the file
 * @param old_name old filename
 * @param new_name new filename
 * @return result - true if success; false - if failed
 */
bool filesystem_file_rename(const char *old_name, const char *new_name);

bool filesystem_open_write_file(const char *name);
bool filesystem_open_read_file(const char *name);
bool filesystem_write_to_file(const uint8_t *in_buffer, uint32_t* size);
bool filesystem_read_from_file(uint8_t *out_buffer, uint32_t* size);
bool filesystem_close_write_file();
bool filesystem_close_read_file();