#ifndef _GAME_PLAYERITEMANIM_H
#define _GAME_PLAYERITEMANIM_H

#include "types.h"

struct XObjChrItemAnim {
	s32 mItem;     // _00, -1 ends the table
	s32 mDepth;    // _04
	s32 mPath[16]; // _08
	f32 mFrame;    // _48
};
STATIC_ASSERT(sizeof(XObjChrItemAnim) == 0x4C);

extern "C" XObjChrItemAnim gPlayerItemAnimationTable[];

#endif
