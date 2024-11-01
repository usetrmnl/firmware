#ifndef BL_H
#define BL_H

/**
 * @brief Function to init business logic module
 * @param none
 * @return none
 */
void bl_init(void);

/**
 * @brief Function to process business logic module
 * @param none
 * @return none
 */
void bl_process(void);

typedef struct 
{
    char current_image[100];
    char current_error_message[100];
} ScreenStatus;

typedef struct DeviceStatusStamp
{
    int wifi_rssi_level;
    char wifi_status[30];
    int current_sleep_time;
    char current_fw_version[10];
    char special_function[100];
    float battery_voltage;
    char wakeup_reason[30];
    int free_heap_size;

    ScreenStatus screen_status;
    
} DeviceStatusStamp;

#endif