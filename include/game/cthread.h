#ifndef _GAME_CTHREAD_H
#define _GAME_CTHREAD_H

#include "Dolphin/OS/OSThread.h"

class CThread {
public:
	CThread()
	    : mStack(NULL)
	{
	}

	OSThread mThread; // _00
	u8* mStack;       // _318
	u32 mStackSize;   // _31C
	// _320 vtable

	virtual ~CThread();                                                                            // _08
	virtual BOOL Create(void* (*func)(void*), void* param, u32 stackSize, s32 priority, u16 attr); // _0C

	// todo: cleanup, card jobs and the movie all start threads like this
	inline void Start(OSThreadStartFunction entry, void* arg, u8* stack, u32 size)
	{
		mStack     = stack;
		mStackSize = size;
		OSCreateThread(&mThread, entry, arg, stack + size, size, 16, OS_THREAD_ATTR_DETACH);
		OSResumeThread(&mThread);
	}
};
#ifndef DEFER_THREAD_DESTRUCTOR
inline CThread::~CThread()
{
}
#endif

typedef char CThread_size_check[sizeof(CThread) == 0x328 ? 1 : -1];

extern "C" {
// the memory card jobs run on this one
extern CThread gMemoryCardThread;
extern u8 gMemoryCardThreadStack[0x1000];
}

#endif
