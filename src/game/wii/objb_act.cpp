// chibi robo's actions, the GameCube's objb_act.cpp: .text 0x800F48E0-0x801067C0 (wii_units.md Nr. 67). The
// functions come in the reverse of the GameCube's order, GetTargetFloorPosition first and UpdatePushedObject
// last; ResetTumbleAction (the GameCube's xacttumble.cpp) follows the file.
// NONMATCHING: the small helpers have source; the big per-action updates (lift, held object, hang, jump,
// ladder, rope, push) and the input handlers are still to port. The Wii rewrote them for the nw4r models
// and the Wii remote, so the GameCube's src/game/objb_act.cpp is a template for them, not a copy.
#include "game/wii/xobjflags.h" // first: its AnimSfx needs the real nw4r::math, ahead of xobj.h's shim
#include "game/wii/xobj.h"
#include "game/wii/cgame.h"
#include "game/wii/csound.h"
#include "game/wii/xact.h"
#include "game/inventory.h"
#include "game/gmath.h"

extern "C" {
void fn_800A12F8(void* effects, u32 id, BOOL retireParticles); // stops the event effect with that id
CVec GetFloorContactPosition(XObj* object, s32 index, const Vec* offset);
}

// a held object of the given pool and kind
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

static inline bool IsTargetPool(XAction* act, s16 pool)
{
	bool result = false;
	if (act->mTarget != NULL && act->mTarget->GetPool() == pool) {
		result = true;
	}
	return result;
}

static inline BOOL IsState(const XAction* act, s32 state)
{
	if (act->mState == state) {
		return TRUE;
	}
	return FALSE;
}

static inline bool IsDrinking(XAction* act)
{
	return act->mState == 34 || act->mState == 35;
}

// drinking from the mug or putting it down after
static inline bool IsStill(XAction* act)
{
	return IsDrinking(act) || act->mState == 36;
}

static inline bool IsCarryingStill(XAction* act, s16 kind)
{
	bool result = false;
	if (IsCarrying(act, kind) && IsStill(act)) {
		result = true;
	}
	return result;
}

// lets go of a pushed object and resets
static inline void Release(XAction* act)
{
	XObj* target = act->mTarget;
	if (target != NULL && (target->mStatus.mFlags & 0x0800000000000000ULL)) {
		target->mStatus.mFlags &= ~0x0800000000000000ULL;
	}
	act->Reset();
}

static inline Vec GetFloorOffset(const XObj* obj, s32 index)
{
	Vec value;
	value.x = obj->mFloors[index].mCenterOffset[0] / 100.0f;
	value.y = obj->mFloors[index].mCenterOffset[1] / 100.0f;
	value.z = obj->mFloors[index].mCenterOffset[2] / 100.0f;
	return value;
}

static inline Vec GetFloorExtent(const XObj* obj, s32 index)
{
	Vec value;
	value.x = obj->mFloors[index].mExtent[0] / 100.0f;
	value.y = obj->mFloors[index].mExtent[1] / 100.0f;
	value.z = obj->mFloors[index].mExtent[2] / 100.0f;
	return value;
}

static inline f32 GetFloorYawRadians(const XObj* obj, s32 index)
{
	return DEG2RAD * obj->mFloors[index].mYawDegrees;
}

static inline Vec GetPlacementScale(const UnkStageObj* placement)
{
	Vec value;
	value.x = placement->mScale[0] / 100.0f;
	value.y = placement->mScale[1] / 100.0f;
	value.z = placement->mScale[2] / 100.0f;
	return value;
}

extern "C" {

/**
 * @note Address: 0x80105798
 * @note Size: 0xC8
 */
void CancelPlayerPushAction(XActPush* act)
{
	if (act->mState != 0) {
		Release(act);
		act->mState         = 0;
		WiiPlayerState* chr = act->mOwner;
		chr->SetAnim(0x11, FALSE, 0.0f, 1.0f, 1.0f, 10.0f);
		chr->mIdleLoops = 3;
	}
}

/**
 * @note Address: 0x80105750
 * @note Size: 0x48
 */
void ReleasePushAction(XActPush* act)
{
	Release(act);
}

/**
 * @note Address: 0x80101554
 * @note Size: 0x2C
 */
CVec GetPlayerFloorSlope(WiiPlayerState* chr)
{
	return chr->mFloorSlope;
}

/**
 * @note Address: 0x800FFBA0
 * @note Size: 0x18
 */
void SuppressMovementDuringJump(XActJump* act, const CPadOne* pad, f32* weight)
{
	if (act->mState != 0) {
		*weight = 0.0f;
	}
}

/**
 * @note Address: 0x800FFB58
 * @note Size: 0x48
 */
// resets the jump and stops its dust effect
void ResetPlayerJumpAction(XActJump* act)
{
	act->Reset();
	fn_800A12F8(&gGame.mEvtPtcl, 0x15, TRUE);
}

/**
 * @note Address: 0x800FD34C
 * @note Size: 0x20
 */
void SetLiftPlugConnection(XActLift* act, u32 value, BOOL apply)
{
	act->mPlugConnection = value;
	if (apply) {
		act->mOwner->mPlug.mConnection = value | 0x10000;
	}
}

/**
 * @note Address: 0x800FCE60
 * @note Size: 0x70
 */
void fn_800FCE60(XActLift* act)
{
	if (IsState(act, 38) || IsState(act, 39)) {
		act->mOwner->SetAnim(0x11, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
		act->mState = 4;
	}
}

/**
 * @note Address: 0x800FCDF0
 * @note Size: 0x70
 */
void fn_800FCDF0(XActLift* act)
{
	if (IsState(act, 32) || IsState(act, 33)) {
		act->mOwner->SetAnim(0x894, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
		act->mState = 4;
	}
}

/**
 * @note Address: 0x800FCD1C
 * @note Size: 0xD4
 */
void fn_800FCD1C(XActLift* act)
{
	if (IsState(act, 28) || IsState(act, 29) || IsState(act, 30) || IsState(act, 31)) {
		act->mOwner->SetAnim(0x894, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
		act->mState = 4;
		if (act->mContactEffect) {
			fn_800A12F8(&gGame.mEvtPtcl, act->mContactEffectId, FALSE);
			act->mContactEffect = FALSE;
		}
		InfoScrn_ApplyObjectGaugeCommand(&gGame.mInfoScrn, NULL, 0, 1, 3);
	}
}

/**
 * @note Address: 0x800FCBCC
 * @note Size: 0x150
 */
Vec GetLiftTargetContactOffset(const XActLift* act, XObj* target)
{
	// a plug (24) is held by its end joint
	if (target->GetPool() == 24) {
		CVec pos = target->GetWorldPos("plug_model");
		return pos - *target->GetPos();
	}
	Vec offset = *target->GetPos() * -1.0f;
	return GetFloorContactPosition(target, act->mTargetFloor, &offset);
}

/**
 * @note Address: 0x800FCB14
 * @note Size: 0xB8
 */
CVec GetStagePlacementScale(const XObj* obj)
{
	return CVec(GetPlacementScale(obj->mCoord.mPlacement));
}

/**
 * @note Address: 0x800F5368
 * @note Size: 0x5C
 */
// the held item takes his item tint
void ApplyPlayerItemTintToTarget(XAction* act)
{
	act->mTarget->SetDiffuseColor(act->mOwner->mItemTint);
}

/**
 * @note Address: 0x800F51AC
 * @note Size: 0x1BC
 */
// drops the item he's carrying, it goes in his inventory
void PutAwayLiftedObject(XActLift* act)
{
	s32 anim = act->mOwner->GetAnimId();
	if (anim == 0x8B1 || anim == 0x8B3) {
		act->mOwner->SetAnim(0x8B2, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
	} else if (anim == 11) {
		act->mOwner->SetAnim(8, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
	}
	act->mState              = 0;
	act->mOwner->mAttachment = 0;
	act->mTarget->mStatus.mFlags &= ~0x0200000000000000ULL;
	act->mTarget->SetAnim(7, TRUE, 0.0f, 1.0f, 1.0f, 10.0f);
	act->mTargetState = 20;
	XObjItem* item    = (XObjItem*)act->mTarget;
	SetOrAddInventoryItemCount(&act->mOwner->mParams.mInventory, item->GetItem(), 1, TRUE);
	item->SetFlag();
	PlayWiiSoundEffect(&gGame.mSound, 0xA6, 1.0f, 0.0f);
}

/**
 * @note Address: 0x800F50A4
 * @note Size: 0x108
 */
BOOL IsUsingMug(XActLift* act)
{
	return IsCarryingStill(act, 0x16);
}

/**
 * @note Address: 0x800F4FE8
 * @note Size: 0xBC
 */
BOOL IsCarryingSquirter(XActLift* act)
{
	return IsCarrying(act, 0x38);
}

/**
 * @note Address: 0x800F4F2C
 * @note Size: 0xBC
 */
BOOL IsCarryingMug(XActLift* act)
{
	return IsCarrying(act, 0x16);
}

/**
 * @note Address: 0x800F4E70
 * @note Size: 0xBC
 */
BOOL IsCarryingSpoon(XActLift* act)
{
	return IsCarrying(act, 0x15);
}

/**
 * @note Address: 0x800F4DB4
 * @note Size: 0xBC
 */
BOOL IsCarryingToothbrush(XActLift* act)
{
	return IsCarrying(act, 0x10);
}

/**
 * @note Address: 0x800F4D70
 * @note Size: 0x44
 */
BOOL IsLiftTargetPlug(XActLift* act)
{
	return IsTargetPool(act, 0x18);
}

/**
 * @note Address: 0x800F4CB8
 * @note Size: 0xB8
 */
void ResetPlayerLiftAction(XActLift* act)
{
	if (act->mTarget != NULL) {
		act->mTarget->mStatus.mFlags &= ~0x10000ULL;
		act->mTarget->mStatus.mFlags &= ~0x0400000000000000ULL;
	}
	act->Reset();
	act->mContactEffect = FALSE;
	act->mUseEffect     = FALSE;
	act->mToolEffect    = FALSE;
	act->_58            = 0;
	act->_80.x = act->_80.y = act->_80.z = 0.0f;
	act->_AC                             = 0;
}

/**
 * @note Address: 0x800F4BE4
 * @note Size: 0xD4
 */
void InitializePlayerLiftAction(XActLift* act, WiiPlayerState* owner)
{
	act->Init(owner);
	act->mPendingItemId = -1;
	act->mTimer         = 0.0f;
	ResetPlayerLiftAction(act);
}

/**
 * @note Address: 0x800F48E0
 * @note Size: 0x304
 */
// where the held floor of the target is
Vec GetTargetFloorPosition(XAction* act)
{
	if (act->mTarget != NULL) {
		CVec position = *act->mTarget->GetPos();
		CVec offset   = *act->mTarget->GetScale() * GetFloorOffset(act->mTarget, act->mTargetFloor);
		CVec extent   = *act->mTarget->GetScale() * GetFloorExtent(act->mTarget, act->mTargetFloor);
		f32 yaw       = act->mTarget->mMoveRotate.y + GetFloorYawRadians(act->mTarget, act->mTargetFloor);
		Mtx translate, local, rotate, scale, result;
		PSMTXTrans(translate, position.x, position.y, position.z);
		PSMTXTrans(local, offset.x, offset.y, offset.z);
		PSMTXScale(scale, extent.x, extent.y, extent.z);
		PSMTXRotRad(rotate, 'y', yaw);
		PSMTXConcat(translate, rotate, result);
		PSMTXConcat(result, local, result);
		PSMTXConcat(result, scale, result);
		Vec point = Vec();
		PSMTXMultVec(result, &point, &point);
		return point;
	}
	return Vec();
}

} // extern "C"
