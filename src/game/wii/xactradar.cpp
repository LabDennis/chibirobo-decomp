// chibi robo's tumble fade and the ring on the floor around him, the GameCube's
// xactradar.cpp tail: retail 0x801078C8-0x80107D40, right behind the tumble actions in
// wii/xacttumble.cpp. The fade struct is the GameCube's, the draws go through the Wii
// CGs layer; HandleCopterActionInput reads the remote's B button through gWiiPointer
// where the GameCube read the pad trigger, and plays the descent animation on the
// copter object at _B580.
#include "game/wii/cgs.h"
#include "game/wii/cgame.h"
#include "game/wii/cpadact.h"
#include "game/wii/xact.h"
#include "game/wii/ceffect.h"
#include "game/globals.h"
#include "game/gmath.h"
#include <math.h>

extern "C" BOOL IsPlayerStopped(const WiiPlayerState* chr);
extern "C" void ShowAndDeployCopter(XObj* copter);
extern "C" void PlayCopterStowAnimation(XObj* copter);
extern "C" void PlayCopterDescentAnimation(XObj* copter);
extern "C" void PlayCopterLandingAnimation(XObj* copter);
extern "C" CVec GetPlayerFloorSlope(WiiPlayerState* chr);
extern "C" u32 GetCopterParameter(s32 index);
extern "C" s32 gRefreshRate;
extern "C" EffectInstance* fn_800A11A4(EffectInstance* effects, u32 id, s32 group); // takes a free slot; ids 0x4E-0x50 here
extern "C" void fn_80108BCC(XActPopperShot* shot);                                  // the shot's impact-detection pass (scan + floor probe)

// the tumble ring sits just above the floor
static inline Vec RingPos(const Vec& pos, const Vec& offset)
{
	Vec v = { pos.x + offset.x, 0.01f + pos.y, pos.z + offset.z };
	return v;
}

// retires the instance's effect when there is one
static inline void KillInstance(EffectInstance* inst)
{
	if (inst != NULL) {
		RetireEffect(&inst->handle, FALSE);
	}
}

// the GameCube's KillGen: on the Wii a generator is an event-effect instance, so the
// retail code shows the caller's own guard in front of the guarded call
static inline void KillGen(EffectInstance*& gen)
{
	if (gen != NULL) {
		KillInstance(gen);
		gen = NULL;
	}
}

// clears a shot and its effects, the GameCube's ResetShot
static inline void ResetShot(XActPopperShot* shot)
{
	shot->mActive    = FALSE;
	shot->mHitTarget = NULL;
	ClearParticlePool(&shot->mDust.mPtcl);
	shot->mRing[0].mAmount = 0.0f;
	shot->mRing[1].mAmount = 0.0f;
	KillGen(shot->mGen);
	KillGen(shot->mChargeGen);
}

extern "C" {

/**
 * @note Address: 0x801078C8
 * @note Size: 0xAC
 */
void UpdateTumbleFade(XActTumbleFade* fade, BOOL increase, u32 frames)
{
	if (fade->mActive) {
		if (increase) {
			if (fade->mAmount < 1.0f) {
				if ((fade->mAmount += 0.01f * frames) > 1.0f) {
					fade->mAmount = 1.0f;
				}
			}
		} else if (fade->mAmount > 0.0f) {
			if ((fade->mAmount -= 0.05f * frames) < 0.0f) {
				fade->mAmount = 0.0f;
				fade->mActive = FALSE;
			}
		}
	}
}

// the ring on the floor while he tumbles
/**
 * @note Address: 0x80107974
 * @note Size: 0x2DC
 */
// NONMATCHING: 97.31% - the CVec offset copy schedules as three batched loads then three
// stores in retail (z,y,x into f3,f2,f0), ours interleaves load/store pairs; some callee
// register colours differ in the loop constants. The ring loop itself matches, including
// the per-call rematerialization of the angle.
void DrawTumbleRing(XActTumbleFade* fade, const Vec& pos, f32 angle)
{
	if (fade->mAmount > 0.0f) {
		CVec offset(fade->mOffset);
		Mtx m;
		PSMTXRotRad(m, 'y', angle);
		PSMTXMultVec(m, &offset, &offset);
		Vec center = RingPos(pos, offset);
		f32 size   = 6.0f * fade->mAmount;
		u32 color  = (u32)(160.0f * fade->mAmount) | 0x641E00;
		Vec rot    = { -HALF_PI, 0.0f, 0.0f };
		DrawGsCircle(&gGame.mGs, 0x18, color, color, center.x, center.y, center.z, size, size, rot.x, rot.y, rot.z);
		PrimVertex verts[50];
		f32 outer = size * 0.5f + 1.0f;
		for (int i = 0; i < 25; i++) {
			verts[i * 2].pos.x     = center.x - size * std::sinf((TWO_PI / 24) * i) / 2.0f;
			verts[i * 2].pos.z     = center.z + size * std::cosf((TWO_PI / 24) * i) / 2.0f;
			verts[i * 2].pos.y     = center.y;
			verts[i * 2].color     = color;
			verts[i * 2 + 1].pos.x = center.x - outer * std::sinf((TWO_PI / 24) * i);
			verts[i * 2 + 1].pos.z = center.z + outer * std::cosf((TWO_PI / 24) * i);
			verts[i * 2 + 1].pos.y = center.y;
			verts[i * 2 + 1].color = 0;
		}
		DrawGsTriangleVertices(&gGame.mGs, GX_TRIANGLESTRIP, gGame.mGs.mCamera->mViewMtx, verts, 50);
	}
}

// the copter: B or standing still ends the flight
/**
 * @note Address: 0x80107C50
 * @note Size: 0xF0
 */
// NONMATCHING: 95.92% - GC/3.0a5 folds the 7||8 equality tests into a range check
// (subi r0, r4, 7 / cmplwi / bgt) where retail and the GameCube's GC/2.6 emit cmpwi 7,
// cmpwi 8; mState also colors r4 here against retail's r0, the GameCube build kept r4
// live through pad->mTrigger. A switch for the states compiled to a bge tree instead.
void HandleCopterActionInput(XAction* act, const CPadOne* pad, f32* weight)
{
	if (act->mState == 3) {
		if (gWiiPointer.Check(2) || IsPlayerStopped(act->mOwner)) {
			act->mState = 4;
			PlayCopterDescentAnimation((XObj*)act->mOwner->mCopterObj);
		}
		*weight = 1.0f;
	} else if (act->mState == 4 || act->mState == 7 || act->mState == 8) {
		if (!gWiiPointer.Check(2)) {
			IsPlayerStopped(act->mOwner);
		}
		*weight = 1.0f;
	} else if (act->mState != 0) {
		*weight = 0.0f;
	}
}

// the propeller joint's bob, the GameCube's UpdateCopterBobbing: the Wii moves the player
// model's "second_null" node where the GameCube walked a joint path
static inline void UpdateCopterBobbing(XActCopter* act, u32 frames, f32 speed)
{
	if (speed > 0.0f) {
		act->mPhase = fmod(speed * frames + act->mPhase, TWO_PI);
		act->mBob   = 0.4f * (f32)sin(act->mPhase) + 0.4f;
		act->mOwner->GetResNode("second_null").SetTranslate(0.0f, act->mBob, 0.0f);
	} else {
		if (act->mBob > 0.0f) {
			if ((act->mBob -= 0.2f * frames) <= 0.0f) {
				act->mBob = 0.0f;
			}
		}
		act->mOwner->GetResNode("second_null").SetTranslate(0.0f, act->mBob, 0.0f);
	}
}

// flies him while the copter is out
/**
 * @note Address: 0x80107D40
 * @note Size: 0x888
 */
void UpdatePlayerCopterFlight(XActCopter* act, s32 anim, WiiPlayerState* owner, u32 frames, f32 frame, f32 end)
{
	// the flight time (copter parameter 0, in hundredths of a second) fills up while idle and runs down in flight
	if (act->mState == 0) {
		act->mTime = gRefreshRate * ((s32)GetCopterParameter(0) / 100.0f);
	} else if (act->mState == 3) {
		if ((act->mTime -= frames) < 0.0f) {
			act->mTime  = 0.0f;
			act->mState = 4;
		}
	}
	// 1 takes the copter out, 2 puts it away, 3 flies. 4 and 5 come down and 6 lands, 7 (off a rope)
	// and 8 (off a ledge) come down and 9 lands; on the Wii the anim-done flag is the player's
	// IsAnimDone where the GameCube read the aobj flags
	switch (act->mState) {
	case 1:
		if (anim != 0x814) {
			act->mOwner->SetAnim(0x814, TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
			ShowAndDeployCopter((XObj*)act->mOwner->mCopterObj);
		} else if (owner->IsAnimDone()) {
			act->mOwner->SetAnim(0x4A, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			act->mState = 0;
		}
		break;
	case 2:
		if (anim != 0x817) {
			act->mOwner->SetAnim(0x817, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			PlayCopterStowAnimation((XObj*)act->mOwner->mCopterObj);
		} else if (owner->IsAnimDone()) {
			WiiPlayerState* chr = act->mOwner;
			chr->SetAnim(0x11, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
			chr->mIdleLoops = 3;
			act->mState     = 0;
		}
		break;
	case 3:
		UpdateCopterBobbing(act, frames, DEG2RAD * 5);
		break;
	case 4:
		UpdateCopterBobbing(act, frames, -1.0f);
		if (anim != 0x28) {
			act->mOwner->SetAnim(0x28, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			PlayCopterDescentAnimation((XObj*)act->mOwner->mCopterObj);
		}
		if (act->mOwner->GetHeight() <= 0.1f) {
			act->mOwner->SetAnim(0x816, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
			PlayCopterLandingAnimation((XObj*)act->mOwner->mCopterObj);
			act->mState = 6;
		}
		break;
	case 5:
		UpdateCopterBobbing(act, frames, -1.0f);
		if (anim != 0x28) {
			act->mOwner->SetAnim(0x28, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			((XObj*)act->mOwner->mCopterObj)->SetAnim(0, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
		}
		if (act->mOwner->GetHeight() <= 0.0f || (act->mOwner->GetHeight() <= 1.0f && GetPlayerFloorSlope(act->mOwner).Length() > 0.5f)) {
			act->mOwner->SetAnim(0x816, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
			PlayCopterLandingAnimation((XObj*)act->mOwner->mCopterObj);
			act->mState = 6;
		}
		break;
	case 6:
		UpdateCopterBobbing(act, frames, -1.0f);
		if (owner->IsAnimDone()) {
			WiiPlayerState* chr = act->mOwner;
			chr->SetAnim(0x11, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
			chr->mIdleLoops = 3;
			act->mState     = 0;
		}
		break;
	case 7:
		if (anim != 0x38) {
			act->mOwner->SetAnim(0x38, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			PlayCopterDescentAnimation((XObj*)act->mOwner->mCopterObj);
		}
		if (act->mOwner->GetHeight() <= 0.1f) {
			act->mOwner->SetAnim(0x59, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
			PlayCopterLandingAnimation((XObj*)act->mOwner->mCopterObj);
			act->mState = 9;
		}
		break;
	case 8:
		if (anim != 0x1A) {
			act->mOwner->SetAnim(0x1A, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			PlayCopterDescentAnimation((XObj*)act->mOwner->mCopterObj);
		}
		if (act->mOwner->GetHeight() <= 0.1f) {
			act->mOwner->SetAnim(0x1B, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
			PlayCopterLandingAnimation((XObj*)act->mOwner->mCopterObj);
			act->mState = 9;
		}
		break;
	case 9:
		if (owner->IsAnimDone()) {
			WiiPlayerState* chr = act->mOwner;
			chr->SetAnim(0x11, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
			chr->mIdleLoops = 3;
			act->mState     = 0;
		}
		break;
	}
}

// resets one shot and readies its pool and sound actors, the per-shot body of the
// GameCube's InitializePlayerPopperAction loop on a function of its own
/**
 * @note Address: 0x801085C8
 * @note Size: 0x140
 */
void InitPopperShot(XActPopperShot* shot)
{
	ResetShot(shot);
	InitParticlePool(&shot->mDust.mPtcl, shot->mDust.mPtcls, sizeof(CPtclDust2), 40);
	shot->mRing[0].mAmount = 0.0f;
	shot->mRing[1].mAmount = 0.0f;
	shot->mGen             = NULL;
	shot->mChargeGen       = NULL;
	for (s32 i = 0; i < 2; i++) {
		shot->mSfxActors[i].Reset();
	}
	for (s32 i = 0; i < 2; i++) {
		shot->mSfxActors[i].Create();
	}
}

/**
 * @note Address: 0x80108708
 * @note Size: 0x88
 */
void ResetPopperShot(XActPopperShot* shot)
{
	ResetShot(shot);
}

// fires one shot; kind 1 is the small one. Same field order as the GameCube version:
// on the Wii the joint-attached particles are event-effect instances (pos/rot zero,
// scale one, like the tumble effects) and the launch sound plays through both the
// shot's 3D actor and the flat PlayWiiSoundEffect while the archive mReady flag is 2
/**
 * @note Address: 0x80108790
 * @note Size: 0x43C
 */
// NONMATCHING: 68.27% - the semantics match retail field-for-field (same ids, same TRS
// {0,0,0}/{0,0,0}/{1,1,1} effect writes, same mReady=2/PlayWiiSoundEffect/mReady=1,
// same actor detail_StartSound + SetVolume, same trailing impact-detection call). The
// residue is stack-frame codegen: retail keeps a bytewise Vec stack copy of pos
// (stw-bit-copy + lfs/frsp reload into mPos) plus two separate TRS triples for the
// two effect calls - frame 0xC0 with f28-f31 + r28-r31 saved; this source compiles to
// a 0x70 frame with shared temporaries. Tested: explicit CVec(pos) temp and
// per-call-site Vec triples, both scored lower.
void StartPopperShot(XActPopperShot* shot, const Vec& pos, s32 kind, XObj* target, f32 yaw, f32 pitch, f32 power, f32 speed, f32 range,
                     f32 strength)
{
	shot->mActive    = TRUE;
	shot->mHitTarget = NULL;
	shot->mPos       = pos;
	shot->mYaw       = yaw;
	shot->mPitch     = pitch;
	shot->mSpeed     = speed;
	shot->mDistance  = 0.0f;
	shot->mRange     = range;
	shot->_28        = power;
	for (int i = 0; i < 10; i++) {
		shot->mHistory[i].x = shot->mPos.x;
		shot->mHistory[i].y = shot->mPos.y;
		shot->mHistory[i].z = shot->mPos.z;
	}
	shot->mRing[0].mPos.x  = pos.x;
	shot->mRing[0].mPos.y  = pos.y;
	shot->mRing[0].mPos.z  = pos.z;
	shot->mRing[0].mAmount = 0.9f;
	shot->mRing[0].mRadius = 100.0f;
	shot->mRing[0].mColor  = 0x00FF0096;
	shot->mStrength        = strength;
	shot->mKind            = kind;
	if (target != NULL) {
		shot->mHomingTarget = target;
	} else {
		shot->mHomingTarget = NULL;
	}
	Vec zero   = { 0.0f, 0.0f, 0.0f };
	Vec unit   = { 1.0f, 1.0f, 1.0f };
	shot->mGen = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, kind == 1 ? 0x4E : 0x4F, 1);
	if (shot->mGen != NULL) {
		shot->mGen->pos   = zero;
		shot->mGen->rot   = zero;
		shot->mGen->scale = unit;
		UpdateEffectMtx(shot->mGen);
	}
	if (strength >= (s32)GetPlayerActionParameter(31) / 100.0f) {
		shot->mChargeGen = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x50, 1);
		if (shot->mChargeGen != NULL) {
			shot->mChargeGen->pos   = zero;
			shot->mChargeGen->rot   = zero;
			shot->mChargeGen->scale = unit;
			UpdateEffectMtx(shot->mChargeGen);
		}
	}
	f32 full             = (s32)GetPlayerActionParameter(31) / 100.0f;
	s32 sfx              = strength >= full ? (kind == 1 ? 0x200 : 0x203) : (kind == 1 ? 0x78 : 0x202);
	CSound3DActor* actor = shot->mSfxActors[0].mActor;
	if (actor != NULL && actor->detail_StartSound(&actor->mHandle, sfx, NULL) == nw4r::snd::SoundStartable::START_SUCCESS) {
		actor->mHandle.SetVolume(1.0f, 0);
	}
	gGame.mSound.mArchivePlayer.mReady = 2;
	PlayWiiSoundEffect(&gGame.mSound, sfx, 1.0f, 0.0f);
	gGame.mSound.mArchivePlayer.mReady = 1;
	fn_80108BCC(shot);
}

} // extern "C"
