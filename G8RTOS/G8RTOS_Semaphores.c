/**
 * G8RTOS_Semaphores.c
 * uP2 - Fall 2022
 */

/*********************************************** Dependencies and Externs *************************************************************/

#include <G8RTOS/G8RTOS_CriticalSection.h>
#include <G8RTOS/G8RTOS_Scheduler.h>
#include <G8RTOS/G8RTOS_Semaphores.h>
#include <stdint.h>
#include "inc/hw_types.h"
#include "inc/hw_ints.h"
#include "inc/hw_nvic.h"


/*********************************************** Dependencies and Externs *************************************************************/


/*********************************************** Public Functions *********************************************************************/

/*
 * Initializes a semaphore to a given value
 * Param "s": Pointer to semaphore
 * Param "value": Value to initialize semaphore to
 * THIS IS A CRITICAL SECTION
 */
void G8RTOS_InitSemaphore(semaphore_t *s, int32_t value)
{
    // your code
    IBit_State = StartCriticalSection();
    *s = value;
    EndCriticalSection(IBit_State);
}

/*
 * No longer waits for semaphore
 *  - Decrements semaphore
 *  - Blocks thread if sempahore is unavailable
 * Param "s": Pointer to semaphore to wait on
 * THIS IS A CRITICAL SECTION
 */
void G8RTOS_WaitSemaphore(semaphore_t *s)
{
    // your code
    IBit_State = StartCriticalSection();
    (*s)--;
    if(*s < 0){
        CurrentlyRunningThread->blocked = s;
        HWREG(NVIC_INT_CTRL) |= NVIC_INT_CTRL_PEND_SV; //yield to next
        //check if INT can be trig'd before PRIMASK update
    }
    EndCriticalSection(IBit_State);
}

/*
 * Signals the completion of the usage of a semaphore
 *  - Increments the semaphore value by 1
 *  - Unblocks any threads waiting on that semaphore
 * Param "s": Pointer to semaphore to be signaled
 * THIS IS A CRITICAL SECTION
 */
void G8RTOS_SignalSemaphore(semaphore_t *s)
{
    // your code
    IBit_State = StartCriticalSection();
    (*s)++;
    if((*s)<=0){
        tcb_t* iter = CurrentlyRunningThread->nextTCB;
        while(iter->blocked != s && iter != CurrentlyRunningThread)
            iter = iter->nextTCB;
        if(iter == CurrentlyRunningThread){
            iter = getSleepers();
            while(iter->blocked != s)
                iter = iter->nextTCB;
        }
        iter->blocked = 0;
        //replace unblocked thread at CRT->next
        //HWREG(NVIC_INT_CTRL) |= NVIC_INT_CTRL_PEND_SV;
    }
    EndCriticalSection(IBit_State);
}

/*********************************************** Public Functions *********************************************************************/


