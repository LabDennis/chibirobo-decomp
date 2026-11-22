#include "types.h"
#include "revolution/mem.h"
#include "game/wii/cgame.h"

// the expanded heap mm::InitMem1 creates over the MEM1 arena; every operator delete site reads it
extern MEMiHeapHead* gWiiMem1Heap;

// the original file defines operator delete itself (0x80019A68, game/wii/delete.cpp) and the
// compiler expands it in the destructors it emits behind ~CGame; this piece of the file sees it
// as an inline
inline void operator delete(void* ptr) throw()
{
	MEMFreeToExpHeap(gWiiMem1Heap, ptr);
}

/**
 * @note Address: 0x8002CFB8
 * @note Size: 0x264
 */
// retail calls the destructors of all 36 members and CBase and frees through __dl__FPv: the member
// destructors together put the function over the compiler's inline budget
CGame::~CGame()
{
}
