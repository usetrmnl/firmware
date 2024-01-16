#ifndef EPD7IN5_HD_H
#define EPD7IN5_HD_H

#include "DEV_Config.h"

// Display resolution
#define EPD_7IN5_HD_WIDTH       880
#define EPD_7IN5_HD_HEIGHT      528


void EPD_7IN5_HD_Init(void);
void EPD_7IN5_HD_Clear(void);
void EPD_7IN5_HD_Display(const UBYTE *blackimage);
void EPD_7IN5_HD_Sleep(void);



#endif /* EPD7IN5_H */

