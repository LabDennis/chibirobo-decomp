#ifndef _GAME_SCROLLROOMDATA_H
#define _GAME_SCROLLROOMDATA_H

#include "types.h"
#include "Version.h"

// how far the room camera can pan, at its widest and its narrowest fov
struct Scroll2Limits {
	f32 mWideMinX;   // _00
	f32 mWideMaxX;   // _04
	f32 mWideMinZ;   // _08
	f32 mWideMaxZ;   // _0C
	f32 mNarrowMinX; // _10
	f32 mNarrowMaxX; // _14
	f32 mNarrowMinZ; // _18
	f32 mNarrowMaxZ; // _1C
	f32 mMaxFov;     // _20
	f32 mMinFov;     // _24
	f32 mMidFov;     // _28, z narrows only below it
};

#if defined(VERSION_PAL)
struct ScrollRoomData {
	Scroll2Limits* limits[20];
	f32 captionX[4];
	f32 captionY[4];
	f32 markerX[4];
	f32 markerY[4];
};
STATIC_ASSERT(sizeof(ScrollRoomData) == 0x90);
extern ScrollRoomData gScrollRoomData;
#else
extern Scroll2Limits* gScrollCameraLimitTables[20];
extern f32 gScrollCaptionOutlineX[4];
extern f32 gScrollCaptionOutlineY[4];
#endif

#endif
