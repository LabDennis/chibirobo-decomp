#include "revolution/mem.h"

// the expanded heap mm::InitMem1 creates over the MEM1 arena; every operator new[] site reads it
extern MEMiHeapHead* gWiiMem1Heap;
// the expanded heap over the game arena; only the aligned operator new[] reads it
extern MEMiHeapHead* gWiiGameHeap;

/**
 * @note Address: 0x80019AB4
 * @note Size: 0x50
 */
// the game's own operator new[]: array news (new T[n], cookie and all) come out of the MEM1
// heap four-aligned; unlike the runtime's version there is no null check before the alloc
void* operator new[](u32 size) throw()
{
	return MEMAllocFromExpHeapEx(gWiiMem1Heap, size, 4);
}

/**
 * @note Address: 0x80019B04
 * @note Size: 0x4C
 */
// the game's own operator delete[]: retail uses it for array members and as the unwind
// cleanup of new T[n] expressions; the block goes back to the MEM1 heap
void operator delete[](void* ptr) throw()
{
	MEMFreeToExpHeap(gWiiMem1Heap, ptr);
}

/**
 * @note Address: 0x80019B50
 * @note Size: 0x54
 */
// the game's aligned operator new[] (new (align) T[n]): it is the only one of the
// allocation operators that takes memory from the game heap instead of MEM1
void* operator new[](u32 size, int align) throw()
{
	return MEMAllocFromExpHeapEx(gWiiGameHeap, size, align);
}
