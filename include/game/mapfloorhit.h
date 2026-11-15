#ifndef _GAME_MAPFLOORHIT_H
#define _GAME_MAPFLOORHIT_H

#include "types.h"

struct XObjMapHit {
	s32 mTriangle;
	s32 mTrigon;
	f32 mHeight;
};

// one of an object's floors, a list of them ends at an id of -1
struct XObjFloor {
	s16 mId; // _00
#if defined(VERSION_R24J01)
	u8 mMaterial; // _02, ground types 13 and up count as none
	u8 mFlags;    // _03, bit 0 includes this floor in contact tests, bit 1 in screen bounds
#else
	s16 mType; // _02, ground type, 13 and up count as none
#endif
	s32 mCenterOffset[3]; // _04, in hundredths
	s32 mYawDegrees;      // _10
	s32 mExtent[3];       // _14, in hundredths
};
typedef char XObjFloor_size_check[sizeof(XObjFloor) == 0x20 ? 1 : -1];

#endif
