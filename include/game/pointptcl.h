#ifndef _GAME_POINTPTCL_H
#define _GAME_POINTPTCL_H

#include "types.h"

struct PointPtclOne {
	s32 mState; // _00
	u32 mColor;
	f32 mFade, mAngle, mAngularSpeed, mSpeed;
	f32 mLastScreenX, mLastScreenY;
	f32 mLastAngle;
};
STATIC_ASSERT(sizeof(PointPtclOne) == 0x24);

struct PointDigit {
	s32 glyph;
	f32 fade;
};

struct PointDisplay {
	s32 mState, mAmount, mCount;
	union {
		PointDigit digits[11];
		struct {
			PointDigit firstTen[10];
			u32 mColor, mColor2;
		} colors;
	};
	f32 holdFrames;
};

struct PointBurst {
	s32 mState, mNum;
	PointPtclOne mPtcls[40];
};

// the sparks that fly in when points change, a pool of 10 at gPointParticlePool. type 0, 2 and 3
// have their own colors, a negative amount draws them black
struct PointPtcl {
	f32 mStartY;
	f32 mStartX;          // _00
	s32 mType;            // _08
	PointDisplay display; // _0C
	PointBurst burst;     // _74
	u8 IsFree() const { return display.mState == 0 && burst.mState == 0; }
};
STATIC_ASSERT(sizeof(PointPtcl) == 0x61C);

#endif
