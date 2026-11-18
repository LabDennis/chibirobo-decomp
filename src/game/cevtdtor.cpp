#include "types.h"

// game/evt.h's inline CEvtData/CStack dtors would come out here as weak copies next to
// these, so the members' inlined dtors are spelled out by hand like memcarddtor.cpp does
class CEvtData;
class CStack;
class CEvt;

void operator delete(void* object);

extern "C" {
extern void* __vt__8CEvtData[];
extern void* __vt__6CStack[];
extern void* __vt__4CEvt[];
}

// todo: weak copy
/**
 * @note Address: 0x80018FB4
 * @note Size: 0x48
 */
extern "C" CEvtData* __dt__8CEvtDataFv(CEvtData* data, s32 flags)
{
	if (data != NULL) {
		*(void***)((u8*)data + 0x8) = __vt__8CEvtData; // _8 vtable
		if ((s16)flags > 0) {
			::operator delete(data);
		}
	}
	return data;
}

// todo: weak copy
/**
 * @note Address: 0x80018FFC
 * @note Size: 0x48
 */
extern "C" CStack* __dt__6CStackFv(CStack* stack, s32 flags)
{
	if (stack != NULL) {
		*(void***)((u8*)stack + 0xC) = __vt__6CStack; // _C vtable
		if ((s16)flags > 0) {
			::operator delete(stack);
		}
	}
	return stack;
}

// todo: weak copy
/**
 * @note Address: 0x80019044
 * @note Size: 0x9C
 */
extern "C" CEvt* __dt__4CEvtFv(CEvt* evt, s32 flags)
{
	if (evt != NULL) {
		*(void***)((u8*)evt + 0x64) = __vt__4CEvt; // _64 vtable
		// the inlined dtors of mValues (_44), mFrames (_34), mCalls (_24) and mData (_04)
		if ((CStack*)((u8*)evt + 0x44) != NULL) {
			*(void***)((u8*)evt + 0x44 + 0xC) = __vt__6CStack;
		}
		if ((CStack*)((u8*)evt + 0x34) != NULL) {
			*(void***)((u8*)evt + 0x34 + 0xC) = __vt__6CStack;
		}
		if ((CStack*)((u8*)evt + 0x24) != NULL) {
			*(void***)((u8*)evt + 0x24 + 0xC) = __vt__6CStack;
		}
		if ((CEvtData*)((u8*)evt + 0x4) != NULL) {
			*(void***)((u8*)evt + 0x4 + 0x8) = __vt__8CEvtData;
		}
		if ((s16)flags > 0) {
			::operator delete(evt);
		}
	}
	return evt;
}
