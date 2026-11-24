#ifndef _GAME_XACTDATA_H
#define _GAME_XACTDATA_H

#include "game/cscrn.h"

// the ring on the floor around him while he tumbles
struct XActTumbleFade {
	BOOL mActive; // _00
	Vec mOffset;  // _04, from his feet, turned with him
	f32 mAmount;  // _10
};
STATIC_ASSERT(sizeof(XActTumbleFade) == 0x14);

// one of the popper's rings, drawn as two glow sprites and a soft disc
struct XActPopperRing {
	Vec mPos;    // _00
	f32 mAmount; // _0C, 0 when it's gone
	f32 mRadius; // _10
	u32 mColor;  // _14
};
STATIC_ASSERT(sizeof(XActPopperRing) == 0x18);

struct RadarAimAxis {
	f32 angle;       // _00
	f32 targetAngle; // _04
	f32 reached;     // _08
};
STATIC_ASSERT(sizeof(RadarAimAxis) == 0x0C);

// the beam's two colours. retail's destructor tests this object's address before it destroys them, so they sit
// in an object of their own. whether it holds more than the colours is not known
struct RadarBeamColors {
	CScrnColor mColor[2]; // _00
};
STATIC_ASSERT(sizeof(RadarBeamColors) == 0x68);

#endif
