/**
 * G8RTOS_Scheduler.c
 * uP2 - Fall 2022
 */
#include <G8RTOS/G8RTOS_CriticalSection.h>
#include <G8RTOS/G8RTOS_Scheduler.h>
#include <G8RTOS/G8RTOS_Structures.h>
#include <stdint.h>
#include "inc/hw_types.h"
#include "inc/hw_ints.h"
#include "inc/hw_nvic.h"
#include "driverlib/systick.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "BoardSupport/inc/RGBLedDriver.h"

/*
 * G8RTOS_Start exists in asm
 */
extern void G8RTOS_Start();

/* System Core Clock From system_msp432p401r.c */
extern uint32_t SystemCoreClock;

/*
 * Pointer to the currently running Thread Control Block
 */
//extern tcb_t * CurrentlyRunningThread; in header

/*********************************************** Dependencies and Externs *************************************************************/


/*********************************************** Defines ******************************************************************************/

/* Status Register with the Thumb-bit Set */
#define THUMBBIT 0x01000000

/*********************************************** Defines ******************************************************************************/


/*********************************************** Data Structures Used *****************************************************************/

static tcb_t *sleepingThreads = 0;
static uint32_t NumberOfSleepers;

/* Thread Control Blocks
 *	- An array of thread control blocks to hold pertinent information for each thread
 */
static tcb_t threadControlBlocks[MAX_THREADS];

/* Thread Stacks
 *	- An array of arrays that will act as individual stacks for each thread
 */
static int32_t threadStacks[MAX_THREADS][STACKSIZE];

/* Periodic Event Threads
 * - An array of periodic events to hold pertinent information for each thread
 */
static ptcb_t Pthread[MAXPTHREADS];

/*********************************************** Data Structures Used *****************************************************************/


/*********************************************** Private Variables ********************************************************************/

/*
 * Current Number of Threads currently in the scheduler
 */
static uint32_t NumberOfThreads;

/*
 * Current Number of Periodic Threads currently in the scheduler
 */
static uint32_t NumberOfPthreads;

//protos
void SysTick_Handler();
void sleepThread(tcb_t* threadToSleep);
void wakeThreads();

/*********************************************** Private Variables ********************************************************************/


/*********************************************** Private Functions ********************************************************************/

/*
 * Initializes the Systick and Systick Interrupt
 * The Systick interrupt will be responsible for starting a context switch between threads
 * Param "numCycles": Number of cycles for each systick interrupt
 */
static void InitSysTick(uint32_t numCycles)
{
    // your code
    SysTickPeriodSet(numCycles);
    SysTickIntRegister(&SysTick_Handler);
    SysTickIntEnable();
    SysTickEnable();
}

/*
 * Chooses the next thread to run.
 */
void G8RTOS_Scheduler()
{
    // your code
    if(CurrentlyRunningThread->asleep)
        sleepThread(CurrentlyRunningThread);
    wakeThreads();

    uint8_t nextThreadPriority = UINT8_MAX;
    tcb_t* iter = CurrentlyRunningThread->nextTCB;
    for(uint8_t i = 1; i<GetNumberOfThreads();i++){
        if(!iter->asleep && !iter->blocked){
            if(iter->priority<=nextThreadPriority){
                CurrentlyRunningThread = iter;
                nextThreadPriority = CurrentlyRunningThread->priority;
            }
        }
        iter = iter->nextTCB;
    }
}


/*
 * SysTick Handler
 * The Systick Handler now will increment the system time,
 * set the PendSV flag to start the scheduler,
 * and be responsible for handling sleeping and periodic threads
 */
void SysTick_Handler()
{
    // your code
    SystemTime++;
    ptcb_t* PTptr = &Pthread[0];

    //periodic thread loop
    while(1){
        if( (SystemTime - PTptr->executeTime) % PTptr->period == 0 )
            (PTptr->handler)();  //run Pthread
        if(PTptr->nextPTCB != 0)
            PTptr = PTptr->nextPTCB;
        else
            break;
    }
    HWREG(NVIC_INT_CTRL) |= NVIC_INT_CTRL_PEND_SV;
}

/*********************************************** Private Functions ********************************************************************/


/*********************************************** Public Variables *********************************************************************/

/* Holds the current time for the whole System */
uint32_t SystemTime;

/*********************************************** Public Variables *********************************************************************/


/*********************************************** Public Functions *********************************************************************/

/*
 * Sets variables to an initial state (system time and number of threads)
 * Enables board for highest speed clock and disables watchdog
 */
void G8RTOS_Init()
{
    // your code
    // Initialize system time to zero
    SystemTime = 0;
    // Set the number of threads to zero
    NumberOfThreads = 0;
    // Initialize all hardware on the board
    InitializeBoard();
}

/*
 * Starts G8RTOS Scheduler
 * 	- Initializes the Systick
 * 	- Sets Context to first thread
 * Returns: Error Code for starting scheduler. This will only return if the scheduler fails
 */
int G8RTOS_Launch()
{
    // your code
    InitSysTick(SysCtlClockGet() / 1000); // 1 ms tick (1Hz / 1000)

    // add your code
    CurrentlyRunningThread = &threadControlBlocks[0];
    currentMaxPriority=255;
    IntPrioritySet(FAULT_SYSTICK, 0xE0);
    IntPrioritySet(FAULT_PENDSV, 0xE0);

    G8RTOS_Start(); // call the assembly function
    return 0;
}


/*
 * Adds threads to G8RTOS Scheduler
 * 	- Checks if there are stil available threads to insert to scheduler
 * 	- Initializes the thread control block for the provided thread
 * 	- Initializes the stack for the provided thread to hold a "fake context"
 * 	- Sets stack tcb stack pointer to top of thread stack
 * 	- Sets up the next and previous tcb pointers in a round robin fashion
 * Param "threadToAdd": Void-Void Function to add as preemptable main thread
 * Returns: Error code for adding threads
 */
threadId_t G8RTOS_AddThread(void (*threadToAdd)(void), uint8_t priority, char *name)
{
    uint8_t openSpot = findFirstDeadTCB();
    IBit_State = StartCriticalSection();
    if (NumberOfThreads >= MAX_THREADS)
    {
        EndCriticalSection(IBit_State);
        return THREAD_LIMIT_REACHED;
    }
    else
    {
        if (NumberOfThreads == 0){
            CurrentlyRunningThread = &threadControlBlocks[0];
            CurrentlyRunningThread->previousTCB = &threadControlBlocks[0];
            CurrentlyRunningThread->nextTCB = &threadControlBlocks[0];
        }
        else{
            threadControlBlocks[openSpot].nextTCB = CurrentlyRunningThread;
            CurrentlyRunningThread->previousTCB->nextTCB = &threadControlBlocks[openSpot];
            tcb_t* prev = CurrentlyRunningThread->previousTCB;
            CurrentlyRunningThread->previousTCB = &threadControlBlocks[openSpot];
            threadControlBlocks[openSpot].previousTCB = prev;

        }

        // Set up the stack pointer
        threadControlBlocks[openSpot].stackPointer = &threadStacks[openSpot][STACKSIZE - 16];
        threadStacks[openSpot][STACKSIZE - 1] = THUMBBIT;
        threadStacks[openSpot][STACKSIZE - 2] = (uint32_t)threadToAdd;
        if(SystemTime == 0)
            CurrentlyRunningThread = &threadControlBlocks[openSpot];

        threadControlBlocks[openSpot].asleep = 0;
        threadControlBlocks[openSpot].blocked =0;
        threadControlBlocks[openSpot].sleepCount =0;
        threadControlBlocks[openSpot].isAlive = 1;
        threadControlBlocks[openSpot].priority = priority;
        threadControlBlocks[openSpot].ThreadID = openSpot;// > SystemTime ? openSpot : SystemTime;
        uint8_t index =0;
        while(*name != 0 && index < 16){
            threadControlBlocks[openSpot].ThreadNameArr[index] = *name;
            name++;
            index++;
        }
        threadControlBlocks[openSpot].ThreadName = &threadControlBlocks[openSpot].ThreadNameArr;
        // Increment number of threads present in the scheduler
        NumberOfThreads++;
    }
    EndCriticalSection(IBit_State);
    return threadControlBlocks[openSpot].ThreadID;
}

uint8_t findFirstDeadTCB(){
    uint8_t index = 0;
    while(threadControlBlocks[index].isAlive)
        index++;
    return index;
}

/*
 * Adds periodic threads to G8RTOS Scheduler
 * Function will initialize a periodic event struct to represent event.
 * The struct will be added to a linked list of periodic events
 * Param Pthread To Add: void-void function for P thread handler
 * Param period: period of P thread to add
 * Returns: Error code for adding threads
 */
int G8RTOS_AddPeriodicEvent(void (*PthreadToAdd)(void), uint32_t period, uint32_t execution)
{
    // your code
    if (NumberOfPthreads > MAXPTHREADS)
    {
        return 0;
    }
    else{
        Pthread[NumberOfPthreads].period = period;
        Pthread[NumberOfPthreads].executeTime = execution;
        Pthread[NumberOfPthreads].handler = PthreadToAdd;
        if(NumberOfPthreads == 0){
            Pthread[NumberOfPthreads].nextPTCB = 0;
            Pthread[NumberOfPthreads].previousPTCB = 0;
        }
        else{
            Pthread[NumberOfPthreads].nextPTCB = 0;
            Pthread[NumberOfPthreads].previousPTCB = &Pthread[NumberOfPthreads-1];
            Pthread[NumberOfPthreads-1].nextPTCB = &Pthread[NumberOfPthreads];
        }
    }
    NumberOfPthreads++;
    return 1;
}

sched_ErrCode_t G8RTOS_AddAPeriodicEvent(void (*AthreadToAdd)(void), uint8_t priority, int32_t IRQn)
{
    // your code
    if(priority >6 || priority<0)
        return HWI_PRIORITY_INVALID;
    if(IRQn>154 || IRQn<0)
        return IRQn_INVALID;
    IntMasterEnable();
    IntRegister(IRQn, AthreadToAdd);
    IntPrioritySet(IRQn, priority);
    IntEnable(IRQn);
}


/*
 * Puts the current thread into a sleep state.
 *  param durationMS: Duration of sleep time in ms
 */
void sleep(uint32_t durationMS)
{
    // your code
    CurrentlyRunningThread->sleepCount = durationMS + SystemTime;   //Sets sleep count
    CurrentlyRunningThread->asleep = 1;                             //Puts the thread to sleep
    HWREG(NVIC_INT_CTRL) |= NVIC_INT_CTRL_PEND_SV;
}

threadId_t G8RTOS_GetThreadId()
{
    return CurrentlyRunningThread->ThreadID;        //Returns the thread ID
}

sched_ErrCode_t G8RTOS_KillThread(threadId_t threadID)
{
    // your code
    IBit_State = StartCriticalSection();
    if(GetNumberOfThreads()<=1)
        return CANNOT_KILL_LAST_THREAD;

    sched_ErrCode_t runningErr = THREAD_DOES_NOT_EXIST;

    for(uint8_t i = 0; i<MAX_THREADS; i++){
        if(threadControlBlocks[i].isAlive == 1 && threadControlBlocks[i].ThreadID == threadID){
            threadControlBlocks[i].isAlive = 0; //kill thread
            NumberOfThreads--;
            //remove from priorityQueue or sleeping
            threadControlBlocks[i].nextTCB->previousTCB = threadControlBlocks[i].previousTCB;
            threadControlBlocks[i].previousTCB->nextTCB = threadControlBlocks[i].nextTCB;
            if(&threadControlBlocks[i] == sleepingThreads){
                sleepingThreads = sleepingThreads->nextTCB;
            }
            //remove from sleeping threads
            if(threadControlBlocks[i].asleep == 1){
                NumberOfSleepers--;
            }

            runningErr = NO_ERROR;
            if(CurrentlyRunningThread == &threadControlBlocks[i])
                HWREG(NVIC_INT_CTRL) |= NVIC_INT_CTRL_PEND_SV;
            if(threadControlBlocks[i].blocked)
                (*threadControlBlocks[i].blocked)++;
            break;
        }
    }
    EndCriticalSection(IBit_State);
    return runningErr;

}

//Thread kills itself
sched_ErrCode_t G8RTOS_KillSelf()
{
    // your code
    threadId_t curThread = CurrentlyRunningThread->ThreadID;
    return G8RTOS_KillThread(curThread);

}

uint32_t GetNumberOfThreads(void)
{
    return NumberOfThreads;         //Returns the number of threads
}



void G8RTOS_KillAllThreads()    //all but Thread[0] "NOP Thread"
{
    // your code
    IBit_State = StartCriticalSection();
    for(uint8_t i = 1; i<GetNumberOfThreads();){
        if(threadControlBlocks[i].isAlive == 1){
            i++;
            threadControlBlocks[i].isAlive = 0; //kill thread
            NumberOfThreads--;
        }
    }
    CurrentlyRunningThread = &threadControlBlocks[0];
    CurrentlyRunningThread->nextTCB = CurrentlyRunningThread;
    CurrentlyRunningThread->previousTCB = CurrentlyRunningThread;
    HWREG(NVIC_INT_CTRL) |= NVIC_INT_CTRL_PEND_SV;
    EndCriticalSection(IBit_State);
}

//move Thread to sleeping LL and return prev CurRunThrd
//sleeping only active thread will destroy OS
void sleepThread(tcb_t* threadToSleep){
    if(NumberOfThreads-NumberOfSleepers <= 1){  //cannot sleep last active thread
        threadToSleep->asleep=0;
        return;
    }
    //remove from CurRunning LL
    CurrentlyRunningThread = threadToSleep->previousTCB;
    threadToSleep->nextTCB->previousTCB = threadToSleep->previousTCB;
    threadToSleep->previousTCB->nextTCB = threadToSleep->nextTCB;


    //add to Sleeping LL in sleepCount increasing order
    if(NumberOfSleepers==0){
        sleepingThreads = threadToSleep;
        threadToSleep->nextTCB = 0;
        threadToSleep->previousTCB = 0;
    }
    else{
        tcb_t* iter = sleepingThreads;
        if(iter->sleepCount > threadToSleep->sleepCount){//check if before first
            threadToSleep->nextTCB = iter;
            iter->previousTCB = threadToSleep;
            threadToSleep->previousTCB = 0;
            sleepingThreads = threadToSleep;
        }
        else{
            while(iter->nextTCB->sleepCount < threadToSleep->sleepCount && iter->nextTCB != 0){
                iter = iter->nextTCB;
            }
            threadToSleep->previousTCB = iter;
            threadToSleep->nextTCB = iter->nextTCB;
            if(iter->nextTCB != 0)
                iter->nextTCB->previousTCB = threadToSleep;
            iter->nextTCB = threadToSleep;
        }
    }
    //inc sleepers count
    NumberOfSleepers++;
}


//wakes all threads that need to wake
void wakeThreads(){
    while(sleepingThreads->sleepCount < SystemTime && sleepingThreads != 0){
        //move thread from sleep LL to CurRunThrd
        tcb_t* moveThrd = sleepingThreads;
        moveThrd->asleep = 0;
        moveThrd->sleepCount = 0;
        sleepingThreads = sleepingThreads->nextTCB;
        sleepingThreads->previousTCB = 0;

        //move to CurRnThrd
        moveThrd->nextTCB = CurrentlyRunningThread;
        moveThrd->previousTCB = CurrentlyRunningThread->previousTCB;
        CurrentlyRunningThread->previousTCB = moveThrd;
        moveThrd->previousTCB->nextTCB = moveThrd;

        CurrentlyRunningThread = moveThrd;

        NumberOfSleepers--;
    }
}

tcb_t * getSleepers(void){
    return sleepingThreads;
}





/*********************************************** Public Functions *********************************************************************/
