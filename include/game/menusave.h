#ifndef _GAME_MENUSAVE_H
#define _GAME_MENUSAVE_H

#include "types.h"

// a save file as the save and title menus list it
struct MenuSaveSlot {
	u32 mUsed;           // _00
	s32 mLevel;          // _04
	f32 mBattery;        // _08
	f32 mMaxBattery;     // _0C
	u32 mMoney;          // _10
	u32 mExp;            // _14
	u32 mScrap;          // _18
	u32 mRank;           // _1C
	u32 mHours;          // _20
	u32 mMinutes;        // _24
	s32 mStage;          // _28
	u32 mTimeMode;       // _2C
	u8 _30[0x40 - 0x30]; // _30
	s32 mState;          // _40, 1 when the file is broken
	u8 mActive;          // _44
};
STATIC_ASSERT(sizeof(MenuSaveSlot) == 0x48);

inline void ClearSaveSlot(MenuSaveSlot* slot)
{
	slot->mUsed       = 0;
	slot->mLevel      = 0;
	slot->mBattery    = 0.0f;
	slot->mMaxBattery = 0.0f;
	slot->mMoney      = 0;
	slot->mExp        = 0;
	slot->mScrap      = 0;
	slot->mRank       = 0;
	slot->mHours      = 0;
	slot->mMinutes    = 0;
	slot->mStage      = 0;
	slot->mTimeMode   = 0;
	slot->mState      = 0;
	slot->mActive     = 0;
}

extern "C" {
bool IsSaveSlotIntact(const MenuSaveSlot* slot);
void ResetSaveSlotRecord(MenuSaveSlot* slot);

// the three save files as the card menus last read them
extern MenuSaveSlot gTitleSaveSlots[3];
}

#endif
