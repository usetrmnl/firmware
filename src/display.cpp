#include <Arduino.h>

#include <display.h>
#include <ArduinoLog.h>
#include "DEV_Config.h"
#include "EPD.h"
#include "GUI_Paint.h"
#include <config.h>
#include <ImageData.h>

/**
 * @brief Function to init the display
 * @param none
 * @return bool true - if success; false - if failed
 */
void display_init(void)
{
    Log.info("%s [%d]: dev module start\r\n", __FILE__, __LINE__);
    DEV_Module_Init();
    Log.info("%s [%d]: dev module end\r\n", __FILE__, __LINE__);

    Log.info("%s [%d]: screen hw start\r\n", __FILE__, __LINE__);
    EPD_7IN5_V2_Init_New();
    Log.info("%s [%d]: screen hw end\r\n", __FILE__, __LINE__);
}

/**
 * @brief Function to reset the display
 * @param none
 * @return bool true - if success; false - if failed
 */
void display_reset(void)
{
    Log.info("%s [%d]: e-Paper Clear start\r\n", __FILE__, __LINE__);
    EPD_7IN5_V2_Clear();
    Log.info("%s [%d]:  e-Paper Clear end\r\n", __FILE__, __LINE__);
    // DEV_Delay_ms(500);
}

/**
 * @brief Function to show the image on the display
 * @param image_buffer - pointer to the uint8_t bmp image buffer with header
 * @return bool true - if success; false - if failed
 */
void display_show_image(uint8_t *image_buffer, bool reverse)
{
    //  Create a new image cache
    UBYTE *BlackImage;
    /* you have to edit the startup_stm32fxxx.s file and set a big enough heap size */
    UWORD Imagesize = ((EPD_7IN5_V2_WIDTH % 8 == 0) ? (EPD_7IN5_V2_WIDTH / 8) : (EPD_7IN5_V2_WIDTH / 8 + 1)) * EPD_7IN5_V2_HEIGHT;
    Log.error("%s [%d]: free heap - %d\r\n", __FILE__, __LINE__, ESP.getFreeHeap());
    Log.error("%s [%d]: free alloc heap - %d\r\n", __FILE__, __LINE__, ESP.getMaxAllocHeap());
    if ((BlackImage = (UBYTE *)malloc(Imagesize)) == NULL)
    {
        Log.fatal("%s [%d]: Failed to apply for black memory...\r\n", __FILE__, __LINE__);
        ESP.restart();
    }
    Log.info("%s [%d]: Paint_NewImage %d\r\n", __FILE__, __LINE__, reverse);
    // if (reverse)
    //     Paint_NewImage(BlackImage, EPD_7IN5_V2_WIDTH, EPD_7IN5_V2_HEIGHT, 0, BLACK);
    // else
    Paint_NewImage(BlackImage, EPD_7IN5_V2_WIDTH, EPD_7IN5_V2_HEIGHT, 0, WHITE);

    Log.info("%s [%d]: show image for array\r\n", __FILE__, __LINE__);
    Paint_SelectImage(BlackImage);
    Paint_Clear(WHITE);
    if (reverse)
    {
        for (size_t i = 0; i < DISPLAY_BMP_IMAGE_SIZE; i++)
        {
            image_buffer[i] = ~image_buffer[i];
        }
    }
    Paint_DrawBitMap(image_buffer + 62);
    EPD_7IN5_V2_Display(BlackImage);
    Log.info("%s [%d]: display\r\n", __FILE__, __LINE__);

    free(BlackImage);
    BlackImage = NULL;
}

/**
 * @brief Function to show the image with message on the display
 * @param image_buffer - pointer to the uint8_t image buffer
 * @return bool true - if success; false - if failed
 */
void display_show_msg(uint8_t *image_buffer, MSG message_type)
{
    UBYTE *BlackImage;
    /* you have to edit the startup_stm32fxxx.s file and set a big enough heap size */
    UWORD Imagesize = ((EPD_7IN5_V2_WIDTH % 8 == 0) ? (EPD_7IN5_V2_WIDTH / 8) : (EPD_7IN5_V2_WIDTH / 8 + 1)) * EPD_7IN5_V2_HEIGHT;
    Log.error("%s [%d]: free heap - %d\r\n", __FILE__, __LINE__, ESP.getFreeHeap());
    Log.error("%s [%d]: free alloc heap - %d\r\n", __FILE__, __LINE__, ESP.getMaxAllocHeap());
    if ((BlackImage = (UBYTE *)malloc(Imagesize)) == NULL)
    {
        Log.fatal("%s [%d]: Failed to apply for black memory...\r\n", __FILE__, __LINE__);
        ESP.restart();
    }

    Log.info("%s [%d]: Paint_NewImage\r\n", __FILE__, __LINE__);
    Paint_NewImage(BlackImage, EPD_7IN5_V2_WIDTH, EPD_7IN5_V2_HEIGHT, 0, WHITE);

    Log.info("%s [%d]: show image for array\r\n", __FILE__, __LINE__);
    Paint_SelectImage(BlackImage);
    Paint_Clear(WHITE);
    Paint_DrawBitMap(image_buffer + 62);
    switch (message_type)
    {
    case WIFI_CONNECT:
    {
        char string1[] = "Connect to TRMNL WiFi";
        Paint_DrawString_EN((800 - sizeof(string1) * 17 > 9) ? (800 - sizeof(string1) * 17) / 2 + 9 : 0, 400, string1, &Font24, WHITE, BLACK);
        char string2[] = "on your phone or computer";
        Paint_DrawString_EN((800 - sizeof(string2) * 17 > 9) ? (800 - sizeof(string2) * 17) / 2 + 9 : 0, 430, string2, &Font24, WHITE, BLACK);
    }
    break;
    case WIFI_FAILED:
    {
        char string1[] = "Can't establish WiFi connection";
        Paint_DrawString_EN((800 - sizeof(string1) * 17 > 9) ? (800 - sizeof(string1) * 17) / 2 + 9 : 0, 400, string1, &Font24, WHITE, BLACK);
        char string2[] = "Hold button on the back to reset WiFi";
        Paint_DrawString_EN((800 - sizeof(string2) * 17 > 9) ? (800 - sizeof(string2) * 17) / 2 + 9 : 0, 430, string2, &Font24, WHITE, BLACK);
    }
    break;
    case WIFI_INTERNAL_ERROR:
    {
        char string1[] = "WiFi connected";
        Paint_DrawString_EN((800 - sizeof(string1) > 9) * 17 ? (800 - sizeof(string1) * 17) / 2 + 9 : 0, 400, string1, &Font24, WHITE, BLACK);
        char string2[] = "But API connection cannot be established";
        Paint_DrawString_EN((800 - sizeof(string2) > 9) * 17 ? (800 - sizeof(string2) * 17) / 2 + 9 : 0, 430, string2, &Font24, WHITE, BLACK);
    }
    break;
    case WIFI_WEAK:
    {
        char string1[] = "WiFi connected but signal is weak";
        Paint_DrawString_EN((800 - sizeof(string1) > 9) * 17 ? (800 - sizeof(string1) * 17) / 2 + 9 : 0, 400, string1, &Font24, WHITE, BLACK);
    }
    break;
    case API_ERROR:
    {
        char string1[] = "WiFi connected, TRMNL not responding.";
        Paint_DrawString_EN((800 - sizeof(string1) > 9) * 17 ? (800 - sizeof(string1) * 17) / 2 + 9 : 0, 400, string1, &Font24, WHITE, BLACK);
        char string2[] = "Wait or reset by holding button on back.";
        Paint_DrawString_EN((800 - sizeof(string2) > 9) * 17 ? (800 - sizeof(string2) * 17) / 2 + 9 : 0, 430, string2, &Font24, WHITE, BLACK);
    }
    break;
    case API_SIZE_ERROR:
    {
        char string1[] = "WiFi connected, TRMNL content malformed.";
        Paint_DrawString_EN((800 - sizeof(string1) > 9) * 17 ? (800 - sizeof(string1) * 17) / 2 + 9 : 0, 400, string1, &Font24, WHITE, BLACK);
        char string2[] = "Wait or reset by holding button on back.";
        Paint_DrawString_EN((800 - sizeof(string2) > 9) * 17 ? (800 - sizeof(string2) * 17) / 2 + 9 : 0, 430, string2, &Font24, WHITE, BLACK);
    }
    break;
    case FW_UPDATE:
    {
        char string1[] = "Firmware update available! Starting now...";
        Paint_DrawString_EN((800 - sizeof(string1) > 9) * 17 ? (800 - sizeof(string1) * 17) / 2 + 9 : 0, 400, string1, &Font24, WHITE, BLACK);
    }
    break;
    case FW_UPDATE_FAILED:
    {
        char string1[] = "Firmware update failed. Device will restart...";
        Paint_DrawString_EN((800 - sizeof(string1) > 9) * 17 ? (800 - sizeof(string1) * 17) / 2 + 9 : 0, 400, string1, &Font24, WHITE, BLACK);
    }
    break;
    case FW_UPDATE_SUCCESS:
    {
        char string1[] = "Firmware update success. Device will restart..";
        Paint_DrawString_EN((800 - sizeof(string1) > 9) * 17 ? (800 - sizeof(string1) * 17) / 2 + 9 : 0, 400, string1, &Font24, WHITE, BLACK);
    }
    break;
    case BMP_FORMAT_ERROR:
    {
        char string1[] = "The image format is incorrect";
        Paint_DrawString_EN((800 - sizeof(string1) > 9) * 17 ? (800 - sizeof(string1) * 17) / 2 + 9 : 0, 400, string1, &Font24, WHITE, BLACK);
    }
    break;
    case TEST:
    {
        Paint_DrawString_EN(0, 0, "ABCDEFGHIYABCDEFGHIYABCDEFGHIYABCDEFGHIYABCDEFGHIY", &Font24, WHITE, BLACK);
        Paint_DrawString_EN(0, 40, "abcdefghiyabcdefghiyabcdefghiyabcdefghiyabcdefghiy", &Font24, WHITE, BLACK);
        Paint_DrawString_EN(0, 80, "A B C D E F G H I Y A B C D E F G H I Y A B C D E", &Font24, WHITE, BLACK);
        Paint_DrawString_EN(0, 120, "a b c d e f g h i y a b c d e f g h i y a b c d e", &Font24, WHITE, BLACK);
    }
    break;
    default:
        break;
    }

    EPD_7IN5_V2_Display(BlackImage);
    Log.info("%s [%d]: display\r\n", __FILE__, __LINE__);
    free(BlackImage);
    BlackImage = NULL;
}

/**
 * @brief Function to show the image with message on the display
 * @param image_buffer - pointer to the uint8_t image buffer
 * @return bool true - if success; false - if failed
 */
void display_show_msg(uint8_t *image_buffer, MSG message_type, String friendly_id, bool id, const char *fw_version, String message)
{
    UBYTE *BlackImage;
    /* you have to edit the startup_stm32fxxx.s file and set a big enough heap size */
    UWORD Imagesize = ((EPD_7IN5_V2_WIDTH % 8 == 0) ? (EPD_7IN5_V2_WIDTH / 8) : (EPD_7IN5_V2_WIDTH / 8 + 1)) * EPD_7IN5_V2_HEIGHT;
    Log.error("%s [%d]: free heap - %d\r\n", __FILE__, __LINE__, ESP.getFreeHeap());
    Log.error("%s [%d]: free alloc heap - %d\r\n", __FILE__, __LINE__, ESP.getMaxAllocHeap());
    if ((BlackImage = (UBYTE *)malloc(Imagesize)) == NULL)
    {
        Log.fatal("%s [%d]: Failed to apply for black memory...\r\n", __FILE__, __LINE__);
        ESP.restart();
    }

    Log.info("%s [%d]: Paint_NewImage\r\n", __FILE__, __LINE__);
    Paint_NewImage(BlackImage, EPD_7IN5_V2_WIDTH, EPD_7IN5_V2_HEIGHT, 0, WHITE);

    Log.info("%s [%d]: show image for array\r\n", __FILE__, __LINE__);
    Paint_SelectImage(BlackImage);
    Paint_Clear(WHITE);
    Paint_DrawBitMap(image_buffer + 62);
    switch (message_type)
    {
    case FRIENDLY_ID:
    {
        Log.info("%s [%d]: friendly id case\r\n", __FILE__, __LINE__);
        char string1[] = "Please sign up at usetrmnl.com/signup";
        Paint_DrawString_EN((800 - sizeof(string1) * 17 > 9) ? (800 - sizeof(string1) * 17) / 2 + 9 : 0, 400, string1, &Font24, WHITE, BLACK);

        String string2 = "with Friendly ID ";
        if (id)
        {
            string2 += friendly_id;
        }
        string2 += " to finish setup";
        Paint_DrawString_EN((800 - string2.length() * 17 > 9) ? (800 - string2.length() * 17) / 2 + 9 : 0, 430, string2.c_str(), &Font24, WHITE, BLACK);
    }
    break;
    case WIFI_CONNECT:
    {
        Log.info("%s [%d]: wifi connect case\r\n", __FILE__, __LINE__);
        String string1 = "FW: ";
        string1 += fw_version;
        string1 += ". WiFi is not saved.";
        Log.info("%s [%d]: size of string1 - %d\r\n", __FILE__, __LINE__, string1.length());
        Paint_DrawString_EN((800 - string1.length() * 17 > 9) ? (800 - string1.length() * 17) / 2 + 9 : 0, 400, string1.c_str(), &Font24, WHITE, BLACK);
        char string2[] = "Please connect to TRMNL WiFi";
        Paint_DrawString_EN((800 - sizeof(string2) * 17 > 9) ? (800 - sizeof(string2) * 17) / 2 + 9 : 0, 430, string2, &Font24, WHITE, BLACK);
    }
    break;
    default:
        break;
    }
    Log.info("%s [%d]: Start drawing...\r\n", __FILE__, __LINE__);
    EPD_7IN5_V2_Display(BlackImage);
    Log.info("%s [%d]: display\r\n", __FILE__, __LINE__);
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
    Log.info("%s [%d]: Goto Sleep...\r\n", __FILE__, __LINE__);
    EPD_7IN5B_V2_Sleep();
}
