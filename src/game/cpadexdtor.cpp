#include "Version.h"
#if defined(VERSION_R24J01)
#include "game/wii/cpad.h"
#include "revolution/mem.h"

// the expanded heap mm::InitMem1 creates over the MEM1 arena; every operator delete site reads it
extern MEMiHeapHead* gWiiMem1Heap;

// retail's game file defines operator delete itself (0x80019A68, wii/delete.cpp here) and expands it
// in its destructors; this part of the file sees it as the same inline body cspritedtor.cpp uses
inline void operator delete(void* ptr) throw()
{
	MEMFreeToExpHeap(gWiiMem1Heap, ptr);
}

CPadEx::~CPadEx()
{
}
#else
extern "C" {
#include <sysdolphin/controller.h>
}
#include "game/cpad.h"
#include "PowerPC_EABI_Support/Runtime/MWCPlusLib.h"

void operator delete(void* object);
extern "C" {
extern void* __vt__6CPadEx[];
void* __dt__7CPadOneFv(void* object, s32 flags);

// todo: weak copy
/**
 * @note Address: 0x80018B18
 * @note Size: 0x70
 */
CPadEx* __dt__6CPadExFv(CPadEx* pad, s32 flags)
{
	if (pad != NULL) {
		*(void***)((u8*)pad + 0x204) = __vt__6CPadEx; // _204 vtable
		__destroy_arr(pad->mPad, (ConstructorDestructor*)__dt__7CPadOneFv, sizeof(CPadOne), 4);
		if ((s16)flags > 0) {
			::operator delete(pad);
		}
	}
	return pad;
}
}
#endif
