#include "EPD_7in5_HD.h"
#include "Debug.h"



/******************************************************************************
function :	Software reset
parameter:
******************************************************************************/
static void EPD_7IN5_HD_Reset(void)
{

//    DEV_Digital_Write(EPD_RST_PIN, 1);
//    DEV_Delay_ms(200);
    DEV_Digital_Write(EPD_RST_PIN, 0);
    DEV_Delay_ms(2);
    DEV_Digital_Write(EPD_RST_PIN, 1);
    DEV_Delay_ms(200);
}

/******************************************************************************
function :	send command
parameter:
     Reg : Command register
******************************************************************************/
static void EPD_7IN5_HD_SendCommand(UBYTE Reg)
{
	DEV_Digital_Write(EPD_DC_PIN, 0);
    DEV_Digital_Write(EPD_CS_PIN, 0);
    DEV_SPI_WriteByte(Reg);
    DEV_Digital_Write(EPD_CS_PIN, 1);
}

/******************************************************************************
function :	send data
parameter:
    Data : Write data
******************************************************************************/
static void EPD_7IN5_HD_SendData(UBYTE Data)
{
	DEV_Digital_Write(EPD_DC_PIN, 1);
    DEV_Digital_Write(EPD_CS_PIN, 0);
    DEV_SPI_WriteByte(Data);
    DEV_Digital_Write(EPD_CS_PIN, 1);
}

/******************************************************************************
function :	Wait until the busy_pin goes LOW
parameter:
******************************************************************************/
void EPD_7IN5_HD_ReadBusy(void)
{
    UBYTE busy;
    printf("e-Paper busy\r\n");
    do {
        busy = DEV_Digital_Read(EPD_BUSY_PIN);
    } while(busy);
    printf("e-Paper busy release\r\n");
	DEV_Delay_ms(200);
}

/******************************************************************************
function :	Initialize the e-Paper register
parameter:
******************************************************************************/
void EPD_7IN5_HD_Init(void)
{
	EPD_7IN5_HD_Reset();
    
    EPD_7IN5_HD_SendCommand(0x12); 		  //SWRESET
    EPD_7IN5_HD_ReadBusy();        //waiting for the electronic paper IC to release the idle signal

    EPD_7IN5_HD_SendCommand(0x46);  // Auto Write RAM
    EPD_7IN5_HD_SendData(0xF7);
    EPD_7IN5_HD_ReadBusy();        //waiting for the electronic paper IC to release the idle signal

    EPD_7IN5_HD_SendCommand(0x47);  // Auto Write RAM
    EPD_7IN5_HD_SendData(0xF7);
    EPD_7IN5_HD_ReadBusy();        //waiting for the electronic paper IC to release the idle signal

    EPD_7IN5_HD_SendCommand(0x0C);  // Soft start setting
    EPD_7IN5_HD_SendData(0xAE);
	EPD_7IN5_HD_SendData(0xC7);
    EPD_7IN5_HD_SendData(0xC3);
    EPD_7IN5_HD_SendData(0xC0);
    EPD_7IN5_HD_SendData(0x40);   

    EPD_7IN5_HD_SendCommand(0x01);  // Set MUX as 527
    EPD_7IN5_HD_SendData(0xAF);
    EPD_7IN5_HD_SendData(0x02);
    EPD_7IN5_HD_SendData(0x01);

    EPD_7IN5_HD_SendCommand(0x11);  // Data entry mode
    EPD_7IN5_HD_SendData(0x01);

    EPD_7IN5_HD_SendCommand(0x44);
    EPD_7IN5_HD_SendData(0x00); // RAM x address start at 0
    EPD_7IN5_HD_SendData(0x00);
    EPD_7IN5_HD_SendData(0x6F); // RAM x address end at 36Fh -> 879
    EPD_7IN5_HD_SendData(0x03);
    EPD_7IN5_HD_SendCommand(0x45);
    EPD_7IN5_HD_SendData(0xAF); // RAM y address start at 20Fh;
    EPD_7IN5_HD_SendData(0x02);
    EPD_7IN5_HD_SendData(0x00); // RAM y address end at 00h;
    EPD_7IN5_HD_SendData(0x00);

    EPD_7IN5_HD_SendCommand(0x3C); // VBD
    EPD_7IN5_HD_SendData(0x01); // LUT1, for white

    EPD_7IN5_HD_SendCommand(0x18);
    EPD_7IN5_HD_SendData(0X80);
    EPD_7IN5_HD_SendCommand(0x22);
    EPD_7IN5_HD_SendData(0XB1);	//Load Temperature and waveform setting.
    EPD_7IN5_HD_SendCommand(0x20);
    EPD_7IN5_HD_ReadBusy();        //waiting for the electronic paper IC to release the idle signal

    EPD_7IN5_HD_SendCommand(0x4E); 
    EPD_7IN5_HD_SendData(0x00);
    EPD_7IN5_HD_SendData(0x00);
    EPD_7IN5_HD_SendCommand(0x4F); 
    EPD_7IN5_HD_SendData(0x00);
    EPD_7IN5_HD_SendData(0x00);
	

}

/******************************************************************************
function :	Clear screen
parameter:
******************************************************************************/
void EPD_7IN5_HD_Clear(void)
{
	UDOUBLE i, j, width, height;
    width = (EPD_7IN5_HD_WIDTH % 8 == 0)? (EPD_7IN5_HD_WIDTH / 8 ): (EPD_7IN5_HD_WIDTH / 8 + 1);
    height = EPD_7IN5_HD_HEIGHT;
	//EPD_7IN5_HD_ReadBusy();
	EPD_7IN5_HD_SendCommand(0x4F); 
    EPD_7IN5_HD_SendData(0x00);
    EPD_7IN5_HD_SendData(0x00);
	EPD_7IN5_HD_SendCommand(0x24);			//BLOCK
	for (j = 0; j < height; j++) {
        for (i = 0; i < width; i++){
			EPD_7IN5_HD_SendData(0xff);
		}
	}
	EPD_7IN5_HD_SendCommand(0x26);			
	for (j = 0; j < height; j++) {
        for (i = 0; i < width; i++){
			EPD_7IN5_HD_SendData(0xff);
		}
	}
	EPD_7IN5_HD_SendCommand(0x22);
	EPD_7IN5_HD_SendData(0xC7);
	EPD_7IN5_HD_SendCommand(0x20);
	DEV_Delay_ms(200);
	EPD_7IN5_HD_ReadBusy();
	printf("clear EPD\r\n");
}

/******************************************************************************
function :	Sends the image buffer in RAM to e-Paper and displays
parameter:
******************************************************************************/
void EPD_7IN5_HD_Display(const UBYTE *blackimage)
{
    UDOUBLE i, j, width, height;
    width = (EPD_7IN5_HD_WIDTH % 8 == 0)? (EPD_7IN5_HD_WIDTH / 8 ): (EPD_7IN5_HD_WIDTH / 8 + 1);
    height = EPD_7IN5_HD_HEIGHT;
	
	EPD_7IN5_HD_SendCommand(0x4F); 
    EPD_7IN5_HD_SendData(0x00);
    EPD_7IN5_HD_SendData(0x00);
	EPD_7IN5_HD_SendCommand(0x24);			//BLOCK
	for (j = 0; j < height; j++) {
        for (i = 0; i < width; i++){
			EPD_7IN5_HD_SendData(blackimage[i + j * width]);
		}
	}
	EPD_7IN5_HD_SendCommand(0x26);			//RED
	for (j = 0; j < height; j++) {
        for (i = 0; i < width; i++){
			EPD_7IN5_HD_SendData(0xff);
		}
	}
	EPD_7IN5_HD_SendCommand(0x22);
	EPD_7IN5_HD_SendData(0xC7);
	EPD_7IN5_HD_SendCommand(0x20);
	DEV_Delay_ms(100);
	EPD_7IN5_HD_ReadBusy(); 
	printf("display\r\n");
}

/******************************************************************************
function :	Enter sleep mode
parameter:
******************************************************************************/
void EPD_7IN5_HD_Sleep(void)
{
	EPD_7IN5_HD_SendCommand(0x10);  	//deep sleep
    EPD_7IN5_HD_SendData(0x01);
}