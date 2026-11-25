#ifndef _GAME_SAVEPREVIEW_H
#define _GAME_SAVEPREVIEW_H

#include "Dolphin/card.h"

// the part of the save the slot list reads without loading the rest
struct SaveHeader {
	u32 mSignature;  // _00
	u32 mVersion;    // _04, the low two bits are flags
	s32 mStage;      // _08, 100 and up for the stages after the table's null entry
	s32 mTimeMode;   // _0C, EvtTimeState's
	f32 mTime;       // _10
	f32 mDuration;   // _14
	s32 mExp;        // _18
	s32 mLevel;      // _1C
	s32 mRank;       // _20
	f32 mBattery;    // _24
	f32 mMaxBattery; // _28
	s32 mMoney;      // _2C
	s32 mScrap;      // _30
	s32 mActive;     // _34, a copy of Params _1C[0]: script command 850 sets it, the battery HUD hides and the slot list shows an icon
	u8 _38[0x1C];    // _38
	s32 mFrames;     // _54, play time
};
STATIC_ASSERT(sizeof(SaveHeader) == 0x58);

STATIC_ASSERT(sizeof(CARDStat) == 0x6C);

static inline BOOL HasSaveFileLayout(const CARDStat& stat)
{
	if (stat.length != 0xC000)
		return FALSE;
	if (stat.bannerFormat != CARD_STAT_BANNER_RGB5A3)
		return FALSE;
	if (stat.iconAddr != 0x40)
		return FALSE;
	if (stat.iconFormat != CARD_STAT_ICON_RGB5A3)
		return FALSE;
	if (stat.iconSpeed != CARD_STAT_SPEED_SLOW)
		return FALSE;
	if (stat.commentAddr != 0)
		return FALSE;
	if (stat.offsetBanner != 0x40)
		return FALSE;
	if (stat.offsetIcon[0] != 0x1840)
		return FALSE;
	if (stat.offsetData != 0x2040)
		return FALSE;
	return TRUE;
}

// a save file the slot list shows, its card status and the start of its save
struct SavePreviewRecord {
	s32 mState;         // _00, nonzero when the slot has a file
	s32 mStatus;        // _04, nonzero when reading it failed
	CARDStat mStat;     // _08
	SaveHeader mHeader; // _74
};
STATIC_ASSERT(sizeof(SavePreviewRecord) == 0xCC);

extern "C" {
// is the save slot's data intact
BOOL IsValidSavePreview(SavePreviewRecord* save);
}

#endif
