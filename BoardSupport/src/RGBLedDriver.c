#include <stdint.h>
#include <inc/RGBLedDriver.h>
#include <inc/I2CDriver.h>
#include "inc/hw_memmap.h"
#include <stdbool.h>

/* Algorithm to map the 16 bit LED data to 32 bits to send to the LS registers on the LP3943.
*  The algorithm zero-extends the 16 bit LED data to 32 bits, then interleaves the 0's with the LED data
*  to give each LED 2 bits to match the format of the LS registers.
*/
static uint32_t ConvertData(uint16_t LED_DATA, bool PWM)
{
    REGISTER_LEDS = PWM ? LED_DATA << 16 : LED_DATA; //ternary for PWM mode
    LEDShiftTemp = (REGISTER_LEDS ^ (REGISTER_LEDS >> 8)) & 0x0000ff00;
    REGISTER_LEDS ^= (LEDShiftTemp ^ (LEDShiftTemp << 8));
    LEDShiftTemp = (REGISTER_LEDS ^ (REGISTER_LEDS >> 4)) & 0x00f000f0;
    REGISTER_LEDS ^= (LEDShiftTemp ^ (LEDShiftTemp << 4));
    LEDShiftTemp = (REGISTER_LEDS ^ (REGISTER_LEDS >> 2)) & 0x0c0c0c0c;
    REGISTER_LEDS ^= (LEDShiftTemp ^ (LEDShiftTemp << 2));
    LEDShiftTemp = (REGISTER_LEDS ^ (REGISTER_LEDS >> 1)) & 0x22222222;
    REGISTER_LEDS ^= (LEDShiftTemp ^ (LEDShiftTemp << 1));
    return REGISTER_LEDS;
}


void LP3943_LedModeSet(uint32_t color, uint16_t LED_DATA, uint8_t PWM_duty)
{
    // Step 1: Set "SlaveAddress" for all colors
    // Hint: use the SetSlaveAddress function 
    uint8_t slaveAddr = 0b01100000 | color; //base addr | color
    SetSlaveAddress(I2C0_BASE, slaveAddr);
    // Step 2: Convert LED_DATA
    bool PWM_mode = PWM_duty != 255;
    uint32_t LED_CONFIG = ConvertData(LED_DATA, PWM_mode);
    //PWM set
    StartTransmission(I2C0_BASE, 0x12);
    ContinueTransmission(I2C0_BASE, 0x00);
    ContinueTransmission(I2C0_BASE, PWM_duty);
    EndTransmission(I2C0_BASE);
    // Send out LS register address
    StartTransmission(I2C0_BASE, 0x16);
    // Step 3: Send out LED Data
    // Hint: use the ContinueTransmission function
    ContinueTransmission(I2C0_BASE, LED_CONFIG);
    ContinueTransmission(I2C0_BASE, LED_CONFIG>>8);
    ContinueTransmission(I2C0_BASE, LED_CONFIG>>16);
    ContinueTransmission(I2C0_BASE, LED_CONFIG>>24);

    // End communication
    EndTransmission(I2C0_BASE);
}

void TurnOffLEDs(uint_desig color)
{
    LP3943_LedModeSet(color, 0x0000, 0x00);
}


void InitializeRGBLEDs()
{
    // Step 1: initialize I2C
    InitializeLEDI2C(I2C0_BASE);
    // Step 2: turn off LEDs (Red, Green, and Blue)
    TurnOffLEDs(LED_RED);
    TurnOffLEDs(LED_GREEN);
    TurnOffLEDs(LED_BLUE);

}
