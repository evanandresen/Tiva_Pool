/**
 * main.c
 * @author:
 * uP2 - Fall 2022
 */

// Include all headers you might need
#include <G8RTOS/G8RTOS_IPC.h>
#include <G8RTOS/G8RTOS_Semaphores.h>
#include <G8RTOS/G8RTOS_Structures.h>
#include "threads.h"
#include <stdbool.h>
#include <stdlib.h>
#include "driverlib/watchdog.h"
#include "inc/hw_memmap.h"
#include "inc/tm4c123gh6pm.h"
#include "BoardSupport/inc/BoardInitialization.h"
#include "inc/RGBLedDriver.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "BoardSupport/inc/ILI9341_lib.h"
#include "BoardSupport/inc/AsciiLib.h"
#include <string.h>
#include <math.h>


void main(void)
{
    // your code
    // Initialize the G8RTOS
    G8RTOS_Init();
    // Initialize the BSP
    //bool isBoardSetup = InitializeBoard();

    // Initialize Sensor semaphore
    LEDMutex = (semaphore_t*) malloc(sizeof(semaphore_t));
    G8RTOS_InitSemaphore(LEDMutex, 1);
    LCDMutex = (semaphore_t*) malloc(sizeof(semaphore_t));
    G8RTOS_InitSemaphore(LCDMutex, 1);
    joyMutex = (semaphore_t*) malloc(sizeof(semaphore_t));
    G8RTOS_InitSemaphore(joyMutex, 1);


    // Add background thread 0 to 4
    G8RTOS_AddThread(NullThread, 255, "NOP");
    G8RTOS_AddThread(startGameT, 50, "startGame");


    float num = -14.38;
    int yes = fabsf(num) > 4.2;
    // Add periodic thread 0


    //Add aperiodic threads
    G8RTOS_AddAPeriodicEvent(&LCDtapISR, 2, INT_GPIOB);
    G8RTOS_AddAPeriodicEvent(&ButtonsISR, 3, INT_GPIOC);
    G8RTOS_AddAPeriodicEvent(&JoystickPressISR, 4, INT_GPIOE);
    srand(time(NULL));


    // Launch the G8RTOS
    G8RTOS_Launch();
}

