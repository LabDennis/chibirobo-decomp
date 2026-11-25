#ifndef _GAME_CPADEX_H
#define _GAME_CPADEX_H

extern "C" {
#include <sysdolphin/controller.h>
}
#include "game/cpad.h"

// owns the four controller ports and their input and rumble queues
class CPadEx {
public:
	HSD_PadData mQueue;               // _00
	HSD_PadRumbleListData mRumble[4]; // _34
	CPadOne mPad[4];                  // _B4
	// _204 vtable

	CPadEx();
	virtual ~CPadEx() { } // _08

	void Init();
	void Update(s32 frames);
};
STATIC_ASSERT(sizeof(CPadEx) == 0x208);

#endif
