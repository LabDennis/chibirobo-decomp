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

// todo: pools 4, 5 and 17 keep a save block right after XObj, no idea what class
static inline void* PoolSaveData(s32 pool)
{
	return *(void**)(gGame.mXObjs.mPools[pool + 3].mObjects + sizeof(XObj));
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

static inline void SetMoney(Params* stats, s32 value)
{
	stats->mMoney = value > 0 ? value : 0;
	if (stats->mMoney < 0)
		stats->mMoney = 0;
	if (stats->mMoney > 99999)
		stats->mMoney = 99999;
}

static inline void SetScrap(Params* stats, s32 value)
{
	stats->mScrap = value > 0 ? value : 0;
	if (stats->mScrap < 0)
		stats->mScrap = 0;
	if (stats->mScrap > 99999)
		stats->mScrap = 99999;
}

static inline void RequestSuit(XObjChr* chr, s32 value)
{
	if (chr->mSuits.mChangeStep == FALSE) {
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
	if (stage >= 100) {
		stage += GetStageNum();
		stage -= 99;
	}
	gGame.mPrevStage           = gGame.mStage;
	gGame.mStage               = stage;
	gGame.mTimeState.mMode     = save->mHeader.mTimeMode;
	gGame.mTimeState.mTime     = save->mHeader.mTime;
	gGame.mTimeState.mRunning  = 0;
	gGame.mTimeState.mDuration = save->mHeader.mDuration;
	SetExperience(Stats(), save->mHeader.mExp);
	SetLevel(Stats(), save->mHeader.mLevel);
	SetRank(Stats(), save->mHeader.mRank);
	SetMaxBattery(Stats(), save->mHeader.mMaxBattery);
	Stats()->SetBattery(save->mHeader.mBattery);
	SetMoney(Stats(), save->mHeader.mMoney);
	SetScrap(Stats(), save->mHeader.mScrap);
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
	memcpy(PoolSaveData(4), save->mObjects[0], sizeof(save->mObjects[0]));
	memcpy(PoolSaveData(5), save->mObjects[1], sizeof(save->mObjects[1]));
	memcpy(PoolSaveData(17), save->mObjects[2], sizeof(save->mObjects[2]));
	memcpy(&Player()->mAttachments, save->mPlayerRecord, sizeof(save->mPlayerRecord));
	RequestSuit(Player(), save->mSuit);
	Player()->mSuits.mSpecial  = save->mSpecialSuit;
	Player()->mSquirterContent = save->mSquirterLiquid;
	Player()->mItemTintWord    = save->mItemTint;
}
