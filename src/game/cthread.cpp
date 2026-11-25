#include "Dolphin/OS/OSAlloc.h"
#include "game/cthread.h"
#include "game/arcache.h"

/**
 * @note Address: 0x80019480
 * @note Size: 0x8C
 */
BOOL CThread::Create(OSThreadStartFunction func, void* arg, u32 stackSize, OSPriority priority, u16 attr)
{
	mStack     = static_cast<u8*>(OSAllocFromHeap(__OSCurrHeap, stackSize));
	mStackSize = stackSize;
	if (OSCreateThread(&mThread, func, arg, mStack + mStackSize, mStackSize, priority, attr)) {
		mStackSize = stackSize;
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x8001950C
 * @note Size: 0x48
 */
CThread::~CThread()
{
}

/**
 * @note Address: 0x80019554
 * @note Size: 0x74
 */
ARPreCache::~ARPreCache()
{
}
