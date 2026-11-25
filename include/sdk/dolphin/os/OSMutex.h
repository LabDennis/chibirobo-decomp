#ifndef _DOLPHIN_OSMUTEX_H_
#define _DOLPHIN_OSMUTEX_H_

#include <sdk/dolphin/os/OSThread.h>

#ifdef __cplusplus
extern "C" {
#endif

struct OSMutex {
	OSThreadQueue queue; // _00
	OSThread* thread;    // _08, current owner.
	s32 count;           // _0C, lock count.
	OSMutexLink link;    // _10
};

// Condition variable.
typedef struct OSCond {
	OSThreadQueue queue; // _00
} OSCond;

// Mutex functions.
void OSInitMutex(OSMutex* mutex);
void OSLockMutex(OSMutex* mutex);
void OSUnlockMutex(OSMutex* mutex);
BOOL OSTryLockMutex(OSMutex* mutex);

// Condition variable functions.
void OSInitCond(OSCond* cond);
void OSWaitCond(OSCond* cond, OSMutex* mutex);
void OSSignalCond(OSCond* cond);


#ifdef __cplusplus
}
#endif

#endif
