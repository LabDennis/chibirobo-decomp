#include <dolphin/mtx/vec.h>
#include "game/ccard.h"
#include "game/memcard.h"
#include "game/savedata.h"
#include "game/cgame.h"
#include "game/xchr.h"
#include "game/inventory.h"
#include "game/evt.h"
#include "game/cmotioncontrolcamera.h"
#include "game/xobjchr.h"
#include <string.h>

extern "C" void SetMapCameraKeepPreviousEye(CInterfaceCameraAction* action);

// NONMATCHING: retail loads slot->mStat.length before it saves r31, the build after
/**
 * @note Address: 0x80086F24
 * @note Size: 0x26C
 */
extern "C" BOOL IsValidSavePreview(SavePreviewRecord* slot)
{
	if (!HasSaveFileLayout(slot->mStat))
		return FALSE;
	return IsValidSave(&slot->mHeader) != FALSE;
}

static inline BOOL ReadFailed(Memcard* card)
{
	card->mCard.Close();
	return TRUE;
}

/**
 * @note Address: 0x80087190
 * @note Size: 0x378
 */
extern "C" BOOL ReadValidatedSaveFileCopy(Memcard* card, SaveFile* file)
{
	card->mState = 12;
	for (s32 copy = 0; copy < 2; ++copy) {
		card->mCard.Read(file, sizeof(SaveFile), copy * (s32)sizeof(SaveFile));
		card->LogResult();
		if (card->mCard.mResult != CARD_RESULT_READY)
			return ReadFailed(card);
		SaveHeader* save = &file->mData.mHeader;
		if (save->mSignature != SAVE_SIGNATURE)
			card->mCard.mResult = CARD_RESULT_FATAL_ERROR;
		card->LogResult();
		if (card->mCard.mResult != CARD_RESULT_READY) {
			if (copy == 1)
				return ReadFailed(card);
			continue;
		}
		u32 checksum   = 0;
		const u8* data = reinterpret_cast<const u8*>(save);
		for (u32 i = 0; i < sizeof(SaveData); ++i)
			checksum += *data++;
		if (checksum != file->mChecksum)
			card->mCard.mResult = CARD_RESULT_FATAL_ERROR;
		card->LogResult();
		if (card->mCard.mResult != CARD_RESULT_READY) {
			if (copy == 1)
				return ReadFailed(card);
			continue;
		}
		if (!IsValidSave(save)) {
			if (copy == 1) {
				card->mCard.mResult = CARD_RESULT_FATAL_ERROR;
				return ReadFailed(card);
			}
			continue;
		}
		break;
	}
	return FALSE;
}

static inline BOOL ReadFailedUnmount(Memcard* card)
{
	card->mCard.Close();
	card->mCard.Unmount();
	return TRUE;
}

/**
 * @note Address: 0x80087508
 * @note Size: 0x34C
 */
extern "C" BOOL ReadValidatedSaveIntoCardBuffer(Memcard* card)
{
	card->mState   = 12;
	SaveFile* file = &card->mFile;
	s32 offset;
	s32 copy;
	SaveHeader* save = &file->mData.mHeader;
	for (copy = 0, offset = 0; copy < 2; ++copy, offset += sizeof(SaveFile)) {
		card->mCard.Read(card->mBuffer, sizeof(SaveFile), offset);
		card->LogResult();
		if (card->mCard.mResult != CARD_RESULT_READY)
			return ReadFailedUnmount(card);
		if (file->mData.mHeader.mSignature != SAVE_SIGNATURE) {
			if (copy == 1) {
				card->mCard.mResult = CARD_RESULT_FATAL_ERROR;
				return ReadFailedUnmount(card);
			}
			continue;
		}
		u32 checksum   = 0;
		const u8* data = reinterpret_cast<const u8*>(save);
		for (u32 i = 0; i < sizeof(SaveData); ++i)
			checksum += *data++;
		if (checksum != file->mChecksum) {
			if (copy == 1) {
				card->mCard.mResult = CARD_RESULT_FATAL_ERROR;
				return ReadFailedUnmount(card);
			}
			continue;
		}
		if (!IsValidSave(save)) {
			if (copy == 1) {
				card->mCard.mResult = CARD_RESULT_FATAL_ERROR;
				return ReadFailedUnmount(card);
			}
			continue;
		}
		break;
	}
	return FALSE;
}

/**
 * @note Address: 0x80087854
 * @note Size: 0x1C8
 */
extern "C" BOOL OpenOrCreateSaveFile(Memcard* card, char* name, BOOL create)
{
	if (create) {
		card->mState = 7;
		card->mCard.Create(name, 0xC000);
		card->LogResult();
		if (card->mCard.mResult != CARD_RESULT_READY) {
			switch (card->mCard.mResult) {
			case CARD_RESULT_EXIST:
				card->mState = 8;
				card->mCard.Open(name);
				card->LogResult();
				if (card->mCard.mResult != CARD_RESULT_READY) {
					card->mCard.Unmount();
					return TRUE;
				}
				break;
			default:
				card->mCard.Unmount();
				return TRUE;
			}
		}
	} else {
		card->mState = 8;
		card->mCard.Open(name);
		card->LogResult();
		if (card->mCard.mResult != CARD_RESULT_READY) {
			card->mCard.Unmount();
			return TRUE;
		}
	}

	card->mState = 10;
	card->mCard.GetStatus(&card->mStat);
	card->LogResult();
	if (card->mCard.mResult != CARD_RESULT_READY) {
		card->mCard.Close();
		card->mCard.Unmount();
		return TRUE;
	}
	if (!create && card->mStat.length != 0xC000) {
		card->mCard.mResult = CARD_RESULT_FATAL_ERROR;
		card->mCard.Close();
		card->mCard.Unmount();
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x80087A1C
 * @note Size: 0x24
 */
extern "C" s32 ProbeMemoryCardCapacity(Memcard* card, s32 channel, s32* memorySize, s32* sectorSize)
{
	return card->mCard.Probe(channel, memorySize, sectorSize);
}

/**
 * @note Address: 0x80087A40
 * @note Size: 0xC
 */
extern "C" s32 GetSaveFileStage(Memcard* card)
{
	return card->mFile.mData.mHeader.mStage;
}

/**
 * @note Address: 0x80087A4C
 * @note Size: 0x23C
 */
extern "C" void RestoreSavedPlayerAndCamera(Memcard* card)
{
	SaveData* save = &card->mFile.mData;
	XObj* player   = gGame.GetPlayer();
	player->SetPos(save->mPos, 0);
	gGame.GetPlayer()->SetRotationY(save->mRotY, 0);
	ResetPlayerHeadOffset(gGame.GetPlayer());
	ResetPlayerPlug(gGame.GetPlayer());
	if (save->mHeader.mVersion <= 0x10100 || (save->mHeader.mVersion & 2)) {
		CCamera* camera              = &gGame.mMotionControlCamera.mCamera;
		CMotionControlCamera* motion = &gGame.mMotionControlCamera;
		if (save->mCameraAction >= 0 && save->mCameraAction < CAMACTION_NUM) {
			motion->ChangeAction(save->mCameraAction);
			camera->SetEye(save->mCameraEye);
			camera->SetInterest(save->mCameraInterest);
			camera->SetAngle(save->mCameraAngle);
			camera->SetDistance(save->mCameraDistance);
			camera->SetFov(save->mCameraFov);
			CCameraMemento memento;
			memento.SetEye(save->mCameraEye);
			memento.SetInterest(save->mCameraInterest);
			memento.SetAngle(save->mCameraAngle);
			memento.mDistance = save->mCameraDistance;
			memento.mFov      = save->mCameraFov;
			memento.mAction   = gGame.mCameraActions.mTable[save->mCameraAction];
			SetMapCameraKeepPreviousEye(gGame.mCameraActions.mTable[CAMACTION_MAP]);
			motion->SetMemento(&memento);
		}
	}
}

void CCamera::SetAngle(const Vec& angle)
{
	SetAngle(angle.x, angle.y, angle.z);
}

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
		stage += GetStageNum() - 99;
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
