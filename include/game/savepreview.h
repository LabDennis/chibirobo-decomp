#ifndef _GAME_SAVEPREVIEW_H
#define _GAME_SAVEPREVIEW_H

#include "types.h"

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
	s32 _34;         // _34
	u8 _38[0x1C];    // _38
	s32 mFrames;     // _54, play time
};
STATIC_ASSERT(sizeof(SaveHeader) == 0x58);

#if defined(_GAME_CCARD_H)
typedef CARDStat SaveCardStat;
#else
// CARDStat, the sdk card header can't come in next to the hsd ones
struct SaveCardStat {
	char fileName[32];    // _00
	u32 length;           // _20
	u32 time;             // _24
	u8 gameName[4];       // _28
	u8 company[2];        // _2C
	u8 bannerFormat;      // _2E
	u8 _2F;               // _2F
	u32 iconAddr;         // _30
	u16 iconFormat;       // _34
	u16 iconSpeed;        // _36
	u32 commentAddr;      // _38
	u32 offsetBanner;     // _3C
	u32 offsetBannerTlut; // _40
	u32 offsetIcon[8];    // _44
	u32 offsetIconTlut;   // _64
	u32 offsetData;       // _68
};
#endif
STATIC_ASSERT(sizeof(SaveCardStat) == 0x6C);

static inline BOOL HasSaveFileLayout(const SaveCardStat& stat)
{
	if (stat.length != 0xC000)
		return FALSE;
	if (stat.bannerFormat != 2)
		return FALSE;
	if (stat.iconAddr != 0x40)
		return FALSE;
	if (stat.iconFormat != 2)
		return FALSE;
	if (stat.iconSpeed != 3)
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
	SaveCardStat mStat; // _08
	SaveHeader mHeader; // _74
};
STATIC_ASSERT(sizeof(SavePreviewRecord) == 0xCC);

extern "C" {
// is the save slot's data intact
BOOL IsValidSavePreview(SavePreviewRecord* save);
}

#endif
