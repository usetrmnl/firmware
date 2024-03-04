#include <Arduino.h>

#include <display.h>
#include <ArduinoLog.h>
#include "DEV_Config.h"
#include "EPD.h"
#include "GUI_Paint.h"
#include <config.h>
#include <ImageData.h>

static const char *TAG = "display.cpp";

/**
 * @brief Function to init the display
 * @param none
 * @return bool true - if success; false - if failed
 */
void display_init(void)
{
    Log.info("%s [%d]: EPD_7IN5_V2_test Demo\r\n", TAG, __LINE__);
    DEV_Module_Init();

    Log.info("%s [%d]: e-Paper Init and Clear...\r\n", TAG, __LINE__);
    EPD_7IN5_V2_Init();
}

/**
 * @brief Function to reset the display
 * @param none
 * @return bool true - if success; false - if failed
 */
void display_reset(void)
{
    Log.info("%s [%d]: e-Paper Clear...\r\n", TAG, __LINE__);
    EPD_7IN5_V2_Clear();
    DEV_Delay_ms(500);
}

/**
 * @brief Function to show the image on the display
 * @param image_buffer - pointer to the uint8_t bmp image buffer with header
 * @return bool true - if success; false - if failed
 */
void display_show_image(uint8_t *image_buffer)
{
    //  Create a new image cache
    UBYTE *BlackImage;
    /* you have to edit the startup_stm32fxxx.s file and set a big enough heap size */
    UWORD Imagesize = ((EPD_7IN5_V2_WIDTH % 8 == 0) ? (EPD_7IN5_V2_WIDTH / 8) : (EPD_7IN5_V2_WIDTH / 8 + 1)) * EPD_7IN5_V2_HEIGHT;
    if ((BlackImage = (UBYTE *)malloc(Imagesize)) == NULL)
    {
        Log.error("%s [%d]: free heap - %d\r\n", TAG, __LINE__, ESP.getFreeHeap());
        Log.fatal("%s [%d]: Failed to apply for black memory...\r\n", TAG, __LINE__);
        while (1)
            ;
    }
    Log.info("%s [%d]: Paint_NewImage\r\n", TAG, __LINE__);
    Paint_NewImage(BlackImage, EPD_7IN5_V2_WIDTH, EPD_7IN5_V2_HEIGHT, 0, WHITE);

    Log.info("%s [%d]: show image for array\r\n", TAG, __LINE__);
    Paint_SelectImage(BlackImage);
    Paint_Clear(WHITE);
    Paint_DrawBitMap(image_buffer + 62);
    EPD_7IN5_V2_Display(BlackImage);
    Log.info("%s [%d]: display\r\n", TAG, __LINE__);
    // printf("Goto Sleep...\r\n");
    // DEV_Delay_ms(2000);
    // EPD_7IN5_V2_Sleep();
    free(BlackImage);
    BlackImage = NULL;
}

/**
 * @brief Function to show the image with message on the display
 * @param image_buffer - pointer to the uint8_t image buffer
 * @return bool true - if success; false - if failed
 */
void display_show_msg(uint8_t * image_buffer, MSG message_type)
{
    UBYTE *BlackImage;
    /* you have to edit the startup_stm32fxxx.s file and set a big enough heap size */
    UWORD Imagesize = ((EPD_7IN5_V2_WIDTH % 8 == 0) ? (EPD_7IN5_V2_WIDTH / 8) : (EPD_7IN5_V2_WIDTH / 8 + 1)) * EPD_7IN5_V2_HEIGHT;
    if ((BlackImage = (UBYTE *)malloc(Imagesize)) == NULL)
    {
        Log.error("%s [%d]: free heap - %d\r\n", TAG, __LINE__, ESP.getFreeHeap());
        Log.fatal("%s [%d]: Failed to apply for black memory...\r\n", TAG, __LINE__);
        while (1)
            ;
    }

    Log.info("%s [%d]: Paint_NewImage\r\n", TAG, __LINE__);
    Paint_NewImage(BlackImage, EPD_7IN5_V2_WIDTH, EPD_7IN5_V2_HEIGHT, 0, WHITE);

    Log.info("%s [%d]: show image for array\r\n", TAG, __LINE__);
    Paint_SelectImage(BlackImage);
    Paint_Clear(WHITE);
    Paint_DrawBitMap(image_buffer + 62);
    switch (message_type)
    {
    case WIFI_CONNECT:
    {
        Paint_DrawString_EN(225, 400, "Connect to TRMNL WiFi", &Font24, WHITE, BLACK);
        Paint_DrawString_EN(260, 430, "and plug in USB cable", &Font24, WHITE, BLACK);
    }
    break;
    case WIFI_FAILED:
    {
        Paint_DrawString_EN(0, 400, "Can't establsih WiFi connection with saved credentials", &Font24, WHITE, BLACK);
        Paint_DrawString_EN(75, 430, "Hold button on the back to reset WiFi", &Font24, WHITE, BLACK);
    }
    break;
    case WIFI_INTERNAL_ERROR:
    {
        Paint_DrawString_EN(300, 400, "WiFi connected", &Font24, WHITE, BLACK);
        Paint_DrawString_EN(100, 430, "But API connection cannot be established", &Font24, WHITE, BLACK);
    }
    break;
    case API_ERROR:
    {
        Paint_DrawString_EN(50, 400, "WiFi connected, TRMNL API not responding.", &Font24, WHITE, BLACK);
        Paint_DrawString_EN(25, 430, "Wait or reset WiFi by holding BOOT button.", &Font24, WHITE, BLACK);
    }
    break;
    case API_SIZE_ERROR:
    {
        Paint_DrawString_EN(30, 400, "WiFi connected, TRMNL API content malformed.", &Font24, WHITE, BLACK);
        Paint_DrawString_EN(55, 430, "Wait or reset by holding BOOT button.", &Font24, WHITE, BLACK);
    }
    break;
    case FW_UPDATE:
    {
        Paint_DrawString_EN(25, 400, "Firmware update is available! Starting now...", &Font24, WHITE, BLACK);
        Paint_DrawString_EN(170, 430, "This may take up to 5 minutes...", &Font24, WHITE, BLACK); 
    }
    break;
    case FW_UPDATE_FAILED:
    {
        Paint_DrawString_EN(15, 400, "Firmware update failed. Device will restart...", &Font24, WHITE, BLACK);
    }
    break;
    case FW_UPDATE_SUCCESS:
    {
        Paint_DrawString_EN(15, 400, "Firmware update success. Device will restart..", &Font24, WHITE, BLACK);
    }
    break;
    default:
        break;
    }

    EPD_7IN5_V2_Display(BlackImage);
    // DEV_Delay_ms(500);
    free(BlackImage);
    BlackImage = NULL;
}

/**
 * @brief Function to show the image with message on the display
 * @param image_buffer - pointer to the uint8_t image buffer
 * @return bool true - if success; false - if failed
 */
void display_show_msg(uint8_t * image_buffer, MSG message_type, String friendly_id)
{
    UBYTE *BlackImage;
    /* you have to edit the startup_stm32fxxx.s file and set a big enough heap size */
    UWORD Imagesize = ((EPD_7IN5_V2_WIDTH % 8 == 0) ? (EPD_7IN5_V2_WIDTH / 8) : (EPD_7IN5_V2_WIDTH / 8 + 1)) * EPD_7IN5_V2_HEIGHT;
    if ((BlackImage = (UBYTE *)malloc(Imagesize)) == NULL)
    {
        Log.error("%s [%d]: free heap - %d\r\n", TAG, __LINE__, ESP.getFreeHeap());
        Log.fatal("%s [%d]: Failed to apply for black memory...\r\n", TAG, __LINE__);
        while (1)
            ;
    }

    Log.info("%s [%d]: Paint_NewImage\r\n", TAG, __LINE__);
    Paint_NewImage(BlackImage, EPD_7IN5_V2_WIDTH, EPD_7IN5_V2_HEIGHT, 0, WHITE);

    Log.info("%s [%d]: show image for array\r\n", TAG, __LINE__);
    Paint_SelectImage(BlackImage);
    Paint_Clear(WHITE);
    Paint_DrawBitMap(image_buffer + 62);
    switch (message_type)
    {
    case WIFI_CONNECT:
    {
        Paint_DrawString_EN(250, 370, "Friendly ID: ", &Font24, WHITE, BLACK);
        Paint_DrawString_EN(460, 370, friendly_id.c_str(), &Font24, WHITE, BLACK);
        Paint_DrawString_EN(225, 400, "Connect to TRMNL WiFi", &Font24, WHITE, BLACK);
        Paint_DrawString_EN(270, 430, "And plug in USB", &Font24, WHITE, BLACK);
    }
    break;
    case WIFI_FAILED:
    {
        Paint_DrawString_EN(0, 400, "Can't establsih WiFi connection with saved WiFi", &Font24, WHITE, BLACK);
        Paint_DrawString_EN(75, 430, "Hold button on the back to reset WiFi", &Font24, WHITE, BLACK);
    }
    break;
    case API_ERROR:
    {
        Paint_DrawString_EN(50, 400, "WiFI connected, TRMNL API not responding.", &Font24, WHITE, BLACK);
        Paint_DrawString_EN(25, 430, "Wait or reset WiFi holding button on the back", &Font24, WHITE, BLACK);
    }
    break;
    case API_SIZE_ERROR:
    {
        Paint_DrawString_EN(15, 400, "WiFI connected, TRMNL API responded bad value.", &Font24, WHITE, BLACK);
        Paint_DrawString_EN(15, 430, "Wait or reset WiFi holding button on the back.", &Font24, WHITE, BLACK);
    }
    break;
    default:
        break;
    }

    EPD_7IN5_V2_Display(BlackImage);
    // DEV_Delay_ms(500);
    free(BlackImage);
    BlackImage = NULL;
}

/**
 * @brief Function to got the display to the sleep
 * @param none
 * @return none
 */
void display_sleep(void)
{
    Log.info("%s [%d]: Goto Sleep...\r\n", TAG, __LINE__);
    EPD_7IN5B_V2_Sleep();
}
