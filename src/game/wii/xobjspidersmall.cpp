#include "game/wii/cgame.h"
#include "game/wii/xobj.h"
#include "game/wii/infoscrn.h"
#include "nw4r/snd/snd_BasicSound.h"
#include "nw4r/snd/snd_SoundHandle.h"

// the Wii file of the GameCube's xobjspidersmall.cpp (retail 0x80128C84-0x80132330), its
// functions in the Wii's order; the web effect helpers it calls are defined in xobjspider.cpp

struct SmallSpiderStageStatus {
	CStatus mSpiders[16];
};
STATIC_ASSERT(sizeof(SmallSpiderStageStatus) == 0x80);

extern "C" {
void ResetSmallSpiderState(XObjSpiderSmall* spider);
void ResetSmallSpiderWebEffect(XObjSpiderSmallFx* fx);
void ResetSpiderCoinBounces(void* unused);
f32 GetSoundVolumeAtPosition(XSound* sound, const Vec* pos, s32 which);
void DrawSmallSpiderWebEffect(XObjSpiderSmallFx* fx, u8 index);

extern s32 gRefreshRate;
extern u32 gSpiderSmallSoundId;
extern AnimSfxEvent gSmallSpiderAnimSfxEvents[];
extern XObj* gSmallSpiderEffectObjects[12];
extern s32 gSmallSpiderEffectSlotOwners[12];
extern SmallSpiderStageStatus gSmallSpiderStatusByStage[39];
extern XObj* gSmallSpiderWebFound;
extern u32 gSpiderCoinBounceManager; // the coins' manager token, the callbacks never touch it
extern s32 gSpiderDrainFlashTimers[2];
extern u8 gSpiderDrainFlashClaimed;
extern s32 gAttachedSmallSpiderCount;
extern u8 gSmallSpiderWebFlag;
}

/**
 * @note Address: 0x80128C84
 * @note Size: 0x174
 */
void XObjSpiderSmall::InitializeObject()
{
	XObj::InitializeObject();
	mStage = -1;
	ResetSmallSpiderState(this);
	SetState(0);
	mSpiderStatus.mFlags |= 0x4000000;
	gSpiderSmallSoundId = 0;
	mPrevBgm            = -1;
	mLoadPoolIndex      = -1;
	if (mPoolIndex == 0) {
		for (s32 i = 0; i < 0x27; i++) {
			for (int j = 0; j < 16; j++) {
				gSmallSpiderStatusByStage[i].mSpiders[j].mFlags = 0;
			}
		}
	}
	mTimeMode = gGame.mTimeState.mMode;
}

/**
 * @note Address: 0x80128DF8
 * @note Size: 0x16C
 */
void XObjSpiderSmall::ResetForNewGame()
{
	mStage = -1;
	ResetSmallSpiderState(this);
	SetState(0);
	mSpiderStatus.mFlags |= 0x4000000;
	gSpiderSmallSoundId = 0;
	mPrevBgm            = -1;
	mLoadPoolIndex      = -1;
	if (mPoolIndex == 0) {
		for (s32 i = 0; i < 0x27; i++) {
			for (int j = 0; j < 16; j++) {
				gSmallSpiderStatusByStage[i].mSpiders[j].mFlags = 0;
			}
		}
	}
	mTimeMode = gGame.mTimeState.mMode;
}

/**
 * @note Address: 0x80128F64
 * @note Size: 0x258
 */
extern "C" void ResetSmallSpiderState(XObjSpiderSmall* spider)
{
	spider->mWalkHeading         = 0.0f;
	spider->mWalkSpeed           = 0.0f;
	spider->_9B0                 = 0.0f;
	spider->mTurnSpeed           = 0.1f;
	spider->mFallSpeed           = 0.0f;
	spider->mScale               = 0.0f;
	spider->mTargetScale         = 0.0f;
	spider->mState               = 0;
	spider->mPrevState           = 0;
	spider->mStep                = 0;
	spider->mTimer               = 0;
	spider->mSpiderStatus.mFlags = 0;
	spider->_9E8                 = 0;
	spider->_9EC                 = 1.0f;
	spider->_9F0                 = 1.0f;
	Vec v0                       = { 0.0f, 0.0f, 0.0f };
	spider->mHitPos              = v0;
	Vec v1                       = { 0.0f, 0.0f, 0.0f };
	spider->mHome                = v1;
	Vec v2                       = { 0.0f, 0.0f, 0.0f };
	spider->_A18                 = v2;
	Vec v3                       = { 0.0f, 0.0f, 0.0f };
	spider->mWalkTarget          = v3;
	spider->mHitStrength         = 0.0f;
	spider->mMaxHits             = 3;
	spider->mHitsLeft            = spider->mMaxHits;
	spider->mEvtArgs[0]          = 0;
	spider->mEvtArgs[1]          = 0;
	spider->_A70                 = 0;
	spider->_9BC                 = 1.0f;
	spider->mStatus.mFlags |= 8;
	spider->mStatus.mFlags |= 0x10000000;
	ResetSmallSpiderWebEffect(&spider->mFx);
	spider->mRadius = 16.0f;
	if (spider->mPoolIndex == 0) {
		ResetSpiderCoinBounces(&gSpiderCoinBounceManager);
		gSpiderDrainFlashTimers[0] = 0;
		gSpiderDrainFlashTimers[1] = 0;
		gSpiderDrainFlashClaimed   = 0;
		gAttachedSmallSpiderCount  = 0;
	}
	spider->mFxObj         = NULL;
	spider->mFxIndex       = 0xFF;
	spider->mClingAngle    = 0.0f;
	spider->mShakeStrength = 0.0f;
	spider->mDrainTimer    = 0;
	spider->mHoldTime      = 0.0f;
	Vec v4                 = { 0.0f, 0.0f, 0.0f };
	spider->mClingOffset   = v4;
	Vec v5                 = { 0.0f, 0.0f, 0.0f };
	spider->mAreaPos       = v5;
	spider->mAreaWidth     = 0.0f;
	spider->mAreaDepth     = 0.0f;
	Vec v6                 = { 0.0f, 0.0f, 0.0f };
	spider->mTargetPos     = v6;
	spider->mTargetRadius  = 0.0f;
	spider->SetColor(1, 0, 0, 0, 0.0f);
	spider->mAreaGraceTimer = 0;
	spider->mWarpsLeft      = 0;
	spider->mBattleBgmState = 0;
	spider->_AD0            = 0;
}

/**
 * @note Address: 0x80129C00
 * @note Size: 0x78
 */
void XObjSpiderSmall::ResetObjectState()
{
	XObj::ResetObjectState();
	mAnimSfx.Init((WiiPlayerState*)this, gSmallSpiderAnimSfxEvents);
	mPrevBgm = gGame.mSound.mMusicFadeOutFrames <= 0 ? gGame.mSound.mCurrentMusicId : 0;
	if (mPoolIndex == 0) {
		FindWebs();
	}
}

/**
 * @note Address: 0x80129C78
 * @note Size: 0xE4
 */
// NONMATCHING: regalloc, retail keeps gGame in r29 and the fade frames in r31
// lets go of the battle track it started
void XObjSpiderSmall::Release()
{
	if (gSpiderSmallSoundId == 0xFF000C) {
		s32 frames = 0.2f * gRefreshRate;
		for (int i = 0; i < 0x40; i++) {
			if (gGame.mSound.mArchivePlayer.mTracks.mHandles[i].IsAttachedSound()
			    && gGame.mSound.mArchivePlayer.mTracks.mHandles[i].GetId() == 0xC) {
				gGame.mSound.mArchivePlayer.mTracks.mHandles[i].Stop(frames);
			}
		}
		gSpiderSmallSoundId = 0;
	}
	XObj::Release();
}

/**
 * @note Address: 0x8012AA50
 * @note Size: 0x14
 */
void XObjSpiderSmall::DrawObject()
{
	DrawWithView(NULL);
}

/**
 * @note Address: 0x8012AA64
 * @note Size: 0xC
 */
void XObjSpiderSmall::DrawLate()
{
	DrawSmallSpiderWebEffect(&mFx, mFxIndex);
}

/**
 * @note Address: 0x8012AA70
 * @note Size: 0x4
 */
void XObjSpiderSmall::DrawGroundShadow(f32 opacity)
{
}

/**
 * @note Address: 0x8012ABEC
 * @note Size: 0x4
 */
void XObjSpiderSmall::DrawScreenOverlay()
{
}

/**
 * @note Address: 0x8012ABF0
 * @note Size: 0x4
 */
void XObjSpiderSmall::DrawDebugStatus()
{
}

/**
 * @note Address: 0x8012ABF4
 * @note Size: 0x4
 */
void XObjSpiderSmall::DrawObjectEffects()
{
}

/**
 * @note Address: 0x8012ABF8
 * @note Size: 0xAC
 */
s32 XObjSpiderSmall::HandleEventCommand(s32 n, s32* args)
{
	if (n < 1) {
		return 0;
	}
	mEvtArgs[0] = args[0];
	if (n < 2) {
		mEvtArgs[1] = 0;
	} else {
		mEvtArgs[1] = args[1];
	}
	if (n < 3) {
		mEvtArgs[2] = 0;
	} else {
		mEvtArgs[2] = args[2];
	}
	if (n < 4) {
		mEvtArgs[3] = 0;
	} else {
		mEvtArgs[3] = args[3];
	}
	if (n < 5) {
		mEvtArgs[4] = 0;
	} else {
		mEvtArgs[4] = args[4];
	}
	if (n < 6) {
		mEvtArgs[5] = 0;
	} else {
		mEvtArgs[5] = args[5];
	}
	return 0;
}

/**
 * @note Address: 0x8012ACA4
 * @note Size: 0x9C
 */
// keeps the hit for UpdatePlayerCaughtInSpiderWeb, unless one is pending or it can't be hit (0x10, or 0x8000 in its web)
void XObjSpiderSmall::OnHitFromPosition(const Vec& pos, f32 strength)
{
	if (!(mSpiderStatus.mFlags & 2) && !(mSpiderStatus.mFlags & 0x10) && !(mSpiderStatus.mFlags & 0x8000)) {
		XObj::OnHitFromPosition(pos, strength);
		mSpiderStatus.mFlags |= 2;
		Vec hitPos   = { pos.x, pos.y, pos.z };
		mHitPos      = hitPos;
		mHitStrength = strength;
	}
}

/**
 * @note Address: 0x8012AD40
 * @note Size: 0x18
 */
s32 XObjSpiderSmall::OnPlayerContact(s32 unused, s32 value)
{
	mSpiderStatus.mFlags |= 1;
	return value;
}

/**
 * @note Address: 0x80131DF4
 * @note Size: 0x13C
 */
// finds the stage's webs, the objects of kind 0x1BC
void XObjSpiderSmall::FindWebs()
{
	for (int i = 0; i < 12; i++) {
		gSmallSpiderEffectObjects[i]    = NULL;
		gSmallSpiderEffectSlotOwners[i] = -1;
	}
	gSmallSpiderWebFlag  = 0;
	gSmallSpiderWebFound = NULL;
	s32 found            = 0;
	for (s32 i = 0; gSmallSpiderWebFound == NULL && i < gGame.mXObjs.mObjectCount; i++) {
		XObj* obj = gGame.mXObjs.mObjects[i];
		if ((obj->mStatus.mFlags & 1) && obj->mCoord.mPlacement->mObjectId == 0x1BC) {
			gSmallSpiderEffectObjects[found] = obj;
			gSmallSpiderEffectObjects[found]->SetAnim(0, TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
			found++;
			if (found >= 12) {
				break;
			}
		}
	}
}

/**
 * @note Address: 0x80131F30
 * @note Size: 0x64
 */
// NONMATCHING: retail sets a second register to 0 before the loop and to 1 where it takes a web, never
// reading it; a found flag the compiler did not drop. Here it is dropped
// takes the first of the webs nobody has
void XObjSpiderSmall::TakeWeb()
{
	for (int i = 0; i < 12; i++) {
		if (gSmallSpiderEffectSlotOwners[i] == -1) {
			mFxObj                          = gSmallSpiderEffectObjects[i];
			mFxIndex                        = i;
			gSmallSpiderEffectSlotOwners[i] = mPoolIndex;
			break;
		}
	}
}

/**
 * @note Address: 0x801320A4
 * @note Size: 0xE4
 */
void XObjSpiderSmall::UpdateAnimationSounds()
{
	if ((mStatus.mFlags & 1) && (mStatus.mFlags & 0x10000) && !InfoScrn_IsBatteryGaugeEmpty(&gGame.mInfoScrn)) {
		f32 volume = GetSoundVolumeAtPosition(&gGame.mSound, GetPos(), 0);
		mAnimSfx.Update(GetAnimId(), GetAnimFrame(), volume);
	}
}
