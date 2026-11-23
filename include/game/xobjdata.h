#ifndef _GAME_XOBJDATA_H
#define _GAME_XOBJDATA_H

#ifdef _MTXTYPES_h_
#include <dolphin/gx/gxtypes.h>
#else
#include "Dolphin/mtx.h"
#endif
#include "game/ut.h"

class XObj;

// a floor's edge, offsets from the object's position
struct XObjEdge {
	Vec mContactOffset; // _00
	Vec mFrom;          // _0C
	Vec mTo;            // _18
};
STATIC_ASSERT(sizeof(XObjEdge) == 0x24);

// a material color stepping towards a target
struct XObjColorStep {
	Vec mValue;  // _00
	Vec mTarget; // _0C
	Vec mSpeed;  // _18
};
STATIC_ASSERT(sizeof(XObjColorStep) == 0x24);

// a carried model and the object whose joint it follows
struct CarryInfo {
	CVec mPos;      // _00
	CVec _10;       // _10
	f32 mAngle;     // _20
	f32 _24;        // _24
	s32 mState;     // _28
	u32 _2C;        // _2C
	CStatus mFlags; // _30, 1 drawn, 2 the robot was hit with it
	XObj* mObj;     // _38
};
STATIC_ASSERT(sizeof(CarryInfo) == 0x3C);

// one read waiting in a reader
struct XObjReadReq {
	void* mObj;  // _00, null when the slot is free
	void* mInfo; // _04
};
STATIC_ASSERT(sizeof(XObjReadReq) == 0x08);

#endif
