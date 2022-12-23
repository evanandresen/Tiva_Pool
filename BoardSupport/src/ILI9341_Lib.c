/**
 * LCD Library: ILI9341_Lib.c
 * uP2 - Fall 2022
 */

#include "ILI9341_Lib.h"
#include "AsciiLib.h"
#include "G8RTOS.h"
#include "G8RTOS_CriticalSection.h"

#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/ssi.h"
#include "driverlib/interrupt.h"
#include "BoardSupport/inc/demo_sysctl.h"

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/tm4c123gh6pm.h"

#include <stdbool.h>
#include <stdlib.h>

#include "src/tableIMG.c"

#define ADC_X_INVERSE .000565
#define ADC_Y_INVERSE .000560

/************************************  Private Functions  *******************************************/

/*
 * Write to TFT CS (PE0)
 */
static void WriteTFT_CS(uint8_t value)
{
    GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_0, value);
}


/*
 * Write to TFT DC for Registers (PD0)
 */
static void WriteTFT_DC(uint8_t value){
    GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_0, value);
}

/*
 * Write to TP CS (PE4)
 */
static void WriteTP_CS(uint8_t value)
{
    if (value == 1)
    {
        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_4, GPIO_PIN_4);
    }
    else
    {
        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_4, value);
    }
}

/*
 * Delay x ms
 */
static void Delay(unsigned long interval)
{
    while(interval > 0)
    {
        SysDelay(50000);
        interval--;
    }
}

/*******************************************************************************
 * Function Name  : LCD_initSPI
 * Description    : Configures LCD Control lines
 * Input          : None
 * Output         : None
 * Return         : None
 * Attention      : None
 *******************************************************************************/
static void LCD_initSPI()
{
    // Enable SPI Peripheral
    SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI0);

    // Enable GPIOA (SPI pins)
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    // Configure alternate pin functions
    GPIOPinConfigure(GPIO_PA2_SSI0CLK);
    //GPIOPinConfigure(GPIO_PA3_SSI0FSS);
  //  GPIOPinTypeGPIOOutput(GPIO_PORTA_BASE, GPIO_PIN_6);
    GPIOPinConfigure(GPIO_PA4_SSI0RX);
    GPIOPinConfigure(GPIO_PA5_SSI0TX);
    GPIOPinTypeSSI(GPIO_PORTA_BASE, GPIO_PIN_5 | GPIO_PIN_4 | GPIO_PIN_2);

    // Configure SPI Master mode
    SSIConfigSetExpClk(SSI0_BASE, SysCtlClockGet(), SSI_FRF_MOTO_MODE_3, SSI_MODE_MASTER, SysCtlClockGet()/2, 8);

    // Enable SPI
    SSIEnable(SSI0_BASE);

    // Enable TFT CS and TP CS
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    GPIOPinTypeGPIOOutput(GPIO_PORTE_BASE, GPIO_PIN_0 | GPIO_PIN_4);

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    GPIOPinTypeGPIOOutput(GPIO_PORTD_BASE, GPIO_PIN_0);

}

/*******************************************************************************
 * Function Name  : LCD_reset
 * Description    : Resets LCD
 * Input          : None
 * Output         : None
 * Return         : None
 * Attention      : Uses PD0 for reset
 *******************************************************************************/
static void LCD_reset()
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

    // Set PD0 as output
    GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_0);

    // Set pin high, low, and high
    GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_0, GPIO_PIN_0);
    //Delay(100);
    GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_0, 0);
    //Delay(100);
    GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_0, GPIO_PIN_0);
}
/************************************  End of Private Functions  *******************************************/



/************************************  Public Functions  *******************************************/

/******************************************************************************
 * Function Name  : PutChar
 * Description    : Lcd screen displays a character
 * Input          : - Xpos: Horizontal coordinate
 *                  - Ypos: Vertical coordinate
 *                  - ASCI: Displayed character
 *                  - charColor: Character color
 * Output         : None
 * Return         : None
 * Attention      : None
 *******************************************************************************/
inline void PutChar( uint16_t Xpos, uint16_t Ypos, uint8_t ASCI, uint16_t charColor)
{
    uint16_t i, j;
    uint8_t buffer[16], tmp_char;
    GetASCIICode(buffer,ASCI);  /* get font data */
    for( i=0; i<16; i++ )
    {
        tmp_char = buffer[i];
        for( j=0; j<8; j++ )
        {
            if( (tmp_char >> 7 - j) & 0x01 == 0x01 )
            {

                LCD_SetPoint( Xpos - j, Ypos - 4 + i, charColor );  /* Character color */
            }
        }
    }
}

/******************************************************************************
 * Function Name  : GUI_Text
 * Description    : Displays the string
 * Input          : - Xpos: Horizontal coordinate
 *                  - Ypos: Vertical coordinate
 *                  - str: Displayed string
 *                  - charColor: Character color
 * Output         : None
 * Return         : None
 * Attention      : None
 *******************************************************************************/
void LCD_Text(uint16_t Xpos, uint16_t Ypos, uint8_t *str, uint16_t Color)
{
    uint8_t TempChar;

    do
    {
        TempChar = *str++;
        PutChar( Xpos, Ypos, TempChar, Color);
        if(Xpos > 12 )
        {
            Xpos -= 8;

        }
        else if ( Ypos > 16)
        {
            Ypos += 10;
            Xpos = MAX_SCREEN_X-2;
        }
        else
        {
            Xpos = 8;
            Ypos = 16;
        }
    }
    while ( *str != 0 );
}

/******************************************************************************
 * Function Name  : LCD_SetPoint
 * Description    : Drawn at a specified point coordinates
 * Input          : - Xpos: Row Coordinate
 *                  - Ypos: Line Coordinate
 * Output         : None
 * Return         : None
 * Attention      : 18N Bytes Written
 *******************************************************************************/
void LCD_SetPoint(uint16_t Xpos, uint16_t Ypos, uint16_t color)
{
    // your code
    if(Xpos < MAX_SCREEN_X && Ypos < MAX_SCREEN_Y){
        LCD_SetAddress(Xpos,Ypos,Xpos,Ypos);
        WriteTFT_CS(0);
        LCD_PushColor(color);
        WriteTFT_CS(1);
        LCD_WriteIndex(0x00);
    }
}

/*******************************************************************************
 * Function Name  : LCD_Write_Data_Only
 * Description    : Data writing to the LCD controller
 * Input          : - data: data to be written
 * Output         : None
 * Return         : None
 * Attention      : None
 *******************************************************************************/
inline void LCD_Write_Data_Only(uint8_t data)
{

    SSI0_DR_R = ((data & 0xFF));                    /* Write D0..D7                 */
    while(SSIBusy(SSI0_BASE));
}


/*******************************************************************************
 * Function Name  : LCD_WriteData
 * Description    : LCD write register data
 * Input          : - data: register data
 * Output         : None
 * Return         : None
 * Attention      : None
 *******************************************************************************/
inline void LCD_WriteData(uint8_t data)
{
    WriteTFT_CS(0);
    WriteTFT_DC(1);
    SSI0_DR_R = ((data & 0xFF));                     /* Write D0..D7                 */

    while(SSIBusy(SSI0_BASE));

    WriteTFT_CS(1);
    WriteTFT_DC(1);
    //SysDelay(2);
}

/*******************************************************************************
 * Function Name  : LCD_WriteReg
 * Description    : Reads the selected LCD Register.
 * Input          : None
 * Output         : None
 * Return         : LCD Register Value.
 * Attention      : None
 *******************************************************************************/
inline uint8_t LCD_ReadReg(uint8_t LCD_Reg)
{
    // your code
    return 0;
}

/*******************************************************************************
 * Function Name  : LCD_WriteIndex
 * Description    : LCD write register address
 * Input          : - index: register address
 * Output         : None
 * Return         : None
 * Attention      : None
 *******************************************************************************/
inline void LCD_WriteIndex(uint8_t index)
{
    /* SPI write data */
    WriteTFT_CS(0);
    WriteTFT_DC(0);
    SSI0_DR_R = index;
    while(SSIBusy(SSI0_BASE));

    WriteTFT_CS(1);
    WriteTFT_DC(1);
    //SysDelay(2);
}


/*******************************************************************************
 * Function Name  : LCD_Write_Data_Start
 * Description    : Start of data writing to the LCD controller
 * Input          : None
 * Output         : None
 * Return         : None
 * Attention      : None
 *******************************************************************************/
//nline void LCD_Write_Data_Start(void)
//{
//    SSI0_DR_R = (SPI_START | SPI_WR | SPI_DATA);   /* Write : RS = 1, RW = 0 */
//    while(SSIBusy(SSI0_BASE));
//}

/*******************************************************************************
 * Function Name  : LCD_ReadData
 * Description    : LCD read data
 * Input          : None
 * Output         : None
 * Return         : return data
 * Attention      : Diagram (d) in datasheet
 *******************************************************************************/
//inline uint16_t LCD_ReadData()
//{
//    uint16_t value;     //Reads data
//    WriteTFT_CS(0);
//
//    SPISendRecvByte(SPI_START | SPI_RD | SPI_DATA);   /* Read: RS = 1, RW = 1   */
//    SPISendRecvByte(0);                               /* Dummy read 1           */
//    value = (SPISendRecvByte(0) << 8);                /* Read D8..D15           */
//    value |= SPISendRecvByte(0);                      /* Read D0..D7            */
//
//    WriteTFT_CS(1);
//    return value;
//}

/*******************************************************************************
 * Function Name  : LCD_WriteReg
 * Description    : Writes to the selected LCD register.
 * Input          : - LCD_Reg: address of the selected register.
 *                  - LCD_RegValue: value to write to the selected register.
 * Output         : None
 * Return         : None
 * Attention      : None
 *******************************************************************************/
inline void LCD_WriteReg(uint16_t LCD_Reg, uint16_t LCD_RegValue)
{
    LCD_WriteIndex(LCD_Reg);
    LCD_WriteData(LCD_RegValue);
}

/*******************************************************************************
 * Function Name  : LCD_SetCursor
 * Description    : Sets the cursor position.
 * Input          : - Xpos: specifies the X position.
 *                  - Ypos: specifies the Y position.
 * Output         : None
 * Return         : None
 * Attention      : None
 *******************************************************************************/
inline void LCD_SetCursor(uint16_t Xpos, uint16_t Ypos)
{
  // your code
}

/*******************************************************************************
 * Function Name  : LCD_Init
 * Description    : Configures LCD Control lines, sets whole screen black
 * Input          : bool usingTP: determines whether or not to enable TP interrupt
 * Output         : None
 * Return         : None
 * Attention      : None
 *******************************************************************************/
void LCD_Init(bool usingTP){
    LCD_initSPI();
    WriteTFT_CS(1);
    WriteTFT_DC(1);
    if (usingTP)
        IntDisable(INT_GPIOB);

    LCD_reset();

    LCD_WriteIndex(0x01);
    Delay(1);

    LCD_WriteIndex(0xEF);
    LCD_WriteData(0x03);
    LCD_WriteData(0x80);
    LCD_WriteData(0x02);

    // Power Control B
    LCD_WriteIndex(0xCF);
    LCD_WriteData(0x00);
    LCD_WriteData(0xC1);
    LCD_WriteData(0x30);

    // Power On Sequence Control
    LCD_WriteIndex(0xED);
    LCD_WriteData(0x64);
    LCD_WriteData(0x03);
    LCD_WriteData(0x12);
    LCD_WriteData(0x81);

    // Driver Timing Control A
    LCD_WriteIndex(0xE8);
    LCD_WriteData(0x85);
    LCD_WriteData(0x00);
    LCD_WriteData(0x78);

    // Power Control A
    LCD_WriteIndex(0xCB);
    LCD_WriteData(0x39);
    LCD_WriteData(0x2C);
    LCD_WriteData(0x00);
    LCD_WriteData(0x34);
    LCD_WriteData(0x02);

    // Pump Ratio Control
    LCD_WriteIndex(0xF7);
    LCD_WriteData(0x20);

    // Driver Timing Control B
    LCD_WriteIndex(0xEA);
    LCD_WriteData(0x00);
    LCD_WriteData(0x00);

    //Power Control, VRH[5:0]
    LCD_WriteIndex(0xC0);
    LCD_WriteData(0x23);

    //POWER CONTROL,SAP[2:0];BT[3:0]
    LCD_WriteIndex(0xC1);
    LCD_WriteData(0x10);

    //VCM CONTROL
    LCD_WriteIndex(0xC5);
    LCD_WriteData(0x3E);
    LCD_WriteData(0x28);

    //VCM CONTROL 2
    LCD_WriteIndex(0xC7);
    LCD_WriteData(0x86);

    //MEMORY ACCESS CONTROL
    LCD_WriteIndex(0x36);
    LCD_WriteData(0x48);

    //PIXEL FORMAT
    LCD_WriteIndex(0x3A);
    LCD_WriteData(0x55);

    //FRAME RATIO CONTROL, STANDARD RGB COLOR
    LCD_WriteIndex(0xB1);
    LCD_WriteData(0x00);
    LCD_WriteData(0x18);

    //DISPLAY FUNCTION CONTROL
    LCD_WriteIndex(0xB6);
    LCD_WriteData(0x08);
    LCD_WriteData(0x82);
    LCD_WriteData(0x27);

    //3GAMMA FUNCTION DISABLE
    LCD_WriteIndex(0xF2);
    LCD_WriteData(0x00);

    //GAMMA CURVE SELECTED
    LCD_WriteIndex(0x26);
    LCD_WriteData(0x01);

    //POSITIVE GAMMA CORRECTION
    LCD_WriteIndex(0xE0);
    WriteTFT_CS(0);
    LCD_Write_Data_Only(0x0F);
    LCD_Write_Data_Only(0x31);
    LCD_Write_Data_Only(0x2B);
    LCD_Write_Data_Only(0x0C);
    LCD_Write_Data_Only(0x0E);
    LCD_Write_Data_Only(0x08);
    LCD_Write_Data_Only(0x4E);
    LCD_Write_Data_Only(0xF1);
    LCD_Write_Data_Only(0x37);
    LCD_Write_Data_Only(0x07);
    LCD_Write_Data_Only(0x10);
    LCD_Write_Data_Only(0x03);
    LCD_Write_Data_Only(0x0E);
    LCD_Write_Data_Only(0x09);
    LCD_Write_Data_Only(0x00);
    WriteTFT_CS(1);

    //NEGATIVE GAMMA CORRECTION
    LCD_WriteIndex(0xE1);
    WriteTFT_CS(0);
    LCD_Write_Data_Only(0x00);
    LCD_Write_Data_Only(0x0E);
    LCD_Write_Data_Only(0x14);
    LCD_Write_Data_Only(0x03);
    LCD_Write_Data_Only(0x11);
    LCD_Write_Data_Only(0x07);
    LCD_Write_Data_Only(0x31);
    LCD_Write_Data_Only(0xC1);
    LCD_Write_Data_Only(0x48);
    LCD_Write_Data_Only(0x08);
    LCD_Write_Data_Only(0x0F);
    LCD_Write_Data_Only(0x0C);
    LCD_Write_Data_Only(0x31);
    LCD_Write_Data_Only(0x36);
    LCD_Write_Data_Only(0x0F);
    WriteTFT_CS(1);

    LCD_WriteIndex(0x11);
    Delay(120);

    LCD_WriteIndex(0x29);
    LCD_Clear(BLACK);
    if (usingTP)
    {
        // Initialize PB4 as input
        SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
        GPIOPinTypeGPIOInput(GPIO_PORTB_BASE, GPIO_PIN_4);
        GPIO_PORTB_PUR_R |= 0x10;

        // Initialize PB4 interrupt on falling edge
        GPIOIntClear(GPIO_PORTB_BASE, GPIO_INT_PIN_4);
        GPIOIntTypeSet(GPIO_PORTB_BASE, GPIO_PIN_4, GPIO_FALLING_EDGE);
        GPIOIntEnable(GPIO_PORTB_BASE, GPIO_INT_PIN_4);
        //IntEnable(INT_GPIOB); //moving to end of function
    }

}

/*******************************************************************************
* Function Name  : LCD_SetAddress
* Description    : Sets the draw area of the LCD
* Input          : uin16_t x1, y1, x2, y2: Represents the start and end LCD address for drawing
* Output         : None
* Return         : None
* Attention      : None
*******************************************************************************/
void LCD_SetAddress(uint16_t x1,uint16_t y1,uint16_t x2,uint16_t y2)//set coordinate for print or other function
{
    // your code

    LCD_WriteIndex(0x2A);
    WriteTFT_CS(0);
    LCD_Write_Data_Only(y1>>8);
    LCD_Write_Data_Only(y1);
    LCD_Write_Data_Only(y2>>8);
    LCD_Write_Data_Only(y2);
    WriteTFT_CS(1);

    LCD_WriteIndex(0x2B);
    WriteTFT_CS(0);
    LCD_Write_Data_Only(x1>>8);
    LCD_Write_Data_Only(x1);
    LCD_Write_Data_Only(x2>>8);
    LCD_Write_Data_Only(x2);
    WriteTFT_CS(1);

    LCD_WriteIndex(0x2C);
}

/*******************************************************************************
* Function Name  : LCD_PushColor
* Description    : Sets a pixel on the LCD to a color
* Input          : uint16_t color: 16 bit value of the color to output
* Output         : None
* Return         : None
* Attention      : Need to handle Write enable outside
*******************************************************************************/
void LCD_PushColor(uint16_t color)
{
    // your code
    LCD_Write_Data_Only(color>>8);
    LCD_Write_Data_Only(color);
}

/*******************************************************************************
* Function Name  : LCD_Clear
* Description    : Fill the screen as the specified color
* Input          : - Color: Screen Color
* Output         : None
* Return         : None
* Attention      : None
*******************************************************************************/
void LCD_Clear(uint16_t color)
{
    // your code
    LCD_DrawRectangle(0,0,MAX_SCREEN_X,MAX_SCREEN_Y,color);

}


/*******************************************************************************
 * Function Name  : LCD_DrawRectangle
 * Description    : Draw a rectangle as the specified color
 * Input          : x, y, w, h, color
 * Output         : None
 * Return         : None
 * Attention      : None
 *******************************************************************************/
void LCD_DrawRectangle(uint16_t x,uint16_t y,uint16_t w,uint16_t h,uint16_t color)
{
    // your code
    LCD_SetAddress(x,y,x+w-1,y+h-1);
    WriteTFT_CS(0);
    for(uint32_t i=0; i<h*w;i++){
            LCD_PushColor(color);
        }
    WriteTFT_CS(1);
    LCD_WriteIndex(0x00);
}

/*******************************************************************************
 * Function Name  : TP_ReadXY
 * Description    : Obtain X and Y touch coordinates
 * Input          : None
 * Output         : None
 * Return         : Pointer to "Point" structure
 * Attention      : None
 *******************************************************************************/
Point TP_ReadXY()
{
    Point coor;
    uint8_t highByte, lowByte;

    WriteTP_CS(0);
    SSI0_DR_R = (CHX);              //Reads X data
    while(SSIBusy(SSI0_BASE));
    highByte = SSI0_DR_R;
    while(SSIBusy(SSI0_BASE));
    lowByte = SSI0_DR_R;
    while(SSIBusy(SSI0_BASE));

    coor.x = highByte << 8;              /* Read D8..D15           */
    coor.x |= lowByte;              /* Read D0..D7            */
    coor.x >>= 4;                   //Accounts for offset and scales down
    coor.x -= 190;
    coor.x *= (ADC_X_INVERSE * MAX_SCREEN_X);

    SSI0_DR_R = (CHY);              //Reads Y data
    while(SSIBusy(SSI0_BASE));
    highByte = SSI0_DR_R;
    while(SSIBusy(SSI0_BASE));
    lowByte = SSI0_DR_R;
    while(SSIBusy(SSI0_BASE));

    coor.y = highByte << 8;              /* Read D8..D15           */
    coor.y |= lowByte;              /* Read D0..D7            */
    coor.y >>= 4;                   //Accounts for offset and scales down
    coor.y -= 140;
    coor.y *= (ADC_Y_INVERSE * MAX_SCREEN_Y);
    WriteTP_CS(1);

    return coor;
}

void drawTablePoint(uint16_t X, uint16_t Y){    //tableIMG(pixel) -> LCD
    if(Y < MAX_SCREEN_Y && X < MAX_SCREEN_X)
        LCD_SetPoint(X, Y, POOLTABLE_IMAGE[Y+X*240]);
}

void drawTable(){                           //tableIMG -> LCD
    for(int i=0; i<240; i++){
        for(int j=0; j<320; j++){
            drawTablePoint(j,i);
        }
    }
}

void drawCue(uint16_t Xtip, uint16_t Ytip, uint16_t Xend, uint16_t Yend, int del){
    drawCueLine(Xtip-1,Ytip, Xend-1,Yend, del);
    drawCueLine(Xtip,Ytip+2, Xend,Yend+2, del);
    drawCueLine(Xtip,Ytip-1, Xend,Yend-1, del);
    drawCueLine(Xtip+2,Ytip+1, Xend+2,Yend+1, del);
    drawCueLine(Xtip+1,Ytip-1, Xend+1,Yend-1, del);
    drawCueLine(Xtip+1,Ytip+2, Xend+1,Yend+2, del);
    drawCueLine(Xtip-1,Ytip+1, Xend-1,Yend+1, del);
    drawCueLine(Xtip+2,Ytip, Xend+2,Yend, del);

}

void drawCueLine(uint16_t Xcenter, uint16_t Ycenter, uint16_t Xtip, uint16_t Ytip, int del){
    int16_t deltY = Ycenter-Ytip;
    int16_t deltX = Xcenter-Xtip;
    uint16_t slope;
    if(deltX == 0 || deltY == 0)
        slope = 999;
    else
        slope = abs(deltX) < abs(deltY) ? abs(deltY/deltX) : abs(deltX/deltY);
    uint16_t slopeStep = slope >= 1 ? slope : 1;

    signed int xDir = deltX >= 0 ? -1 : 1;
    signed int yDir = deltY >= 0 ? -1 : 1;
    int colorPick = 0;
    signed int length = 130;
    if(abs(deltY) <= abs(deltX)){
        while(length > 0){
            for(int i = 0; i < slopeStep; i++){
                drawPix(Xtip, Ytip, colorPick, del);
                Xtip += xDir;
                length--;
                if(length == 90 || length == 128)
                    colorPick++;
                if(length==0 || Ytip >4000 || Xtip >4000)
                    break;
            }
            drawPix(Xtip, Ytip, colorPick, del);
            Ytip += yDir;
            length--;
            if(length == 90 || length == 128)
                colorPick++;
        }
    }
    else{
        while(length > 0){
            for(int i = 0; i < slopeStep; i++){
                drawPix(Xtip, Ytip, colorPick, del);
                Ytip += yDir;
                length--;
                if(length == 90 || length == 128)
                    colorPick++;
                if(length==0 || Ytip >4000 || Xtip >4000)
                    break;
            }
            drawPix(Xtip, Ytip, colorPick, del);
            Xtip += xDir;
            length--;
            if(length == 90 || length == 128)
                colorPick++;
        }
    }
}

void drawPix(uint16_t x, uint16_t y, int color, int del){
    if(del){
        drawTablePoint(x, y);
    }
    else{
        uint16_t colors[3]= {0xffff, 0xDDF0, 0x0000};
        LCD_SetPoint(x, y, colors[color]);
    }
}

//a 0bxxx1 will mean green and 0bxxxx0 black

//x and y are LCD res, not physics scale
void drawBall(uint16_t x, uint16_t y, uint16_t color, int stripped){
    //do in 5 horizontal strips

    //center box
    for(int xDir = x-6; xDir <x+7; xDir++){
        for(int yDir = y-2; yDir <y+3; yDir++){
            LCD_SetPoint(xDir, yDir, color);
        }
    }
    //upper and low mid fifth
    if(stripped)
        color = WHITE;
    for(int xDir = x-4; xDir <x+5; xDir++){
        for(int yDir = y+3; yDir <y+5; yDir++){
            LCD_SetPoint(xDir, yDir, color);
        }
    }
    for(int xDir = x-4; xDir <x+5; xDir++){
        for(int yDir = y-4; yDir <y-2; yDir++){
            LCD_SetPoint(xDir, yDir, color);
        }
    }
    //top and bottom rows
    for(int xDir = x-2; xDir <x+3; xDir++){
        for(int yDir = y+5; yDir <y+7; yDir++){
            LCD_SetPoint(xDir, yDir, color);
        }
    }
    for(int xDir = x-2; xDir <x+3; xDir++){
        for(int yDir = y-6; yDir <y-4; yDir++){
            LCD_SetPoint(xDir, yDir, color);
        }
    }
}

void eraseBall(uint16_t x, uint16_t y){
    for(int xDir = x-6; xDir <x+7; xDir++){
        for(int yDir = y-2; yDir <y+3; yDir++){
            drawTablePoint(xDir, yDir);
        }
    }
    //upper and low mid fifth
    for(int xDir = x-4; xDir <x+5; xDir++){
        for(int yDir = y+3; yDir <y+5; yDir++){
            drawTablePoint(xDir, yDir);
        }
    }
    for(int xDir = x-4; xDir <x+5; xDir++){
        for(int yDir = y-4; yDir <y-2; yDir++){
            drawTablePoint(xDir, yDir);
        }
    }
    //top and bottom rows
    for(int xDir = x-2; xDir <x+3; xDir++){
        for(int yDir = y+5; yDir <y+7; yDir++){
            drawTablePoint(xDir, yDir);
        }
    }
    for(int xDir = x-2; xDir <x+3; xDir++){
        for(int yDir = y-6; yDir <y-4; yDir++){
            drawTablePoint(xDir, yDir);
        }
    }
}
