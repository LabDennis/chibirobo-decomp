#include "game/wii/xobjflags.h" // first: its AnimSfx needs the real nw4r::math, ahead of xobj.h's shim
#include "game/wii/xobj.h"
#include "game/wii/animsfx.h"
#include "game/wii/cgame.h"
#include "game/wii/csound.h"
#include "game/coordinfo.h"
#include <math.h>
#include <stdlib.h>

// the Wii coin and item objects and the sound objects next to them; the unit also owns the
// XObjHBox part forwards behind the coin code. The GameCube's xobjcoinupdate.cpp covers the
// coin half of this; here the sound, item and hbox pieces share the file.

extern "C" u32 GetPickupRewardValue(s32 index);
extern "C" void AdjustPlayerMoney(WiiPlayerParams* params, s32 amount);
extern "C" void AddPlayerScrap(WiiPlayerParams* params, s32 amount);
extern "C" void AdjustPlayerExperience(WiiPlayerParams* params, s32 amount);
extern "C" u32 GetStageCollectionFlagBase(s32 set);
extern "C" void* GetSpawnData(s32 id);
extern "C" f32 GetSoundVolumeAtPosition(XSound* sound, const Vec* pos, s32 which);
extern "C" void PlayGameSound(XSound* sound, s32 id, s32 volume);

extern const AnimSfxEvent lbl_80307DA8[]; // the kind 0x1D2 sound objects' event table
extern u32 lbl_8045DB20[];                // the game's coin handle table, indexed by mHandleSlot

extern "C" {
void* fn_800A11A4(void* particle, u32 code, s32 status);
void fn_802BEABC(void* spawned);
}

// what the Wii's particle spawn hands back (fn_800A11A4); the caller fills its TRS
// before fn_802BEABC registers it. the coin's milestone effects also take a joint's
// world matrix at _60
struct SpawnedEffect {
	u8 _00[0xC];
	Vec mTranslate; // _0C
	Vec mRotate;    // _18
	Vec mScale;     // _24
	u8 _30[0x30];
	MtxP mWorldMtx; // _60
};

// the money pickups' milestone sparkle effects, indexed by GetItem(); retail keeps the
// table in .data (0x803080B8): copper, silver, gold
static const struct {
	s32 mEffect;
	const char* mJoint;
} sCoinSparkles[3] = {
	{ 14, "coin_gold_body1" },
	{ 16, "coin_silver_body1" },
	{ 17, "coin_copper_body1" },
};

static inline bool IsCoinItemInRange(XObjCoin* coin, s32 first, s32 end)
{
	bool inRange = false;
	if (coin->GetItem() >= first && coin->GetItem() < end)
		inRange = true;
	return inRange;
}

/**
 * @note Address: 0x801141C0
 * @note Size: 0x3C
 */
void XObjSound::InitializeObject()
{
	XObj::InitializeObject();
	mAnimSfx.Init((WiiPlayerState*)this, NULL);
}

/**
 * @note Address: 0x801141FC
 * @note Size: 0x70
 */
void XObjSound::ResetObjectState()
{
	XObj::ResetObjectState();
	_A04 = 0;
	_A08 = 0;
	if (GetKind() == 0x1D2) // the kind carries its own event table
		mAnimSfx.Init((WiiPlayerState*)this, (AnimSfxEvent*)lbl_80307DA8);
}

/**
 * @note Address: 0x8011426C
 * @note Size: 0x54
 */
void XObjSound::UpdateObject(u32 frames)
{
	XObj::UpdateObject(frames);
	SoundUpdate(GetAnimFrame());
}

/**
 * @note Address: 0x801142C0
 * @note Size: 0xB0
 * @note NONMATCHING: retail calls the AnimSfx::Update vtable slot with the volume in f2 as a
 * float; the declared Update(s32, f32, u8) takes it in r5, so the original unit must have seen
 * a different declaration of the parameter's type.
 */
void XObjSound::SoundUpdate(f32 frame)
{
	if ((mStatus.mFlags & 1) && (mStatus.mFlags & 0x10000)) {
		f32 volume = GetSoundVolumeAtPosition(&gGame.mSound, GetPos(), 0);
		mAnimSfx.Update(GetAnimId(), frame, volume);
	}
}

/**
 * @note Address: 0x80114370
 * @note Size: 0x14
 */
void XObjSound::ResumeSounds(f32 fade)
{
	mAnimSfx.Resume(fade);
}

/**
 * @note Address: 0x80114384
 * @note Size: 0x14
 */
void XObjSound::PauseSounds(f32 fade)
{
	mAnimSfx.Pause(fade);
}

/**
 * @note Address: 0x801143A0
 * @note Size: 0x44
 */
s32 XObjItem::GetFlagIndex()
{
	s32 index = mPlacementParam1;
	return index > 0 ? GetStageCollectionFlagBase(0) + index : 0;
}

/**
 * @note Address: 0x801143E4
 * @note Size: 0x34
 */
void XObjItem::ResetObjectState()
{
	XObj::ResetObjectState();
	mAnimId2 = -1;
}

/**
 * @note Address: 0x80114418
 * @note Size: 0xC4
 */
BOOL XObjItem::LoadFromPlacement(const CoordInfo& info)
{
	BOOL result = XObj::LoadFromPlacement(info);
	if (result && IsFlagSet())
		mStatus.mFlags &= ~1ULL;
	return result;
}

/**
 * @note Address: 0x801144DC
 * @note Size: 0x88
 */
void XObjItem::UpdateObject(u32 frames)
{
	XObj::UpdateObject(frames);
	if (mAnimId2 >= 0 && IsAnimDone()) {
		SetAnim(mAnimId2, TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
		mAnimId2 = -1;
	}
}

/**
 * @note Address: 0x80114564
 * @note Size: 0x1DC
 * @note NONMATCHING: the kind table and the anim arguments were verified against the retail
 * compare chain (0x398..0x39A fall into the anim-1 group, only 0x397 adds 3 to mAnimId2);
 * still unbuilt, kept NonMatching until the unit is wired up and proven.
 */
void XObjItem::SetUseAnim()
{
	switch (GetKind()) {
	case 0x90:
	case 0x135:
	case 0x136:
	case 0x1CA:
	case 0x1CB:
	case 0x283:
	case 0x398:
	case 0x399:
	case 0x39A:
		if (mAnimId2 < 0)
			mAnimId2 = GetAnimId();
		SetAnim(1, TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
		break;
	case 0x1B5:
		if (mAnimId2 < 0)
			mAnimId2 = 0;
		SetAnim(3, TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
		break;
	case 0x342:
		if (mAnimId2 < 0)
			mAnimId2 = 0;
		SetAnim(2, TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
		break;
	case 0x397:
		if (mAnimId2 < 0)
			mAnimId2 = GetAnimId();
		SetAnim(mAnimId2 + 3, TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
		break;
	}
}

/**
 * @note Address: 0x80114740
 * @note Size: 0x54
 */
void XObjCoin::InitializeObject()
{
	XObj::InitializeObject();
	for (s32 i = 0; i < 2; i++)
		mActors[i].Reset();
}

/**
 * @note Address: 0x80114794
 * @note Size: 0x44
 */
s32 XObjCoin::GetFlagIndex()
{
	s32 index = mPlacementParam1;
	return index > 0 ? GetStageCollectionFlagBase(1) + index : 0;
}

/**
 * @note Address: 0x801147D8
 * @note Size: 0x170
 */
void XObjCoin::ResetObjectState()
{
	XObj::ResetObjectState();
	s32 sign = (rand() & 1) ? 1 : -1;
	mPhase   = 0.13962634f; // 8 degrees
	mSpin    = sign * 0.017453292f * (f32)(rand() % 360);
	if (GetAnimEndFrame() > 0.0f) {
		f32 frame = (f32)fmod((f64)rand(), (f64)GetAnimEndFrame());
		SetAnim(mCoord.mPlacement->mStartAnim, TRUE, frame, 1.0f, 1.0f, 15.0f);
	}
	mFade           = 0.0f;
	lbl_8045DB20[0] = 0;
	mHandleSlot     = -1;
	mState          = 0;
	mParam0         = 0;
	mBouncing       = 0;
}

/**
 * @note Address: 0x80114948
 * @note Size: 0x128
 */
BOOL XObjCoin::LoadFromPlacement(const CoordInfo& info)
{
	mPlacementParam1 = info.mPlacement->mParam1;
	if (IsFlagSet())
		return TRUE;
	if (!XObj::LoadFromPlacement(info))
		return FALSE;
	mStatus.mFlags |= 0x10000000;
	for (s32 i = 0; i < 2; i++)
		mActors[i].Create();
	return TRUE;
}

/**
 * @note Address: 0x80114A70
 * @note Size: 0xAC
 */
void XObjCoin::Release()
{
	XObj::Release();
	for (s32 i = 0; i < 2; i++)
		mActors[i].Release();
}

/**
 * @note Address: 0x80114B1C
 * @note Size: 0xC2C
 * @note NONMATCHING: the collected-money branch's timed sound events and the two sound
 * actors' per-frame position/fade updates are only partially decoded; the structure below
 * follows the GameCube's file for the shared parts and the observed retail flow for the rest.
 */
void XObjCoin::UpdateObject(u32 frames)
{
	const f32 radians = 0.017453292f; // degrees to radians
	if (IsCoinItemInRange(this, 0, 100)) {
		if (mPhase > 0.13962634f) {           // spins fast for the first while, then settles at 8 degrees
			mPhase -= 0.0034906585f * frames; // 0.2 degrees a frame
			if (mPhase < 0.13962634f)
				mPhase = 0.13962634f;
		}
		f32 speed;
		if (mSpin >= 0.0f) {
			if (mPhase == 0.13962634f)
				mSpin += 0.06981317f; // 4 degrees
			speed = (1.5f + (f32)sin(mSpin)) * 0.5f;
		} else {
			if (mPhase == 0.13962634f)
				mSpin -= 0.06981317f;
			speed = -(1.5f + (f32)sin(mSpin)) * 0.5f;
		}
		mMoveRotate.y += mPhase * speed * frames;
	} else if (IsCoinItemInRange(this, 100, 200)) {
		if (mPhase > 0.034906585f) { // scrap coins settle at 2 degrees
			mPhase -= 0.0034906585f * frames;
			if (mPhase < 0.034906585f)
				mPhase = 0.034906585f;
		}
		if (mSpin >= 0.0f)
			mMoveRotate.y += mPhase * frames;
		else
			mMoveRotate.y -= mPhase * frames;
	}
	XObj::UpdateObject(frames);
	if (mStatus.mFlags & 0x4000000000000000ULL) { // picked up
		if (IsCoinItemInRange(this, 0, 100)) {
			switch (GetAnimId()) {
			case 0:
				SetAnim(2, TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
				break;
			case 1:
				break;
			case 2:
				if (IsAnimDone())
					Release();
				break;
			}
		} else {
			if (mMoveScale == (Vec) { 0.0f, 0.0f, 0.0f })
				Release();
			if ((mFade -= 0.02f * frames) <= 0.0f)
				mFade = 0.0f;
			PSVECScale(&mMoveScale, &mMoveScale, 1.12f * mFade);
		}
	}
	if (mParam0 == 1) { // a dropped coin gets its heading and fall speed
		mState      = 1;
		mFallFrames = 0;
		mParam0     = 0;
		mStatus.mFlags |= 0x100000;
		mFallSpeed = (f32)(s32)mParams[0] / 100.0f;
		mHeading   = radians * (s32)mParam1;
	}
	UpdateFallingCoin(this, frames);
	if (IsCoinItemInRange(this, 0, 100)) {
		// the pickup sparkle effects ride on anim-frame milestones of the two spins
		s32 item = GetItem();
		switch (GetAnimId()) {
		case 1:
			if ((mLastAnimFrame < 2.0f && GetAnimFrame() >= 2.0f) || (mLastAnimFrame < 8.0f && GetAnimFrame() >= 8.0f) ||
			    (mLastAnimFrame < 14.0f && GetAnimFrame() >= 14.0f) || (mLastAnimFrame < 20.0f && GetAnimFrame() >= 20.0f) ||
			    (mLastAnimFrame < 26.0f && GetAnimFrame() >= 26.0f) || (mLastAnimFrame < 32.0f && GetAnimFrame() >= 32.0f) ||
			    (mLastAnimFrame < 38.0f && GetAnimFrame() >= 38.0f) || (mLastAnimFrame < 44.0f && GetAnimFrame() >= 44.0f) ||
			    (mLastAnimFrame < 50.0f && GetAnimFrame() >= 50.0f) || (mLastAnimFrame < 56.0f && GetAnimFrame() >= 56.0f) ||
			    (mLastAnimFrame < 64.0f && GetAnimFrame() >= 64.0f)) {
				MtxP mtx                 = GetWorldMtx(sCoinSparkles[item].mJoint);
				SpawnedEffect* spawned = (SpawnedEffect*)fn_800A11A4(&gGame.mEvtPtcl, sCoinSparkles[item].mEffect, 0);
				if (spawned != NULL) {
					spawned->mWorldMtx = mtx;
					fn_802BEABC(spawned);
				}
			}
			break;
		case 2:
			if ((mLastAnimFrame < 2.0f && GetAnimFrame() >= 2.0f) || (mLastAnimFrame < 11.0f && GetAnimFrame() >= 11.0f) ||
			    (mLastAnimFrame < 17.0f && GetAnimFrame() >= 17.0f) || (mLastAnimFrame < 20.0f && GetAnimFrame() >= 20.0f)) {
				MtxP mtx                 = GetWorldMtx(sCoinSparkles[item].mJoint);
				SpawnedEffect* spawned = (SpawnedEffect*)fn_800A11A4(&gGame.mEvtPtcl, sCoinSparkles[item].mEffect, 0);
				if (spawned != NULL) {
					spawned->mWorldMtx = mtx;
					fn_802BEABC(spawned);
				}
			}
			break;
		}
	}
	// a finished bounce sound gives its claim slot back
	if (mHandleSlot >= 0 && mActors[1].GetHandle() == NULL) {
		lbl_8045DB20[mHandleSlot] = 0;
		mHandleSlot             = -1;
	}
	for (s32 i = 0; i < 2; i++) {
		mActors[i].Update(GetPos());
	}
}

/**
 * @note Address: 0x80115748
 * @note Size: 0x56C
 * @note NONMATCHING: same GameCube flow; the Wii tail around the actors' handles, the
 * CEvtPtcl effect at gGame.mEvtPtcl and the picked-up sound's cleanup is not fully decoded.
 */
s32 XObjCoin::OnPlayerContact(s32 unused, s32 value)
{
	if ((value & 3U) == 1)
		return value;
	if (mStatus.mFlags & 0x100000) // still falling, not collectible yet
		return 0;

	WiiPlayerState* player = gGame.GetPlayer();
	((XObj*)player)->SetColor(1, 0, 120, 120, 0.0f);
	((XObj*)player)->SetColor(1, 0, 0, 0, 0.2f);
	mStatus.mFlags |= 0x4000000000000000ULL;
	SetFlag();

	s32 sound  = -1;
	s32 effect = -1;
	if (IsCoinItemInRange(this, 0, 100)) { // items 0..99 are money
		AdjustPlayerMoney(&player->mParams, GetPickupRewardValue(GetItem()));
		sound = 0xf6;
	} else if (IsCoinItemInRange(this, 100, 200)) { // items 100..199 are scrap
		s32 scrap = GetPickupRewardValue(GetItem() - 97);
		if (!scrap) {
			AttrInfo attr(CoordInfo(GetSpawnData(GetAnimId() + 41)));
			scrap = GetPickupRewardValue(attr.mObjData->GetItem() - 97);
		}
		AddPlayerScrap(&player->mParams, scrap);
		sound = 0x190;
	} else if (IsCoinItemInRange(this, 200, 300)) { // items 200..299 are battery
		s32 reward             = GetPickupRewardValue(GetItem() - 193);
		WiiPlayerParams& stats = player->mParams;
		stats.SetBattery(reward + stats.mBattery);
		sound  = 0xfb;
		effect = 20;
	} else if (IsCoinItemInRange(this, 300, 400)) { // items 300..399 are experience
		AdjustPlayerExperience(&player->mParams, GetPickupRewardValue(GetItem() - 292));
		sound = 0x18a;
	}
	if (effect >= 0) {
		Vec rotation           = { 0.0f, 0.0f, 0.0f };
		SpawnedEffect* spawned = (SpawnedEffect*)fn_800A11A4(&gGame.mEvtPtcl, effect, 1);
		if (spawned != NULL) {
			spawned->mTranslate = mMoveTranslate;
			spawned->mRotate    = rotation;
			spawned->mScale     = mMoveScale;
			fn_802BEABC(spawned);
		}
	}
	if (sound >= 0 && mActors[0].GetHandle() == NULL) {
		CSound3DActor* actor = mActors[0].mActor;
		if (actor != NULL && actor->detail_StartSound(&actor->mHandle, sound, NULL) == nw4r::snd::SoundStartable::START_SUCCESS) {
			actor->mHandle.SetVolume(1.0f, 0);
		}
	}
	mFade = 1.0f;
	return 0;
}

/**
 * @note Address: 0x80115CB4
 * @note Size: 0x7C
 */
s32 XObjCoin::HandleEventCommand(s32 count, s32* args)
{
	if (count < 1)
		return 0;
	mParam0 = args[0];
	if (count < 2)
		mParam1 = 0;
	else
		mParam1 = args[1];
	// the other eight go over whatever the count
	mParams[0] = args[2];
	mParams[1] = args[3];
	mParams[2] = args[4];
	mParams[3] = args[5];
	mParams[4] = args[6];
	mParams[5] = args[7];
	mParams[6] = args[8];
	mParams[7] = args[9];
	return 0;
}

static inline void SetPosAndTarget(XObjCoin* coin, const Vec& value)
{
	coin->mMoveTranslate.Set(value);
	coin->mMoveTarget0.Set(value);
}

/**
 * @note Address: 0x80115D30
 * @note Size: 0x1E4
 */
extern "C" void UpdateFallingCoin(XObjCoin* coin, u32 frames)
{
	CVec step;
	switch (coin->mState) {
	case 0:
		break;
	case 1:
		++coin->mFallFrames;
		coin->mFallSpeed -= 0.09f * frames;
		coin->mMoveTranslate.y += coin->mFallSpeed;
		if (coin->mMoveTranslate.y <= 0.0f) {
			coin->mFallSpeed       = 0.0f;
			coin->mMoveTranslate.y = 0.0f;
			coin->mState           = 0;
			coin->mStatus.mFlags &= ~0x100000ULL;
		}
		if (coin->mFallSpeed >= -2.3f) {
			coin->mFallSpeed -= 0.36f * frames;
		} else {
			coin->mFallSpeed = -2.3f;
		}
		step.x = 0.4f * (f32)sin(coin->mHeading);
		step.z = 0.4f * (f32)cos(coin->mHeading);
		step.y = 0.0f;
		SetPosAndTarget(coin, coin->mMoveTranslate + step * (f32)frames);
		coin->SetPos(coin->mMoveTranslate);
		break;
	}
}

/**
 * @note Address: 0x80115F14
 * @note Size: 0x5C4
 * @note NONMATCHING: the pointer flick that makes a coin bounce. The first guard reads as a
 * status check, but retail dispatches it through *(gGame+0x41E4) slot _40 with this=&gGame,
 * r4=pointer and the mask in r5:r6 — GetStatusFlags(u64) has only the mask arg, so the real
 * callee's first parameter is still unknown. The two action flags (+0x2D0 of action 1,
 * +0x11F8 of action 7) and the first bounce constant's exact roles are not modelled.
 */
void XObjCoin::vf208(const Vec* pointer, const Vec* motion, BOOL over, f32 speed)
{
	WiiPlayerState* player;
	CSound3DActor* actor;
	if (gGame.GetStatusFlags(0x100))
		goto end;
	player = gGame.GetPlayer();
	if (player->GetAttachment() == 2)
		goto end;
	if (gGame.mMotionControlCamera.IsAction(1) && *(u8*)((u8*)gGame.mCameraActions.mTable[1] + 0x2D0))
		goto end;
	if (gGame.mMotionControlCamera.IsAction(7) && *(u8*)((u8*)gGame.mCameraActions.mTable[7] + 0x11F8))
		goto end;
	if (_6DC == 0) // the pointer has to have swept across the bounds this frame
		goto end;
	if (!IsCoinItemInRange(this, 0, 100) && !IsCoinItemInRange(this, 100, 200))
		goto end;
	{
		const f32 radians = 0.017453292f;
		f32 absX          = (f32)fabs(motion->x);
		if (absX < 0.002f) // a flick has to pass the dead zone
			goto end;
		if (absX > 0.05f)
			absX = 0.05f;
		// the flick's direction picks the spin's reset angle: 90 or -270 degrees
		mSpin = motion->x < 0.0f ? 1.5707964f : -4.712389f;
		if (IsCoinItemInRange(this, 0, 100)) {
			f32 strength = radians * (400.0f * absX + 8.0f);
			if (strength < 0.0f || strength > mPhase)
				mPhase = strength;
		} else {
			f32 strength = radians * (800.0f * absX + 2.0f);
			if (strength < 0.0f || strength > mPhase)
				mPhase = strength;
		}
		if (absX < 0.015f) // the bounce sound needs a stronger flick than the tilt
			goto end;
		if (IsCoinItemInRange(this, 0, 100)) {
			if (mActors[1].GetHandle() != NULL)
				goto end;
			actor = mActors[1].mActor;
			if (actor == NULL)
				goto end;
			if (actor->detail_StartSound(&actor->mHandle, 0x227, NULL) == nw4r::snd::SoundStartable::START_SUCCESS)
				actor->mHandle.SetVolume(1.0f, 0); // lbl_805A5404
		} else {
			if (mActors[1].GetHandle() != NULL)
				goto end;
			actor = mActors[1].mActor;
			if (actor == NULL)
				goto end;
			// five bounce sounds share a global claim table, one slot each
			s32 start = rand() % 5;
			for (s32 i = 0; i < 5; i++) {
				s32 idx = (start + i) % 5;
				if (lbl_8045DB20[idx] == 0) {
					lbl_8045DB20[idx] = 1;
					mHandleSlot       = idx;
					if (actor->detail_StartSound(&actor->mHandle, idx + 0x228, NULL) == nw4r::snd::SoundStartable::START_SUCCESS)
						actor->mHandle.SetVolume(1.0f, 0);
					break;
				}
			}
		}
	}
end:
	XObj::vf208(pointer, motion, over, speed);
}

/**
 * @note Address: 0x801164D8
 * @note Size: 0x40
 */
void XObjHBox::InitializeObject()
{
	XObj::InitializeObject();
	mPart.mObj.InitializeObject();
}

/**
 * @note Address: 0x80116518
 * @note Size: 0x10
 */
void XObjHBox::ResetForNewGame()
{
	mPart.mObj.ResetForNewGame();
}
