#include "game/evt.h"

void operator delete(void* object);

extern "C" {
extern void* __vt__4CEvt[];
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
