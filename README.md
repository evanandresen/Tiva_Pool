# Tiva_Pool
__Implementation of a pool game on a Tiva TM4C123G and daughterboard__

##Implementation

__G8RTOS__
This game was written under a custom Real-Time Operating System for the TM4C123G called G8RTOS. It uses a priority scheduler for the dynamic threads along with sleeping and semaphores for shared resources and data. Periodic threads and Aperiodic threads are also possible under the RTOS but only Aperiodic threads (ISRs) are used for button detection.

__Pool Threads__
The following threads coordinate the functionality of the game.

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

Mutex blocked data structures are used in the control of the game. 



## Gameplay
Joystick is used to aim cue stick and button B2 is used to release the cue stick and strike the cue ball.
The LEDs show the power level when a shot is being taken but otherwise show the remaining balls that need to be potted. 
The LEDs on either side show the solids and stripes remaining and each side blinks when it is that players turn. 
When the cue ball is potted, it can be replaced back on the table using the joystick and B2.

Demo is shown on /PoolDemo.mp4

