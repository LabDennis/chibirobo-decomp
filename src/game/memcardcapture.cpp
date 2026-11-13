#include <dolphin/mtx/vec.h>
#include "game/savedata.h"
#include "game/cgame.h"
#include "game/xchr.h"
#include "game/cmotioncontrolcamera.h"
#include "game/evt.h"
#include <string.h>

static inline XObjChr* Player()
{
	return gGame.GetPlayer();
}

static inline void CopyVec(Vec& dst, const Vec& src)
{
	dst.x = src.x;
	dst.y = src.y;
	dst.z = src.z;
}

/**
 * @note Address: 0x800880EC
 * @note Size: 0x404
 */
extern "C" void CaptureGameSaveData(Memcard* card)
{
	SaveFile* file                 = &card->mFile;
	file->mData.mHeader.mSignature = SAVE_SIGNATURE;
	file->mData.mHeader.mVersion   = SAVE_VERSION;
	SaveData* save                 = &file->mData;
	s32 stage                      = gGame.mStage;
	if (stage > GetStageNum())
		stage = stage - GetStageNum() + 99;
	save->mHeader.mStage      = stage;
	save->mHeader.mTimeMode   = gGame.mTimeState.mMode;
	save->mHeader.mTime       = gGame.mTimeState.mTime;
	save->mHeader.mDuration   = gGame.mTimeState.mDuration;
	save->mHeader.mExp        = Player()->mParams.mExperience;
	save->mHeader.mLevel      = Player()->mParams.mLevel;
	save->mHeader.mRank       = Player()->mParams.mRank;
	save->mHeader.mBattery    = Player()->mParams.mBattery;
	save->mHeader.mMaxBattery = Player()->mParams.mMaxBattery;
	save->mHeader.mMoney      = Player()->mParams.mMoney;
	save->mHeader.mScrap      = Player()->mParams.mScrap;
	save->mHeader.mActive     = Player()->mParams._1C[0];
#if defined(VERSION_PAL)
	save->mHeader.mFrames = (s32)gGame.mPlayTime;
#else
	save->mHeader.mFrames = gGame.mPlayTime;
#endif
	memcpy(save->mPlayer, Player()->mParams.mInventory.mItems, sizeof(save->mPlayer));
	memcpy(save->mEvtFlags, gEventFlags.mFlags, sizeof(save->mEvtFlags));
	memcpy(save->mEvtVars, gEventVars.mVars, sizeof(save->mEvtVars));
	memcpy(save->mCoinFlags, gGame.mXObjs.GetCollectionFlags(XOBJS::COINS).mFlags, sizeof(save->mCoinFlags));
	memcpy(save->mItemFlags, gGame.mXObjs.GetCollectionFlags(XOBJS::ITEMS).mFlags, sizeof(save->mItemFlags));
	memcpy(save->mDustFlags, gGame.mXObjs.GetCollectionFlags(XOBJS::DUST).mFlags, sizeof(save->mDustFlags));
	memcpy(save->mPlayerRecord, &Player()->mAttachments, sizeof(save->mPlayerRecord));
	save->mSuit = Player()->mSuits.mCurrent;
	CopyVec(save->mPos, gGame.GetPlayer()->mMoveTranslate);
	save->mRotY        = gGame.GetPlayer()->mMoveRotate.y;
	save->mSpecialSuit = Player()->mSuits.mSpecial;
	if (gGame.mStatus.mFlags & 0x0000000800000000ULL)
		save->mHeader.mVersion |= 1;
	else
		save->mHeader.mVersion &= ~1;
	if (gGame.mStatus.mFlags & 1ULL)
		save->mHeader.mVersion |= 2;
	else
		save->mHeader.mVersion &= ~2;
	if (save->mHeader.mVersion & 2) {
		CMotionControlCamera* camera = &gGame.mMotionControlCamera;
		save->mCameraAction          = camera->GetActionType();
		CopyVec(save->mCameraEye, camera->mCamera.mEye);
		CopyVec(save->mCameraInterest, camera->mCamera.mInterest);
		CopyVec(save->mCameraAngle, camera->mCamera.mAngle);
		save->mCameraDistance = camera->mCamera.mDistance;
		save->mCameraFov      = camera->mCamera.mFov;
	}
	save->mSquirterLiquid = Player()->mSquirterContent;
	save->mItemTint       = Player()->mItemTintWord;
	file->mChecksum       = 0;
	const u8* cursor      = reinterpret_cast<const u8*>(save);
	for (u32 i = 0; i < sizeof(SaveData); ++i)
		file->mChecksum += *cursor++;
}
