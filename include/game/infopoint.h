#ifndef _GAME_INFOPOINT_H
#define _GAME_INFOPOINT_H

#include "types.h"
#include "Version.h"
#include <dolphin/mtx/mtxtypes.h>

// one of the three counters InfoScrn slides in from the left, class name from RTTI
class InfoPoint {
public:
	s32 mIndex;              // _00
	s32 mResource;           // _04
	s32 mSlideState;         // _08
	s32 mFadeState;          // _0C
	s32 mSlideDelay;         // _10
	f32 mOpacity;            // _14
	s32 mPulseFrames;        // _18
	f32 mValue[4];           // _1C, the count shown and the real one, then the box's width shown and wanted
	Vec mPos[6];             // _2C
	u8 mVisible;             // _74
	u8 mSuppressIdleDisplay; // _75
	s32 mState;              // _78
	f32 mIconAlphaScale;     // _7C
	s32 _80;                 // _80
	// _84 vtable

	virtual ~InfoPoint() { } // _08 (weak)
};
STATIC_ASSERT(sizeof(InfoPoint) == 0x88);

// the points' ids, the first is gHudMoneyCounter's. japan has two fewer before them
#if defined(VERSION_JP)
#define INFOPOINT_ID 0x96
#else
#define INFOPOINT_ID 0x98
#endif

extern "C" {
extern InfoPoint gHudMoneyCounter;
extern InfoPoint gHudHappyPointCounter;
extern InfoPoint gHudScrapCounter;

void UpdateHudPointValue(InfoPoint* point, u32 frames);
void InitHudPointCounter(InfoPoint* point, s32 index);
}

#endif
