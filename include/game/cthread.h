#ifndef _GAME_CTHREAD_H
#define _GAME_CTHREAD_H

#include "Dolphin/OS/OSThread.h"
#if defined(VERSION_R24J01)
#include "revolution/mem.h"
extern MEMiHeapHead* gWiiGameHeap;
#endif

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

	virtual ~CThread() { } // _08
#if defined(VERSION_R24J01)
	// inline on the Wii: no caller in retail calls it, they all have the body, and its out-of-line
	// copy (0x800331D4) sits among the weak virtuals of the file that holds gGame
	virtual BOOL Create(void* (*func)(void*), void* param, u32 stackSize, s32 priority, u16 attr) // _0C
	{
		mStack     = (u8*)MEMAllocFromExpHeapEx(gWiiGameHeap, stackSize, 4);
		mStackSize = stackSize;
		if (OSCreateThread(&mThread, func, param, mStack + stackSize, stackSize, priority, attr)) {
			mStackSize = stackSize;
			return TRUE;
		}
		return FALSE;
	}
#else
	virtual BOOL Create(void* (*func)(void*), void* param, u32 stackSize, s32 priority, u16 attr); // _0C
#endif

	// todo: cleanup, card jobs and the movie all start threads like this
	inline void Start(OSThreadStartFunction entry, void* arg, u8* stack, u32 size)
	{
		mStack     = stack;
		mStackSize = size;
		OSCreateThread(&mThread, entry, arg, stack + size, size, 16, OS_THREAD_ATTR_DETACH);
		OSResumeThread(&mThread);
	}
};

typedef char CThread_size_check[sizeof(CThread) == 0x328 ? 1 : -1];

extern "C" {
// the memory card jobs run on this one
extern CThread gMemoryCardThread;
extern u8 gMemoryCardThreadStack[0x1000];
}

#endif
