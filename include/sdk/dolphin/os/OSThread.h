#ifndef _DOLPHIN_OSTHREAD_H_
#define _DOLPHIN_OSTHREAD_H_

#include <sdk/dolphin/os/OSContext.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OS_THREAD_SPECIFIC_MAX 2
#define OS_THREAD_STACK_MAGIC 0xDEADBABE

// Forward declarations.
typedef struct OSThread OSThread;
typedef struct OSThreadQueue OSThreadQueue;
typedef struct OSThreadLink OSThreadLink;
typedef struct OSMutex OSMutex;
typedef struct OSMutexQueue OSMutexQueue;
typedef struct OSMutexLink OSMutexLink;

// Useful typedef for priority calls - 0 (high) to 31 (low).
typedef s32 OSPriority;

// Idle function type.
typedef void (*OSIdleFunction)(void* param);

// Start function.
typedef void* (*OSThreadStartFunction)(void*);

// Thread switching function.
typedef void (*OSSwitchThreadCallback)(OSThread* from, OSThread* to);

// Queues and links for threads.
struct OSThreadQueue {
	OSThread* head; // _00
	OSThread* tail; // _04
};

struct OSThreadLink {
	OSThread* next; // _00
	OSThread* prev; // _04
};

// Queues and links for mutexes.
struct OSMutexQueue {
	OSMutex* head; // _00
	OSMutex* tail; // _04
};

struct OSMutexLink {
	OSMutex* next; // _00
	OSMutex* prev; // _04
};

// Struct for managing threads.
struct OSThread {
	OSContext context;       // _000
	u16 state;               // _2C8, see OS_THREAD_STATE enum.
	u16 attr;                // _2CA, 1 = detached.
	s32 suspend;             // _2CC, if > 0, suspend.
	OSPriority priority;     // _2D0, effective scheduling prio.
	OSPriority base;         // _2D4, base scheduling prio.
	void* val;               // _2D8, exit value.
	OSThreadQueue* queue;    // _2DC, owner queue.
	OSThreadLink link;       // _2E0
	OSThreadQueue queueJoin; // _2E8, threads waiting for termination.
	OSMutex* mutex;          // _2F0
	OSMutexQueue queueMutex; // _2F4, owned mutexes.
	OSThreadLink linkActive; // _2FC, list of active threads
	u8* stackBase;           // _304, stack high addr.
	u32* stackEnd;           // _308, stack low addr (last word).
	s32 error;               // _30C
	void* specific[OS_THREAD_SPECIFIC_MAX]; // _310
};

// Thread states.
enum OS_THREAD_STATE {
	OS_THREAD_STATE_NULL     = 0,
	OS_THREAD_STATE_READY    = 1,
	OS_THREAD_STATE_RUNNING  = 2,
	OS_THREAD_STATE_WAITING  = 4,
	OS_THREAD_STATE_MORIBUND = 8, // set for death.
};

// Thread attributes.
#define OS_THREAD_ATTR_DETACH 0x1U

// Thread priority.
#define OS_PRIORITY_MIN  (0)               // highest prio
#define OS_PRIORITY_MAX  (31)              // lowest prio
#define OS_PRIORITY_IDLE (OS_PRIORITY_MAX) // idle = lowest prio

#define IsSuspended(suspend) ((suspend) > 0)

// Basic thread functions.
void OSInitThreadQueue(OSThreadQueue* queue);
OSThread* OSGetCurrentThread(void);
BOOL OSIsThreadTerminated(OSThread* thread);

// Scheduler functions.
s32 OSDisableScheduler(void);
s32 OSEnableScheduler(void);

// Thread manip functions.
void OSYieldThread(void);
BOOL OSCreateThread(OSThread* thread, OSThreadStartFunction func, void* param, void* stack, u32 stackSize, OSPriority priority, u16 attr);
void OSExitThread(void* val);
void OSCancelThread(OSThread* thread);
void OSDetachThread(OSThread* thread);
s32 OSResumeThread(OSThread* thread);
s32 OSSuspendThread(OSThread* thread);
void OSSleepThread(OSThreadQueue* queue);
void OSWakeupThread(OSThreadQueue* queue);

void OSClearStack(u8 val);

// Priority functions.
OSPriority OSGetThreadPriority(OSThread* thread);

BOOL OSIsThreadSuspended(OSThread* thread);
BOOL OSJoinThread(OSThread* thread, void** val);
BOOL OSSetThreadPriority(OSThread* thread, OSPriority prio);
OSThread* OSSetIdleFunction(OSIdleFunction idleFunc, void* param, void* stack, u32 stackSize);
OSThread* OSGetIdleFunction(void);
s32 OSCheckActiveThreads(void);

void OSSetThreadSpecific(s32 index, void* ptr);
void* OSGetThreadSpecific(s32 index);
OSSwitchThreadCallback OSSetSwitchThreadCallback(OSSwitchThreadCallback callback);

#ifdef __cplusplus
}
#endif

#endif
