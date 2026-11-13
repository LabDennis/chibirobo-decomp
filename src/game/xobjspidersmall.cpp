#include "game/xobjspidersmall.h"
#include "game/hsd_jobj.h"
#include "game/spiderplacement.h"
#include "game/spidercontact.h"
#include "game/cgame.h"
#include "game/cgs.h"
#include "game/cpad.h"
#include "game/cscrn.h"
#include "game/xchr.h"
#include "game/infoscrn.h"
#include "game/stagedata.h"
#include "game/ut.h"
#include "game/gmath.h"
#include "game/animframe.h"
#include "game/video.h"
#include <stdlib.h>

XObjSpiderSmall::~XObjSpiderSmall()
{
}

extern "C" {
void SpawnSpiderBounceCoin(Vec* position, Vec* target, s32 anim);
void UpdateSpiderCoinBounces(void* unused, u32 frames);
void ResetSpiderCoinBounces(void* unused);
BOOL IsUsingMug(XActLift* lift);
void ApproachHudValue(f32* value, f32* target, f32 step);
s16 GetPlayerItemCount(s32 id);
f32 GetPlayerCollisionRadius(XObjChr* chr);
void ApplyPlayerHitReaction(XObjChr* chr, BOOL react, BOOL stop, f32 time);
void DetachSmallSpiderFromPlayer(XObjSpiderSmall* spider);
void HandleSmallSpiderEventCommand(XObjSpiderSmall* spider, u32 frames);

u32 gSpiderCoinBounceManager; // the coins' manager token, the callbacks never touch it; its real type is unknown
s32 gSpiderDrainFlashTimers[2];
u8 gSpiderDrainFlashClaimed;
s32 gAttachedSmallSpiderCount; // how many spiders hold on to the robot
XObj* lbl_8065B2DC;
u8 lbl_8065B2E0;

u8 gSmallSpiderPlacementWork[0xD4];
Vec gSmallSpiderPosition, gSmallSpiderScale, gSmallSpiderRotation;
#if defined(VERSION_JP)
u8 gSmallSpiderWorkEnd[0x134 - 0xF8];
#else
u8 gSmallSpiderWorkEnd[0x140 - 0xF8];
#endif
XObj* gSmallSpiderEffectObjects[12];
s32 gSmallSpiderEffectSlotOwners[12];
SmallSpiderStageStatus gSmallSpiderStatusByStage[39];
}

extern "C" s32 sSmallSpiderStateData[26]    = { 0, 3, 0, 0, 3, 0, 0, 2, 0, 0, 2, 0, 0, 4, 0, 0, 0, 4, 0, 0, 0, 3, 0, 0, 3, 0 };
extern "C" XObjFloor sSmallSpiderLowFloor[] = {
	{ 0, 0, { 0, 135, 0 }, 0, { 377, 158, 377 } },
	{ -1 },
};
extern "C" XObjFloor sSmallSpiderHighFloor[] = {
	{ 0, 0, { 0, 135, 0 }, 0, { 377, 632, 377 } },
	{ -1 },
};
extern "C" XObjFloor* sSmallSpiderFloors[2] = { sSmallSpiderLowFloor, sSmallSpiderHighFloor };
extern "C" {
AnimSfxEvent gSmallSpiderAnimSfxEvents[] = {
	{ 2, 1.0f, 0.0f, 1, 0x137, 0xFF, 0, 0, 0 },
	{ 7, 1.0f, 0.0f, 1, 0x137, 0xFF, 0, 0, 0 },
	{ -1 },
};
}

void (XObjSpiderSmall::*gSmallSpiderStateHandlers[10])(u32) = {
	&XObjSpiderSmall::State0, &XObjSpiderSmall::State1, &XObjSpiderSmall::State2, &XObjSpiderSmall::State3, &XObjSpiderSmall::State4,
	&XObjSpiderSmall::State5, &XObjSpiderSmall::State6, &XObjSpiderSmall::State7, &XObjSpiderSmall::State8, &XObjSpiderSmall::State9,
};

static inline void TakeSmallSpiderSlot(XObjSpiderSmall* spider)
{
	for (int i = 0; i < 12; i++) {
		if (gSmallSpiderEffectSlotOwners[i] == -1) {
			spider->mFxObj                  = gSmallSpiderEffectObjects[i];
			spider->mFxIndex                = i;
			gSmallSpiderEffectSlotOwners[i] = spider->mPoolIndex;
			break;
		}
	}
}

// whether a sound effect may play now
static inline bool CanPlaySe()
{
	bool play = false;
	if (gGame.IsRunning() && gGame.mEvt.mData.mScript == NULL && gGame.mMenu.mActive == 0 && !(gGame.mStatus.mFlags & 0xC00)
	    && gGame.mTimeState.mDuration > 0.0f && gGame.mTimeState.mRunning == 0) {
		play = true;
	}
	return play;
}

/**
 * @note Address: 0x800F4AAC
 * @note Size: 0x164
 */
// NONMATCHING: anim and jobj swap r29 and r30
void XObjSpiderSmall::UpdateAnimationSounds()
{
	if ((mStatus.mFlags & 1) && (mStatus.mFlags & 0x10000) && !InfoScrn_IsBatteryGaugeEmpty(&gGame.mInfoScrn)) {
		s32 anim = mAnimId;
		mAnimSfx.Update(anim, GetAnimFrame(mJObj), GetSoundVolumeAtPosition(&gGame.mSound, &mMoveTranslate, 0));
	}
}

/**
 * @note Address: 0x800F4C10
 * @note Size: 0x1A8
 */
// the web in slot has played its anim to the end, or the slot has none
extern "C" bool IsSmallSpiderEffectAnimationFinished(XObjSpiderSmallFx* fx, u8 slot, u32 frames)
{
	if (gSmallSpiderEffectObjects[slot] == NULL) {
		return TRUE;
	}
	f32 end = GetAnimEndFrame(gSmallSpiderEffectObjects[slot]->mJObj);
	if (end - frames <= GetAnimFrame(gSmallSpiderEffectObjects[slot]->mJObj)) {
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x800F4DB8
 * @note Size: 0x190
 */
extern "C" void StartSmallSpiderWebEffect(XObjSpiderSmallFx* fx, const CVec& pos)
{
	fx->mStatus.mFlags |= 1;
	fx->mPos   = pos;
	fx->mState = 1;
	if (CanPlaySe()) {
		PlayGameSound(&gGame.mSound, 0x184, 0xFF);
		GetSoundVolumeAtPosition(&gGame.mSound, &fx->mPos, 0);
	}
}

/**
 * @note Address: 0x800F4F48
 * @note Size: 0x4B0
 */
// plays the web's anims, 1 and 2 shoot it, 10 drops it
extern "C" void UpdateSmallSpiderWebEffect(XObjSpiderSmallFx* shot, u8 i, u32 frames, BOOL unused)
{
	if (gGame.mStatus.mFlags & 0x400) {
		return;
	}
	if (i == 0xFF) {
		shot->mState = 0;
		shot->mStatus.mFlags &= ~1;
		return;
	}
	if (gSmallSpiderEffectObjects[i] == NULL) {
		shot->mState = 0;
		shot->mStatus.mFlags &= ~1;
		return;
	}
	if (!(shot->mStatus.mFlags & 1)) {
		return;
	}
	switch (shot->mState) {
	case 0:
		break;
	case 1:
		gSmallSpiderEffectObjects[i]->SetAnim(1, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
		shot->mState++;
		break;
	case 2: {
		f32 end = GetAnimEndFrame(gSmallSpiderEffectObjects[i]->mJObj);
		if (end - frames <= GetAnimFrame(gSmallSpiderEffectObjects[i]->mJObj)) {
			gSmallSpiderEffectObjects[i]->SetAnim(0, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			shot->mState = 0;
			shot->mStatus.mFlags &= ~1;
		}
		break;
	}
	case 3:
		gSmallSpiderEffectObjects[i]->SetAnim(2, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
		shot->mState++;
		break;
	case 4: {
		f32 end = GetAnimEndFrame(gSmallSpiderEffectObjects[i]->mJObj);
		if (end - frames <= GetAnimFrame(gSmallSpiderEffectObjects[i]->mJObj)) {
			gSmallSpiderEffectObjects[i]->SetAnim(0, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			shot->mState = 0;
			shot->mStatus.mFlags &= ~1;
		}
		break;
	}
	case 10:
		gSmallSpiderEffectObjects[i]->SetAnim(0, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
		shot->mState = 0;
		shot->mStatus.mFlags &= ~1;
		break;
	}
}

/**
 * @note Address: 0x800F53F8
 * @note Size: 0xBC
 */
extern "C" void SetJunkUnkoFlag(u8 on)
{
	for (s32 i = 0; i < 0x60; i++) {
		XObj* obj = gGame.mXObjs.mPools[7].Get(i);
		if ((obj->mStatus.mFlags & 1) && obj->GetKind() == 0x104) {
			if (on) {
				obj->mStatus.mFlags |= 0x10000;
			} else {
				obj->mStatus.mFlags &= ~0x10000ULL;
			}
		}
	}
}

/**
 * @note Address: 0x800F5588
 * @note Size: 0xF0
 */
// how long it waits in step 2 before it jumps at the robot, by the suit the robot wears
extern "C" s32 GetSmallSpiderGripDuration()
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

/**
 * @note Address: 0x800F5678
 * @note Size: 0xC
 */
extern "C" u8 HasSmallSpiderEmerged(XObjSpiderSmall* spider)
{
	return (spider->mSpiderStatus.mFlags >> 6) & 1;
}

/**
 * @note Address: 0x800F5684
 * @note Size: 0xC
 */
extern "C" u8 IsSmallSpiderGrabbingPlayer(XObjSpiderSmall* spider)
{
	return (spider->mSpiderStatus.mFlags >> 23) & 1;
}

/**
 * @note Address: 0x800F5690
 * @note Size: 0x130
 */
extern "C" bool IsSmallSpiderWithinTargetRadius(XObjSpiderSmall* spider)
{
	f32 dx = spider->mTargetPos.x - spider->mMoveTranslate.x;
	f32 dy = spider->mTargetPos.y - spider->mMoveTranslate.y;
	f32 dz = spider->mTargetPos.z - spider->mMoveTranslate.z;
	return sqrtf(dx * dx + dy * dy + dz * dz) <= spider->mTargetRadius + spider->mRadius;
}

/**
 * @note Address: 0x800F57C0
 * @note Size: 0x174
 */
// the player stands in the spider's area
extern "C" bool IsPlayerInSmallSpiderArea(XObjSpiderSmall* spider)
{
	Vec pos = gGame.GetPlayer()->mMoveTranslate;
	f32 y   = pos.y;
	f32 z   = pos.z;
	CVec unused;
	if (y < HSD_JObjGetTranslationY(spider->mJObj) - 20.0f || y > 20.0f + HSD_JObjGetTranslationY(spider->mJObj)) {
		return false;
	}
	f32 minX = spider->mAreaPos.x + spider->mRadius;
	f32 maxX = spider->mAreaPos.x + spider->mAreaWidth - spider->mRadius;
	f32 minZ = spider->mAreaPos.z + spider->mRadius;
	f32 maxZ = spider->mAreaPos.z + spider->mAreaDepth - spider->mRadius;
	if (minX > pos.x || maxX < pos.x || minZ > z || maxZ < z) {
		return false;
	}
	return true;
}

// the distances on the floor and in space
static inline f32 DistanceXZ(const Vec* a, const Vec* b)
{
	f32 dx = a->x - b->x;
	f32 dy = 0.0f;
	f32 dz = a->z - b->z;
	return sqrtf(dx * dx + dy * dy + dz * dz);
}

/**
 * @note Address: 0x800F841C
 * @note Size: 0x104
 */
extern "C" f32 GetSmallSpiderDistanceXZ(Vec* a, Vec* b)
{
	return DistanceXZ(a, b);
}

/**
 * @note Address: 0x800F8520
 * @note Size: 0x114
 */
extern "C" f32 GetSmallSpiderContactDistance(Vec* a, Vec* b)
{
	return SpiderContactDistance(a, b);
}

/**
 * @note Address: 0x800FC930
 * @note Size: 0x40
 */
// state 2, dead
extern "C" void State2__15XObjSpiderSmallFUl(XObjSpiderSmall* spider)
{
	spider->mStatus.mFlags &= ~0x10000ULL;
	spider->mSpiderStatus.mFlags |= 0x80000;
	spider->mSpiderStatus.mFlags &= ~0x100000;
}

// jumps onto the robot
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
	spider->SetAnim(5, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
}

/**
 * @note Address: 0x800F6408
 * @note Size: 0x2014
 */
// state 8: chases the robot and jumps onto it; when the robot gets away, it warps next to it through its web
void XObjSpiderSmall::State8(u32 frames)
{
	CPad pad(0);
	XObjChr* player = gGame.GetPlayer();
	Vec ppos        = player->mMoveTranslate;
	Vec prot        = player->mMoveRotate;
	// 0 chases the robot when it is near, else warps; 1 walks up to it; 2 waits, then jumps at it (10) or, when it got
	// away, hops in place (22 to 24); 10 and 11 fly at it and grab it, 12 lands after a miss; 4 clings, 5 drops off,
	// 6 blinks and loses a hit; 30 to 33 let go for a script; 100 to 105 warp through the web
	switch (mStep) {
	case 0:
		mTurnSpeed = 0.0f;
		if (GetSmallSpiderContactDistance(&ppos, &mMoveTranslate) < 48.0f) {
			mWalkTarget.x = ppos.x;
			mWalkTarget.y = ppos.y;
			mWalkTarget.z = ppos.z;
			SetAnim(2, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
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
			SetAnim(3, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
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
		f32 end = GetAnimEndFrame(mJObj);
		if (end - frames <= GetAnimFrame(mJObj)) {
			if (--mTimer < 0) {
				mWalkTarget.x = ppos.x;
				mWalkTarget.y = ppos.y;
				mWalkTarget.z = ppos.z;
				if (GetSmallSpiderContactDistance(&mWalkTarget, &mMoveTranslate) >= 20.0f) {
					SetAnim(12, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
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
					SetAnim(4, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
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
		_AA8             = 0;
		f32 size         = GetPlayerCollisionRadius(gGame.GetPlayer());
		mMoveTranslate.x = ppos.x + size * (f32)sin(prot.y + mClingAngle);
		mMoveTranslate.y = 10.0f + ppos.y + mClingOffset.y;
		mMoveTranslate.z = ppos.z + size * (f32)cos(prot.y + mClingAngle);
		mWalkHeading     = atan2(ppos.x - mMoveTranslate.x, ppos.z - mMoveTranslate.z);
		if (gGame.IsRunning() && (pad.mPad->mTrigger & 0xF0000)) {
			mHoldTime      = mHoldTime - mShakeStrength;
			mShakeStrength = 8.0f;
		}
		if (mShakeStrength > 1.0f) {
			mShakeStrength -= 1.0f;
		}
		if (gGame.mEvt.mData.mScript != NULL) {
			mWalkTarget.x = ppos.x + size * (f32)sin(prot.y + mClingAngle);
			mWalkTarget.y = ppos.y;
			mWalkTarget.z = ppos.z + size * (f32)cos(prot.y + mClingAngle);
			DetachSmallSpiderFromPlayer(this);
			SetAnim(6, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			mWalkSpeed = 0.0f;
			mStatus.mFlags |= 0x4000000000000ULL;
			mStep = 30;
		} else if (Abs(ppos.y - mHome.y) > 10.0f) {
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
			SetAnim(6, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			mWalkSpeed = 1.0f;
			mStatus.mFlags |= 0x4000000000000ULL;
			if (size >= 2.0f) {
				_AA8 = 1;
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
			SetAnim(7, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
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
		f32 end = GetAnimEndFrame(mJObj);
		if (end - frames <= GetAnimFrame(mJObj)) {
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
				SetAnim(1, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
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
				SetAnim(6, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
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
				SetAnim(7, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				mWalkSpeed = 0.0f;
				mSpiderStatus.mFlags &= ~8;
				mStatus.mFlags &= ~0x4000000000000ULL;
				mStep++;
			}
		}
		break;
	}
	case 12: {
		f32 end = GetAnimEndFrame(mJObj);
		if (end - frames <= GetAnimFrame(mJObj)) {
			mStep = 0;
			SetAnim(1, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
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
			SetAnim(7, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			mWalkSpeed = 0.0f;
			mSpiderStatus.mFlags &= ~8;
			mStatus.mFlags &= ~0x4000000000000ULL;
			mStep++;
		}
		break;
	case 24: {
		f32 end = GetAnimEndFrame(mJObj);
		if (end - frames <= GetAnimFrame(mJObj)) {
			SetAnim(1, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
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
			SetAnim(7, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			mStep = 31;
		}
		break;
	case 31: {
		f32 end = GetAnimEndFrame(mJObj);
		if (end - frames <= GetAnimFrame(mJObj)) {
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
			SetAnim(1, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
		} else if (gGame.mEvt.mData.mScript == NULL) {
			mStep = 0;
		}
		break;
	case 33:
		if (gGame.mEvt.mData.mScript == NULL) {
			mStep = 0;
		}
		break;
	case 100:
		mTurnSpeed = 0.0f;
		mWalkSpeed = 0.0f;
		mSpiderStatus.mFlags |= 0x8000;
		SetAnim(1, FALSE, NULL, NULL, NULL, 0.0f, 0.0f, 0.0f, 10.0f);
		mScale = 1.0f;
		SetScale(mScale, mScale, mScale, 0);
		{
			f32 x = mMoveTranslate.x;
			f32 y = mMoveTranslate.y;
			f32 z = mMoveTranslate.z;
			StartSmallSpiderWebEffect(&mFx, CVec(x, y, z));
		}
		mStep++;
		break;
	case 101:
		mScale -= 0.01f * frames;
		if (mScale <= 0.0f) {
			mScale = 0.0f;
			mTimer = 210 / frames;
			mStep++;
		}
		SetScale(mScale, mScale, mScale, 0);
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
				// one an event dropped in (command 12 sets 0x40000) is gone for good instead
				if (mSpiderStatus.mFlags & 0x40000) {
					State2__15XObjSpiderSmallFUl(this);
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
		SetScale(mScale, mScale, mScale, 0);
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
 * @note Address: 0x800FA4B0
 * @note Size: 0x2480
 */
// state 3: wanders about home, chases the robot and jumps onto it
void XObjSpiderSmall::State3(u32 frames)
{
	CPad pad(0);
	Vec ppos = gGame.GetPlayer()->mMoveTranslate;
	Vec prot = gGame.GetPlayer()->mMoveRotate;
	if (!IsPlayerInSmallSpiderArea(this)) {
		if (--mAreaGraceTimer < 0) {
			mAreaGraceTimer = 0;
		}
	} else {
		mAreaGraceTimer = (3 * FRAMES_PER_SECOND) / frames;
	}
	// around its jumps its floor box is four times as tall
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
	// 0 idles until it wanders (20) or the robot comes near (1); a wander may end in a hop in place (21, then 22 to 24);
	// the other steps are state 8's without the warps, kept inside its area
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
			SetAnim(2, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			mStep++;
		}
		break;
	case 20:
		mWalkHeading = atan2(mWalkTarget.x - mMoveTranslate.x, mWalkTarget.z - mMoveTranslate.z);
		if (GetSmallSpiderDistanceXZ(&mWalkTarget, &mMoveTranslate) < 2.0f) {
			mStep      = 0;
			mWalkSpeed = 0.0f;
			mTimer     = rand() % 30 + 30;
			SetAnim(1, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
		} else {
			if (--mTimer < 0) {
				mWalkSpeed = 0.0f;
				SetAnim(3, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				mStep = 21;
			}
			if (!InArea(mMoveTranslate)) {
				mWalkTarget.x = mHome.x;
				mWalkTarget.y = mHome.y;
				mWalkTarget.z = mHome.z;
			}
			// the robot has been out of its area for 3 seconds: a follower (0x20) goes after it, the others vanish home
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
		HSD_AObj* aobj = GetAObj(mJObj);
		f32 end        = aobj != NULL ? HSD_AObjGetEndFrame(aobj) : 0.0f;
		aobj           = GetAObj(mJObj);
		if (end - frames <= (aobj != NULL ? HSD_AObjGetCurrentFrame(aobj) : 0.0f)) {
			SetAnim(12, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
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
			SetAnim(7, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			mWalkSpeed = 0.0f;
			mSpiderStatus.mFlags &= ~8;
			mStatus.mFlags &= ~0x4000000000000ULL;
			mStep++;
		}
		break;
	case 24: {
		HSD_AObj* aobj = GetAObj(mJObj);
		f32 end        = aobj != NULL ? HSD_AObjGetEndFrame(aobj) : 0.0f;
		aobj           = GetAObj(mJObj);
		if (end - frames <= (aobj != NULL ? HSD_AObjGetCurrentFrame(aobj) : 0.0f)) {
			SetAnim(1, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
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
			SetAnim(7, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			mStep = 31;
		}
		break;
	case 31: {
		HSD_AObj* aobj = GetAObj(mJObj);
		f32 end        = aobj != NULL ? HSD_AObjGetEndFrame(aobj) : 0.0f;
		aobj           = GetAObj(mJObj);
		if (end - frames <= (aobj != NULL ? HSD_AObjGetCurrentFrame(aobj) : 0.0f)) {
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
			SetAnim(1, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
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
			SetAnim(3, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
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
		f32 end = GetAnimEndFrame(mJObj);
		if (end - frames <= GetAnimFrame(mJObj)) {
			if (--mTimer < 0) {
				mWalkTarget.x = ppos.x;
				mWalkTarget.y = ppos.y;
				mWalkTarget.z = ppos.z;
				if (GetSmallSpiderContactDistance(&mWalkTarget, &mMoveTranslate) >= 20.0f) {
					SetAnim(12, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
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
					SetAnim(4, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
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
		_AA8             = 0;
		f32 size         = GetPlayerCollisionRadius(gGame.GetPlayer());
		mMoveTranslate.x = ppos.x + size * (f32)sin(prot.y + mClingAngle);
		mMoveTranslate.y = 10.0f + ppos.y + mClingOffset.y;
		mMoveTranslate.z = ppos.z + size * (f32)cos(prot.y + mClingAngle);
		mWalkHeading     = atan2(ppos.x - mMoveTranslate.x, ppos.z - mMoveTranslate.z);
		if (pad.mPad->mTrigger & 0xF0000) {
			mHoldTime      = mHoldTime - mShakeStrength;
			mShakeStrength = 8.0f;
		}
		if (mShakeStrength > 1.0f) {
			mShakeStrength -= 1.0f;
		}
		if (gGame.mEvt.mData.mScript != NULL) {
			mWalkTarget.x = ppos.x + size * (f32)sin(prot.y + mClingAngle);
			mWalkTarget.y = ppos.y;
			mWalkTarget.z = ppos.z + size * (f32)cos(prot.y + mClingAngle);
			DetachSmallSpiderFromPlayer(this);
			SetAnim(6, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			mWalkSpeed = 0.0f;
			mStatus.mFlags |= 0x4000000000000ULL;
			mStep = 30;
		} else if (Abs(ppos.y - mHome.y) > 10.0f) {
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
			SetAnim(6, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			mWalkSpeed = 1.0f;
			mStatus.mFlags |= 0x4000000000000ULL;
			if (size >= 2.0f) {
				_AA8 = 1;
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
			SetAnim(7, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
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
		f32 end = GetAnimEndFrame(mJObj);
		if (end - frames <= GetAnimFrame(mJObj)) {
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
				SetAnim(1, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
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
				SetAnim(6, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
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
				SetAnim(7, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				mWalkSpeed = 0.0f;
				mSpiderStatus.mFlags &= ~8;
				mStatus.mFlags &= ~0x4000000000000ULL;
				mStep++;
			}
		}
		break;
	}
	case 12: {
		f32 end = GetAnimEndFrame(mJObj);
		if (end - frames <= GetAnimFrame(mJObj)) {
			SetAnim(1, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			mTimer = rand() % 30 + 30;
			mStep  = 0;
		}
		break;
	}
	}
}

/**
 * @note Address: 0x800F54B4
 * @note Size: 0xD4
 */
extern "C" void DetachSmallSpiderFromPlayer(XObjSpiderSmall* spider)
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
 * @note Address: 0x800F5934
 * @note Size: 0xAD4
 */
// state 9, from event command 21: lets go of the robot, runs away from it for a moment and vanishes into its web for good
void XObjSpiderSmall::State9(u32 frames)
{
	XObjChr* player = gGame.GetPlayer();
	Vec ppos        = player->mMoveTranslate;
	Vec prot        = player->mMoveRotate;
	switch (mStep) {
	case 0:
		if ((mSpiderStatus.mFlags & 0x800000) || mMoveTranslate.y > mHome.y) {
			f32 size      = GetPlayerCollisionRadius(player);
			mWalkTarget.x = ppos.x + size * (f32)sin(prot.y + mClingAngle);
			mWalkTarget.y = ppos.y;
			mWalkTarget.z = ppos.z + size * (f32)cos(prot.y + mClingAngle);
			DetachSmallSpiderFromPlayer(this);
			SetAnim(6, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			mWalkSpeed = 0.0f;
			mStatus.mFlags |= 0x4000000000000ULL;
			if (mFallSpeed >= -2.3f) {
				mFallSpeed -= 0.36f * frames;
			} else {
				mFallSpeed = -2.3f;
			}
			mStep = 1;
		} else {
			SetAnim(2, TRUE, NULL, NULL, NULL, 0.0f, 2.0f, 0.0f, 10.0f);
			mTimer = 60 / frames;
			mStep  = 10;
		}
		break;
	case 1:
		if (mFallSpeed >= -2.3f) {
			mFallSpeed -= 0.36f * frames;
		} else {
			mFallSpeed = -2.3f;
		}
		if (mMoveTranslate.y <= mHome.y + frames) {
			mFallSpeed       = 0.0f;
			mMoveTranslate.y = mHome.y;
			SetAnim(7, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			mWalkSpeed = 0.0f;
			mSpiderStatus.mFlags &= ~8;
			mStatus.mFlags &= ~0x4000000000000ULL;
			mStep++;
		}
		break;
	case 2: {
		f32 end = GetAnimEndFrame(mJObj);
		if (end - frames <= GetAnimFrame(mJObj)) {
			mWalkTarget.x = 12.0f + mHome.x - rand() % 2400 / 100.0f;
			mWalkTarget.y = mHome.y;
			mWalkTarget.z = 12.0f + mHome.z - rand() % 2400 / 100.0f;
			SetAnim(2, TRUE, NULL, NULL, NULL, 0.0f, 2.0f, 0.0f, 10.0f);
			mTimer = 60 / frames;
			mStep  = 10;
		}
		break;
	}
	case 10:
		mWalkSpeed    = 1.0f;
		mWalkTarget.x = ppos.x;
		mWalkTarget.y = ppos.y;
		mWalkTarget.z = ppos.z;
		mWalkHeading  = PI + (f32)atan2(mWalkTarget.x - mMoveTranslate.x, mWalkTarget.z - mMoveTranslate.z);
		SpiderContactDistance(&mWalkTarget, &mMoveTranslate);
		if (--mTimer < 0) {
			mTurnSpeed = 0.0f;
			mWalkSpeed = 0.0f;
			mSpiderStatus.mFlags |= 0x8000;
			SetAnim(1, FALSE, NULL, NULL, NULL, 0.0f, 0.0f, 0.0f, 10.0f);
			mScale = 1.0f;
			SetScale(mScale, mScale, mScale, 0);
			f32 x = mMoveTranslate.x;
			f32 y = mMoveTranslate.y;
			f32 z = mMoveTranslate.z;
			StartSmallSpiderWebEffect(&mFx, CVec(x, y, z));
			mStep++;
		}
		break;
	case 11:
		mScale -= 0.01f * frames;
		if (mScale <= 0.0f) {
			mScale = 0.0f;
			mTimer = 210 / frames;
			mStep++;
		}
		SetScale(mScale, mScale, mScale, 0);
		break;
	case 12:
		if (IsSmallSpiderEffectAnimationFinished(&mFx, mFxIndex, frames)) {
			mTimer = 0;
			State2__15XObjSpiderSmallFUl(this);
			if (mFxIndex != 0xFF) {
				gSmallSpiderEffectSlotOwners[mFxIndex] = -1;
				mFxObj                                 = NULL;
				mFxIndex                               = 0xFF;
			}
			mStep++;
		}
		break;
	case 13:
		break;
	}
}

/**
 * @note Address: 0x800F9AA0
 * @note Size: 0xA10
 */
// state 4: once the robot is 30 away, vanishes into its web and pops up next to it, then stays put
void XObjSpiderSmall::State4(u32 frames)
{
	CPad pad(0);
	Vec ppos = gGame.GetPlayer()->mMoveTranslate;
	Vec prot = gGame.GetPlayer()->mMoveRotate;
	switch (mStep) {
	case 0:
		SetAnim(1, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
		if (SpiderContactDistance(&ppos, &mMoveTranslate) >= 30.0f) {
			mWalkSpeed = 0.0f;
			mTimer     = 1;
			mStep++;
		}
		break;
	case 1:
		if (--mTimer < 0) {
			mScale = 1.0f;
			SetScale(mScale, mScale, mScale, 0);
			f32 x = mMoveTranslate.x;
			f32 y = mMoveTranslate.y;
			f32 z = mMoveTranslate.z;
			StartSmallSpiderWebEffect(&mFx, CVec(x, y, z));
			mStep++;
		}
		break;
	case 2:
		mScale -= 0.01f * frames;
		if (mScale <= 0.0f) {
			mScale = 0.0f;
			mTimer = 210 / frames;
			mStep++;
		}
		SetScale(mScale, mScale, mScale, 0);
		break;
	case 3:
		if (IsSmallSpiderEffectAnimationFinished(&mFx, mFxIndex, frames)) {
			mTimer = rand() % 30 + 30;
			mStep++;
		}
		break;
	case 4:
		if (--mTimer < 0) {
			f32 x, z, y, dist;
			f32 size         = GetPlayerCollisionRadius(gGame.GetPlayer());
			mClingAngle      = DEG2RAD * (rand() % 360);
			dist             = 10.0f + size;
			mMoveTranslate.x = ppos.x + dist * (f32)sin(prot.y + mClingAngle);
			mMoveTranslate.y = ppos.y;
			mMoveTranslate.z = ppos.z + dist * (f32)cos(prot.y + mClingAngle);
			x                = mMoveTranslate.x;
			z                = mMoveTranslate.z;
			y                = mMoveTranslate.y;
			mWalkHeading     = atan2(ppos.x - x, ppos.z - z);
			StartSmallSpiderWebEffect(&mFx, CVec(x, y, z));
			mStep++;
		}
		break;
	case 5:
		mScale += 0.01f * frames;
		if (mScale >= 1.0f) {
			mScale = 1.0f;
			mTimer = 210 / frames;
			mStep++;
		}
		SetScale(mScale, mScale, mScale, 0);
		break;
	case 6:
		IsSmallSpiderEffectAnimationFinished(&mFx, mFxIndex, frames);
		break;
	}
}

/**
 * @note Address: 0x800F9350
 * @note Size: 0x750
 */
// NONMATCHING: r9 and r10 swap around the web sound
// state 5: hides in its web until the robot comes into its area, then pops out
void XObjSpiderSmall::State5(u32 frames)
{
	CJObj obj;
	Vec ppos = gGame.GetPlayer()->mMoveTranslate;
	CPad pad(0);
	switch (mStep) {
	case 0:
		mSpiderStatus.mFlags |= 0x8000;
		SetAnim(1, FALSE, NULL, NULL, NULL, 0.0f, 0.0f, 0.0f, 10.0f);
		mScale = 0.0f;
		SetScale(mScale, mScale, mScale, 0);
		DistanceXZ(&ppos, &mMoveTranslate);
		if (IsPlayerInSmallSpiderArea(this) && gGame.mEvt.mData.mScript == NULL) {
			f32 x = mMoveTranslate.x;
			f32 y = mMoveTranslate.y;
			f32 z = mMoveTranslate.z;
			StartSmallSpiderWebEffect(&mFx, CVec(x, y, z));
			mStep++;
		}
		break;
	case 1:
		mScale += 0.01f * frames;
		if (mScale >= 1.0f) {
			mScale = 1.0f;
			mStep++;
			mTimer = 210 / frames;
		}
		SetScale(mScale, mScale, mScale, 0);
		break;
	case 2:
		if (IsSmallSpiderEffectAnimationFinished(&mFx, mFxIndex, frames)) {
			mSpiderStatus.mFlags &= ~0x8000;
			SetState(3);
			mSpiderStatus.mFlags |= 0x40;
			gSmallSpiderStatusByStage[gGame.mStage].mSpiders[mLoadPoolIndex].mFlags |= 0x40;
			mAreaGraceTimer = (3 * FRAMES_PER_SECOND) / frames;
			// one in four may follow the robot out of its area (0x20), except on stages 3, 7 and 9
			switch (gGame.mStage) {
			case 3:
			case 7:
			case 9:
				break;
			default:
				if (rand() % 4 == 0) {
					mSpiderStatus.mFlags |= 0x20;
				}
				break;
			}
		}
		break;
	}
}

/**
 * @note Address: 0x800F8634
 * @note Size: 0x784
 */
// state 7: spits a web and shrinks away
void XObjSpiderSmall::State7(u32 frames)
{
	CJObj obj;
	CPad pad(0);
	switch (mStep) {
	case 0:
		mTurnSpeed = 0.0f;
		mWalkSpeed = 0.0f;
		mSpiderStatus.mFlags |= 0x8000;
		SetAnim(1, FALSE, NULL, NULL, NULL, 0.0f, 0.0f, 0.0f, 10.0f);
		mScale = 1.0f;
		SetScale(mScale, mScale, mScale, 0);
		{
			f32 x = mMoveTranslate.x;
			f32 y = mMoveTranslate.y;
			f32 z = mMoveTranslate.z;
			StartSmallSpiderWebEffect(&mFx, CVec(x, y, z));
		}
		mStep++;
		break;
	case 1:
		mScale -= 0.01f * frames;
		if (mScale <= 0.0f) {
			mScale = 0.0f;
			mStep++;
			mTimer = 210 / frames;
		}
		SetScale(mScale, mScale, mScale, 0);
		break;
	case 2:
		if (mSpiderStatus.mFlags & 0x40000) {
			if (IsSmallSpiderEffectAnimationFinished(&mFx, mFxIndex, frames)) {
				mStep++;
				mTimer = 0;
				State2__15XObjSpiderSmallFUl(this);
				if (mFxIndex != 0xFF) {
					gSmallSpiderEffectSlotOwners[mFxIndex] = -1;
					mFxObj                                 = NULL;
					mFxIndex                               = 0xFF;
				}
			}
		} else if (IsSmallSpiderEffectAnimationFinished(&mFx, mFxIndex, frames)) {
			// it let go because the robot left its floor (0x2000), so it goes back home
			if (mSpiderStatus.mFlags & 0x2000) {
				mMoveTranslate.x = mHome.x;
				mMoveTranslate.y = mHome.y;
				mMoveTranslate.z = mHome.z;
				mSpiderStatus.mFlags &= ~0x2000;
			}
			SetState(5);
			mSpiderStatus.mFlags &= ~0x40;
			mSpiderStatus.mFlags &= ~0x20;
			gSmallSpiderStatusByStage[gGame.mStage].mSpiders[mLoadPoolIndex].mFlags &= ~0x40;
		}
		break;
	case 3:
		break;
	}
}

/**
 * @note Address: 0x800F8DB8
 * @note Size: 0x598
 */
// state 6: drops down on its thread and lands
void XObjSpiderSmall::State6(u32 frames)
{
	CJObj obj;
	CPad pad(0);
	switch (mStep) {
	case 0:
		mSpiderStatus.mFlags |= 0x8000;
		SetAnim(1, FALSE, NULL, NULL, NULL, 0.0f, 0.0f, 0.0f, 10.0f);
		mScale       = 0.0f;
		mTargetScale = 0.0f;
		if (mSpiderStatus.mFlags & 0x40000) {
			mTimer = 2 / frames;
			mStep++;
		}
		break;
	case 1:
		if (--mTimer < 0) {
			mWalkSpeed   = 0.0f;
			mTargetScale = 1.0f;
			SetAnim(12, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			mSpiderStatus.mFlags |= 8;
			mWalkTarget.x = mMoveTranslate.x;
			mWalkTarget.y = mMoveTranslate.y;
			mWalkTarget.z = mMoveTranslate.z;
			mWalkSpeed    = 0.0f;
			mFallSpeed    = 4.0f;
			mWalkHeading  = atan2(mWalkTarget.x - mMoveTranslate.x, mWalkTarget.z - mMoveTranslate.z);
			mSpiderStatus.mFlags &= ~1;
			mStep++;
		}
		break;
	case 2:
		if (mFallSpeed >= -2.3f) {
			mFallSpeed -= 0.36f * frames;
		} else {
			mFallSpeed = -2.3f;
		}
		if (mMoveTranslate.y <= mHome.y + frames) {
			mFallSpeed       = 0.0f;
			mMoveTranslate.y = mHome.y;
			SetAnim(7, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			mWalkSpeed = 0.0f;
			mSpiderStatus.mFlags &= ~8;
			mStatus.mFlags &= ~0x4000000000000ULL;
			mStep++;
		}
		break;
	case 3: {
		f32 end = GetAnimEndFrame(mJObj);
		if (end - frames <= GetAnimFrame(mJObj)) {
			SetAnim(1, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			mTimer = rand() % 30 + 30;
			mSpiderStatus.mFlags &= ~0x8000;
			SetState(3);
			mStatus.mFlags &= ~0x4000000000000ULL;
			mSpiderStatus.mFlags |= 0x40;
			mStep = 0;
		}
		break;
	}
	}
	ApproachHudValue(&mScale, &mTargetScale, 0.08f * frames);
	SetScale(mScale, mScale, mScale, 0);
}

/**
 * @note Address: 0x800FCF00
 * @note Size: 0x4
 */
void XObjSpiderSmall::State0(u32 frames)
{
}

/**
 * @note Address: 0x800FC970
 * @note Size: 0x590
 */
// NONMATCHING: register order
// state 1, hit: blinks green, drops an item and waits out the web
void XObjSpiderSmall::State1(u32 frames)
{
	mTimer++;
	if (mTimer % 30 < 15) {
		SetColor(1, 0, 0x80, 0, 0.0f);
	} else {
		SetColor(1, 0, 0, 0, 0.0f);
	}
	switch (mStep) {
	case 0:
		if (mSpiderStatus.Check(0x800000)) {
			DetachSmallSpiderFromPlayer(this);
		}
		PlayGameSound(&gGame.mSound, 0x127, 0xFF);
		if (!(gGame.mStatus.mFlags & 0x100)) {
			StartScreenShake(&gGame.mQuake, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.4f);
		}
		gSmallSpiderStatusByStage[gGame.mStage].mSpiders[mLoadPoolIndex].mFlags |= 0x80000;
		{
			f32 x = mMoveTranslate.x;
			f32 y = mMoveTranslate.y;
			f32 z = mMoveTranslate.z;
			mFx.Show(CVec(x, y, z), 3);
		}
		mStep++;
		break;
	case 1: {
		mStep++;
		mTimer = 0;
		State2__15XObjSpiderSmallFUl(this);
		Vec pos;
		pos.x = mMoveTranslate.x;
		pos.y = mMoveTranslate.y;
		pos.z = mMoveTranslate.z;
		s32 kind;
		if (IsStage("stage07")) {
			switch (mPoolIndex) {
			case 0:
			case 3:
			case 5:
			case 7:
			case 10:
				kind = 1;
				break;
			case 2:
			case 8:
				kind = 0;
				break;
			case 1:
			case 4:
			case 6:
			case 9:
			default:
				kind = 2;
				break;
			}
		} else if (rand() % 2 > 0) {
			kind = 2;
		} else if (rand() % 4 > 1) {
			kind = 1;
		} else {
			kind = 0;
		}
		if (_AA8) {
			kind = 2;
		}
		SpawnSpiderBounceCoin(&pos, &mHome, kind);
		break;
	}
	case 2:
		mStatus.mFlags &= ~0x10000ULL;
		if (IsSmallSpiderEffectAnimationFinished(&mFx, mFxIndex, frames)) {
			if (mFxIndex != 0xFF) {
				gSmallSpiderEffectSlotOwners[mFxIndex] = -1;
				mFxObj                                 = NULL;
				mFxIndex                               = 0xFF;
			}
			mStep++;
		}
		break;
	case 3:
		break;
	}
}

/**
 * @note Address: 0x800FCF04
 * @note Size: 0x6DC
 */
// runs the command an event gave it with HandleEventCommand: 1 knocks it out, 3 asks whether it is dead, 7 freezes it
// (arg 1 frees it), 8 resets it to its placement, 9 moves it and its home, 10 sets its area (positions in hundredths),
// 11 hides it in its web, 12 drops it in, 21 sends it off for good, 23 shows or hides the junk unko
extern "C" void HandleSmallSpiderEventCommand(XObjSpiderSmall* spider, u32 frames)
{
	switch (spider->mEvtArgs[0]) {
	case 0:
		break;
	case 1:
		if (!(spider->mSpiderStatus.mFlags & 0x8000)) {
			spider->SetState(1);
		}
		spider->mEvtArgs[0] = 0;
		break;
	case 2:
		if (spider->mSpiderStatus.mFlags & 0x10000) {
			gGame.mEvt.mSelectResult = 1;
		} else {
			gGame.mEvt.mSelectResult = 0;
		}
		spider->mEvtArgs[0] = 0;
		break;
	case 3:
		if (spider->mSpiderStatus.mFlags & 0x80000) {
			gGame.mEvt.mSelectResult = 1;
		} else {
			gGame.mEvt.mSelectResult = 0;
		}
		spider->mEvtArgs[0] = 0;
		break;
	case 4:
		spider->mSpiderStatus.mFlags |= 0x200000;
		spider->mEvtArgs[0] = 0;
		break;
	case 5:
		spider->mSpiderStatus.mFlags |= 0x400000;
		spider->mEvtArgs[0] = 0;
		break;
	case 6:
		spider->mStatus.mFlags |= 0x4000000000000ULL;
		spider->mEvtArgs[0] = 0;
		break;
	case 7:
		if (spider->mEvtArgs[1] == 1) {
			spider->mSpiderStatus.mFlags &= ~0x2000000;
		} else {
			spider->mSpiderStatus.mFlags |= 0x2000000;
		}
		spider->mEvtArgs[0] = 0;
		spider->mEvtArgs[1] = 0;
		break;
	case 8:
		ResetSmallSpiderState(spider);
		ApplySmallSpiderPlacement(spider);
		spider->mEvtArgs[0] = 0;
		break;
	case 9:
		spider->mHome.x          = spider->mEvtArgs[1] / 100.0f;
		spider->mHome.y          = spider->mEvtArgs[2] / 100.0f;
		spider->mHome.z          = spider->mEvtArgs[3] / 100.0f;
		spider->mTargetPos.x     = spider->mHome.x;
		spider->mTargetPos.y     = spider->mHome.y;
		spider->mTargetPos.z     = spider->mHome.z;
		spider->mMoveTranslate.x = spider->mHome.x;
		spider->mMoveTranslate.y = spider->mHome.y;
		spider->mMoveTranslate.z = spider->mHome.z;
		spider->mMoveTranslate.mTarget.Set(spider->mMoveTranslate);
		spider->mEvtArgs[0] = 0;
		break;
	case 10:
		spider->mAreaPos.x  = spider->mEvtArgs[1] / 100.0f;
		spider->mAreaPos.y  = spider->mEvtArgs[2] / 100.0f;
		spider->mAreaPos.z  = spider->mEvtArgs[3] / 100.0f;
		spider->mAreaWidth  = spider->mEvtArgs[4];
		spider->mAreaDepth  = spider->mEvtArgs[5];
		spider->mEvtArgs[0] = 0;
		break;
	case 11:
		spider->mWalkSpeed = 0.0f;
		spider->SetState(5);
		spider->mFx.mStatus.mFlags |= 1;
		spider->mFx.mState  = 10;
		spider->mEvtArgs[0] = 0;
		break;
	case 12:
		spider->mSpiderStatus.mFlags |= 0x40000;
		spider->mStatus.mFlags |= 0x10000;
		spider->mStatus.mFlags |= 0x4000000000000ULL;
		spider->SetState(6);
		spider->mSpiderStatus.mFlags |= 0x8000;
		spider->SetAnim(1, FALSE, NULL, NULL, NULL, 0.0f, 0.0f, 0.0f, 10.0f);
		spider->mScale       = 0.0f;
		spider->mTargetScale = 0.0f;
		spider->mWalkSpeed   = 0.0f;
		TakeSmallSpiderSlot(spider);
		spider->mEvtArgs[0] = 0;
		break;
	case 21:
		if (!(spider->mSpiderStatus.mFlags & 0x8000) && !(spider->mSpiderStatus.mFlags & 0x80000) && (spider->mStatus.mFlags & 0x10000)) {
			spider->SetState(9);
		}
		spider->mEvtArgs[0] = 0;
		break;
	case 23:
		if (spider->mEvtArgs[1] == 1) {
			SetJunkUnkoFlag(1);
		} else {
			SetJunkUnkoFlag(0);
		}
		spider->mEvtArgs[0] = 0;
		spider->mEvtArgs[1] = 0;
		break;
	}
}

// takes amount off the robot's battery, not below empty
static inline void DrainPlayerBattery(f32 amount)
{
	f32 battery = gGame.GetPlayer()->mParams.mBattery - amount;
	if (battery < 0.0f) {
		battery = 0.0f;
	}
	gGame.GetPlayer()->mParams.SetBattery(battery);
}

/**
 * @note Address: 0x800FD5E0
 * @note Size: 0x40C
 */
// caught in the web: drains the robot's battery and tints the screen red
extern "C" void UpdatePlayerCaughtInSpiderWeb(XObjSpiderSmall* spider, u32 frames)
{
	CPad pad(0);
	if ((spider->mSpiderStatus.mFlags & 2) && !(spider->mSpiderStatus.mFlags & 0x10)) {
		StopGameSfx(&gGame.mSound, 0x12A);
		spider->mSpiderStatus.mFlags &= ~2;
		spider->SetState(1);
		if (spider->mHitStrength >= 10.0f) {
			spider->mHitsLeft -= 5;
			if (spider->mHitsLeft <= 0) {
				spider->mHitsLeft = 0;
			}
		} else if (--spider->mHitsLeft <= 0) {
			spider->mHitsLeft = 0;
		}
	}
	{
		XObjChr* player = gGame.GetPlayer();
		Vec ppos        = player->mMoveTranslate;
		CVec pos(ppos);
		f32 dist = SpiderContactDistance(&pos, &spider->mMoveTranslate);
		if ((spider->mSpiderStatus.mFlags & 0x800000) && gGame.mEvt.mData.mScript == NULL
		    && GetPlayerCollisionRadius(gGame.GetPlayer()) < 2.0f) {
			if (--spider->mDrainTimer <= 0) {
				DrainPlayerBattery(3.0f);
				spider->mDrainTimer = 30;
				if (gSpiderDrainFlashClaimed == 0) {
					spider->mSpiderStatus.mFlags |= 0x1000000;
					gSpiderDrainFlashClaimed = 1;
					InfoScrn_ApplyScoreHudCommand(&gGame.mInfoScrn, 1, 1);
				}
				if (spider->mSpiderStatus.mFlags & 0x1000000) {
					gSpiderDrainFlashTimers[0] = FRAMES_PER_SECOND;
					gSpiderDrainFlashTimers[1] = 0;
				}
			}
		}
		if (spider->mSpiderStatus.mFlags & 0x1000000) {
			ApplyPlayerHitReaction(player, FALSE, TRUE, 0.0f);
			if (gSpiderDrainFlashTimers[0] > 0) {
				if ((gSpiderDrainFlashTimers[0] -= frames) < 0) {
					gSpiderDrainFlashTimers[0] = 0;
				}
				gSpiderDrainFlashTimers[1]++;
				if (gSpiderDrainFlashTimers[0] > 0) {
					f32 red = 128.0f * (gSpiderDrainFlashTimers[0] / FRAME_RATE);
					player->SetColor(1, red, 0, 0, 0.0f);
					StartScreenFade(&gGame.mScrnTex.mColor, red, 0, 0, 0, red, 0, 0, 0, 0.0f);
				} else {
					player->SetColor(1, 0, 0, 0, 0.0f);
					StartScreenFade(&gGame.mScrnTex.mColor, 0, 0, 0, 0, 100, 100, 200, 0, 0.0f);
				}
			}
		}
	}
	HandleSmallSpiderEventCommand(spider, frames);
}

/**
 * @note Address: 0x800FD9EC
 * @note Size: 0x14
 */
s32 XObjSpiderSmall::OnPlayerContact(s32 unused, s32 value)
{
	mSpiderStatus.mFlags |= 1;
	return value;
}

/**
 * @note Address: 0x800FDA00
 * @note Size: 0xBC
 */
// keeps the hit for UpdatePlayerCaughtInSpiderWeb, unless one is pending or it can't be hit (0x10, or 0x8000 in its web)
void XObjSpiderSmall::OnHitFromPosition(const Vec& pos, f32 strength)
{
	if (!(mSpiderStatus.mFlags & 2) && !(mSpiderStatus.mFlags & 0x10) && !(mSpiderStatus.mFlags & 0x8000)) {
		OnHitFromPosition__4XObjFRC3Vecf(this, &pos, strength);
		mSpiderStatus.mFlags |= 2;
		Vec hitPos   = { 0.0f, 0.0f, 0.0f };
		hitPos.x     = pos.x;
		hitPos.y     = pos.y;
		hitPos.z     = pos.z;
		mHitPos      = hitPos;
		mHitStrength = strength;
	}
}

/**
 * @note Address: 0x800FDABC
 * @note Size: 0xF4
 */
s32 XObjSpiderSmall::HandleEventCommand(s32 n, s32* args)
{
	for (int i = 0; i < n; i++) { }
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
 * @note Address: 0x800FDBB0
 * @note Size: 0x4
 */
void XObjSpiderSmall::DrawObjectEffects()
{
}

/**
 * @note Address: 0x800FDBB4
 * @note Size: 0x4
 */
void XObjSpiderSmall::DrawDebugStatus()
{
}

/**
 * @note Address: 0x800FDBB8
 * @note Size: 0x4
 */
void XObjSpiderSmall::DrawScreenOverlay()
{
}

/**
 * @note Address: 0x800FDEEC
 * @note Size: 0x4
 */
void XObjSpiderSmall::DrawGroundShadow()
{
}

/**
 * @note Address: 0x800FDBBC
 * @note Size: 0x330
 */
// NONMATCHING: float register order
void XObjSpiderSmall::DrawGroundShadowAt(const Vec* pos, f32 opacity)
{
	if ((mStatus.mFlags & 8) && (mSpiderStatus.mFlags & 0x40) && (mStatus.mFlags & 0x10000)) {
		Vec p    = *pos;
		Vec rot  = { -HALF_PI, 0.0f, 0.0f };
		Vec size = mMoveScale * (mAttr.mObjData != NULL ? mAttr.mObjData->mShadowSize : 0);
		size.x   = mRadius;
		size.y   = mRadius;
		size.z   = mRadius;
		SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 4, 5, 15);
		u32 edge   = 128.0f * opacity;
		u32 center = 160.0f * opacity;
		DrawGsCircle(&gGame.mGs, 12, center, edge, p.x, p.y, p.z, size.x, size.z, rot.x, rot.y, rot.z);
		PrimVertex vtx[26];

		// a soft rim around it, a pair of points every 30 degrees (0.5235988), fading out over 1.5
		for (s32 i = 0; i < 13; i++) {
			f32 hx               = size.x * 0.5f;
			f32 hz               = size.z * 0.5f;
			f32 ang              = 0.5235988f * i;
			vtx[i * 2].pos.x     = p.x - hx * (f32)sin(ang);
			vtx[i * 2].pos.z     = p.z + hz * (f32)cos(ang);
			vtx[i * 2].pos.y     = p.y;
			vtx[i * 2].color     = edge;
			vtx[i * 2 + 1].pos.x = p.x - (1.5f + hx) * (f32)sin(ang);
			vtx[i * 2 + 1].pos.z = p.z + (1.5f + hz) * (f32)cos(ang);
			vtx[i * 2 + 1].pos.y = p.y;
			vtx[i * 2 + 1].color = 0;
		}
		DrawGsTriangleVertices(&gGame.mGs, GX_TRIANGLESTRIP, gGame.mGs.mCamera->mViewMtx, vtx, 26);
	}
}

/**
 * @note Address: 0x800FDEF0
 * @note Size: 0x28C
 */
void XObjSpiderSmall::DrawObject()
{
	DrawWithView(0);
	if (mFxIndex != 0xFF && (mFx.mStatus.mFlags & 1) && gSmallSpiderEffectObjects[mFxIndex] != NULL) {
		HSD_JObj* jobj = gSmallSpiderEffectObjects[mFxIndex]->mJObj;
		HSD_JObjSetTranslateX(jobj, mFx.mPos.x);
		HSD_JObjSetTranslateY(jobj, mFx.mPos.y);
		HSD_JObjSetTranslateZ(jobj, mFx.mPos.z);
		HSD_JObjSetRotationY(jobj, mFx.mRotY);
		HSD_JObjDispAll(jobj, NULL, 7, 0);
	}
}

// the bgm playing, none while one is queued
static inline s32 GetBgm()
{
	return gGame.mSound.mMusicFadeOutFrames <= 0 ? gGame.mSound.mCurrentMusicId : 0;
}

// the player stands in a live spider's area
static inline bool IsPlayerNear()
{
	Vec pos = gGame.GetPlayer()->mMoveTranslate;
	for (int i = 0; i < 16; i++) {
		XObjSpiderSmall* spider = (XObjSpiderSmall*)gGame.mXObjs.mPools[18].Get(i);
		if ((spider->mStatus.mFlags & 1) && !spider->IsDead() && (spider->mStatus.mFlags & 0x10000)
		    && !(spider->mSpiderStatus.mFlags & 0x2000000)) {
			if (pos.y < HSD_JObjGetTranslationY(spider->mJObj) - 20.0f || pos.y > 20.0f + HSD_JObjGetTranslationY(spider->mJObj)) {
				continue;
			}
			f32 minX = 1.5f + spider->mAreaPos.x;
			f32 maxX = spider->mAreaPos.x + spider->mAreaWidth - 1.5f;
			f32 minZ = 1.5f + spider->mAreaPos.z;
			f32 maxZ = spider->mAreaPos.z + spider->mAreaDepth - 1.5f;
			if (minX > pos.x || maxX < pos.x || minZ > pos.z || maxZ < pos.z) {
				continue;
			}
			return true;
		}
	}
	return false;
}

static inline void SetObjVec(CObjVec& v, const Vec& pos)
{
	v.Set(pos);
	v.mTarget.Set(pos);
}

/**
 * @note Address: 0x800FE17C
 * @note Size: 0xA38
 */
// NONMATCHING: register allocation
void XObjSpiderSmall::UpdateObject(u32 frames)
{
	CPad pad(0);
	bool battleBgm = true;
	if (IsStage("stage00") || IsStage("stage19")) {
		battleBgm = false;
	}
	// the first spider runs the battle bgm, music 0xFFFF000C (-0xFFF4): 0 starts it when the robot comes near, 1 plays it
	// until the robot leaves or the battery runs out, 2 waits for the robot to leave after the battery ran out
	if (mPoolIndex == 0 && battleBgm && !IsStage("stage07")) {
		switch (mBattleBgmState) {
		case 0:
			if (IsPlayerNear() && gGame.mEvt.mData.mScript == NULL && GetBgm() != -0xFFF4) {
				mPrevBgm       = GetBgm();
				mPrevBgmVolume = gGame.mSound.mMusicVolume;
				PlayGameSound(&gGame.mSound, -0xFFF4, 0xFF);
				mBattleBgmState++;
			}
			break;
		case 1:
			if (InfoScrn_IsBatteryGaugeEmpty(&gGame.mInfoScrn)) {
				if (GetPlayerItemCount(0x9E) > 0) {
					if (IsGameSoundPlaying(&gGame.mSound, -0xFFF4)) {
						StopGameSfx(&gGame.mSound, -0xFFF4);
					}
					mBattleBgmState = 2;
				} else {
					if (IsGameSoundPlaying(&gGame.mSound, -0xFFF4)) {
						StopGameSfx(&gGame.mSound, -0xFFF4);
					}
					mBattleBgmState = 0;
				}
			} else if (!IsPlayerNear() || gGame.mEvt.mData.mScript != NULL) {
				if (mPrevBgm != 0) {
					PlayGameSound(&gGame.mSound, mPrevBgm, mPrevBgmVolume);
					mBattleBgmState = 0;
				} else if (GetBgm() == -0xFFF4) {
					StopGameSfx(&gGame.mSound, -0xFFF4);
					mBattleBgmState = 0;
				}
			}
			break;
		case 2:
			if (!IsPlayerNear() || gGame.mEvt.mData.mScript != NULL) {
				if (mPrevBgm != 0) {
					PlayGameSound(&gGame.mSound, mPrevBgm, mPrevBgmVolume);
					mBattleBgmState = 0;
				} else if (GetBgm() == -0xFFF4) {
					StopGameSfx(&gGame.mSound, -0xFFF4);
					mBattleBgmState = 0;
				}
			}
			break;
		}
	}
	if (gGame.mStatus.mFlags & 0x400) {
		return;
	}
	if (mPoolIndex == 0) {
		UpdateSpiderCoinBounces(&gSpiderCoinBounceManager, frames);
	}
	if (!(mStatus.mFlags & 0x10000) && mState != 1) {
		return;
	}
	UpdateAnimationSounds();
	UpdatePlayerCaughtInSpiderWeb(this, frames);
	if (!(mSpiderStatus.mFlags & 0x2000000)) {
		(this->*gSmallSpiderStateHandlers[mState])(frames);
		CVec dir;
		// 0x100: the walk speed moves it up rather than forward; 8: in the air, it rises and falls by mFallSpeed,
		// else it stays at its home height
		if (mSpiderStatus.mFlags & 0x100) {
			dir.x = 0.0f;
			dir.z = 0.0f;
			dir.y = mWalkSpeed;
			SetRotateY(mWalkHeading, mTurnSpeed);
		} else {
			if (mSpiderStatus.mFlags & 8) {
				mMoveTranslate.y += mFallSpeed;
			} else {
				mMoveTranslate.y = mHome.y;
			}
			dir.x = mWalkSpeed * (f32)sin(mWalkHeading);
			dir.z = mWalkSpeed * (f32)cos(mWalkHeading);
			dir.y = 0.0f;
			SetRotateY(mWalkHeading, mTurnSpeed);
		}
		SetObjVec(mMoveTranslate, mMoveTranslate + dir * frames);
		SetPos(mMoveTranslate, 0);
	}
	UpdateObject__4XObjFUl(this, frames);
	UpdateSmallSpiderWebEffect(&mFx, mFxIndex, frames, FALSE);
}

/**
 * @note Address: 0x800FEBB4
 * @note Size: 0x70
 */
void XObjSpiderSmall::Release()
{
	if (IsGameSoundPlaying(&gGame.mSound, -0xFFF4)) {
		StopGameSfx(&gGame.mSound, -0xFFF4);
	}
	XObj::Release();
}

// finds the stage's webs, the objects of kind 0x1BC
static inline void FindWebs()
{
	for (int i = 0; i < 12; i++) {
		gSmallSpiderEffectObjects[i]    = NULL;
		gSmallSpiderEffectSlotOwners[i] = -1;
	}
	lbl_8065B2E0 = 0;
	lbl_8065B2DC = NULL;
	s32 found    = 0;
	for (s32 i = 0; lbl_8065B2DC == NULL && i < gGame.mXObjs.mObjectCount; i++) {
		XObj* obj = gGame.mXObjs.mObjects[i];
		if ((obj->mStatus.mFlags & 1) && obj->mCoord.mPlacement->mObjectId == 0x1BC) {
			gSmallSpiderEffectObjects[found] = obj;
			gSmallSpiderEffectObjects[found]->SetAnim(0, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			found++;
			if (found >= 12) {
				break;
			}
		}
	}
}

/**
 * @note Address: 0x800FEC24
 * @note Size: 0x1AC
 */
void XObjSpiderSmall::ResetObjectState()
{
	ResetObjectState__4XObjFv(this);
	mAnimSfx.mEvents = gSmallSpiderAnimSfxEvents;
	mPrevBgm         = gGame.mSound.mMusicFadeOutFrames <= 0 ? gGame.mSound.mCurrentMusicId : 0;
	if (mPoolIndex == 0) {
		FindWebs();
	}
}

/**
 * @note Address: 0x800FEDD0
 * @note Size: 0x398
 */
// NONMATCHING: the spider pool loop folds 0x9B4 into the index
BOOL XObjSpiderSmall::LoadFromPlacement(const CoordInfo& info)
{
	if (LoadFromPlacement__4XObjFRC9CoordInfo(this, &info)) {
		if (mPoolIndex == 0) {
			for (int i = 0; i < 16; i++) {
				((XObjSpiderSmall*)gGame.mXObjs.mPools[18].Get(i))->Hide();
			}
			if (mTimeMode != gGame.mTimeState.mMode) {
				for (s32 i = 0; i < 0x27; i++) {
					for (int j = 0; j < 16; j++) {
						gSmallSpiderStatusByStage[i].mSpiders[j].mFlags = 0;
					}
				}
			}
			mTimeMode = gGame.mTimeState.mMode;
		}
		mStage = gGame.mStage;
		ResetSmallSpiderState(this);
		ApplySmallSpiderPlacement(this);
		mLoadPoolIndex = mPoolIndex;
		if (gSmallSpiderStatusByStage[mStage].mSpiders[mPoolIndex].mFlags & 0x80000) {
			SetState(2);
		}
		if (mPoolIndex == 0) {
			FindWebs();
		}
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x800FFDE0
 * @note Size: 0xDC
 */
void XObjSpiderSmall::ResetForNewGame()
{
	mStage = -1;
	ResetSmallSpiderState(this);
	SetState(0);
	mSpiderStatus.mFlags |= 0x4000000;
	mPrevBgm       = -1;
	mLoadPoolIndex = -1;
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
 * @note Address: 0x800FFEBC
 * @note Size: 0xE4
 */
void XObjSpiderSmall::InitializeObject(u32 id)
{
	InitializeObject__4XObjFUl(this, id);
	mStage = -1;
	ResetSmallSpiderState(this);
	SetState(0);
	mSpiderStatus.mFlags |= 0x4000000;
	mPrevBgm       = -1;
	mLoadPoolIndex = -1;
	if (mPoolIndex == 0) {
		for (s32 i = 0; i < 0x27; i++) {
			for (int j = 0; j < 16; j++) {
				gSmallSpiderStatusByStage[i].mSpiders[j].mFlags = 0;
			}
		}
	}
	mTimeMode = gGame.mTimeState.mMode;
}

static inline void SetSmallSpiderArea(XObjSpiderSmall* spider, f32 x, f32 y, f32 z, f32 width, f32 depth)
{
	spider->mAreaPos.x = x;
	spider->mAreaPos.y = y;
	spider->mAreaPos.z = z;
	spider->mAreaWidth = width;
	spider->mAreaDepth = depth;
}

static inline void SetSmallSpiderTarget(XObjSpiderSmall* spider)
{
	spider->mRadius       = 4.0f / 3.0f;
	spider->mMaxHits      = 3;
	spider->mHitsLeft     = spider->mMaxHits;
	spider->mTargetRadius = 96.0f;
	spider->mTargetPos    = spider->mHome;
}

/**
 * @note Address: 0x800FF168
 * @note Size: 0x998
 */
// NONMATCHING: retail keeps a second fdivs where this code reuses the quotient; different spill-store ordering
extern "C" void ApplySmallSpiderPlacement(XObjSpiderSmall* spider)
{
	spider->SetColor(1, 0, 0, 0, 0.0f);
	spider->mMoveTranslate.x = PlacementPosition(spider->mCoord, gSmallSpiderPosition).x;
	spider->mMoveTranslate.y = PlacementPosition(spider->mCoord, gSmallSpiderPosition).y;
	spider->mMoveTranslate.z = PlacementPosition(spider->mCoord, gSmallSpiderPosition).z;
	spider->mHome.x          = PlacementPosition(spider->mCoord, gSmallSpiderPosition).x;
	spider->mHome.y          = PlacementPosition(spider->mCoord, gSmallSpiderPosition).y;
	spider->mHome.z          = PlacementPosition(spider->mCoord, gSmallSpiderPosition).z;
	spider->mWalkHeading     = PlacementRotation(spider->mCoord, gSmallSpiderRotation).y;
	spider->mRadius          = 16.0f * PlacementScale(spider->mCoord, gSmallSpiderScale).x;
	// the placement's first param picks how it starts: 0 roams its area, 1 hides in its web, 2 waits for an event
	// to drop it in, 3 chases the robot
	switch (spider->mPlacementParam0) {
	case 0:
		spider->SetState(3);
		SetSmallSpiderTarget(spider);
		if (IsStage("stage07")) {
			SetSmallSpiderArea(spider, -102.85f, 0.0f, -335.27f, 150.0f, 134.0f);
		} else if (IsStage("stage01")) {
			if (spider->mHome.y > 70.0f) {
				SetSmallSpiderArea(spider, -306.38f, 75.84f, -33.31f, 80.0f, 100.0f);
			} else {
				SetSmallSpiderArea(spider, -150.97f, 0.0f, -100.32f, 150.0f, 134.0f);
			}
		} else if (IsStage("stage02")) {
			if (spider->mHome.x > 0.0f) {
				SetSmallSpiderArea(spider, 25.65f, 0.31f, 2.28f, 166.0f, 197.0f);
			} else {
				SetSmallSpiderArea(spider, -184.75f, 0.31f, -219.72f, 138.0f, 171.0f);
			}
		} else {
			SetSmallSpiderArea(spider, 2.97f, 0.0f, 47.15f, 180.0f, 180.0f);
		}
		TakeSmallSpiderSlot(spider);
		break;
	case 1:
		spider->SetState(5);
		SetSmallSpiderTarget(spider);
		TakeSmallSpiderSlot(spider);
		break;
	case 2:
		spider->SetState(6);
		SetSmallSpiderTarget(spider);
		TakeSmallSpiderSlot(spider);
		break;
	case 3:
		spider->SetState(8);
		SetSmallSpiderTarget(spider);
		TakeSmallSpiderSlot(spider);
		break;
	default:
		spider->SetState(0);
		spider->mSpiderStatus.mFlags |= 0x8000;
		break;
	}
}

/**
 * @note Address: 0x800FFB00
 * @note Size: 0x2E0
 */
// NONMATCHING: its Vec constants sit 0x18 lower in .rodata
extern "C" void ResetSmallSpiderState(XObjSpiderSmall* spider)
{
	spider->mWalkHeading         = 0.0f;
	spider->mWalkSpeed           = 0.0f;
	spider->_988                 = 0.0f;
	spider->mTurnSpeed           = 0.1f;
	spider->mFallSpeed           = 0.0f;
	spider->mScale               = 0.0f;
	spider->mTargetScale         = 0.0f;
	spider->mState               = 0;
	spider->mPrevState           = 0;
	spider->mStep                = 0;
	spider->mTimer               = 0;
	spider->mSpiderStatus.mFlags = 0;
	spider->_9C0                 = 0;
	spider->_9C4                 = 1.0f;
	spider->_9C8                 = 1.0f;
	Vec v0                       = { 0.0f, 0.0f, 0.0f };
	spider->mHitPos              = v0;
	Vec v1                       = { 0.0f, 0.0f, 0.0f };
	spider->mHome                = v1;
	Vec v2                       = { 0.0f, 0.0f, 0.0f };
	spider->_9F0                 = v2;
	Vec v3                       = { 0.0f, 0.0f, 0.0f };
	spider->mWalkTarget          = v3;
	spider->mHitStrength         = 0.0f;
	spider->mMaxHits             = 3;
	spider->mHitsLeft            = spider->mMaxHits;
	spider->mEvtArgs[0]          = 0;
	spider->mEvtArgs[1]          = 0;
	spider->_A48                 = 0;
	spider->_994                 = 1.0f;
	spider->mStatus.mFlags |= 8;
	spider->mStatus.mFlags |= 0x10000000;
	spider->mFx.mPos.x         = 0.0f;
	spider->mFx.mPos.y         = 0.0f;
	spider->mFx.mPos.z         = 0.0f;
	spider->mFx._10.x          = 0.0f;
	spider->mFx._10.y          = 0.0f;
	spider->mFx._10.z          = 0.0f;
	spider->mFx.mRotY          = 0.0f;
	spider->mFx._24            = 0.0f;
	spider->mFx.mState         = 0;
	spider->mFx._2C            = 0;
	spider->mFx.mStatus.mFlags = 0;
	spider->mRadius            = 16.0f;
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
	spider->_AA8            = 0;
}
