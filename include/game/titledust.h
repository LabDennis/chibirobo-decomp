#ifndef _GAME_TITLEDUST_H
#define _GAME_TITLEDUST_H

#include "types.h"
#include "Version.h"

// one mote of dust drifting over the menu
struct TitleDust {
	u32 mState; // _00
#if defined(VERSION_R24J01)
	s32 mTimer; // _04
#else
	u32 mTimer; // _04
#endif
	f32 mX;      // _08
	f32 mY;      // _0C
	f32 mSpeedX; // _10
	f32 mSpeedY; // _14
};
STATIC_ASSERT(sizeof(TitleDust) == 0x18);

#endif
