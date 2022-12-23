/**
 * thread.c
 * @author:
 * uP2 - Fall 2022
 */


// Include all headers you might need
#include <G8RTOS/G8RTOS_IPC.h>
#include <G8RTOS/G8RTOS_Scheduler.h>

#include "inc/hw_memmap.h"
#include "inc/tm4c123gh6pm.h"
#include <stdint.h>
#include "BoardSupport/inc/bmi160_support.h"
#include "BoardSupport/inc/bme280_support.h"
#include "driverlib/sysctl.h"
#include "threads.h"
#include "BoardSupport/inc/RGBLedDriver.h"
#include "BoardSupport/inc/ILI9341_lib.h"
#include "driverlib/gpio.h"
#include <string.h>
#include <time.h>
#include <math.h>


// declare all global variables you might need
struct gameStatus{
    uint8_t numSolidsLeft;
    uint8_t numStripesLeft;
    uint8_t eightLeft;
    uint8_t stripesWon;
    uint8_t solidsWon;
    uint8_t turnsPicked;
    uint8_t solidsTurn;
    uint8_t blinkSolidsLEDs;
    uint8_t blinkStripesLEDs;
    uint8_t takingShotFlag;
    semaphore_t mutex;
};
struct turnStats{

    uint8_t solidPocketed;
    uint8_t stripePocketed;
    uint8_t eightPocketed;
    uint8_t cuePocketed;
    semaphore_t mutex;
};
struct buttons{
    uint8_t b0;
    uint8_t b1;
    uint8_t b2;
    uint8_t b3;
    semaphore_t mutex;
};
struct gameStatus gameStats;
struct turnStats turnStats;
struct buttons buttonFlags;
uint32_t joyCoords[2];





void NullThread(void)
{
    while(1);   //empty thread
}

void startGameT(void)   //game thread
{
    initBallData();
    initBallFIFO();
    G8RTOS_InitSemaphore(&gameStats.mutex, 1);
    G8RTOS_InitSemaphore(&turnStats.mutex, 1);
    G8RTOS_InitSemaphore(&buttonFlags.mutex, 1);
    resetGameStats();
    resetTurnStats();
    drawTable();
    displayInactiveBalls();
    setShotFlag(0);
    G8RTOS_AddThread(gameStatusLEDT, 100, "Game LEDs");
    G8RTOS_AddThread(flickReadT, 10, "flickRead");
    G8RTOS_KillSelf();


}

void gameStatusLEDT(void){
    uint16_t SolidsLEDmasks[8]={0x0000,0x0001,0x0003,0x0007,0x000f,0x001f,0x003f,0x007f};
    uint16_t StripesLEDmasks[8]={0x0000,0x8000,0xc000,0xe000,0xf000,0xf800,0xfc00,0xfe00};
    uint16_t eightLEDmask = 0x0180;
    uint16_t LEDoutputRED;
    uint16_t LEDoutputGREEN;
    uint16_t LEDoutputBLUE;
    uint16_t blinkMask = 0xffff;

    while(!getShotFlag()){
        G8RTOS_WaitSemaphore(&gameStats.mutex);
        LEDoutputBLUE = gameStats.eightLeft ? eightLEDmask : 0x0000;
        LEDoutputRED = (~SolidsLEDmasks[7-gameStats.numSolidsLeft] & 0x007f) |(~StripesLEDmasks[7-gameStats.numStripesLeft] & 0xfe00);
        LEDoutputGREEN = (SolidsLEDmasks[7-gameStats.numSolidsLeft] & 0x007f) |(StripesLEDmasks[7-gameStats.numStripesLeft] & 0xfe00);
        if(gameStats.stripesWon){
            LEDoutputGREEN = 0x0100 | StripesLEDmasks[7] | SolidsLEDmasks[7-gameStats.numStripesLeft];
            LEDoutputBLUE = 0x0100;
        }
        else if(gameStats.solidsWon){
            LEDoutputGREEN = 0x0080 | SolidsLEDmasks[7] | StripesLEDmasks[7-gameStats.numStripesLeft];
            LEDoutputBLUE = 0x0080;
        }
        if(gameStats.blinkSolidsLEDs && blinkMask != 0xff00)
            blinkMask = 0xff00;
        else if(gameStats.blinkStripesLEDs && blinkMask != 0x00ff)
            blinkMask = 0x00ff;
        else
            blinkMask = 0xffff;
        G8RTOS_SignalSemaphore(&gameStats.mutex);


        G8RTOS_WaitSemaphore(LEDMutex);
        LP3943_LedModeSet(LED_RED, LEDoutputRED & blinkMask, 120);
        G8RTOS_SignalSemaphore(LEDMutex);
        sleep(1);
        G8RTOS_WaitSemaphore(LEDMutex);
        LP3943_LedModeSet(LED_GREEN, LEDoutputGREEN & blinkMask, 120);
        G8RTOS_SignalSemaphore(LEDMutex);
        sleep(1);
        G8RTOS_WaitSemaphore(LEDMutex);
        LP3943_LedModeSet(LED_BLUE, LEDoutputBLUE & blinkMask, 120);
        G8RTOS_SignalSemaphore(LEDMutex);

        sleep(600);

    }
    G8RTOS_KillSelf();
}

void nextTurnT(void){
    while(1){
        if(ballsStopped()){
            updateGameTurn();
            sleep(1000);
            resetTurnStats();
            G8RTOS_AddThread(flickReadT, 10, "flickRead");
            G8RTOS_KillSelf();
        }
        sleep(100);
    }
}

void activeBallT(void){
    uint8_t ball = popBallFIFO();
    int pocketed = 0;
    while(poolBalls[ball].xVelo != 0 || poolBalls[ball].yVelo != 0){
        G8RTOS_WaitSemaphore(&poolBalls[ball].mutex);
        moveBallTick(ball);
        checkDoCollideWall(ball);
        checkDoBallCollisions(ball);
        pocketed = checkBallPocketed(ball);
        G8RTOS_SignalSemaphore(&poolBalls[ball].mutex);
        if(pocketed){
            if(poolBalls[ball].isStriped){
                sinkStripe();
                UARTprintf("Stripe number %1d pocketed\n", ball);
            }
            else if(ball != 0 && ball != 8){
                sinkSolid();
                UARTprintf("Solid number %1d pocketed\n", ball);
            }
            else if(ball == 8){
                sinkEight();
                UARTprintf("Eight ball pocketed\n");
            }
            else
                sinkCue();
            sleep(400);
            G8RTOS_WaitSemaphore(LCDMutex);
            eraseBall(poolBalls[ball].xPos/coordScale, poolBalls[ball].yPos/coordScale);
            G8RTOS_SignalSemaphore(LCDMutex);
        }

        sleep(14);

    }
    poolBalls[ball].isActive = 0;
    G8RTOS_KillSelf();
}

void displayInactiveBallsShotT(void){
    while(getShotFlag()){
        displayInactiveBalls();
    }
    G8RTOS_KillSelf();
}

void displayInactiveBallsScratchT(void){
    while(!getShotFlag()){
        displayInactiveBalls();
    }
    G8RTOS_KillSelf();
}

void flickReadT(void)
{
    setShotFlag(0);
    threadId_t drawCue, cueLED, displayBalls;
    while(1){
        //poll joystick to see if moved
        GetJoystickCoordinates(joyCoords);
        if((joyCoords[0]>2368 | joyCoords[0]<1728 | joyCoords[1]>2368 | joyCoords[1]<1728)){
            setShotFlag(1);
            drawCue = G8RTOS_AddThread(drawCueT, 50, "drawCue");
            cueLED = G8RTOS_AddThread(cueLEDT, 50, "cueLEDs");
            displayBalls = G8RTOS_AddThread(displayInactiveBallsShotT, 60, "DisplayDed");
            break;
        }
        sleep(300);
    }
    while(1){
        //poll joystick quickly for drawing
        GetJoystickCoordinates(joyCoords);
        if(!getShotFlag()){
            poolBalls[0].xVelo = (2048.0 - joyCoords[0])/64*cuePOWER;
            poolBalls[0].yVelo = (2048.0 - joyCoords[1])/64*cuePOWER;
            poolBalls[0].isActive = 1;
            G8RTOS_AddThread(nextTurnT, 190, "nextTurnCheck");
            G8RTOS_AddThread(displayMovingBallsT, 20, "DisplayMoving");
            sleep(5);
            pushBallFIFO(0);
            G8RTOS_AddThread(activeBallT, 20, "Cueball");
            sleep(1000);
            G8RTOS_AddThread(gameStatusLEDT, 100, "Game LEDs");
            G8RTOS_KillSelf();
        }
        if(joyCoords[0]<2368 && joyCoords[0]>1728 && joyCoords[1]<2368 && joyCoords[1]>1728){
            //stop cue polling
            G8RTOS_WaitSemaphore(LEDMutex);
            G8RTOS_WaitSemaphore(&gameStats.mutex);
            G8RTOS_KillThread(cueLED);
            G8RTOS_SignalSemaphore(LEDMutex);
            G8RTOS_WaitSemaphore(LCDMutex);
            G8RTOS_KillThread(displayBalls);
            G8RTOS_KillThread(drawCue);
            G8RTOS_SignalSemaphore(&gameStats.mutex);
            G8RTOS_SignalSemaphore(LCDMutex);
            setShotFlag(0);
            G8RTOS_AddThread(gameStatusLEDT, 100, "Game LEDs");
            G8RTOS_AddThread(flickReadT, 10, "flickRead");
            G8RTOS_KillSelf();
        }
        sleep(20);
    }
}

void cueLEDT(void)
{
    uint16_t LEDmasks[16]={0x0001,0x0003,0x0007,0x000f,0x001f,0x003f,0x007f,0x00ff,
                           0x01ff,0x03ff,0x07ff,0x0fff,0x1fff,0x3fff,0x7fff,0xffff};
    int16_t power = abs(joyCoords[0]-2048) + abs(joyCoords[1]-2048);
    int16_t LEDs;
    while(getShotFlag()){
        power = abs(joyCoords[0]-2048) + abs(joyCoords[1]-2048);
        LEDs = LEDmasks[(power/246)-1];
        G8RTOS_WaitSemaphore(LEDMutex);
        LP3943_LedModeSet(LED_RED, LEDs & 0x0f00, 150);
        G8RTOS_SignalSemaphore(LEDMutex);
        sleep(1);
        G8RTOS_WaitSemaphore(LEDMutex);
        LP3943_LedModeSet(LED_GREEN, LEDs & 0x00ff, 150);
        G8RTOS_SignalSemaphore(LEDMutex);
        sleep(1);
        G8RTOS_WaitSemaphore(LEDMutex);
        LP3943_LedModeSet(LED_BLUE, LEDs & 0xf000, 150);
        G8RTOS_SignalSemaphore(LEDMutex);
        sleep(50);
    }
    G8RTOS_KillSelf();
}

void drawCueT(void)    //start round
{
    uint16_t xCenter = poolBalls[0].xPos;
    uint16_t yCenter = poolBalls[0].yPos;
    int16_t xPos = xCenter/coordScale + (joyCoords[0]-2048)/40;
    int16_t yPos = yCenter/coordScale + (joyCoords[1]-2048)/40;
    int16_t tempX;
    int16_t tempY;
    while(getShotFlag()){
        tempX = xPos;
        tempY = yPos;
        G8RTOS_WaitSemaphore(LCDMutex);
        drawCue(xCenter/coordScale,yCenter/coordScale, xPos, yPos, 0);
        //G8RTOS_SignalSemaphore(LCDMutex);
        sleep(30);
        //G8RTOS_WaitSemaphore(LCDMutex);
        drawCue(xCenter/coordScale, yCenter/coordScale, xPos, yPos, 1);
        G8RTOS_SignalSemaphore(LCDMutex);
        xPos =  xCenter/coordScale + (joyCoords[0]-2048)/32;
        yPos =  yCenter/coordScale + (joyCoords[1]-2048)/32;

    }
    xPos =  (xCenter*3/coordScale + tempX)/4;
    yPos =  (yCenter*3/coordScale + tempY)/4;
    G8RTOS_WaitSemaphore(LCDMutex);
    drawCue(xCenter/coordScale, yCenter/coordScale, xPos, yPos, 0);
    G8RTOS_SignalSemaphore(LCDMutex);
    sleep(800);
    G8RTOS_WaitSemaphore(LCDMutex);
    drawCue(xCenter/coordScale, yCenter/coordScale, xPos, yPos, 1);
    G8RTOS_SignalSemaphore(LCDMutex);
    displayInactiveBalls();
    G8RTOS_KillSelf();
}

void displayMovingBallsT(void){
    uint16_t xCoords[16];
    uint16_t yCoords[16];
    for(int i=0; i< NUM_BALLS; i++){
        xCoords[i] = poolBalls[i].xPos;
        yCoords[i] = poolBalls[i].yPos;
    }
    while(!ballsStopped()){
        for(int i=0; i< NUM_BALLS; i++){
            if(poolBalls[i].xPos != xCoords[i] ||  poolBalls[i].yPos != yCoords[i]){
                G8RTOS_WaitSemaphore(LCDMutex);
                eraseBall(xCoords[i]/coordScale, yCoords[i]/coordScale);
                G8RTOS_WaitSemaphore(&poolBalls[i].mutex);
                drawBall(poolBalls[i].xPos/coordScale, poolBalls[i].yPos/coordScale, poolBalls[i].color, poolBalls[i].isStriped);
                xCoords[i] = poolBalls[i].xPos;
                yCoords[i] = poolBalls[i].yPos;
                G8RTOS_SignalSemaphore(&poolBalls[i].mutex);
                G8RTOS_SignalSemaphore(LCDMutex);
                //sleep(2);
            }
        }
        sleep(1);
    }
    G8RTOS_KillSelf();
}

void scratchMoveT(void){
    G8RTOS_AddThread(displayInactiveBallsScratchT, 8, "ShowBalls");
    G8RTOS_WaitSemaphore(&buttonFlags.mutex);
    buttonFlags.b2 = 0;
    G8RTOS_SignalSemaphore(&buttonFlags.mutex);
    poolBalls[0].xPos = 240*coordScale;
    poolBalls[0].yPos = 120*coordScale;
    while(1){
        G8RTOS_WaitSemaphore(LCDMutex);
        G8RTOS_WaitSemaphore(&poolBalls[0].mutex);
        eraseBall(poolBalls[0].xPos/coordScale, poolBalls[0].yPos/coordScale);
        GetJoystickCoordinates(joyCoords);
        poolBalls[0].xPos += (joyCoords[0]-2048)/16;
        poolBalls[0].yPos += (joyCoords[1]-2048)/16;
        if(poolBalls[0].xPos < 27*coordScale)
            poolBalls[0].xPos = 27*coordScale;
        else if(poolBalls[0].xPos > (320-27)*coordScale)
            poolBalls[0].xPos = (320-27)*coordScale;
        if(poolBalls[0].yPos < 25*coordScale)
            poolBalls[0].yPos = 25*coordScale;
        else if(poolBalls[0].yPos > (240-25)*coordScale)
            poolBalls[0].yPos = (240-25)*coordScale;
        drawBall(poolBalls[0].xPos/coordScale, poolBalls[0].yPos/coordScale, poolBalls[0].color, poolBalls[0].isStriped);
        G8RTOS_SignalSemaphore(&poolBalls[0].mutex);
        G8RTOS_SignalSemaphore(LCDMutex);

        G8RTOS_WaitSemaphore(&buttonFlags.mutex);
        if(buttonFlags.b2)
            break;
        G8RTOS_SignalSemaphore(&buttonFlags.mutex);
        sleep(20);
    }
    G8RTOS_SignalSemaphore(&buttonFlags.mutex);
    G8RTOS_WaitSemaphore(&poolBalls[0].mutex);
    poolBalls[0].isOnTable = 1;
    G8RTOS_SignalSemaphore(&poolBalls[0].mutex);
    G8RTOS_KillSelf();
}

void pickTurnT(void){
    G8RTOS_WaitSemaphore(&buttonFlags.mutex);
    buttonFlags.b1 = 0;
    buttonFlags.b3 = 0;
    G8RTOS_SignalSemaphore(&buttonFlags.mutex);
    while(1){
        G8RTOS_WaitSemaphore(&buttonFlags.mutex);
        if(buttonFlags.b1){
            gameStats.solidsTurn = 0;
            break;
        }
        else if(buttonFlags.b3){
            gameStats.solidsTurn = 1;
            break;
        }
        G8RTOS_SignalSemaphore(&buttonFlags.mutex);
        sleep(20);
    }
    G8RTOS_SignalSemaphore(&buttonFlags.mutex);
    G8RTOS_KillSelf();
}

void Pthread0(void)
{
}


//Button ISRs
void LCDtapISR(void){
    GPIOIntDisable(GPIO_PORTB_BASE, GPIO_INT_PIN_4);
    UARTprintf("LCD interrupt at  %4d ms\n", SystemTime);
    G8RTOS_AddThread(BGThreadLCDTapReset, 2, "Reset LCD Tap");
}
void ButtonsISR(void){
    uint32_t gpioINTS = GPIOIntStatus(GPIO_PORTC_BASE, false);
    if(gpioINTS & GPIO_INT_PIN_4){
        GPIOIntDisable(GPIO_PORTC_BASE, GPIO_INT_PIN_4);
        UARTprintf("B0 interrupt at  %4d ms\n", SystemTime);
        G8RTOS_AddThread(BGThreadB0Reset, 2, "Button B0 Reset");
    }
    if(gpioINTS & GPIO_INT_PIN_5){
        GPIOIntDisable(GPIO_PORTC_BASE, GPIO_INT_PIN_5);
        UARTprintf("B1 interrupt at  %4d ms\n", SystemTime);
        G8RTOS_AddThread(BGThreadB1Reset, 2, "Button B1 Reset");
    }
    if(gpioINTS & GPIO_INT_PIN_6){
        GPIOIntDisable(GPIO_PORTC_BASE, GPIO_INT_PIN_6);
        UARTprintf("B2 interrupt at  %4d ms\n", SystemTime);
        G8RTOS_AddThread(BGThreadB2Reset, 2, "Button B2 Reset");
    }
    if(gpioINTS & GPIO_INT_PIN_7){
        GPIOIntDisable(GPIO_PORTC_BASE, GPIO_INT_PIN_7);
        UARTprintf("B3 interrupt at  %4d ms\n", SystemTime);
        G8RTOS_AddThread(BGThreadB3Reset, 2, "Button B3 Reset");
    }
}
void JoystickPressISR(void){
    GPIOIntDisable(GPIO_PORTE_BASE, GPIO_INT_PIN_1);
    UARTprintf("Joystick Press interrupt at  %4d ms\n", SystemTime);
    G8RTOS_AddThread(BGThreadJoystickPressReset, 2, "Reset Joystick Press");
}

//ISR RESETERS

void BGThreadB0Reset(void){
    G8RTOS_WaitSemaphore(LCDMutex);
    drawTable();
    G8RTOS_SignalSemaphore(LCDMutex);
    displayInactiveBalls();
    sleep(400);
    GPIOIntClear(GPIO_PORTC_BASE, GPIO_INT_PIN_4);
    GPIOIntEnable(GPIO_PORTC_BASE, GPIO_INT_PIN_4);
    G8RTOS_KillSelf();
}
void BGThreadB1Reset(void){
    G8RTOS_WaitSemaphore(&buttonFlags.mutex);
    buttonFlags.b1 = 1;
    G8RTOS_SignalSemaphore(&buttonFlags.mutex);
    sleep(100);
    GPIOIntClear(GPIO_PORTC_BASE, GPIO_INT_PIN_5);
    GPIOIntEnable(GPIO_PORTC_BASE, GPIO_INT_PIN_5);
    G8RTOS_KillSelf();
}
void BGThreadB2Reset(void){
    setShotFlag(0);
    G8RTOS_WaitSemaphore(&buttonFlags.mutex);
    buttonFlags.b2 = 1;
    G8RTOS_SignalSemaphore(&buttonFlags.mutex);
    sleep(100);
    GPIOIntClear(GPIO_PORTC_BASE, GPIO_INT_PIN_6);
    GPIOIntEnable(GPIO_PORTC_BASE, GPIO_INT_PIN_6);
    G8RTOS_KillSelf();
}
void BGThreadB3Reset(void){
    G8RTOS_WaitSemaphore(&buttonFlags.mutex);
    buttonFlags.b3 = 1;
    G8RTOS_SignalSemaphore(&buttonFlags.mutex);
    sleep(100);
    GPIOIntClear(GPIO_PORTC_BASE, GPIO_INT_PIN_7);
    GPIOIntEnable(GPIO_PORTC_BASE, GPIO_INT_PIN_7);
    G8RTOS_KillSelf();
}
void BGThreadJoystickPressReset(void){
    sleep(300);
    GPIOIntClear(GPIO_PORTE_BASE, GPIO_INT_PIN_1);
    GPIOIntEnable(GPIO_PORTE_BASE, GPIO_INT_PIN_1);
    G8RTOS_KillSelf();
}
void BGThreadLCDTapReset(void){
    sleep(333);
    Point p = TP_ReadXY();
    GPIOIntClear(GPIO_PORTB_BASE, GPIO_INT_PIN_4);
    GPIOIntEnable(GPIO_PORTB_BASE, GPIO_INT_PIN_4);
    G8RTOS_KillSelf();
}

//REGULAR FUNCTIONS
void displayInactiveBalls(void){
    for(int i = 0; i < NUM_BALLS; i++){
        if(!poolBalls[i].isActive && poolBalls[i].isOnTable){
            G8RTOS_WaitSemaphore(LCDMutex);
            drawBall(poolBalls[i].xPos/coordScale, poolBalls[i].yPos/coordScale, poolBalls[i].color, poolBalls[i].isStriped);
            G8RTOS_SignalSemaphore(LCDMutex);
            sleep(3);
        }

    }
}

void resetGameStats(void){
    G8RTOS_WaitSemaphore(&gameStats.mutex);
    gameStats.numSolidsLeft = 7;
    gameStats.numStripesLeft = 7;
    gameStats.eightLeft = 1;
    gameStats.stripesWon = 0;
    gameStats.solidsWon = 0;
    gameStats.turnsPicked = 0;
    gameStats.solidsTurn = 0;
    G8RTOS_SignalSemaphore(&gameStats.mutex);
}

void resetTurnStats(void){
    G8RTOS_WaitSemaphore(&turnStats.mutex);
    turnStats.solidPocketed = 0;
    turnStats.stripePocketed = 0;
    turnStats.eightPocketed = 0;
    turnStats.cuePocketed = 0;
    G8RTOS_SignalSemaphore(&turnStats.mutex);
}

void sinkStripe(void){
    G8RTOS_WaitSemaphore(&gameStats.mutex);
    gameStats.numStripesLeft--;
    G8RTOS_SignalSemaphore(&gameStats.mutex);
    G8RTOS_WaitSemaphore(&turnStats.mutex);
    turnStats.stripePocketed = 1;
    G8RTOS_SignalSemaphore(&turnStats.mutex);
}

void sinkSolid(void){
    G8RTOS_WaitSemaphore(&gameStats.mutex);
    gameStats.numSolidsLeft--;
    G8RTOS_SignalSemaphore(&gameStats.mutex);
    G8RTOS_WaitSemaphore(&turnStats.mutex);
    turnStats.solidPocketed = 1;
    G8RTOS_SignalSemaphore(&turnStats.mutex);
}

void sinkEight(void){
    G8RTOS_WaitSemaphore(&gameStats.mutex);
    gameStats.eightLeft = 0;
    G8RTOS_SignalSemaphore(&gameStats.mutex);
    G8RTOS_WaitSemaphore(&turnStats.mutex);
    turnStats.eightPocketed = 1;
    G8RTOS_SignalSemaphore(&turnStats.mutex);
}

void sinkCue(void){
    G8RTOS_WaitSemaphore(&turnStats.mutex);
    turnStats.cuePocketed = 1;
    G8RTOS_SignalSemaphore(&turnStats.mutex);
}

void updateGameTurn(void){
    G8RTOS_WaitSemaphore(&turnStats.mutex);
    G8RTOS_WaitSemaphore(&gameStats.mutex);

    if(gameStats.turnsPicked){ //all cases where turns switch
        if(turnStats.eightPocketed){
            if(gameStats.solidsTurn && !gameStats.numSolidsLeft && !turnStats.cuePocketed ||
                !gameStats.solidsTurn && (gameStats.numStripesLeft || turnStats.cuePocketed)){
                UARTprintf("Solids won\n");
                gameStats.solidsWon = 1;
                gameStats.blinkSolidsLEDs = 1;
                gameStats.blinkStripesLEDs = 0;
            }
            else{
                UARTprintf("Stripes won\n");
                gameStats.stripesWon = 1;
                gameStats.blinkSolidsLEDs = 0;
                gameStats.blinkStripesLEDs = 1;
            }
        }
        else if(turnStats.solidPocketed && turnStats.stripePocketed || gameStats.solidsTurn &&  turnStats.stripePocketed ||
           !gameStats.solidsTurn &&  turnStats.solidPocketed || !turnStats.solidPocketed && !turnStats.stripePocketed ||
           turnStats.cuePocketed){
            gameStats.solidsTurn = !gameStats.solidsTurn;
            gameStats.blinkSolidsLEDs = gameStats.solidsTurn;
            gameStats.blinkStripesLEDs = !gameStats.solidsTurn;
        }
    }
    else{ //!gameStats.turnsPicked
            if(turnStats.solidPocketed && turnStats.stripePocketed){
                gameStats.turnsPicked = 1;
                UARTprintf("Pick balls\n");
                gameStats.blinkSolidsLEDs=1;
                gameStats.blinkStripesLEDs=1;
                G8RTOS_AddThread(pickTurnT, 3, "PickTurn");
                G8RTOS_SignalSemaphore(&gameStats.mutex);
                sleep(1); //yield to that thread
                while(1){ //wait til picked
                    G8RTOS_WaitSemaphore(&buttonFlags.mutex);
                    if(buttonFlags.b1 || buttonFlags.b3){
                        G8RTOS_SignalSemaphore(&buttonFlags.mutex);
                        break;
                    }
                    G8RTOS_SignalSemaphore(&buttonFlags.mutex);
                    sleep(30);
                }
                G8RTOS_WaitSemaphore(&gameStats.mutex);
                gameStats.blinkSolidsLEDs = gameStats.solidsTurn;
                gameStats.blinkStripesLEDs = !gameStats.solidsTurn;
            }
            else if(turnStats.solidPocketed != turnStats.stripePocketed){
                gameStats.solidsTurn = turnStats.solidPocketed;
                gameStats.turnsPicked = 1;
                gameStats.blinkSolidsLEDs = gameStats.solidsTurn;
                gameStats.blinkStripesLEDs = !gameStats.solidsTurn;
            }
    }
    if(turnStats.cuePocketed && !turnStats.eightPocketed){
        UARTprintf("Scratch\n");
        G8RTOS_AddThread(scratchMoveT, 3, "ScratchMove");
        G8RTOS_SignalSemaphore(&gameStats.mutex);
        sleep(1); //yield to that thread
        while(1){
            G8RTOS_WaitSemaphore(&buttonFlags.mutex);
            if(buttonFlags.b2){
                G8RTOS_SignalSemaphore(&buttonFlags.mutex);
                break;
            }
            G8RTOS_SignalSemaphore(&buttonFlags.mutex);
            sleep(30);
        }
        G8RTOS_WaitSemaphore(&gameStats.mutex);
    }
    if(gameStats.turnsPicked){
        if(gameStats.solidsTurn)
            UARTprintf("Solids' turn\n");
        else
            UARTprintf("Stripes' turn\n");
    }
    G8RTOS_SignalSemaphore(&gameStats.mutex);
    G8RTOS_SignalSemaphore(&turnStats.mutex);
}

void setShotFlag(uint8_t val){
    G8RTOS_WaitSemaphore(&gameStats.mutex);
    gameStats.takingShotFlag = val;
    G8RTOS_SignalSemaphore(&gameStats.mutex);
}

uint8_t getShotFlag(void){
    uint8_t val;
    G8RTOS_WaitSemaphore(&gameStats.mutex);
    val = gameStats.takingShotFlag;
    G8RTOS_SignalSemaphore(&gameStats.mutex);
    return val;
}






