#include "revolution/mem.h"

// the expanded heap mm::InitMem1 creates over the MEM1 arena; every operator delete site reads it
extern MEMiHeapHead* gWiiMem1Heap;

/**
 * @note Address: 0x80019A68
 * @note Size: 0x4C
 */
// The game's own operator delete: the block goes back to the MEM1 heap, without the null check the
// runtime's version has.
void operator delete(void* ptr) throw()
{
	MEMFreeToExpHeap(gWiiMem1Heap, ptr);
}
