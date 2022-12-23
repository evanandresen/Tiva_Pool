/**
 * thread.h
 * @author:
 * uP2 - Fall 2022
 */

#ifndef THREADS_H_
#define THREADS_H_

#include <G8RTOS/G8RTOS.h>
#include <G8RTOS/G8RTOS_Semaphores.h>
#include <G8RTOS/G8RTOS_Structures.h>


#define JOYFIFO 0

#define NUM_BALLS 16 //max 16
#define cuePOWER 8
#define FRICTION 6




// define semaphores
semaphore_t* LEDMutex;
semaphore_t* LCDMutex;
semaphore_t* joyMutex;


//Threads
void NullThread(void);
void startGameT(void);
void displayInactiveBallsShotT(void);
void displayInactiveBallsScratchT(void);
void gameStatusLEDT(void);
void drawCueT(void);
void flickReadT(void);
void cueLEDT(void);
void activeBallT(void);
void nextTurnT(void);
void displayMovingBallsT(void);
void scratchMoveT(void);
void pickTurnT(void);


//Normal functions
void displayInactiveBalls(void);
void resetGameStats(void);
void resetTurnStats(void);
void sinkStripe(void);
void sinkSolid(void);
void sinkEight(void);
void sinkCue(void);
void updateGameTurn(void);

void setShotFlag(uint8_t val);
uint8_t getShotFlag(void);


void Pthread0(void);
void Pthread1(void);

void LCDtapISR(void);
void ButtonsISR(void);
void JoystickPressISR(void);

void BGThreadB0Reset(void);
void BGThreadB1Reset(void);
void BGThreadB2Reset(void);
void BGThreadB3Reset(void);
void BGThreadLCDTapReset(void);
void BGThreadJoystickPressReset(void);

#endif /* THREADS_H_ */
