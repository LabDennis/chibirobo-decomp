#include <dolphin/mtx/vec.h>
#include "game/ccard.h"
#include "game/memcard.h"
#include "game/savedata.h"
#include "game/cgame.h"
#include "game/cthread.h"
#include "game/ctpl.h"
#include "game/xchr.h"
#include "game/inventory.h"
#include "game/evt.h"
#include "game/cmotioncontrolcamera.h"
#include "game/xobjchr.h"
#include "game/globals.h"
#include "Version.h"
#include <stdio.h>
#include <string.h>

extern "C" void SetMapCameraKeepPreviousEye(CInterfaceCameraAction* action);
extern "C" BOOL CARDSetFastMode(BOOL enable);

#if defined(VERSION_JP)
enum { BANNER_TEXTURE = 87, ICON_TEXTURE = 88 };
#elif defined(VERSION_PAL)
enum { BANNER_TEXTURE = 89, ICON_TEXTURE = 90 };
#else
enum { BANNER_TEXTURE = 88, ICON_TEXTURE = 90 };
#endif

CThread gMemoryCardThread;
u8 gMemoryCardThreadStack[0x1000];

// a slot's file name while reading previews
char gSaveReadFileName[0xC8];
// CARDRead's DMA target, so the buffer itself is 32-byte aligned
SaveFile gSavePreviewFileBuffer[2] ATTRIBUTE_ALIGN(32);
// a slot's file name while scanning
char gSaveScanFileName[0x80];

// NONMATCHING: retail loads slot->mStat.length before it saves r31, the build after
/**
 * @note Address: 0x80086F24
 * @note Size: 0x26C
 */
extern "C" BOOL IsValidSavePreview(SavePreviewRecord* slot)
{
	if (!HasSaveFileLayout(slot->mStat))
		return FALSE;
	return ValidateSaveHeader(&slot->mHeader) != FALSE;
}

/**
 * @note Address: 0x80087190
 * @note Size: 0x378
 */
BOOL Memcard::ReadSavePreview(SaveFile* file)
{
	mState = 12;
	for (s32 copy = 0; copy < 2; ++copy) {
		if (ReadSaveCopy(file, copy * (s32)sizeof(SaveFile)) != CARD_RESULT_READY)
			return FailSaveRead(FALSE);
		SaveData* save = &file->mData;
		if (!save->HasValidSignature())
			mCard.mResult = CARD_RESULT_FATAL_ERROR;
		LogResult();
		if (mCard.mResult != CARD_RESULT_READY) {
			if (copy == 1)
				return FailSaveRead(FALSE);
			continue;
		}
		u32 checksum = save->CalculateChecksum();
		if (checksum != file->mChecksum)
			mCard.mResult = CARD_RESULT_FATAL_ERROR;
		LogResult();
		if (mCard.mResult != CARD_RESULT_READY) {
			if (copy == 1)
				return FailSaveRead(FALSE);
			continue;
		}
		if (!ValidateSaveHeader(&save->mHeader)) {
			if (copy == 1) {
				mCard.mResult = CARD_RESULT_FATAL_ERROR;
				return FailSaveRead(FALSE);
			}
			continue;
		}
		break;
	}
	return FALSE;
}

/**
 * @note Address: 0x80087508
 * @note Size: 0x34C
 */
BOOL Memcard::ReadSaveFile()
{
	mState         = 12;
	SaveFile* file = &mFile;
	s32 offset;
	s32 copy;
	SaveData* save = &file->mData;
	for (copy = 0, offset = 0; copy < 2; ++copy, offset += sizeof(SaveFile)) {
		mCard.Read(mBuffer, sizeof(SaveFile), offset);
		LogResult();
		if (mCard.mResult != CARD_RESULT_READY)
			return FailSaveRead(TRUE);
		if (!file->mData.HasValidSignature()) {
			if (copy == 1) {
				mCard.mResult = CARD_RESULT_FATAL_ERROR;
				return FailSaveRead(TRUE);
			}
			continue;
		}
		u32 checksum = save->CalculateChecksum();
		if (checksum != file->mChecksum) {
			if (copy == 1) {
				mCard.mResult = CARD_RESULT_FATAL_ERROR;
				return FailSaveRead(TRUE);
			}
			continue;
		}
		if (!ValidateSaveHeader(&save->mHeader)) {
			if (copy == 1) {
				mCard.mResult = CARD_RESULT_FATAL_ERROR;
				return FailSaveRead(TRUE);
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

static inline void CopyVec(Vec& dst, const Vec& src)
{
	dst.x = src.x;
	dst.y = src.y;
	dst.z = src.z;
}

static inline void* SaveTextureData(const CTexObj& texture)
{
	return texture.mImage != NULL ? texture.mImage->data : NULL;
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
	save->CalculateChecksum(file->mChecksum);
}

/**
 * @note Address: 0x800884F0
 * @note Size: 0x358
 */
// NONMATCHING: America and Europe keep the play time in seconds in a scratch register until the remainder is taken,
// the build has it in the register of the result. Japan's matches. The build also emits it after
// StartMemoryCardSaveRead instead of in source order (cause open; a const CTexObj& banner does not fix it)
extern "C" void BuildSaveBanner(Memcard* card, s32 slot)
{
	SaveBanner* data = &card->mFile.mBanner;
#if defined(VERSION_JP)
	sprintf(data->mTitle, "ちびロボ！");
	s32 time = gGame.mPlayTime;
	sprintf(data->mComment, "(%d) プレイ時間 %2d:%02d:%02d", slot + 1, time / 216000, time / 3600 % 60, time / 60 % 60);
#elif defined(VERSION_PAL)
	sprintf(data->mTitle, GetLocalizedInterfaceText(0x102));
	f32 time    = gGame.mPlayTime;
	s32 seconds = (s32)(time / 60.0f);
	seconds %= 60;
	sprintf(data->mComment, "(%d) %s %2d:%02d:%02d", slot + 1, GetLocalizedInterfaceText(0x103), (s32)(time / 216000.0f),
	        (s32)(time / 3600.0f) % 60, seconds);
#else
	sprintf(data->mTitle, GetLocalizedInterfaceText(0x102));
	s32 time    = gGame.mPlayTime;
	s32 seconds = time / 60;
	seconds %= 60;
	sprintf(data->mComment, "(%d) %s %2d:%02d:%02d", slot + 1, GetLocalizedInterfaceText(0x103), time / 216000, time / 3600 % 60, seconds);
#endif
	CTexObj banner = gGame.mTpl.mTex.Init(((TPLPalette*)gGame.mTpl.mBuffer)->descriptorArray + BANNER_TEXTURE, 0);
	memcpy(data->mBanner, SaveTextureData(banner), sizeof(data->mBanner));
	CTexObj icon = gGame.mTpl.mTex.Init(((TPLPalette*)gGame.mTpl.mBuffer)->descriptorArray + ICON_TEXTURE, 0);
	memcpy(data->mIcon, SaveTextureData(icon), sizeof(data->mIcon));

	CARDSetCommentAddress(&card->mStat, 0);
	CARDSetBannerFormat(&card->mStat, CARD_STAT_BANNER_RGB5A3);
	CARDSetIconAddress(&card->mStat, 0x40);
	CARDSetIconAnim(&card->mStat, CARD_STAT_ANIM_LOOP);
	CARDSetIconFormat(&card->mStat, 0, CARD_STAT_ICON_RGB5A3);
	CARDSetIconSpeed(&card->mStat, 0, CARD_STAT_SPEED_SLOW);
	CARDSetIconSpeed(&card->mStat, 1, CARD_STAT_SPEED_END);
}

/**
 * @note Address: 0x80088848
 * @note Size: 0x58
 */
extern "C" void PollMemoryCardWorker(Memcard* card)
{
	if (card->mBusy && OSIsThreadTerminated(&gMemoryCardThread.mThread)) // the busy flag clears when the worker exits
		card->mBusy = FALSE;
}

/**
 * @note Address: 0x800888A0
 * @note Size: 0x278
 */
extern "C" BOOL DeleteMemoryCardSaveFile(Memcard* card, s32 channel, s32 file)
{
	if (card->Probe(channel))
		return TRUE;
	if (card->Mount(channel, TRUE))
		return TRUE;
	char name[128];
	sprintf(name, "CHIBIROBO(%d)", file);
	if (card->Delete(name))
		return TRUE;
	card->mState = 3;
	card->mCard.UnmountEx();
	card->LogResult();
	if (card->mCard.mResult != CARD_RESULT_READY)
		return TRUE;
	card->mState = 0;
	return FALSE;
}

/**
 * @note Address: 0x80088B18
 * @note Size: 0x34
 */
extern "C" void* DeleteMemoryCardSaveThread(void* arg)
{
	MemcardJob* job = (MemcardJob*)arg;
	DeleteMemoryCardSaveFile(job->mCard, job->mChannel, job->mFile);
	return NULL;
}

/**
 * @note Address: 0x80088B4C
 * @note Size: 0xAC
 */
extern "C" BOOL StartMemoryCardSaveDeletion(Memcard* card, s32 channel, s32 file, BOOL log)
{
	if (card->mBusy) // one job at a time
		return FALSE;
	card->mLog          = log;
	card->mJob.mCard    = card;
	card->mJob.mChannel = channel;
	card->mJob.mFile    = file;
	gMemoryCardThread.Start(DeleteMemoryCardSaveThread, &card->mJob, gMemoryCardThreadStack, sizeof(gMemoryCardThreadStack));
	card->mBusy = TRUE;
	return TRUE;
}

/**
 * @note Address: 0x80088BF8
 * @note Size: 0x250
 */
extern "C" BOOL CheckAndFormatMemoryCard(Memcard* card, s32 channel)
{
	if (card->Probe(channel))
		return TRUE;
	if (card->Mount(channel, FALSE)) {
		switch (card->mCard.mResult) {
		case CARD_RESULT_BROKEN:
		case CARD_RESULT_ENCODING: // a broken or wrong-encoding card gets reformatted
			if (card->Format())
				return TRUE;
			break;
		default:
			card->mCard.Unmount();
			return TRUE;
		}
	}
	card->mState = 3; // 3: unmounting
	card->mCard.UnmountEx();
	card->LogResult();
	if (card->mCard.mResult != CARD_RESULT_READY)
		return TRUE;
	card->mState = 0; // back to idle
	return FALSE;
}

/**
 * @note Address: 0x80088E48
 * @note Size: 0x30
 */
extern "C" void* CheckAndFormatMemoryCardThread(void* arg)
{
	MemcardJob* job = (MemcardJob*)arg;
	CheckAndFormatMemoryCard(job->mCard, job->mChannel);
	return NULL;
}

/**
 * @note Address: 0x80088E78
 * @note Size: 0xA8
 */
extern "C" BOOL StartMemoryCardFormatCheck(Memcard* card, s32 channel, BOOL log)
{
	if (card->mBusy)
		return FALSE;
	card->mLog          = log;
	card->mJob.mCard    = card;
	card->mJob.mChannel = channel;
	gMemoryCardThread.Start(CheckAndFormatMemoryCardThread, &card->mJob, gMemoryCardThreadStack, sizeof(gMemoryCardThreadStack));
	card->mBusy = TRUE;
	return TRUE;
}

// NONMATCHING: retail derives the stat pointer from the card base it keeps, the build
// computes the base into the stat register and keeps a copy of it
/**
 * @note Address: 0x80088F20
 * @note Size: 0x338
 */
extern "C" BOOL ScanMemoryCardForSaveFiles(Memcard* card, s32 channel)
{
	CARDStat* stat;
	BOOL found = FALSE;
	if (card->Probe(channel))
		return TRUE;
	if (card->Mount(channel, TRUE))
		return TRUE;
	stat = &card->mStat;
	for (s32 file = 0; file < 10; ++file) {
		sprintf(gSaveScanFileName, "CHIBIROBO(%d)", file);
		if (!card->OpenStatus(gSaveScanFileName, stat)) {
			card->mState = 9;
			card->mCard.CloseEx();
			card->LogResult();
			if (card->mCard.mResult == CARD_RESULT_READY) {
				found = TRUE;
				if (!(gGame.mStatus.mFlags & 0x4000ULL))
					break;
			}
		}
	}
	card->mState = 3;
	card->mCard.UnmountEx();
	card->LogResult();
	if (card->mCard.mResult != CARD_RESULT_READY)
		return TRUE;
	card->mFound = found;
	card->mState = 0;
	return FALSE;
}

/**
 * @note Address: 0x80089258
 * @note Size: 0x30
 */
extern "C" void* ScanMemoryCardThread(void* arg)
{
	MemcardJob* job = (MemcardJob*)arg;
	ScanMemoryCardForSaveFiles(job->mCard, job->mChannel);
	return NULL;
}

/**
 * @note Address: 0x80089288
 * @note Size: 0xB4
 */
extern "C" BOOL StartMemoryCardScan(Memcard* card, s32 channel, BOOL log)
{
	if (card->mBusy)
		return FALSE;
	card->mLog          = log;
	card->mFound        = FALSE;
	card->mJob.mCard    = card;
	card->mJob.mChannel = channel;
	gMemoryCardThread.Start(ScanMemoryCardThread, &card->mJob, gMemoryCardThreadStack, sizeof(gMemoryCardThreadStack));
	card->mBusy = TRUE;
	return TRUE;
}

// NONMATCHING: register allocation around the loop setup: retail's copy of the card
// base lands one register later, so every callee-saved register shifts by one slot.
// Writing out &card->mStat at both uses instead of the local makes it worse (95.31).
// The build emits it after StartMemoryCardSaveRead: MWCC holds back a function that needs the
// compiler-generated CARDStat assignment (memcpy keeps it in place but changes the code).
/**
 * @note Address: 0x8008933C
 * @note Size: 0x524
 */
extern "C" BOOL ReadMemoryCardPreviews(Memcard* card, s32 channel)
{
	if (card->Probe(channel))
		return TRUE;
	if (card->Mount(channel, TRUE))
		return TRUE;
	CARDStat* stat = &card->mStat;
	for (s32 slot = 0; slot < 10; ++slot) {
		sprintf(gSaveReadFileName, "CHIBIROBO(%d)", slot);
		if (card->OpenStatus(gSaveReadFileName, stat))
			continue;
		if (!card->ReadSavePreview(gSavePreviewFileBuffer)) {
			const SaveHeader& header                  = gSavePreviewFileBuffer[0].mData.mHeader;
			card->mAllSaves[slot].mHeader.mSignature  = header.mSignature;
			card->mAllSaves[slot].mHeader.mVersion    = header.mVersion;
			card->mAllSaves[slot].mHeader.mStage      = header.mStage;
			card->mAllSaves[slot].mHeader.mTimeMode   = header.mTimeMode;
			card->mAllSaves[slot].mHeader.mTime       = header.mTime;
			card->mAllSaves[slot].mHeader.mDuration   = header.mDuration;
			card->mAllSaves[slot].mHeader.mExp        = header.mExp;
			card->mAllSaves[slot].mHeader.mLevel      = header.mLevel;
			card->mAllSaves[slot].mHeader.mRank       = header.mRank;
			card->mAllSaves[slot].mHeader.mBattery    = header.mBattery;
			card->mAllSaves[slot].mHeader.mMaxBattery = header.mMaxBattery;
			card->mAllSaves[slot].mHeader.mMoney      = header.mMoney;
			card->mAllSaves[slot].mHeader.mScrap      = header.mScrap;
			card->mAllSaves[slot].mHeader.mActive     = header.mActive;
			card->mAllSaves[slot].mHeader.mFrames     = header.mFrames;
			card->mState                              = 9;
			card->mCard.CloseEx();
			card->LogResult();
			if (card->mCard.mResult != CARD_RESULT_READY)
				continue;
			card->mAllSaves[slot].mStatus = 0;
		} else {
			card->mAllSaves[slot].mStatus = -1;
		}
		card->mAllSaves[slot].mState = 1;
		card->mAllSaves[slot].mStat  = *stat;
	}
	if (card->GetFreeBlocks())
		return TRUE;
	card->mState = 3;
	card->mCard.UnmountEx();
	card->LogResult();
	if (card->mCard.mResult != CARD_RESULT_READY)
		return TRUE;
	card->mState = 0;
	return FALSE;
}

/**
 * @note Address: 0x80089860
 * @note Size: 0x30
 */
extern "C" void* ReadMemoryCardPreviewsThread(void* arg)
{
	MemcardJob* job = (MemcardJob*)arg;
	ReadMemoryCardPreviews(job->mCard, job->mChannel);
	return NULL;
}

/**
 * @note Address: 0x80089890
 * @note Size: 0xEC
 */
extern "C" BOOL StartMemoryCardPreviewRead(Memcard* card, s32 channel, BOOL log)
{
	if (card->mBusy)
		return FALSE;
	card->mLog = log;
	for (u32 slot = 0; slot < 2; ++slot) {
		for (u32 file = 0; file < 5; ++file) {
			card->mSaves[slot][file].mState = 0;
		}
	}
	card->mJob.mCard    = card;
	card->mJob.mChannel = channel;
	gMemoryCardThread.Start(ReadMemoryCardPreviewsThread, &card->mJob, gMemoryCardThreadStack, sizeof(gMemoryCardThreadStack));
	card->mBusy = TRUE;
	return TRUE;
}

/**
 * @note Address: 0x8008997C
 * @note Size: 0x2E0
 */
extern "C" BOOL ReadMemoryCardSaveFile(Memcard* card, s32 channel, s32 file)
{
	if (card->Probe(channel))
		return TRUE;
	if (card->Mount(channel, TRUE))
		return TRUE;
	char name[128];
	sprintf(name, "CHIBIROBO(%d)", file);
	if (OpenOrCreateSaveFile(card, name, FALSE))
		return TRUE;
	if (card->ReadSaveFile())
		return TRUE;
	if (card->Close())
		return TRUE;
	card->mState = 3;
	card->mCard.UnmountEx();
	card->LogResult();
	if (card->mCard.mResult != CARD_RESULT_READY)
		return TRUE;
	card->mState = 0;
	// retail converts the save time and drops it
	OSCalendarTime calendar;
	OSTicksToCalendarTime(OSSecondsToTicks((OSTime)card->mStat.time), &calendar);
	return FALSE;
}

/**
 * @note Address: 0x80089C5C
 * @note Size: 0x34
 */
extern "C" void* ReadMemoryCardSaveThread(void* arg)
{
	MemcardJob* job = (MemcardJob*)arg;
	ReadMemoryCardSaveFile(job->mCard, job->mChannel, job->mFile);
	return NULL;
}

/**
 * @note Address: 0x80089C90
 * @note Size: 0xAC
 */
extern "C" BOOL StartMemoryCardSaveRead(Memcard* card, s32 channel, s32 file, BOOL log)
{
	if (card->mBusy)
		return FALSE;
	card->mLog          = log;
	card->mJob.mCard    = card;
	card->mJob.mChannel = channel;
	card->mJob.mFile    = file;
	gMemoryCardThread.Start(ReadMemoryCardSaveThread, &card->mJob, gMemoryCardThreadStack, sizeof(gMemoryCardThreadStack));
	card->mBusy = TRUE;
	return TRUE;
}

static inline BOOL WriteSaveCopies(Memcard* card)
{
	s32 copy       = 0;
	s32 offset     = 0;
	CARDStat* stat = &card->mStat;
	for (; copy < 2; ++copy, offset += sizeof(SaveFile)) {
		card->mCard.Write(card->mBuffer, sizeof(SaveFile), offset);
		card->LogResult();
		if (card->mCard.mResult != CARD_RESULT_READY) {
			card->mCard.Close();
			card->mCard.Unmount();
			return TRUE;
		}
		if (copy == 1) {
			CARDStat st;
			card->mCard.GetStatus(&st);
			if (!HasSaveFileLayout(st)) {
				card->mState = 11;
				card->mCard.SetStatus(stat);
				card->LogResult();
				if (card->mCard.mResult != CARD_RESULT_READY) {
					card->mCard.Close();
					card->mCard.Unmount();
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}

/**
 * @note Address: 0x80089D3C
 * @note Size: 0x484
 */
extern "C" BOOL WriteMemoryCardSaveFile(Memcard* card, s32 channel, s32 slot)
{
	if (card->Probe(channel))
		return TRUE;
	if (card->Mount(channel, TRUE))
		return TRUE;
	char name[128];
	sprintf(name, "CHIBIROBO(%d)", slot);
	if (OpenOrCreateSaveFile(card, name, TRUE))
		return TRUE;
	BuildSaveBanner(card, slot);
	CaptureGameSaveData(card);
	card->mState = 13;
	if (WriteSaveCopies(card))
		return TRUE;
	if (card->Close())
		return TRUE;
	card->mState = 3;
	card->mCard.UnmountEx();
	card->LogResult();
	if (card->mCard.mResult != CARD_RESULT_READY)
		return TRUE;
	card->mState = 0;
	OSCalendarTime calendar;
	OSTicksToCalendarTime(OSSecondsToTicks((OSTime)card->mStat.time), &calendar);
	return FALSE;
}

/**
 * @note Address: 0x8008A1C0
 * @note Size: 0x34
 */
extern "C" void* WriteMemoryCardSaveThread(void* arg)
{
	MemcardJob* job = (MemcardJob*)arg;
	WriteMemoryCardSaveFile(job->mCard, job->mChannel, job->mFile);
	return NULL;
}

/**
 * @note Address: 0x8008A1F4
 * @note Size: 0xAC
 */
extern "C" BOOL StartMemoryCardSaveWrite(Memcard* card, s32 channel, s32 file, BOOL log)
{
	if (card->mBusy)
		return FALSE;
	card->mLog          = log;
	card->mJob.mCard    = card;
	card->mJob.mChannel = channel;
	card->mJob.mFile    = file;
	gMemoryCardThread.Start(WriteMemoryCardSaveThread, &card->mJob, gMemoryCardThreadStack, sizeof(gMemoryCardThreadStack));
	card->mBusy = TRUE;
	return TRUE;
}

/**
 * @note Address: 0x8008A2A0
 * @note Size: 0x8
 */
extern "C" BOOL LoadMemoryCardResources(Memcard* card)
{
	return TRUE;
}

/**
 * @note Address: 0x8008A2A8
 * @note Size: 0x6C
 */
extern "C" void InitMemoryCardManager(Memcard* card)
{
	card->mCard.Init();
	CARDSetFastMode(TRUE);
	memset(card->mBuffer, 0, sizeof(card->mBuffer));
	card->mState = 0;
	card->_169F8 = 0;
	card->mBusy  = FALSE;
	card->mFound = FALSE;
	card->mLog   = FALSE;
}

/**
 * @note Address: 0x8008A314
 * @note Size: 0x4C
 */
// __sinit_memcard_cpp, builds gMemoryCardThread and registers its dtor
