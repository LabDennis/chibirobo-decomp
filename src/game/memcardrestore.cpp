#include <dolphin/mtx/vec.h>
#include "game/savedata.h"
#include "game/cgame.h"
#include "game/xchr.h"
#include "game/xobjflags.h"
#include "game/inventory.h"
#include "game/evt.h"
#include <string.h>

static s32 ClampSaveStat(s32 value, s32 minimum, s32 maximum)
{
	if (value < minimum)
		return minimum;
	return value > maximum ? maximum : value;
}

static inline void RequestSuit(XObjChr* chr, s32 value)
{
	if (chr->mSuits.mChangeStep == FALSE) { // only when no suit change is in progress
		chr->mSuits.mRequested  = value;
		chr->mSuits.mChangeStep = TRUE;
	}
}

/**
 * @note Address: 0x80087CC0
 * @note Size: 0x42C
 */
extern "C" void RestoreGameSaveData(Memcard* card)
{
	SaveData* save = &card->mFile.mData;
	s32 stage      = save->mHeader.mStage;
	if (stage >= 100) { // stages >= 100 are relative to the current stage count
		stage += GetStageNum();
		stage -= 99;
	}
	gGame.mPrevStage           = gGame.mStage;
	gGame.mStage               = stage;
	gGame.mTimeState.mMode     = save->mHeader.mTimeMode;
	gGame.mTimeState.mTime     = save->mHeader.mTime;
	gGame.mTimeState.mRunning  = 0; // the clock starts stopped
	gGame.mTimeState.mDuration = save->mHeader.mDuration;
	XObjChr* player            = gGame.GetPlayer();
	Params& stats              = player->mParams;
	stats.mExperience          = ClampSaveStat(save->mHeader.mExp, 0, 99999);
	stats.mLevel               = ClampSaveStat(save->mHeader.mLevel, 0, 30);
	stats.mRank                = ClampSaveStat(save->mHeader.mRank, 1, 1000000);
	const f32 maxBattery       = save->mHeader.mMaxBattery > 0.0f ? save->mHeader.mMaxBattery : 0.0f;
	stats.mMaxBattery          = maxBattery > 999.9f ? 999.9f : maxBattery;
	stats.SetBattery(save->mHeader.mBattery);
	stats.SetMoney(save->mHeader.mMoney);
	stats.SetScrap(save->mHeader.mScrap);
	stats._1C[0] = save->mHeader.mActive != 0;
#if defined(VERSION_PAL)
	gGame.mPlayTime = (f32)save->mHeader.mFrames;
#else
	gGame.mPlayTime = save->mHeader.mFrames;
#endif
	memcpy(stats.mInventory.mItems, save->mPlayer, sizeof(save->mPlayer));
	memcpy(gEventFlags.mFlags, save->mEvtFlags, sizeof(save->mEvtFlags));
	memcpy(gEventVars.mVars, save->mEvtVars, sizeof(save->mEvtVars));
	memcpy(static_cast<XObjCoin*>(gGame.mXObjs.mPools[7].First())->mFlags->mFlags, save->mCoinFlags, sizeof(save->mCoinFlags));
	memcpy(static_cast<XObjItem*>(gGame.mXObjs.mPools[8].First())->mFlags->mFlags, save->mItemFlags, sizeof(save->mItemFlags));
	memcpy(static_cast<XObjDust*>(gGame.mXObjs.mPools[20].First())->mFlags->mFlags, save->mDustFlags, sizeof(save->mDustFlags));
	memcpy(&player->mAttachments, save->mPlayerRecord, sizeof(save->mPlayerRecord));
	RequestSuit(player, save->mSuit);
	player->mSuits.mSpecial  = save->mSpecialSuit;
	player->mSquirterContent = save->mSquirterLiquid;
	player->mItemTintWord    = save->mItemTint;
}
