#include "game/wii/cgame.h"
#include "game/wii/xobj.h"
#include "game/wii/infoscrn.h"
#include "game/wii/cpadact.h"
#include "game/wii/xact.h"
#include "game/gmath.h"
#include "arithmetic.h"
#include <math.h>
#include <stdlib.h>
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
BOOL IsUsingMug(XAction* lift);
f32 GetPlayerCollisionRadius(WiiPlayerState* player);
bool IsPlayerInSmallSpiderArea(XObjSpiderSmall* spider);
void StartSmallSpiderWebEffect(XObjSpiderSmallFx* fx, const CVec& pos);
bool IsSmallSpiderEffectAnimationFinished(XObjSpiderSmallFx* fx, u8 slot, u32 frames);
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
extern XObjFloor* sSmallSpiderFloors[2];
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
 * @note Address: 0x8012BB9C
 * @note Size: 0x38
 */
void XObjSpiderSmall::State2(u32 frames)
{
	mStatus.mFlags &= ~0x10000ULL;
	mSpiderStatus.mFlags |= 0x80000;
	mSpiderStatus.mFlags &= ~0x100000;
}

static inline f32 SmallSpiderDistanceXZ(const Vec* a, const Vec* b)
{
	f32 dx = a->x - b->x;
	f32 dy = 0.0f;
	f32 dz = a->z - b->z;
	return FSqrt(dx * dx + dy * dy + dz * dz);
}

static inline f32 GetSmallSpiderDistanceXZ(Vec* a, Vec* b)
{
	return SmallSpiderDistanceXZ(a, b);
}

static inline f32 GetSmallSpiderContactDistance(Vec* a, Vec* b)
{
	f32 dx = a->x - b->x;
	f32 dy = a->y - b->y;
	f32 dz = a->z - b->z;
	return FSqrt(dx * dx + dy * dy + dz * dz);
}

static inline bool IsSmallSpiderWithinTargetRadius(XObjSpiderSmall* spider)
{
	f32 dx = spider->mTargetPos.x - spider->mMoveTranslate.x;
	f32 dy = spider->mTargetPos.y - spider->mMoveTranslate.y;
	f32 dz = spider->mTargetPos.z - spider->mMoveTranslate.z;
	return FSqrt(dx * dx + dy * dy + dz * dz) <= spider->mTargetRadius + spider->mRadius;
}

static inline s32 GetSmallSpiderGripDuration()
{
	switch (gGame.GetPlayer()->mSuits.mCurrent) {
	case 1:
	case 6:
		return rand() % 10 + 10;
	case 2:
	case 3:
		return rand() % 2 + 2;
	case 4:
	case 8:
		return rand() % 20 + 20;
	default:
		return rand() % 10 + 10;
	}
}

static inline void Catch(XObjSpiderSmall* spider)
{
	spider->mStatus.mFlags |= 0x4000000000000ULL;
	spider->mWalkSpeed       = 0.0f;
	spider->mFallSpeed       = 0.0f;
	spider->mMoveTranslate.y = 10.0f + spider->mClingOffset.y;
	spider->mShakeStrength   = 1.0f;
	spider->mHoldTime        = 40.0f;
	spider->mStep            = 4;
	spider->mSpiderStatus.mFlags |= 0x800000;
	gAttachedSmallSpiderCount++;
	gGame.GetPlayer()->mMoveSpeedScale = 0.4f;
	spider->SetAnim(5, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
}

static inline void DetachSmallSpiderFromPlayer(XObjSpiderSmall* spider)
{
	spider->mSpiderStatus.mFlags &= ~0x800000;
	if (spider->mSpiderStatus.Check(0x1000000)) {
		spider->mSpiderStatus.mFlags &= ~0x1000000;
		gSpiderDrainFlashClaimed = 0;
		gGame.GetPlayer()->SetColor(1, 0, 0, 0, 0.0f);
		InfoScrn_ApplyScoreHudCommand(&gGame.mInfoScrn, 0, 1);
	}
	if (--gAttachedSmallSpiderCount < 0) {
		gAttachedSmallSpiderCount = 0;
	}
	if (gAttachedSmallSpiderCount == 0) {
		gGame.GetPlayer()->mMoveSpeedScale = 1.0f;
	}
}

/**
 * @note Address: 0x8012BBD4
 * @note Size: 0x2734
 */
// NONMATCHING: all 79 out-of-line calls and the CFG match; inlined distance/wander code still has different FPR and load schedules
void XObjSpiderSmall::State3(u32 frames)
{
	CPad pad(0);
	Vec ppos = *gGame.GetPlayer()->GetPos();
	Vec prot = gGame.GetPlayer()->mMoveRotate;
	if (!IsPlayerInSmallSpiderArea(this)) {
		if (--mAreaGraceTimer < 0) {
			mAreaGraceTimer = 0;
		}
	} else {
		mAreaGraceTimer = (3 * gRefreshRate) / frames;
	}
	switch (mStep) {
	case 2:
	case 10:
	case 11:
	case 22:
	case 23:
		mFloors        = sSmallSpiderFloors[1];
		mFloorMasks[1] = 0;
		mFloorMasks[0] = 0;
		break;
	default:
		mFloors        = sSmallSpiderFloors[0];
		mFloorMasks[1] = 0;
		mFloorMasks[0] = 0;
		break;
	}
	switch (mStep) {
	case 0:
		mTurnSpeed = 0.0f;
		mTimer--;
		if (mTimer <= 0) {
			Wander(20);
			mTimer = rand() % 30 + 10;
		} else if (GetSmallSpiderContactDistance(&ppos, &mMoveTranslate) < 64.0f && IsSmallSpiderWithinTargetRadius(this)) {
			mWalkTarget.x = ppos.x;
			mWalkTarget.y = ppos.y;
			mWalkTarget.z = ppos.z;
			SetAnim(2, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			mStep++;
		}
		break;
	case 20:
		mWalkHeading = atan2(mWalkTarget.x - mMoveTranslate.x, mWalkTarget.z - mMoveTranslate.z);
		if (GetSmallSpiderDistanceXZ(&mWalkTarget, &mMoveTranslate) < 2.0f) {
			mStep      = 0;
			mWalkSpeed = 0.0f;
			mTimer     = rand() % 30 + 30;
			SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
		} else {
			if (--mTimer < 0) {
				mWalkSpeed = 0.0f;
				SetAnim(3, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
				mStep = 21;
			}
			if (!InArea(mMoveTranslate)) {
				mWalkTarget.x = mHome.x;
				mWalkTarget.y = mHome.y;
				mWalkTarget.z = mHome.z;
			}
			if (mAreaGraceTimer <= 0) {
				if ((mSpiderStatus.mFlags & 0x20) && gGame.mTimeState.mDuration != 0.0f) {
					mWarpsLeft = rand() % 4 + 3;
					SetState(8);
				} else {
					SetState(7);
				}
			}
		}
		break;
	case 21: {
		f32 end = GetAnimEndFrame();
		if (end - frames <= GetAnimFrame()) {
			SetAnim(12, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			mSpiderStatus.mFlags |= 8;
			mWalkTarget.x = mMoveTranslate.x;
			mWalkTarget.y = mMoveTranslate.y;
			mWalkTarget.z = mMoveTranslate.z;
			mWalkSpeed    = 0.0f;
			ClampSpeed();
			mStep++;
			mFallSpeed   = 4.0f;
			mWalkHeading = atan2(mWalkTarget.x - mMoveTranslate.x, mWalkTarget.z - mMoveTranslate.z);
			mSpiderStatus.mFlags &= ~1;
		}
		break;
	}
	case 22:
		if (GetSmallSpiderDistanceXZ(&mWalkTarget, &mMoveTranslate) < 2.0f) {
			mStep++;
		} else if (mFallSpeed >= -2.3f) {
			mFallSpeed -= 0.36f * frames;
		} else {
			mFallSpeed = -2.3f;
		}
		break;
	case 23:
		if (mFallSpeed >= -2.3f) {
			mFallSpeed -= 0.36f * frames;
		} else {
			mFallSpeed = -2.3f;
		}
		if (mMoveTranslate.y <= mHome.y + frames) {
			mFallSpeed       = 0.0f;
			mMoveTranslate.y = mHome.y;
			SetAnim(7, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			mWalkSpeed = 0.0f;
			mSpiderStatus.mFlags &= ~8;
			mStatus.mFlags &= ~0x4000000000000ULL;
			mStep++;
		}
		break;
	case 24: {
		f32 end = GetAnimEndFrame();
		if (end - frames <= GetAnimFrame()) {
			SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			mTimer = rand() % 30 + 30;
			mStep  = 0;
		}
		break;
	}
	case 30:
		mFallSpeed -= 0.08f;
		if (mMoveTranslate.y <= mHome.y) {
			mFallSpeed       = 0.0f;
			mMoveTranslate.y = mHome.y;
			SetAnim(7, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			mStep = 31;
		}
		break;
	case 31: {
		f32 end = GetAnimEndFrame();
		if (end - frames <= GetAnimFrame()) {
			Wander(32);
		}
		break;
	}
	case 32:
		mWalkHeading = atan2(mWalkTarget.x - mMoveTranslate.x, mWalkTarget.z - mMoveTranslate.z);
		if (GetSmallSpiderDistanceXZ(&mWalkTarget, &mMoveTranslate) < 2.0f) {
			mStep      = 33;
			mWalkSpeed = 0.0f;
			mTimer     = rand() % 30 + 30;
			SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
		} else if (gGame.mEvt.mData.mScript == NULL) {
			mStep = 0;
		}
		break;
	case 33:
		if (gGame.mEvt.mData.mScript == NULL) {
			mStep = 0;
		}
		break;
	case 1:
		mWalkSpeed    = 0.3f;
		mWalkTarget.x = ppos.x;
		mWalkTarget.y = ppos.y;
		mWalkTarget.z = ppos.z;
		mWalkHeading  = atan2(mWalkTarget.x - mMoveTranslate.x, mWalkTarget.z - mMoveTranslate.z);
		if (GetSmallSpiderContactDistance(&mWalkTarget, &mMoveTranslate) < 20.0f) {
			mWalkSpeed = 0.0f;
			SetAnim(3, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			mTimer = GetSmallSpiderGripDuration();
			mStep++;
		}
		if (!IsSmallSpiderWithinTargetRadius(this)) {
			Wander(20);
		}
		if (!InArea(mMoveTranslate)) {
			Wander(20);
		}
		break;
	case 2: {
		f32 end = GetAnimEndFrame();
		if (end - frames <= GetAnimFrame()) {
			if (--mTimer < 0) {
				mWalkTarget.x = ppos.x;
				mWalkTarget.y = ppos.y;
				mWalkTarget.z = ppos.z;
				if (GetSmallSpiderContactDistance(&mWalkTarget, &mMoveTranslate) >= 20.0f) {
					SetAnim(12, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
					mSpiderStatus.mFlags |= 8;
					mWalkTarget.x = mMoveTranslate.x;
					mWalkTarget.y = mMoveTranslate.y;
					mWalkTarget.z = mMoveTranslate.z;
					mWalkSpeed    = 0.0f;
					ClampSpeed();
					mFallSpeed   = 4.0f;
					mWalkHeading = atan2(mWalkTarget.x - mMoveTranslate.x, mWalkTarget.z - mMoveTranslate.z);
					mSpiderStatus.mFlags &= ~1;
					mStep = 22;
				} else {
					f32 size = GetPlayerCollisionRadius(gGame.GetPlayer());
					SetAnim(4, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
					mSpiderStatus.mFlags |= 8;
					mClingAngle    = DEG2RAD * (rand() % 360);
					mClingOffset.x = size * (f32)sin(mClingAngle);
					mClingOffset.y = -rand() % 800 / 100;
					mClingOffset.z = size * (f32)cos(mClingAngle);
					mWalkTarget.x  = ppos.x + size * (f32)sin(mClingAngle);
					mWalkTarget.y  = ppos.y + -rand() % 800 / 100;
					mWalkTarget.z  = ppos.z + size * (f32)cos(mClingAngle);
					mWalkSpeed     = GetSmallSpiderDistanceXZ(&mWalkTarget, &mMoveTranslate) / 20.0f;
					ClampSpeed();
					mFallSpeed   = 4.0f;
					mWalkHeading = atan2(mWalkTarget.x - mMoveTranslate.x, mWalkTarget.z - mMoveTranslate.z);
					mSpiderStatus.mFlags &= ~1;
					mStatus.mFlags |= 0x4000000000000ULL;
					mStep = 10;
				}
			}
		}
		break;
	}
	case 4: {
		_AD0             = 0;
		f32 size         = GetPlayerCollisionRadius(gGame.GetPlayer());
		mMoveTranslate.x = ppos.x + size * (f32)sin(prot.y + mClingAngle);
		mMoveTranslate.y = 10.0f + ppos.y + mClingOffset.y;
		mMoveTranslate.z = ppos.z + size * (f32)cos(prot.y + mClingAngle);
		mWalkHeading     = atan2(ppos.x - mMoveTranslate.x, ppos.z - mMoveTranslate.z);
		if (gGame.IsRunning() && gGame.GetPlayer()->_1C9B8[2] >= 1.0f) {
			mHoldTime -= mShakeStrength;
			f32 strength = 0.0f;
			f32 remote   = nw4r::math::FAbs(gWiiPointer.mRemoteAcceleration[0].y - gWiiPointer.mRemoteAcceleration[1].y);
			if (remote > 1.0f) {
				strength += 1.5f * remote;
			}
			f32 nunchuk = nw4r::math::FAbs(gWiiPointer.mNunchukAcceleration[0].y - gWiiPointer.mNunchukAcceleration[1].y);
			if (nunchuk > 1.0f) {
				strength += 1.5f * nunchuk;
			}
			if (strength > 0.0f) {
				mShakeStrength = strength;
			}
		}
		if (mShakeStrength > 1.0f) {
			mShakeStrength -= 1.0f;
		}
		if (gGame.mEvt.mData.mScript != NULL) {
			mWalkTarget.x = ppos.x + size * (f32)sin(prot.y + mClingAngle);
			mWalkTarget.y = ppos.y;
			mWalkTarget.z = ppos.z + size * (f32)cos(prot.y + mClingAngle);
			DetachSmallSpiderFromPlayer(this);
			SetAnim(6, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			mWalkSpeed = 0.0f;
			mStatus.mFlags |= 0x4000000000000ULL;
			mStep = 30;
		} else if (nw4r::math::FAbs(ppos.y - mHome.y) > 10.0f) {
			DetachSmallSpiderFromPlayer(this);
			mSpiderStatus.mFlags |= 0x2000;
			SetState(7);
		} else if (mHoldTime <= 0.0f || IsUsingMug(&gGame.GetPlayer()->mLift)) {
			f32 dist      = 20.0f + size + rand() % 3000 / 100.0f;
			f32 ang       = PI + (f32)atan2(ppos.x - mMoveTranslate.x, ppos.z - mMoveTranslate.z);
			mWalkTarget.x = ppos.x + dist * (f32)sin(ang);
			mWalkTarget.y = ppos.y;
			mWalkTarget.z = ppos.z + dist * (f32)cos(ang);
			DetachSmallSpiderFromPlayer(this);
			SetAnim(6, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			mWalkSpeed = 1.0f;
			mStatus.mFlags |= 0x4000000000000ULL;
			if (size >= 2.0f) {
				_AD0 = 1;
			}
			mStep++;
		}
		break;
	}
	case 5: {
		s8 wait = 2;
		mFallSpeed -= 0.08f * frames;
		if (mMoveTranslate.y <= mHome.y + frames) {
			mFallSpeed       = 0.0f;
			wait             = 1;
			mMoveTranslate.y = mHome.y;
		}
		if (!InArea(mMoveTranslate)) {
			mWalkTarget.x = mMoveTranslate.x;
			mWalkTarget.z = mMoveTranslate.z;
			mWalkSpeed    = 0.0f;
		}
		mWalkHeading = atan2(mWalkTarget.x - mMoveTranslate.x, mWalkTarget.z - mMoveTranslate.z);
		if (GetSmallSpiderDistanceXZ(&mWalkTarget, &mMoveTranslate) < 0.5f * frames) {
			mWalkSpeed = 0.0f;
			wait--;
		}
		if (wait == 0) {
			SetAnim(7, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			mStep++;
		}
		break;
	}
	case 6: {
		mTimer++;
		if (mTimer % 30 < 15) {
			SetColor(1, 0, 0x80, 0, 0.0f);
			SetColor(1, 0x80, 0, 0, 0.0f);
		} else {
			SetColor(1, 0, 0, 0, 0.0f);
		}
		f32 end = GetAnimEndFrame();
		if (end - frames <= GetAnimFrame()) {
			SetColor(1, 0, 0, 0, 0.0f);
			mTimer = 0;
			if (--mHitsLeft <= 0) {
				mHitsLeft = 0;
			}
			mSpiderStatus.mFlags &= ~0x8000;
			if (mHitsLeft <= 0) {
				SetState(1);
			} else {
				mStep = 0;
				mSpiderStatus.mFlags &= ~8;
				SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
				mTimer = rand() % 30 + 30;
			}
		}
		break;
	}
	case 10: {
		f32 dist = GetSmallSpiderContactDistance(&ppos, &mMoveTranslate);
		if (dist < 12.0f + GetPlayerCollisionRadius(gGame.GetPlayer()) - 1.5f) {
			mStatus.mFlags |= 0x4000000000000ULL;
		}
		if (dist < 8.0f + GetPlayerCollisionRadius(gGame.GetPlayer()) - 1.5f) {
			Catch(this);
		} else {
			if (!InArea(mMoveTranslate)) {
				mWalkTarget.x = mMoveTranslate.x;
				mWalkTarget.z = mMoveTranslate.z;
				mWalkSpeed    = 0.0f;
			}
			if (GetSmallSpiderDistanceXZ(&mWalkTarget, &mMoveTranslate) < 2.0f) {
				SetAnim(6, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
				mStep++;
			} else if (mFallSpeed >= -2.3f) {
				mFallSpeed -= 0.36f * frames;
			} else {
				mFallSpeed = -2.3f;
			}
		}
		break;
	}
	case 11: {
		f32 dist = GetSmallSpiderContactDistance(&ppos, &mMoveTranslate);
		if (dist < 12.0f + GetPlayerCollisionRadius(gGame.GetPlayer()) - 1.5f) {
			mStatus.mFlags |= 0x4000000000000ULL;
		}
		if (dist < 8.0f + GetPlayerCollisionRadius(gGame.GetPlayer()) - 1.5f) {
			Catch(this);
		} else {
			if (mFallSpeed >= -2.3f) {
				mFallSpeed -= 0.36f * frames;
			} else {
				mFallSpeed = -2.3f;
			}
			if (!InArea(mMoveTranslate)) {
				mWalkSpeed = 0.0f;
			}
			if (mMoveTranslate.y <= mHome.y + frames) {
				mFallSpeed       = 0.0f;
				mMoveTranslate.y = mHome.y;
				SetAnim(7, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
				mWalkSpeed = 0.0f;
				mSpiderStatus.mFlags &= ~8;
				mStatus.mFlags &= ~0x4000000000000ULL;
				mStep++;
			}
		}
		break;
	}
	case 12: {
		f32 end = GetAnimEndFrame();
		if (end - frames <= GetAnimFrame()) {
			SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			mTimer = rand() % 30 + 30;
			mStep  = 0;
		}
		break;
	}
	}
}

/**
 * @note Address: 0x8012F200
 * @note Size: 0x219C
 */
// NONMATCHING: shared chase/grab and warp logic is complete; residual differences are inline FPR allocation and load scheduling
void XObjSpiderSmall::State8(u32 frames)
{
	CPad pad(0);
	WiiPlayerState* player = gGame.GetPlayer();
	Vec ppos               = *player->GetPos();
	Vec prot               = player->mMoveRotate;
	switch (mStep) {
	case 0:
		mTurnSpeed = 0.0f;
		if (GetSmallSpiderContactDistance(&ppos, &mMoveTranslate) < 48.0f) {
			mWalkTarget.x = ppos.x;
			mWalkTarget.y = ppos.y;
			mWalkTarget.z = ppos.z;
			SetAnim(2, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			mStep++;
		} else {
			mStep = 100;
		}
		break;
	case 1: {
		mWalkSpeed    = 0.3f;
		mWalkTarget.x = ppos.x;
		mWalkTarget.y = ppos.y;
		mWalkTarget.z = ppos.z;
		mWalkHeading  = atan2(mWalkTarget.x - mMoveTranslate.x, mWalkTarget.z - mMoveTranslate.z);
		f32 dist      = GetSmallSpiderContactDistance(&mWalkTarget, &mMoveTranslate);
		if (dist < 20.0f) {
			mWalkSpeed = 0.0f;
			SetAnim(3, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			mTimer = GetSmallSpiderGripDuration();
			mStep++;
		} else if (dist >= 64.0f) {
			mStep = 100;
		} else if (12.0f + mHome.y < ppos.y || mHome.y - 12.0f > ppos.y) {
			mStep = 100;
		}
		break;
	}
	case 2: {
		f32 end = GetAnimEndFrame();
		if (end - frames <= GetAnimFrame()) {
			if (--mTimer < 0) {
				mWalkTarget.x = ppos.x;
				mWalkTarget.y = ppos.y;
				mWalkTarget.z = ppos.z;
				if (GetSmallSpiderContactDistance(&mWalkTarget, &mMoveTranslate) >= 20.0f) {
					SetAnim(12, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
					mSpiderStatus.mFlags |= 8;
					mWalkTarget.x = mMoveTranslate.x;
					mWalkTarget.y = mMoveTranslate.y;
					mWalkTarget.z = mMoveTranslate.z;
					mWalkSpeed    = 0.0f;
					ClampSpeed();
					mFallSpeed   = 4.0f;
					mWalkHeading = atan2(mWalkTarget.x - mMoveTranslate.x, mWalkTarget.z - mMoveTranslate.z);
					mSpiderStatus.mFlags &= ~1;
					mStep = 22;
				} else {
					f32 size = GetPlayerCollisionRadius(gGame.GetPlayer());
					SetAnim(4, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
					mSpiderStatus.mFlags |= 8;
					mClingAngle    = DEG2RAD * (rand() % 360);
					mClingOffset.x = size * (f32)sin(mClingAngle);
					mClingOffset.y = -rand() % 800 / 100;
					mClingOffset.z = size * (f32)cos(mClingAngle);
					mWalkTarget.x  = ppos.x + size * (f32)sin(mClingAngle);
					mWalkTarget.y  = ppos.y + -rand() % 800 / 100;
					mWalkTarget.z  = ppos.z + size * (f32)cos(mClingAngle);
					mWalkSpeed     = GetSmallSpiderDistanceXZ(&mWalkTarget, &mMoveTranslate) / 20.0f;
					ClampSpeed();
					mFallSpeed   = 4.0f;
					mWalkHeading = atan2(mWalkTarget.x - mMoveTranslate.x, mWalkTarget.z - mMoveTranslate.z);
					mSpiderStatus.mFlags &= ~1;
					mStatus.mFlags |= 0x4000000000000ULL;
					mStep = 10;
				}
			}
		}
		break;
	}
	case 4: {
		_AD0             = 0;
		f32 size         = GetPlayerCollisionRadius(gGame.GetPlayer());
		mMoveTranslate.x = ppos.x + size * (f32)sin(prot.y + mClingAngle);
		mMoveTranslate.y = 10.0f + ppos.y + mClingOffset.y;
		mMoveTranslate.z = ppos.z + size * (f32)cos(prot.y + mClingAngle);
		mWalkHeading     = atan2(ppos.x - mMoveTranslate.x, ppos.z - mMoveTranslate.z);
		if (gGame.IsRunning() && gGame.GetPlayer()->_1C9B8[2] >= 1.0f) {
			mHoldTime -= mShakeStrength;
			f32 strength = 0.0f;
			f32 remote   = nw4r::math::FAbs(gWiiPointer.mRemoteAcceleration[0].y - gWiiPointer.mRemoteAcceleration[1].y);
			if (remote > 1.0f) {
				strength += 1.5f * remote;
			}
			f32 nunchuk = nw4r::math::FAbs(gWiiPointer.mNunchukAcceleration[0].y - gWiiPointer.mNunchukAcceleration[1].y);
			if (nunchuk > 1.0f) {
				strength += 1.5f * nunchuk;
			}
			if (strength > 0.0f) {
				mShakeStrength = strength;
			}
		}
		if (mShakeStrength > 1.0f) {
			mShakeStrength -= 1.0f;
		}
		if (gGame.mEvt.mData.mScript != NULL) {
			mWalkTarget.x = ppos.x + size * (f32)sin(prot.y + mClingAngle);
			mWalkTarget.y = ppos.y;
			mWalkTarget.z = ppos.z + size * (f32)cos(prot.y + mClingAngle);
			DetachSmallSpiderFromPlayer(this);
			SetAnim(6, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			mWalkSpeed = 0.0f;
			mStatus.mFlags |= 0x4000000000000ULL;
			mStep = 30;
		} else if (nw4r::math::FAbs(ppos.y - mHome.y) > 10.0f) {
			DetachSmallSpiderFromPlayer(this);
			mSpiderStatus.mFlags |= 0x2000;
			SetState(7);
		} else if (mHoldTime <= 0.0f || IsUsingMug(&gGame.GetPlayer()->mLift)) {
			f32 ang;
			f32 dist      = 20.0f + size + rand() % 3000 / 100.0f;
			ang           = PI + (f32)atan2(ppos.x - mMoveTranslate.x, ppos.z - mMoveTranslate.z);
			mWalkTarget.x = ppos.x + dist * (f32)sin(ang);
			mWalkTarget.y = ppos.y;
			mWalkTarget.z = ppos.z + dist * (f32)cos(ang);
			DetachSmallSpiderFromPlayer(this);
			SetAnim(6, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			mWalkSpeed = 1.0f;
			mStatus.mFlags |= 0x4000000000000ULL;
			if (size >= 2.0f) {
				_AD0 = 1;
			}
			mStep++;
		}
		break;
	}
	case 5: {
		s8 wait = 2;
		mFallSpeed -= 0.08f * frames;
		if (mMoveTranslate.y <= mHome.y + frames) {
			mFallSpeed       = 0.0f;
			wait             = 1;
			mMoveTranslate.y = mHome.y;
		}
		mWalkHeading = atan2(mWalkTarget.x - mMoveTranslate.x, mWalkTarget.z - mMoveTranslate.z);
		if (GetSmallSpiderDistanceXZ(&mWalkTarget, &mMoveTranslate) < 0.5f * frames) {
			mWalkSpeed = 0.0f;
			wait--;
		}
		if (wait == 0) {
			SetAnim(7, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			mStep++;
		}
		break;
	}
	case 6: {
		mTimer++;
		if (mTimer % 30 < 15) {
			SetColor(1, 0, 0x80, 0, 0.0f);
			SetColor(1, 0x80, 0, 0, 0.0f);
		} else {
			SetColor(1, 0, 0, 0, 0.0f);
		}
		f32 end = GetAnimEndFrame();
		if (end - frames <= GetAnimFrame()) {
			SetColor(1, 0, 0, 0, 0.0f);
			mTimer = 0;
			if (--mHitsLeft <= 0) {
				mHitsLeft = 0;
			}
			mSpiderStatus.mFlags &= ~0x8000;
			if (mHitsLeft <= 0) {
				SetState(1);
			} else {
				mStep = 0;
				mSpiderStatus.mFlags &= ~8;
				SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
				mTimer = rand() % 30 + 30;
			}
		}
		break;
	}
	case 10: {
		f32 dist = GetSmallSpiderContactDistance(&ppos, &mMoveTranslate);
		if (dist < 12.0f + GetPlayerCollisionRadius(gGame.GetPlayer()) - 1.5f) {
			mStatus.mFlags |= 0x4000000000000ULL;
		}
		if (dist < 8.0f + GetPlayerCollisionRadius(gGame.GetPlayer()) - 1.5f) {
			Catch(this);
		} else {
			if (GetSmallSpiderDistanceXZ(&mWalkTarget, &mMoveTranslate) < 2.0f) {
				SetAnim(6, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
				mStep++;
			} else if (mFallSpeed >= -2.3f) {
				mFallSpeed -= 0.36f * frames;
			} else {
				mFallSpeed = -2.3f;
			}
		}
		break;
	}
	case 11: {
		f32 dist = GetSmallSpiderContactDistance(&ppos, &mMoveTranslate);
		if (dist < 12.0f + GetPlayerCollisionRadius(gGame.GetPlayer()) - 1.5f) {
			mStatus.mFlags |= 0x4000000000000ULL;
		}
		if (dist < 8.0f + GetPlayerCollisionRadius(gGame.GetPlayer()) - 1.5f) {
			Catch(this);
		} else {
			if (mFallSpeed >= -2.3f) {
				mFallSpeed -= 0.36f * frames;
			} else {
				mFallSpeed = -2.3f;
			}
			if (mMoveTranslate.y <= mHome.y + frames) {
				mFallSpeed       = 0.0f;
				mMoveTranslate.y = mHome.y;
				SetAnim(7, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
				mWalkSpeed = 0.0f;
				mSpiderStatus.mFlags &= ~8;
				mStatus.mFlags &= ~0x4000000000000ULL;
				mStep++;
			}
		}
		break;
	}
	case 12: {
		f32 end = GetAnimEndFrame();
		if (end - frames <= GetAnimFrame()) {
			mStep = 0;
			SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			mTimer = rand() % 30 + 30;
		}
		break;
	}
	case 22:
		if (GetSmallSpiderDistanceXZ(&mWalkTarget, &mMoveTranslate) < 2.0f) {
			mStep++;
		} else if (mFallSpeed >= -2.3f) {
			mFallSpeed -= 0.36f * frames;
		} else {
			mFallSpeed = -2.3f;
		}
		break;
	case 23:
		if (mFallSpeed >= -2.3f) {
			mFallSpeed -= 0.36f * frames;
		} else {
			mFallSpeed = -2.3f;
		}
		if (mMoveTranslate.y <= mHome.y + frames) {
			mFallSpeed       = 0.0f;
			mMoveTranslate.y = mHome.y;
			SetAnim(7, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			mWalkSpeed = 0.0f;
			mSpiderStatus.mFlags &= ~8;
			mStatus.mFlags &= ~0x4000000000000ULL;
			mStep++;
		}
		break;
	case 24: {
		f32 end = GetAnimEndFrame();
		if (end - frames <= GetAnimFrame()) {
			SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			mTimer = rand() % 30 + 30;
			mStep  = 0;
		}
		break;
	}
	case 30:
		mFallSpeed -= 0.08f;
		if (mMoveTranslate.y <= mHome.y) {
			mFallSpeed       = 0.0f;
			mMoveTranslate.y = mHome.y;
			SetAnim(7, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			mStep = 31;
		}
		break;
	case 31: {
		f32 end = GetAnimEndFrame();
		if (end - frames <= GetAnimFrame()) {
			Wander(32);
		}
		break;
	}
	case 32:
		mWalkHeading = atan2(mWalkTarget.x - mMoveTranslate.x, mWalkTarget.z - mMoveTranslate.z);
		if (GetSmallSpiderDistanceXZ(&mWalkTarget, &mMoveTranslate) < 2.0f) {
			mStep      = 33;
			mWalkSpeed = 0.0f;
			mTimer     = rand() % 30 + 30;
			SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
		} else if (gGame.mEvt.mData.mScript == NULL) {
			mStep = 0;
		}
		break;
	case 33:
		if (gGame.mEvt.mData.mScript == NULL) {
			mStep = 0;
		}
		break;
	case 100: {
		mTurnSpeed = 0.0f;
		mWalkSpeed = 0.0f;
		mSpiderStatus.mFlags |= 0x8000;
		SetAnim(1, FALSE, 0.0f, 0.0f, 0.0f, 15.0f);
		mScale = 1.0f;
		SetScale(mScale, mScale, mScale);
		f32 x = mMoveTranslate.x;
		f32 y = mMoveTranslate.y;
		f32 z = mMoveTranslate.z;
		StartSmallSpiderWebEffect(&mFx, CVec(x, y, z));
		mStep++;
		break;
	}
	case 101:
		mScale -= 0.01f * frames;
		if (mScale <= 0.0f) {
			mScale = 0.0f;
			mTimer = 210 / frames;
			mStep++;
		}
		SetScale(mScale, mScale, mScale);
		break;
	case 102:
		if (IsSmallSpiderEffectAnimationFinished(&mFx, mFxIndex, frames)) {
			mTimer = rand() % 30 + 30;
			mStep++;
		}
		break;
	case 103:
		if (--mTimer < 0) {
			mWarpsLeft--;
			if (12.0f + mHome.y < ppos.y || mHome.y - 12.0f > ppos.y || gGame.mEvt.mData.mScript != NULL || mWarpsLeft < 0
			    || gGame.mTimeState.mDuration == 0.0f) {
				mMoveTranslate.x = mHome.x;
				mMoveTranslate.y = mHome.y;
				mMoveTranslate.z = mHome.z;
				mWarpsLeft       = 0;
				mTimer           = 0;
				if (mSpiderStatus.mFlags & 0x40000) {
					State2(frames);
					if (mFxIndex != 0xFF) {
						gSmallSpiderEffectSlotOwners[mFxIndex] = -1;
						mFxObj                                 = NULL;
						mFxIndex                               = 0xFF;
					}
					mStep = 1000;
				} else {
					SetState(5);
					mSpiderStatus.mFlags &= ~0x40;
					gSmallSpiderStatusByStage[gGame.mStage].mSpiders[mLoadPoolIndex].mFlags &= ~0x40;
				}
			} else {
				f32 size         = GetPlayerCollisionRadius(gGame.GetPlayer());
				mClingAngle      = DEG2RAD * (rand() % 360);
				f32 dist         = 10.0f + size;
				mMoveTranslate.x = ppos.x + dist * (f32)sin(prot.y + mClingAngle);
				mMoveTranslate.y = mHome.y;
				mMoveTranslate.z = ppos.z + dist * (f32)cos(prot.y + mClingAngle);
				f32 y, x, z;
				x            = mMoveTranslate.x;
				z            = mMoveTranslate.z;
				y            = mMoveTranslate.y;
				mWalkHeading = atan2(ppos.x - x, ppos.z - z);
				StartSmallSpiderWebEffect(&mFx, CVec(x, y, z));
				mStep++;
			}
		}
		break;
	case 104:
		mScale += 0.01f * frames;
		if (mScale >= 1.0f) {
			mScale = 1.0f;
			mTimer = 210 / frames;
			mStep++;
		}
		SetScale(mScale, mScale, mScale);
		break;
	case 105:
		if (IsSmallSpiderEffectAnimationFinished(&mFx, mFxIndex, frames)) {
			mSpiderStatus.mFlags &= ~0x8000;
			mStep = 0;
		}
		break;
	}
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
