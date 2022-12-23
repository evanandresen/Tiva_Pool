/**
 * G8RTOS IPC header
 * @author:
 * uP2 - Fall 2022
 */

#ifndef G8RTOS_G8RTOS_IPC_H_
#define G8RTOS_G8RTOS_IPC_H_

#include <G8RTOS/G8RTOS_Structures.h>

#define coordScale 32

typedef struct poolBall_t {
    uint8_t isActive;
    uint16_t xPos;
    uint16_t yPos;
    float xVelo;
    float yVelo;
    uint16_t color;
    uint8_t isStriped;
    uint8_t isOnTable;
    semaphore_t mutex;

} poolBall_t;

poolBall_t poolBalls[16];

//Ball FIFO
void initBallFIFO();
uint8_t popBallFIFO();
void pushBallFIFO(uint8_t queueToBall);

//Ball structs
void initBallData();
void moveBallTick(uint8_t objectBall);
void checkDoBallCollisions(uint8_t objectBall);
void ballCollision(uint8_t objectBall, uint8_t impactedBall);
void checkDoCollideWall(uint8_t objectBall);
uint8_t checkBallPocketed(uint8_t objectBall);
int ballsStopped(void);


#endif /* G8RTOS_G8RTOS_IPC_H_ */
