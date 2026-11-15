#include <stdlib.h>
#include "Version.h"

#if defined(VERSION_R24J01)
#include "revolution/mem.h"
// the expanded heap mm::InitMem1 creates over the MEM1 arena (it stores it to the
// global every operator-delete site reads, retail lbl_805A2858)
extern MEMiHeapHead* gWiiMem1Heap;
#endif

void operator delete(void* ptr) throw()
{
#if defined(VERSION_R24J01)
	MEMFreeToExpHeap(gWiiMem1Heap, ptr);
#else
	if (ptr != 0) {
		free(ptr);
	}
#endif
}
