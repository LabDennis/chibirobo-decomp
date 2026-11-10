#ifndef _GAME_XPAD_H
#define _GAME_XPAD_H

#include "game/cpad.h"

// chibi robo's pad, a CPadOne with a little more. XObjChr has one at _12DA0
class XPad : public CPadOne {
public:
	// _00-_54 = CPadOne
	BOOL mReset; // _54, set when chibi robo is reset

	virtual ~XPad() { } // _08 (weak)
};
STATIC_ASSERT(sizeof(XPad) == 0x58);

#endif
