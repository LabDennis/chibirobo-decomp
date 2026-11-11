#include "game/evt.h"

void operator delete(void* object);

extern "C" {
extern void* __vt__8CEvtData[];
extern void* __vt__6CStack[];
extern void* __vt__4CEvt[];
}

// todo: weak copy
/**
 * @note Address: 0x80019030
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
 * @note Address: 0x80019078
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
 * @note Address: 0x800190C0
 * @note Size: 0x9C
 */
extern "C" CEvt* __dt__4CEvtFv(CEvt* evt, s32 flags)
{
	if (evt != NULL) {
		*(void***)((u8*)evt + 0x64) = __vt__4CEvt; // _64 vtable
		evt->mValues.CStack::~CStack();
		evt->mFrames.CStack::~CStack();
		evt->mCalls.CStack::~CStack();
		evt->mData.CEvtData::~CEvtData();
		if ((s16)flags > 0) {
			::operator delete(evt);
		}
	}
	return evt;
}
