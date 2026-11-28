#ifndef _GAME_XPAD_H
#define _GAME_XPAD_H

#include "Version.h"
#if defined(VERSION_R24J01)
#include "game/wii/cpad.h"
#else
#include "game/cpad.h"
#endif

// chibi robo's pad, a CPadOne with a little more. XObjChr has one at _12DA0,
// at _18BE8 on the Wii (its vptr stays at the end of CPadOne, 0x12A4)
class XPad : public CPadOne {
public:
	// _00-_54 = CPadOne, _00-_12A8 on the Wii
	BOOL mReset; // _54, set when chibi robo is reset; _12A8 on the Wii

#if defined(VERSION_R24J01)
	XPad();          // 0x800C0E98, defined in the CPtcl unit's file
	virtual ~XPad(); // _08, defined in xpaddtor.cpp
#else
	virtual ~XPad(); // _08 (weak)
#endif
};
#if defined(VERSION_R24J01)
STATIC_ASSERT(sizeof(XPad) == 0x12B0);
#else
STATIC_ASSERT(sizeof(XPad) == 0x58);
#endif

#endif
