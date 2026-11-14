#ifndef _GAME_SAVEDATA_H
#define _GAME_SAVEDATA_H

#include "types.h"
#include "game/savepreview.h"

// include a Vec header first, the sdk and hsd ones clash

// the save file, "MCQP" from the signature. a file is two 0x6000 copies of SaveFile

#define SAVE_SIGNATURE 0x4D435150
#define SAVE_VERSION   0x10300

struct SaveData {
	SaveHeader mHeader;     // _0000
	u8 mPlayer[0x400];      // _0058
	u8 mEvtFlags[0x200];    // _0458
	u8 mEvtVars[0x2000];    // _0658
	u32 mCoinFlags[0x80];   // _2658
	u32 mItemFlags[0x80];   // _2858
	u32 mDustFlags[0x80];   // _2A58
	u8 mPlayerRecord[0x48]; // _2C58
	s32 mSuit;              // _2CA0
	Vec mPos;               // _2CA4
	f32 mRotY;              // _2CB0
	s32 mSpecialSuit;       // _2CB4, XObjChr mSuits.mSpecial, the special suit index used when mSuit is 7
	s32 mCameraAction;      // _2CB8
	Vec mCameraEye;         // _2CBC
	Vec mCameraInterest;    // _2CC8
	Vec mCameraAngle;       // _2CD4
	f32 mCameraDistance;    // _2CE0
	f32 mCameraFov;         // _2CE4
	s32 mSquirterLiquid; // _2CE8, XObjChr _158A8: what the squirter holds. Script command 900 sets it with the item tint, a sprayed object
	                     // runs event 3000 + it, cleared to 0 with the tint when the squirter runs dry
	s32 mItemTint;       // _2CEC, XObjChr _158AC
};
STATIC_ASSERT(sizeof(SaveData) == 0x2CF0);

// the comment and the pictures the ipl shows
struct SaveBanner {
	char mTitle[0x20];   // _0000
	char mComment[0x20]; // _0020
	u8 mBanner[0x1800];  // _0040
	u8 mIcon[0x800];     // _1840
};
STATIC_ASSERT(sizeof(SaveBanner) == 0x2040);

struct SaveFile {
	SaveBanner mBanner;        // _0000
	u32 mChecksum;             // _2040, byte sum of mData
	u8 _2044[0x2200 - 0x2044]; // _2044
	SaveData mData;            // _2200
	u8 _4EF0[0x6000 - 0x4EF0]; // _4EF0
};
STATIC_ASSERT(sizeof(SaveFile) == 0x6000);

extern "C" s32 GetStageNum();

static inline BOOL IsValidSave(SaveHeader* save)
{
	if (save->mVersion < 0x400)
		return FALSE;
	if (!((save->mStage >= 0 && save->mStage < GetStageNum()) || (save->mStage >= 100 && save->mStage < 108)))
		return FALSE;
	if (save->mVersion < 0x10100) {
		if (save->mMoney > 99999)
			save->mMoney = 99999;
		if (save->mScrap > 99999)
			save->mScrap = 99999;
	}
	if (save->mExp < 0 || save->mExp > 99999)
		return FALSE;
	if (save->mLevel < 0 || save->mLevel > 30)
		return FALSE;
	if (save->mRank < 0 || save->mRank > 1000000)
		return FALSE;
	if (save->mBattery < 0.0f || save->mBattery > 999.9f)
		return FALSE;
	if (save->mMaxBattery < 0.0f || save->mMaxBattery > 999.9f)
		return FALSE;
	if (save->mMoney < 0 || save->mMoney > 99999)
		return FALSE;
	if (save->mScrap < 0 || save->mScrap > 99999)
		return FALSE;
	return TRUE;
}

#endif
