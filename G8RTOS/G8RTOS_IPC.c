/**
 * G8RTOS IPC - Inter-Process Communication
 * @author:
 * uP2 - Fall 2022
 */
#include <G8RTOS/G8RTOS_IPC.h>
#include <G8RTOS/G8RTOS_Semaphores.h>
#include <G8RTOS/G8RTOS_Structures.h>
#include <stdint.h>
#include "BoardSupport/inc/ILI9341_lib.h"
#include "threads.h"
#include <math.h>

#define FIFOSIZE 16
/*
 *
 * Pos and Velo values are saved 8 times larger for match precision
 *
 */
/*
 * 0-Cue
 * 1-7-Solids
 * 8-Eight ball
 * 9-15-Stripes
 */
void initBallData(){
    for(int i = 0; i < NUM_BALLS;i++){
        poolBalls[i].xPos = 400*coordScale;
        poolBalls[i].yPos = 400*coordScale;
        poolBalls[i].xVelo = 0;
        poolBalls[i].yVelo = 0;
        poolBalls[i].isActive = 0;
        poolBalls[i].isStriped = i > 8;
        poolBalls[i].isOnTable = 1;
        G8RTOS_InitSemaphore(&poolBalls[i].mutex, 1);
    }
    uint8_t xSpacing = 10;
    uint8_t ySpacing = 14;
    uint8_t xRack = 110;
    uint8_t yRack = 120;
    //Cue ball
    poolBalls[0].xPos = 240*coordScale;
    poolBalls[0].yPos = yRack*coordScale;
    poolBalls[0].color = WHITE;

    //colors
    poolBalls[1].color = YELLOW;
    poolBalls[2].color = BLUE;
    poolBalls[3].color = RED;
    poolBalls[4].color = PURPLE;
    poolBalls[5].color = ORANGE;
    poolBalls[6].color = GREEN;
    poolBalls[7].color = CHERRY;
    poolBalls[8].color = BLACK;
    poolBalls[9].color = YELLOW;
    poolBalls[10].color = BLUE;
    poolBalls[11].color = RED;
    poolBalls[12].color = PURPLE;
    poolBalls[13].color = ORANGE;
    poolBalls[14].color = GREEN;
    poolBalls[15].color = CHERRY;


    //position balls in order they are racked

    //first row
    poolBalls[1].xPos = xRack*coordScale;
    poolBalls[1].yPos = yRack*coordScale;

    //second row
    poolBalls[9].xPos = (xRack-xSpacing)*coordScale;
    poolBalls[9].yPos = (yRack+(ySpacing/2))*coordScale;

    poolBalls[2].xPos = (xRack-xSpacing)*coordScale;
    poolBalls[2].yPos = (yRack-(ySpacing/2))*coordScale;

    //third row
    poolBalls[10].xPos = (xRack-2*xSpacing)*coordScale;
    poolBalls[10].yPos = (yRack+ySpacing)*coordScale;

    poolBalls[8].xPos = (xRack-2*xSpacing)*coordScale;
    poolBalls[8].yPos = yRack*coordScale;

    poolBalls[11].xPos = (xRack-2*xSpacing)*coordScale;
    poolBalls[11].yPos = (yRack-ySpacing)*coordScale;

    //fourth row
    poolBalls[3].xPos = (xRack-3*xSpacing)*coordScale;
    poolBalls[3].yPos = (yRack+(3*ySpacing/2))*coordScale;

    poolBalls[12].xPos = (xRack-3*xSpacing)*coordScale;
    poolBalls[12].yPos = (yRack+(ySpacing/2))*coordScale;

    poolBalls[4].xPos = (xRack-3*xSpacing)*coordScale;
    poolBalls[4].yPos = (yRack-(ySpacing/2))*coordScale;

    poolBalls[13].xPos = (xRack-3*xSpacing)*coordScale;
    poolBalls[13].yPos = (yRack-(3*ySpacing/2))*coordScale;

    //fifth row
    poolBalls[7].xPos = (xRack-4*xSpacing)*coordScale;
    poolBalls[7].yPos = (yRack+2*ySpacing)*coordScale;

    poolBalls[5].xPos = (xRack-4*xSpacing)*coordScale;
    poolBalls[5].yPos = (yRack+ySpacing)*coordScale;

    poolBalls[14].xPos = (xRack-4*xSpacing)*coordScale;
    poolBalls[14].yPos = yRack*coordScale;

    poolBalls[6].xPos = (xRack-4*xSpacing)*coordScale;
    poolBalls[6].yPos = (yRack-ySpacing)*coordScale;

    poolBalls[15].xPos = (xRack-4*xSpacing)*coordScale;
    poolBalls[15].yPos = (yRack-2*ySpacing)*coordScale;

}

void checkDoBallCollisions(uint8_t objectBall){
    for(int i = 0; i<NUM_BALLS; i++){
        if(i==objectBall || !poolBalls[i].isOnTable)
            continue;
        if(abs(poolBalls[objectBall].xPos-poolBalls[i].xPos) <= 13*coordScale &&
           abs(poolBalls[objectBall].yPos-poolBalls[i].yPos) <= 13*coordScale){
            if(((poolBalls[objectBall].xPos-poolBalls[i].xPos)*poolBalls[objectBall].xVelo +
              (poolBalls[objectBall].yPos-poolBalls[i].yPos)*poolBalls[objectBall].yVelo) <0.0) {
            ballCollision(objectBall, i);
            }
        }
    }
}

void ballCollision(uint8_t objectBall, uint8_t impactedBall){
    float Obj_min_Imp_DOT = (poolBalls[objectBall].xVelo - poolBalls[impactedBall].xVelo)*(poolBalls[objectBall].xPos - poolBalls[impactedBall].xPos) +
                               (poolBalls[objectBall].yVelo - poolBalls[impactedBall].yVelo)*(poolBalls[objectBall].yPos - poolBalls[impactedBall].yPos);
    float posDist = (poolBalls[objectBall].xPos - poolBalls[impactedBall].xPos)*(poolBalls[objectBall].xPos - poolBalls[impactedBall].xPos) +
                       (poolBalls[objectBall].yPos - poolBalls[impactedBall].yPos)*(poolBalls[objectBall].yPos - poolBalls[impactedBall].yPos);
    float xPos_Obj_min_Imp = (poolBalls[objectBall].xPos - poolBalls[impactedBall].xPos);
    float yPos_Obj_min_Imp = (poolBalls[objectBall].yPos - poolBalls[impactedBall].yPos);
    float deltX = (Obj_min_Imp_DOT*xPos_Obj_min_Imp)/posDist;
    float deltY = (Obj_min_Imp_DOT*yPos_Obj_min_Imp)/posDist;

    poolBalls[objectBall].xVelo -= (0.98*deltX);
    poolBalls[objectBall].yVelo -= (0.98*deltY);
    poolBalls[impactedBall].xVelo += (0.98*deltX);
    poolBalls[impactedBall].yVelo += (0.98*deltY);
    if(!poolBalls[impactedBall].isActive){
        poolBalls[impactedBall].isActive = 1;
        pushBallFIFO(impactedBall);
        G8RTOS_AddThread(activeBallT, 20, impactedBall+65);
    }
}

void moveBallTick(uint8_t objectBall){
    //adjust pos
    poolBalls[objectBall].xPos += ((int16_t)poolBalls[objectBall].xVelo)/2;
    poolBalls[objectBall].yPos += ((int16_t)poolBalls[objectBall].yVelo)/2;

    //adjust velo with friction
    float dotVelo = sqrtf(powf(poolBalls[objectBall].xVelo,2) + powf(poolBalls[objectBall].yVelo,2))*8.0/FRICTION;
    if(poolBalls[objectBall].xVelo){
        float xFriction = poolBalls[objectBall].xVelo/dotVelo;
        if(fabsf(xFriction) >= fabsf(poolBalls[objectBall].xVelo))
            poolBalls[objectBall].xVelo = 0;
        else
            poolBalls[objectBall].xVelo -= xFriction;
    }
    if(poolBalls[objectBall].yVelo){
        float yFriction = poolBalls[objectBall].yVelo/dotVelo;
        if(fabsf(yFriction) >= fabsf(poolBalls[objectBall].yVelo))
            poolBalls[objectBall].yVelo = 0;
        else
            poolBalls[objectBall].yVelo -= yFriction;
    }
}

void checkDoCollideWall(uint8_t objectBall){
    //vertical wall collision
    if(poolBalls[objectBall].xPos <= (18+6)*coordScale){
        poolBalls[objectBall].xVelo = fabsf(poolBalls[objectBall].xVelo);
        poolBalls[objectBall].xPos = (18+6)*coordScale;
    }
    else if(poolBalls[objectBall].xPos >= (320-18-6)*coordScale){
        poolBalls[objectBall].xVelo = -1*fabsf(poolBalls[objectBall].xVelo);
        poolBalls[objectBall].xPos = (320-18-6)*coordScale;
    }
    //horizontal wall collision
    if(poolBalls[objectBall].yPos <= (20+6)*coordScale){
        poolBalls[objectBall].yVelo = fabsf(poolBalls[objectBall].yVelo);
        poolBalls[objectBall].yPos = (20+6)*coordScale;
    }
    else if(poolBalls[objectBall].yPos >= (240-20-6)*coordScale){
        poolBalls[objectBall].yVelo = -1*fabsf(poolBalls[objectBall].yVelo);
        poolBalls[objectBall].yPos = (240-20-6)*coordScale;
    }
}

uint8_t checkBallPocketed(uint8_t objectBall){
    if(poolBalls[objectBall].xPos >= (320-25-6)*coordScale && poolBalls[objectBall].yPos <= (27+6)*coordScale){
        poolBalls[objectBall].xPos = (320-25)*coordScale;
        poolBalls[objectBall].yPos = (27)*coordScale;
        poolBalls[objectBall].xVelo = 0;
        poolBalls[objectBall].yVelo = 0;
        poolBalls[objectBall].isOnTable = 0;
        return 1;
    }
    if(poolBalls[objectBall].xPos >= (160-7)*coordScale && poolBalls[objectBall].xPos <= (160+7)*coordScale &&
       poolBalls[objectBall].yPos <= (27+6)*coordScale){

        poolBalls[objectBall].xPos = (160)*coordScale;
        poolBalls[objectBall].yPos = (27)*coordScale;
        poolBalls[objectBall].xVelo = 0;
        poolBalls[objectBall].yVelo = 0;
        poolBalls[objectBall].isOnTable = 0;
        return 2;
    }

    if(poolBalls[objectBall].xPos <= (25+6)*coordScale && poolBalls[objectBall].yPos <= (27+6)*coordScale){
        poolBalls[objectBall].xPos = (25)*coordScale;
        poolBalls[objectBall].yPos = (27)*coordScale;
        poolBalls[objectBall].xVelo = 0;
        poolBalls[objectBall].yVelo = 0;
        poolBalls[objectBall].isOnTable = 0;
        return 3;
    }


    if(poolBalls[objectBall].xPos <= (25+6)*coordScale && poolBalls[objectBall].yPos >= (240-27-6)*coordScale){
        poolBalls[objectBall].xPos = (25)*coordScale;
        poolBalls[objectBall].yPos = (240-27)*coordScale;
        poolBalls[objectBall].xVelo = 0;
        poolBalls[objectBall].yVelo = 0;
        poolBalls[objectBall].isOnTable = 0;
        return 4;
    }
    if(poolBalls[objectBall].xPos >= (160-7)*coordScale && poolBalls[objectBall].xPos <= (160+7)*coordScale &&
       poolBalls[objectBall].yPos >= (240-27-6)*coordScale){

        poolBalls[objectBall].xPos = (160)*coordScale;
        poolBalls[objectBall].yPos = (240-27)*coordScale;
        poolBalls[objectBall].xVelo = 0;
        poolBalls[objectBall].yVelo = 0;
        poolBalls[objectBall].isOnTable = 0;
        return 5;
    }
    if(poolBalls[objectBall].xPos >= (320-25-6)*coordScale && poolBalls[objectBall].yPos >= (240-27-6)*coordScale){
        poolBalls[objectBall].xPos = (320-25)*coordScale;
        poolBalls[objectBall].yPos = (240-27)*coordScale;
        poolBalls[objectBall].xVelo = 0;
        poolBalls[objectBall].yVelo = 0;
        poolBalls[objectBall].isOnTable = 0;
        return 6;
    }
    return 0;
}

int ballsStopped(void){
    for(int i = 0; i<NUM_BALLS; i++){
        if(poolBalls[i].isActive)
            return 0;
    }
    return 1;
}

typedef struct FIFO_t {
    uint8_t buffer[16];
    uint8_t *head;
    uint8_t *tail;
    uint8_t lostData;
    semaphore_t currentSize;
    semaphore_t mutex;
} FIFO_t;


static FIFO_t ballFIFO;

void initBallFIFO()
{
    ballFIFO.lostData = 0;
    G8RTOS_InitSemaphore(&ballFIFO.mutex, 1);
    G8RTOS_InitSemaphore(&ballFIFO.currentSize, 0);
    ballFIFO.head = &ballFIFO.buffer[0];
    ballFIFO.tail = &ballFIFO.buffer[0];
}
uint8_t popBallFIFO()
{
    // your code
    uint8_t returner;
    // your code
    G8RTOS_WaitSemaphore(&ballFIFO.mutex);
    G8RTOS_WaitSemaphore(&ballFIFO.currentSize);
    returner = *(ballFIFO.head);

    if(ballFIFO.head == &ballFIFO.buffer[15])
        ballFIFO.head = &ballFIFO.buffer[0];
    else
        ballFIFO.head++;
    G8RTOS_SignalSemaphore(&ballFIFO.mutex);
    return returner;

}
void pushBallFIFO(uint8_t Data)
{
    *ballFIFO.tail = Data;
    if(ballFIFO.tail == &ballFIFO.buffer[15])
            ballFIFO.tail = &ballFIFO.buffer[0];
    else
        ballFIFO.tail++;
    if(ballFIFO.currentSize > FIFOSIZE-1){
        ballFIFO.lostData++;
        if(ballFIFO.head == &ballFIFO.buffer[15])
                ballFIFO.head = &ballFIFO.buffer[0];
        else
            ballFIFO.head++;
    }
    else
        G8RTOS_SignalSemaphore(&ballFIFO.currentSize);
}

