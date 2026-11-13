#include <dolphin/mtx/vec.h>
#include "game/savedata.h"
#include "game/cgame.h"
#include "game/xchr.h"
#include "game/inventory.h"
#include "game/evt.h"
#include <string.h>

static inline XObjChr* Player()
{
	return gGame.GetPlayer();
}

static inline Params* Stats()
{
	return &Player()->mParams;
}

static inline void SetExperience(Params* stats, s32 value)
{
	stats->mExperience = value > 0 ? value : 0;
	if (stats->mExperience < 0)
		stats->mExperience = 0;
	if (stats->mExperience > 99999)
		stats->mExperience = 99999;
}

static inline void SetLevel(Params* stats, s32 value)
{
	stats->mLevel = value > 0 ? value : 0;
	if (stats->mLevel < 0)
		stats->mLevel = 0;
	if (stats->mLevel > 30)
		stats->mLevel = 30;
}

static inline void SetRank(Params* stats, s32 value)
{
	stats->mRank = value > 0 ? value : 0;
	if (stats->mRank < 1)
		stats->mRank = 1;
	if (stats->mRank > 1000000)
		stats->mRank = 1000000;
}

static inline void SetMaxBattery(Params* stats, f32 value)
{
	stats->mMaxBattery = value > 0.0f ? value : 0.0f;
	if (stats->mMaxBattery < 0.0f)
		stats->mMaxBattery = 0.0f;
	if (stats->mMaxBattery > 999.9f)
		stats->mMaxBattery = 999.9f;
	stats->SetBattery(stats->mBattery);
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
	SetExperience(Stats(), save->mHeader.mExp);
	SetLevel(Stats(), save->mHeader.mLevel);
	SetRank(Stats(), save->mHeader.mRank);
	SetMaxBattery(Stats(), save->mHeader.mMaxBattery);
	Stats()->SetBattery(save->mHeader.mBattery);
	Stats()->SetMoney(save->mHeader.mMoney);
	Stats()->SetScrap(save->mHeader.mScrap);
	if (save->mHeader.mActive)
		Stats()->_1C[0] = 1;
	else
		Stats()->_1C[0] = 0;
#if defined(VERSION_PAL)
	gGame.mPlayTime = (f32)save->mHeader.mFrames;
#else
	gGame.mPlayTime = save->mHeader.mFrames;
#endif
	memcpy(Player()->mParams.mInventory.mItems, save->mPlayer, sizeof(save->mPlayer));
	memcpy(gEventFlags.mFlags, save->mEvtFlags, sizeof(save->mEvtFlags));
	memcpy(gEventVars.mVars, save->mEvtVars, sizeof(save->mEvtVars));
	memcpy(gGame.mXObjs.GetCollectionFlags(XOBJS::COINS).mFlags, save->mCoinFlags, sizeof(save->mCoinFlags));
	memcpy(gGame.mXObjs.GetCollectionFlags(XOBJS::ITEMS).mFlags, save->mItemFlags, sizeof(save->mItemFlags));
	memcpy(gGame.mXObjs.GetCollectionFlags(XOBJS::DUST).mFlags, save->mDustFlags, sizeof(save->mDustFlags));
	memcpy(&Player()->mAttachments, save->mPlayerRecord, sizeof(save->mPlayerRecord));
	RequestSuit(Player(), save->mSuit);
	Player()->mSuits.mSpecial  = save->mSpecialSuit;
	Player()->mSquirterContent = save->mSquirterLiquid;
	Player()->mItemTintWord    = save->mItemTint;
}
