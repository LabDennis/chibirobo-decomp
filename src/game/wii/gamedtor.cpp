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
// NONMATCHING: retail destroys 36 members and CBase here, each with a call, and frees through
// __dl__FPv: the function was over the compiler's inline budget. The members typed so far give
// 15 of those calls; the rest are still padding in CGame (see cgame.h), so the offsets and the
// call list fall short. ~CCameraActions (0x8002DD80) and ~CBase (0x80030088) come out of the
// compiler as well but expand their members where retail calls them (CCameraActions) or miss
// the classes of CGs and XSound (CBase).
CGame::~CGame()
{
}
