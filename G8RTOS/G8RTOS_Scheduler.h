/*
 * G8RTOS_Scheduler.h
 */

#ifndef G8RTOS_SCHEDULER_H_
#define G8RTOS_SCHEDULER_H_

#include <G8RTOS/G8RTOS_Structures.h>

/*********************************************** Sizes and Limits *********************************************************************/
#define MAX_THREADS 34
#define MAXPTHREADS 1
#define STACKSIZE 128
#define OSINT_PRIORITY 7
/*********************************************** Sizes and Limits *********************************************************************/

//typedef int32_t threadId_t;
/*********************************************** Public Variables *********************************************************************/

/* Holds the current time for the whole System */
extern uint32_t SystemTime;

extern tcb_t * CurrentlyRunningThread;

tcb_t *tempNextThread;

uint16_t currentMaxPriority;

static uint16_t IDCounter;

typedef enum {
    NO_ERROR                    = 0,
    THREAD_LIMIT_REACHED        = -1,
    NO_THREADS_SCHEDULED        = -2,
    THREADS_INCORRECTLY_ALIVE   = -3,
    THREAD_DOES_NOT_EXIST       = -4,
    CANNOT_KILL_LAST_THREAD     = -5,
    IRQn_INVALID                = -6,
    HWI_PRIORITY_INVALID        = -7
} sched_ErrCode_t;

/*********************************************** Public Variables *********************************************************************/


/*********************************************** Public Functions *********************************************************************/
tcb_t * getSleepers(void);
/*
 * Initializes variables and hardware for G8RTOS usage
 */
void G8RTOS_Init();

/*
 * Starts G8RTOS Scheduler
 * 	- Initializes Systick Timer
 * 	- Sets Context to first thread
 * Returns: Error Code for starting scheduler. This will only return if the scheduler fails
 */
int32_t G8RTOS_Launch();

/*
 * Adds threads to G8RTOS Scheduler
 * 	- Checks if there are stil available threads to insert to scheduler
 * 	- Initializes the thread control block for the provided thread
 * 	- Initializes the stack for the provided thread
 * 	- Sets up the next and previous tcb pointers in a round robin fashion
 * Param "threadToAdd": Void-Void Function to add as preemptable main thread
 * Returns: Error code for adding threads
 */
threadId_t G8RTOS_AddThread(void (*threadToAdd)(void), uint8_t priority, char *name);


/*
 * Adds periodic threads to G8RTOS Scheduler
 * Function will initialize a periodic event struct to represent event.
 * The struct will be added to a linked list of periodic events
 * Param Pthread To Add: void-void function for P thread handler
 * Param period: period of P thread to add
 * Returns: Error code for adding threads
 */
int G8RTOS_AddPeriodicEvent(void (*PthreadToAdd)(void), uint32_t period, uint32_t execution);

void G8RTOS_KillAllThreads();

sched_ErrCode_t G8RTOS_AddAPeriodicEvent(void (*AthreadToAdd)(void), uint8_t priority, int32_t IRQn);


/*
 * Puts the current thread into a sleep state.
 *  param durationMS: Duration of sleep time in ms
 */
void sleep(uint32_t durationMS);

uint8_t findFirstDeadTCB();

threadId_t G8RTOS_GetThreadId();

sched_ErrCode_t G8RTOS_KillThread(threadId_t threadID);

sched_ErrCode_t G8RTOS_KillSelf();

uint32_t GetNumberOfThreads(void);

/*********************************************** Public Functions *********************************************************************/

#endif /* G8RTOS_SCHEDULER_H_ */
