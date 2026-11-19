// chibi robo's tumble action, the GameCube's xacttumble.cpp plus the tumble helpers its
// xactradar.cpp carried: retail 0x801067C0-0x801078C8 (the tail of wii_units.md Nr. 67, right
// behind objb_act.cpp). ResetTumbleAction clears the idle effect where the GameCube cleared
// mIdleParticle; FinishTumbleAction is the GameCube's inline FinishTumble emitted out of line
// for the other units that call it, additionally retiring the sleep effect the Wii plays as an
// event effect; HandleTumbleActionInput reads the remote's buttons through gWiiPointer instead
// of the pad trigger. UpdatePlayerTumbleAnimation is the GameCube function of the same name
// with the aobj query replaced by the owner's IsAnimDone/GetAnimId virtuals and the state-10
// particle by an event effect instance.
#include "game/wii/xobjflags.h" // first: its AnimSfx needs the real nw4r::math, ahead of xobj.h's shim
#include "game/wii/xobj.h"
#include "game/wii/cgame.h"
#include "game/wii/cpadact.h"
#include "game/wii/xact.h"
#include "game/wii/ceffect.h"
#include "game/evt.h"
#include "game/stagedata.h"
#include "game/gmath.h"
#include <math.h>
#include "arithmetic.h"

extern "C" EffectInstance* fn_800A11A4(EffectInstance* effects, u32 id, s32 group); // takes a free slot, ids 5/6
extern "C" void fn_800A12F8(void* effects, u32 id, BOOL retireParticles);           // stops the event effect with that id

// how far the stick is pushed
static inline f32 StickLength(const CPadOne* pad)
{
	f32 y = pad->mStick.y;
	f32 x = pad->mStick.x;
	return FSqrt(x * x + y * y);
}

/**
 * @note Address: 0x801067C0
 * @note Size: 0x40
 */
extern "C" void ResetTumbleAction(XActTumble* act)
{
	act->Reset();
	act->mIdleEffect = NULL;
}

/**
 * @note Address: 0x80106800
 * @note Size: 0x8C
 */
// the GameCube's static inline FinishTumble, a real function on the Wii: XObjC's UpdateObject
// and XObjSpider::State3 call it, and it also retires the tumble sleep effect (event effect 5)
extern "C" void FinishTumbleAction(XActTumble* act)
{
	WiiPlayerState* owner = act->mOwner;
	owner->SetAnim(0x11, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
	owner->mIdleLoops = 3;
	act->mState       = 0;
	fn_800A12F8(&gGame.mEvtPtcl, 5, TRUE);
}

/**
 * @note Address: 0x8010688C
 * @note Size: 0x4D4
 */
extern "C" void HandleTumbleActionInput(XActTumble* act, const CPadOne* pad, f32* weight)
{
	// lying down (2, 16) a button or the stick gets him up; after the battery ran out (5) only a button
	if (act->mState == 2) {
		if (gGame.IsRunning() && gGame.mEvt.mData.mScript == NULL
		    && (gWiiPointer.Check(1) || gWiiPointer.Check(2) || StickLength(pad) > 0.0f)) {
			act->mState = 3;
		}
		*weight = 0.0f;
	} else if (act->mState == 16) {
		if (gGame.IsRunning() && gGame.mEvt.mData.mScript == NULL
		    && (gWiiPointer.Check(1) || gWiiPointer.Check(2) || StickLength(pad) > 0.0f)) {
			act->mState = 17;
		}
		*weight = 0.0f;
	} else if (act->mState == 5) {
		if (gGame.IsRunning() && gGame.mEvt.mData.mScript == NULL && (gWiiPointer.Check(1) || gWiiPointer.Check(2))) {
			act->mState = 6;
		}
		*weight = 0.0f;
	} else if (act->mState == 7) {
		*weight = 0.8f;
	} else if (act->mState != 0) {
		*weight = 0.0f;
	}
}

// the held-object checks objb_act.cpp carries file-local copies of; UpdatePlayerTumbleAnimation's
// item guards expand them the same way
static inline bool IsHolding(const XAction* act)
{
	bool result = false;
	if (act->mTarget != NULL && (act->mTarget->mStatus.mFlags & 0x400000000000ULL)) {
		result = true;
	}
	return result;
}

static inline bool IsHoldingPool(const XAction* act, s32 pool)
{
	bool result = false;
	if (IsHolding(act) && act->mTarget->GetPool() == pool) {
		result = true;
	}
	return result;
}

static inline bool IsHoldingKind(const XAction* act, s32 pool, s32 kind)
{
	bool result = false;
	if (IsHoldingPool(act, pool) && act->mTarget->GetObjFlags() == kind) {
		result = true;
	}
	return result;
}

// holding the item and not throwing it
static inline bool IsCarrying(const XAction* act, s32 kind)
{
	bool result = false;
	if (IsHoldingKind(act, 8, kind) && act->mTargetState != 15) {
		result = true;
	}
	return result;
}

static inline void PlayTumbleAnimation(WiiPlayerState* owner, s32 anim, f32 blend)
{
	owner->SetAnim(anim, FALSE, 0.0f, 1.0f, blend, 15.0f);
}

// the GameCube's static inline FinishTumble as the inlines below use it, without the Wii's
// event-effect retire FinishTumbleAction has
static inline void FinishTumble(XActTumble* act, f32 blend)
{
	WiiPlayerState* owner = act->mOwner;
	PlayTumbleAnimation(owner, 0x11, blend);
	owner->mIdleLoops = 3;
	act->mState       = 0;
}

// the tumble effect follows his model matrix; the GameCube spawned a particle on mJObj
static inline void StartTumble(XActTumble* act, s32 anim, WiiPlayerState* owner, s32 next)
{
	if (anim != 0x35 && anim != 0x10D) {
		PlayTumbleAnimation(act->mOwner, 0x35, 0.0f);
		MtxP mtx          = act->mOwner->GetWorldMtx();
		EffectInstance* e = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 6, 1);
		if (e != NULL) {
			e->mtx = mtx;
			UpdateEffectMtx(e);
		}
	} else if (owner->IsAnimDone()) {
		PlayTumbleAnimation(act->mOwner, 0x36, 1.0f);
		act->mState = next;
	}
}

static inline void RecoverTumble(XActTumble* act, s32 anim, WiiPlayerState* owner, s32 recovery, f32 blend)
{
	if (anim != recovery)
		PlayTumbleAnimation(act->mOwner, recovery, blend);
	else if (owner->IsAnimDone())
		FinishTumble(act, 1.0f);
}

/**
 * @note Address: 0x80106D60
 * @note Size: 0xB68
 */
// NONMATCHING: 99.75% - semantics match; the residue is pure codegen. In the state-10
// carry guard MWCC picks different callee registers (r5/r6/r7/r8 swaps, same code), and
// its stack layout puts the effect temporaries at rot@0x8 / pos@0x18 / offset@0x28 while
// this source orders them offset@0x8 / rot@0x14 / pos@0x20; no plausible declaration
// order tried produces the retail slots.
extern "C" void UpdatePlayerTumbleAnimation(XActTumble* act, s32 anim, WiiPlayerState* owner, u32 frames, f32 frame, f32 end)
{
	// 1 and 15 knock him over; 2 and 16 keep him down until a button or the idle timer, then 3 and
	// 17 get him up. 4 is the empty battery, 10 comes after five turns on the spot, 12 turns him to
	// face the camera and 13 back. The Wii carries no idle timer here: 16 goes on at once, and 14
	// only releases the pose, the idle particle is now an event effect handled outside
	switch (act->mState) {
	case 1:
		StartTumble(act, anim, owner, 2);
		break;
	case 3:
		RecoverTumble(act, anim, owner, 0x37, 1.0f);
		break;
	case 4:
		if (anim != 0x43)
			PlayTumbleAnimation(act->mOwner, 0x43, 0.0f);
		else if (owner->IsAnimDone()) {
			// battery empty: the stage's script takes over, otherwise he lies there until a button (5)
			if (gGame.GetStatusFlags(0x40000000) && GetStageBatteryDepletedScript() && gGame.GetPlayer()->mParams.mBattery <= 0.0f) {
				if (gGame.mEvt.mData.mScript == NULL)
					StartOrQueueEventScript(&gGame.mEvt, GetStageBatteryDepletedScript(), 0, -1, -1);
			} else {
				act->mState = 5;
			}
		}
		break;
	case 6:
		RecoverTumble(act, anim, owner, 0x1E, 1.0f);
		break;
	case 7:
		RecoverTumble(act, anim, owner, 0xB3, 0.0f);
		break;
	case 8:
		PlayTumbleAnimation(act->mOwner, 0xD4, 0.0f);
		act->mState = 9;
		break;
	case 9:
		if (owner->IsAnimDone())
			FinishTumble(act, 1.0f);
		break;
	case 10: {
		// the carried object picks the wake-up anim: the items he can put down (toothbrush 0x10,
		// spoon 0x15, squirter 0x38) take 0x166, another carried target 0x165; without a carry the
		// remote attachment takes 0x164, and the plain turn takes 0xE6
		if (act->mOwner->mLift.mState == 4) {
			if (IsCarrying(&act->mOwner->mLift, 0x10) || IsCarrying(&act->mOwner->mLift, 0x15) || IsCarrying(&act->mOwner->mLift, 0x38)) {
				PlayTumbleAnimation(act->mOwner, 0x166, 0.0f);
			} else {
				PlayTumbleAnimation(act->mOwner, 0x165, 0.0f);
			}
		} else if (act->mOwner->GetAttachment() == 2) {
			PlayTumbleAnimation(act->mOwner, 0x164, 0.0f);
		} else {
			PlayTumbleAnimation(act->mOwner, 0xE6, 0.0f);
		}
		// the snore effect the GameCube played as particle 0x10005, as a Wii event effect instance
		Vec offset          = { 0.0f, 12.0f, 0.0f };
		CVec pos            = *act->mOwner->GetPos() + offset;
		CVec* scale         = act->mOwner->GetScale();
		Vec rot             = { 0.0f, 0.0f, 0.0f };
		EffectInstance* inst = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 5, 1);
		if (inst != NULL) {
			inst->pos   = pos;
			inst->rot   = rot;
			inst->scale = *scale;
			UpdateEffectMtx(inst);
		}
		act->mState = 11;
		break;
	}
	case 11:
		{
			s32 id = owner->GetAnimId();
			if (id == 0xE6 || (u32)(id - 0x164) <= 2) {
				if (owner->IsAnimDone())
					FinishTumble(act, 0.0f);
			} else {
				fn_800A12F8(&gGame.mEvtPtcl, 5, TRUE);
				act->mState = 0;
			}
		}
		break;
	case 12:
		act->mSavedYaw = act->mOwner->mMoveRotate.y;
		act->mOwner->SetRotateY(PI + gGame.mMotionControlCamera.mView.mAngle.y, 12.0f * DEG2RAD);
		PlayTumbleAnimation(act->mOwner, 0x5D, 0.0f);
		act->mState = 14;
		break;
	case 13:
		act->mOwner->SetRotateY(act->mSavedYaw, 12.0f * DEG2RAD);
		PlayTumbleAnimation(act->mOwner, 0x5F, 0.0f);
		act->mState = 14;
		break;
	case 14:
		act->mIdleEffect = NULL;
		act->mState      = 0;
		break;
	case 15:
		StartTumble(act, anim, owner, 16);
		break;
	case 16:
		act->mState = 17;
		break;
	case 17:
		RecoverTumble(act, anim, owner, 0x37, 1.0f);
		break;
	}
}
