#ifndef _GAME_CPAD_H
#define _GAME_CPAD_H

#include "types.h"

// one controller port, class name from RTTI. the dir fields fold both sticks into the
// dpad bits so menus can read either
class CPadOne {
public:
	u8 mPort;            // _00
	u32 mButton;         // _04
	u32 mDirButton;      // _08
	u32 mLastButton;     // _0C
	u32 mLastDirButton;  // _10
	u32 mTrigger;        // _14
	u32 mDirTrigger;     // _18
	u32 mRepeat;         // _1C
	u32 mDirRepeat;      // _20
	s32 mRepeatTimer[4]; // _24, one per direction
	f32 mStickX;         // _34
	f32 mStickY;         // _38
	f32 mSubStickX;      // _3C
	f32 mSubStickY;      // _40
	f32 mAnalogL;        // _44
	f32 mAnalogR;        // _48
	BOOL mConnected;     // _4C
	// _50 vtable

	virtual ~CPadOne() { }           // _08, its one copy is in game part 1
	virtual void Init(u8 port);      // _0C
	virtual void Clear();            // _10
	virtual void Update(s32 frames); // _14
};
STATIC_ASSERT(sizeof(CPadOne) == 0x54);

// a handle on one port of the CPadEx, what the rest of the game keeps
class CPad {
public:
	CPadOne* mPad; // _00
	// _04 vtable

	CPad(u8 port);
#if defined(VERSION_R24J01)
	virtual ~CPad(); // _08, defined in cpaddtor.cpp
#else
	virtual ~CPad() { } // _08
#endif
};
STATIC_ASSERT(sizeof(CPad) == 0x8);

#endif
