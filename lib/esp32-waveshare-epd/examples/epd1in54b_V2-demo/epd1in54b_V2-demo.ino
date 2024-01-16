/* Includes ------------------------------------------------------------------*/
#include "DEV_Config.h"
#include "EPD.h"
#include "GUI_Paint.h"
#include "imagedata.h"
#include <stdlib.h>

/* Entry point ----------------------------------------------------------------*/
void setup()
{
  printf("1IN54 B V2 Demo\r\n");
  DEV_Module_Init();

  printf("e-Paper Init and Clear...\r\n");
  EPD_1IN54B_V2_Init();
  EPD_1IN54B_V2_Clear();
  DEV_Delay_ms(500);

  //Create a new image cache
  UBYTE *BlackImage, *RedImage;
  UWORD Imagesize = ((EPD_1IN54B_V2_WIDTH % 8 == 0) ? (EPD_1IN54B_V2_WIDTH / 8 ) : (EPD_1IN54B_V2_WIDTH / 8 + 1)) * EPD_1IN54B_V2_HEIGHT;
  if ((BlackImage = (UBYTE *)malloc(Imagesize)) == NULL) {
    printf("Failed to apply for black memory...\r\n");
    while (1);
  }
  if ((RedImage = (UBYTE *)malloc(Imagesize)) == NULL) {
    printf("Failed to apply for red memory...\r\n");
    while (1);
  }
  printf("NewImage:BlackImage and RedImage\r\n");
  Paint_NewImage(BlackImage, EPD_1IN54B_V2_WIDTH, EPD_1IN54B_V2_HEIGHT, 270, WHITE);
  Paint_NewImage(RedImage, EPD_1IN54B_V2_WIDTH, EPD_1IN54B_V2_HEIGHT, 270, WHITE);

#if 1  //show image for array
  printf("show image for array------------------------\r\n");
  Paint_SelectImage(BlackImage);
  Paint_DrawBitMap(gImage_1in54b_Black);

  Paint_SelectImage(RedImage);
  Paint_DrawBitMap(gImage_1in54b_Red);

  EPD_1IN54B_V2_Display(BlackImage, RedImage);
  DEV_Delay_ms(2000);
#endif
#if 1   // Drawing on the image
  printf("Drawing------------------------\r\n");
  Paint_SelectImage(BlackImage);
  Paint_Clear(WHITE);
  Paint_DrawPoint(5, 10, BLACK, DOT_PIXEL_1X1, DOT_STYLE_DFT);
  Paint_DrawPoint(5, 25, BLACK, DOT_PIXEL_2X2, DOT_STYLE_DFT);
  Paint_DrawLine(20, 10, 70, 60, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
  Paint_DrawLine(70, 10, 20, 60, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
  Paint_DrawRectangle(20, 10, 70, 60, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
  Paint_DrawCircle(170, 85, 20, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
  Paint_DrawString_EN(5, 70, "hello world", &Font16, WHITE, BLACK);
  Paint_DrawString_CN(5, 160, "微雪电子", &Font24CN, WHITE, BLACK);

  Paint_SelectImage(RedImage);
  Paint_Clear(WHITE);
  Paint_DrawPoint(5, 40, BLACK, DOT_PIXEL_3X3, DOT_STYLE_DFT);
  Paint_DrawPoint(5, 55, BLACK, DOT_PIXEL_4X4, DOT_STYLE_DFT);
  Paint_DrawLine(170, 15, 170, 55, BLACK, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);
  Paint_DrawLine(150, 35, 190, 35, BLACK, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);
  Paint_DrawRectangle(85, 10, 130, 60, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
  Paint_DrawCircle(170, 35, 20, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
  Paint_DrawString_EN(5, 90, "waveshare", &Font20, BLACK, WHITE);
  Paint_DrawNum(5, 120, 123456789, &Font20, BLACK, WHITE);
  Paint_DrawString_CN(5, 135, "你好abc", &Font12CN, BLACK, WHITE);

  EPD_1IN54B_V2_Display(BlackImage, RedImage);
  DEV_Delay_ms(2000);
#endif

  printf("Clear...\r\n");
  EPD_1IN54B_V2_Clear();

  printf("Goto Sleep...\r\n");
  EPD_1IN54B_V2_Sleep();
  free(BlackImage);
  free(RedImage);
  BlackImage = NULL;
  RedImage = NULL;

}

/* The main loop -------------------------------------------------------------*/
void loop()
{
  //
}
