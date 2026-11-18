// chibi robo's actions, the GameCube's objb_act.cpp: .text 0x800F48E0-0x801067C0 (wii_units.md Nr. 67). The
// functions come in the reverse of the GameCube's order, GetTargetFloorPosition first and UpdatePushedObject
// last; ResetTumbleAction (the GameCube's xacttumble.cpp) follows the file.
// NONMATCHING: six per-action updates are still to port: UpdateLiftActionAnimation, UpdateHeldObject,
// UpdatePlayerHangAction, UpdatePlayerJumpAction, UpdateLadderActionAnimation and UpdatePlayerRopeAction. The
// Wii rewrote them for the nw4r models and the Wii remote, so the GameCube's src/game/objb_act.cpp is a
// template for them, not a copy. Of the ported functions, the IsCarrying* checks and IsUsingMug differ only in
// which of r4-r7 holds each nested inline's result; GetStagePlacementScale, GetTargetFloorPosition,
// GetLiftTargetContactOffset and UpdatePushedObject in scheduling and stack slots.
#include "rvl_msl/PowerPC_EABI_Support/MSL_C/MSL_Common/math_api.h" // the Wii MSL's out-of-line __fpclassifyf
#include "game/wii/xobjflags.h"                                     // first: its AnimSfx needs the real nw4r::math, ahead of xobj.h's shim
#include "game/wii/xobj.h"
#include "game/wii/cgame.h"
#include "game/wii/csound.h"
#include "game/wii/xact.h"
#include "game/wii/cpadact.h"
#include "game/inventory.h"
#include "game/gmath.h"
#include <math.h>
#include <stdlib.h>
#include "arithmetic.h"

extern "C" {
CVec fn_800E5A2C(XObj* player);                                // where the carried object hangs, in the world
void fn_800A12F8(void* effects, u32 id, BOOL retireParticles); // stops the event effect with that id
CVec GetFloorContactPosition(XObj* object, s32 index, const Vec* offset);
void TryLeaveHang(XActHang* act);
BOOL IsPlayerBlockingEvents(const WiiPlayerState* player);
BOOL SetClampedPushPositionXZ(XObj* object, const Vec* direction, u8 axes, f32 x, f32 z);
BOOL QueryMapFloorHeight(XObj* object, f32 minHeight);
f32 QueryObjectFloorHeight(XObj* self, f32 radius, f32 height);
f32 GetFloorFacingAngle(XObj* object, const Vec* pos, s32 index, s32 requireSegment, Vec* contact, Vec* normalOut);
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

static inline u32 ProbeObjectFloor(XObj* obj, const CVec& from, const CVec& to, f32 radius, f32 height, Vec* push, Vec* contact, s32* floor)
{
	return obj->TestFloorCollision(&from, &to, radius, height, push, contact, floor, 0);
}

// how far the stick is pushed
static inline f32 StickLength(const CPadOne* pad)
{
	f32 y = pad->mStick.y;
	f32 x = pad->mStick.x;
	return FSqrt(x * x + y * y);
}

// a real function in another unit (0x800D57E4), inlined here as in ut.cpp
inline void GetOutwardPerpendicularToEdge(Vec* a, Vec* b, Vec* c, Vec* out)
{
	Vec bc;
	Vec ba;
	Vec n;
	PSVECSubtract(b, c, &bc);
	PSVECSubtract(b, a, &ba);
	PSVECCrossProduct(&ba, &bc, &n);
	PSVECCrossProduct(&bc, &n, out);
}

static const f32 pushAngleTolerance = 0.1f * DEG2RAD;

static inline Vec PushEdgePoint(const XObj* obj, s32 index, bool first, f32 height)
{
	const Vec& point = first ? obj->mEdges[index].mFrom : obj->mEdges[index].mTo;
	Vec v            = { obj->mMoveTranslate.x + point.x, height, obj->mMoveTranslate.z + point.z };
	return v;
}

// the heading straight into the edge of the floor he pushes
static inline f32 FacingPushEdge(XAction* act)
{
	s32 index   = act->mTarget->mSelectedFloorIndex;
	Vec& player = *act->mOwner->GetPos();
	Vec a       = PushEdgePoint(act->mTarget, index, true, player.y);
	Vec b       = PushEdgePoint(act->mTarget, index, false, player.y);
	CVec normal;
	GetOutwardPerpendicularToEdge(&player, &a, &b, &normal);
	return WrapAngle(atan2(normal.x, normal.z));
}

extern "C" {

/**
 * @note Address: 0x80105FE8
 * @note Size: 0x7D8
 */
void UpdatePushedObject(XActPush* act, u32 frames)
{
	if (!act->mTargetState)
		return;
	XObj* target = act->mTarget;
	if (target != NULL && (target->mStatus.mFlags & 1)) {
		switch (act->mTargetState) {
		case 1: {
			target->mStatus.mFlags |= 0x0800000000000000ULL;
			Vec hand = fn_800E5A2C(act->mOwner);
			Vec diff = *target->GetPos() - hand;
			act->mTargetOffset.Set(diff);
			break;
		}
		case 2: {
			Vec hand  = fn_800E5A2C(act->mOwner);
			f32 angle = WrapAngle(FacingPushEdge(act) - act->mTarget->mMoveRotate.y);
			Mtx rotate;
			PSMTXRotRad(rotate, 'y', angle);
			CVec direction(0, 0, act->mMoveDirection);
			PSMTXMultVec(rotate, &direction, &direction);
			if ((angle > (90 * DEG2RAD - pushAngleTolerance) && angle < (90 * DEG2RAD + pushAngleTolerance))
			    || (angle > (270 * DEG2RAD - pushAngleTolerance) && angle < (270 * DEG2RAD + pushAngleTolerance)))
				act->mPushPositionClamped
				    = SetClampedPushPositionXZ(target, &direction, 1, hand.x + act->mTargetOffset.x, hand.z + act->mTargetOffset.z);
			else
				act->mPushPositionClamped
				    = SetClampedPushPositionXZ(target, &direction, 2, hand.x + act->mTargetOffset.x, hand.z + act->mTargetOffset.z);

			target->OnPushDisplacement(*target->GetPos() - target->mLastPos);
			if (target->mStatus.mFlags & 0x4000000000ULL) {
				// pushed over an edge: it topples
				f32 floor = QueryObjectFloorHeight(target, 0.1f, 0.1f);
				if (floor < target->GetPos()->y - 5.0f) {
					QueryMapFloorHeight(target, floor);
					target->mStatus.mFlags &= ~0x0800000000000000ULL;
					Release(act);
					WiiPlayerState* owner = act->mOwner;
					owner->SetAnim(0x11, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
					owner->mIdleLoops = 3;
					act->mState       = 0;
					target->mStatus.mFlags |= 0x1000000000000000ULL;

					Vec rotation = target->mMoveRotate, speed = target->mMoveSpeed1;
					Vec extent = GetFloorExtent(target, 0);
					Vec topple;
					if ((angle > (90 * DEG2RAD - pushAngleTolerance) && angle < (90 * DEG2RAD + pushAngleTolerance))) {
						rotation.z -= HALF_PI;
						speed.z  = 0.1f;
						topple.z = 0;
						topple.x = extent.x * (2.0f / FSqrt(3.0f)) / 2.0f;
					} else if ((angle > (270 * DEG2RAD - pushAngleTolerance) && angle < (270 * DEG2RAD + pushAngleTolerance))) {
						rotation.z += HALF_PI;
						speed.z  = 0.1f;
						topple.z = 0;
						topple.x = -(extent.x * (2.0f / FSqrt(3.0f))) / 2.0f;
					} else if ((angle > (0 * DEG2RAD - pushAngleTolerance) && angle < (0 * DEG2RAD + pushAngleTolerance))) {
						rotation.x += HALF_PI;
						speed.x  = 0.1f;
						topple.x = 0;
						topple.z = extent.z * (2.0f / FSqrt(3.0f)) / 2.0f;
					} else if ((angle > (180 * DEG2RAD - pushAngleTolerance) && angle < (180 * DEG2RAD + pushAngleTolerance))) {
						rotation.x -= HALF_PI;
						speed.x  = 0.1f;
						topple.x = 0;
						topple.z = -(extent.z * (2.0f / FSqrt(3.0f))) / 2.0f;
					}
					target->mMoveTarget1.Set(rotation);
					target->mMoveSpeed1.Set(speed);
					topple.y = -(extent.y * (2.0f / FSqrt(3.0f))) / 2.0f;
					target->mVec714.Set(topple);
				}
			}
			break;
		}
		}
	}
}

/**
 * @note Address: 0x80105B9C
 * @note Size: 0x44C
 */
// the Wii passes the player's model where the GameCube passed the anim object
void UpdatePushActionAnimation(XActPush* act, s32 anim, CObj* model, u32 frames, f32 frame, f32 end)
{
	switch (act->mState) {
	case 1:
		if (anim != 0x3E) {
			XObj* target = act->mTarget;
			if (target != NULL) {
				f32 angle = GetFloorFacingAngle(target, act->mOwner->GetPos(), act->mTargetFloor, 1, NULL, NULL);
				if (fpclassify(angle) != FP_NAN) {
					act->mOwner->SetAnim(0x3E, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
					act->mOwner->SetRotationY(angle);
				} else {
					Release(act);
					act->mState = 0;
				}
			}
		} else if (model->IsAnimDone()) {
			act->mOwner->SetAnim(0x3F, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
			act->mState       = 2;
			act->mTargetState = 1;
		}
		break;
	case 2:
		if (act->mMoveDirection == 1) {
			if (anim == 0x4B) {
				if (model->IsAnimDone()) {
					act->mOwner->SetAnim(0x40, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
					act->mTargetState = 2;
				}
			} else if (anim != 0x40) {
				act->mOwner->SetAnim(0x4B, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
				act->mTargetState = 0;
			}
		} else if (act->mMoveDirection == -1) {
			if (anim == 0x4C) {
				if (model->IsAnimDone()) {
					act->mOwner->SetAnim(0x41, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
					act->mTargetState = 2;
				}
			} else if (anim != 0x41) {
				act->mOwner->SetAnim(0x4C, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
				act->mTargetState = 0;
			}
		} else if (anim != 0x3F) {
			act->mOwner->SetAnim(0x3F, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			act->mTargetState = 0;
		}
		break;
	case 3:
		if (anim != 0x42) {
			act->mOwner->SetAnim(0x42, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
		} else if (model->IsAnimDone()) {
			WiiPlayerState* chr = act->mOwner;
			chr->SetAnim(0x11, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
			chr->mIdleLoops = 3;
			act->mState     = 0;
		}
		break;
	case 4:
		// he lets go once the grab anim is ten frames in
		if (anim == 0x3E && model->GetAnimFrame() >= 10.0f) {
			WiiPlayerState* chr = act->mOwner;
			chr->SetAnim(0x11, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			chr->mIdleLoops = 3;
			act->mState     = 0;
		}
		break;
	}
}

/**
 * @note Address: 0x80105860
 * @note Size: 0x33C
 */
void HandlePushActionInput(XActPush* act, const CPadOne* pad, f32* weight)
{
	if (act->mState == 2) {
		if (gGame.IsRunning() && gGame.mEvt.mData.mScript == NULL) {
			// letting go of a stops pushing
			if (!(pad->mButton & ((1ULL << RemoteA) | (1ULL << GameCubeA)))) {
				Release(act);
				act->mState = 3;
				*weight     = 0.0f;
			}
		}
	} else if (act->mState != 0) {
		if (act->mState == 3) {
		} else if (act->mState != 4) {
			if (!(pad->mButton & ((1ULL << RemoteA) | (1ULL << GameCubeA)))) {
				act->mState = 4;
			}
		} else if (pad->mButton & ((1ULL << RemoteA) | (1ULL << GameCubeA))) {
			// a takes hold again, the stick walks away
			act->mOwner->SetAnim(0x3E, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			act->mState = 1;
		} else if (StickLength(pad) > 0.0f) {
			Release(act);
			WiiPlayerState* chr = act->mOwner;
			chr->SetAnim(0x11, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			chr->mIdleLoops = 3;
			act->mState     = 0;
		}
		*weight = 0.0f;
	}
}

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
		chr->SetAnim(0x11, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
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
 * @note Address: 0x80103978
 * @note Size: 0x1A4
 */
void HandleRopeActionInput(XActRope* act, const CPadOne* pad, f32* weight)
{
	if (act->mState == 5) {
		if (gGame.IsRunning() && gGame.mEvt.mData.mScript == NULL) {
			// b lets go of the rope
			if (gWiiPointer.Check(2)) {
				act->mState = 8;
			}
			if (act->mOwner->GetAnimId() == 0x8A || act->mOwner->GetAnimId() == 0x8C) {
				*weight = 0.0f;
			}
		}
	} else if (act->mState != 0) {
		*weight = 0.0f;
	}
}

/**
 * @note Address: 0x80101580
 * @note Size: 0x15C
 */
void HandleLadderActionInput(XActLadder* act, const CPadOne* pad, f32* weight)
{
	act->mBPressed = FALSE;
	if (act->mState == 5) {
		if (gGame.IsRunning() && gGame.mEvt.mData.mScript == NULL && gWiiPointer.Check(2)) {
			act->mBPressed = TRUE;
		}
	} else if (act->mState != 0) {
		*weight = 0.0f;
	}
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
 * @note Address: 0x800FD5BC
 * @note Size: 0x4BC
 */
// climbs up from the ledge unless something stands where he would come up
void TryLeaveHang(XActHang* act)
{
	f32 radius     = act->mOwner->GetScale()->x * GetFloorExtent(act->mOwner, 0).x;
	f32 halfHeight = 0.5f * (act->mOwner->GetScale()->x * (GetFloorOffset(act->mOwner, 0).y + GetFloorExtent(act->mOwner, 0).y));
	Vec position   = act->mOwner->GetJointWorldPos("second_null");
	position.y += 0.5f;

	u32 blockingHit = 0;
	for (s32 i = 0; i < gGame.mXObjs.mObjectCount; i++) {
		XObj* obj = gGame.mXObjs.mObjects[i];
		if (!(obj->mStatus.mFlags & 1) || !(obj->mStatus.mFlags & 0x10000) || (obj->mStatus.mFlags & 0x100000))
			continue;
		if (obj->GetPool() == 7 || obj->GetPool() == 20 || act->mOwner == obj || obj->GetKind() >= 10000)
			continue;
		CVec* objectPos = obj->GetPos();
		f32 dz = objectPos->z - position.z, dx = objectPos->x - position.x;
		if (FSqrt(dx * dx + dz * dz) > 100.0f + (radius + obj->mFloorXZExtent[0]))
			continue;

		CVec push, contact;
		u32 result = ProbeObjectFloor(obj, CVec(position.x, position.y, position.z), CVec(position.x, position.y, position.z), radius,
		                              halfHeight, &push, &contact, NULL);
		if ((result & 3) == 3) {
			blockingHit = result;
			break;
		}
	}
	if (blockingHit)
		return;
	if ((rand() & 7) == 0)
		act->mState = 7;
	else
		act->mState = 6;
}

/**
 * @note Address: 0x800FD36C
 * @note Size: 0x250
 */
void HandleHangActionInput(XActHang* act, const CPadOne* pad, f32* weight)
{
	act->mStickAway = FALSE;
	if (act->mState == 4) {
		f32 stick = -(f32)atan2(pad->mStick.x, pad->mStick.y);
		f32 angle = WrapAngle(stick + gGame.mMotionControlCamera.mView.mAngle.y - act->mOwner->mMoveRotate.y);
		if (!(angle < 30 * DEG2RAD) && !(angle > 330 * DEG2RAD)) {
			act->mStickAway = TRUE;
		}
		*weight = 0.0f;
	} else if (act->mState == 5) {
		if (gGame.IsRunning() && gGame.mEvt.mData.mScript == NULL) {
			// a climbs up if there is room, b lets go (9)
			if (gWiiPointer.Check(1)) {
				if (act->mTarget == NULL || (act->mTarget->mStatus.mFlags & 0x400000000ULL)) {
					TryLeaveHang(act);
					act->mMoveDirection = 0;
				}
				*weight = 0.0f;
			} else if (gWiiPointer.Check(2)) {
				act->mState         = 9;
				act->mMoveDirection = 0;
				*weight             = 0.0f;
			}
		}
	} else if (act->mState != 0) {
		*weight = 0.0f;
	}
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
 * @note Address: 0x800FCED0
 * @note Size: 0x47C
 */
BOOL CheckCarriedObjectContacts(XActLift* act, const Vec* pos, f32 radius, f32 height, BOOL checkItem, BOOL checkEvent, BOOL checkOthers,
                                Vec* result)
{
	BOOL found = FALSE;
	if (gGame.mEvt.mData.mScript != NULL) {
		return FALSE;
	}
	s32 item = act->mTarget->GetObjFlags();
	for (s32 i = 0; i < gGame.mXObjs.mObjectCount; ++i) {
		XObj* object = gGame.mXObjs.mObjects[i];
		if (!(object->mStatus.mFlags & 1) || !(object->mStatus.mFlags & 0x10000))
			continue;
		CVec* objectPos = object->GetPos();
		f32 dz = objectPos->z - pos->z, dx = objectPos->x - pos->x;
		if (FSqrt(dx * dx + dz * dz) > 100.0f + (radius + object->mFloorXZExtent[0]))
			continue;
		CVec hit, contact;
		if (checkItem && (object->mStatus.mFlags & 0x40000000000ULL)) {
			u32 touch = ProbeObjectFloor(object, CVec(pos->x, pos->y, pos->z), CVec(pos->x, pos->y, pos->z), radius, height, &hit, &contact,
			                             NULL);
			if ((touch & 3) == 3 && object->OnItemUsed(pos, item)) {
				found = TRUE;
				if (result)
					*result = contact;
			}
		}
		if (checkEvent && (object->mStatus.mFlags & 0x80000000000ULL)) {
			u32 touch = ProbeObjectFloor(object, CVec(pos->x, pos->y, pos->z), CVec(pos->x, pos->y, pos->z), radius, height, &hit, &contact,
			                             NULL);
			if ((touch & 3) == 3) {
				object->OnSquirterHit(&contact, act->mOwner->mSquirterContent, act->mOwner->mItemTintWord);
				found = TRUE;
				if (result)
					*result = contact;
			}
		} else if (checkOthers) {
			if (object->mStatus.mFlags & 0x2000000000000ULL)
				continue;
			if (object->mStatus.mFlags & 0x100000)
				continue;
			if (object == act->mTarget)
				continue;
			if (object == act->mOwner)
				continue;
			u32 touch = ProbeObjectFloor(object, CVec(pos->x, pos->y, pos->z), CVec(pos->x, pos->y, pos->z), radius, height, &hit, &contact,
			                             NULL);
			if ((touch & 3) == 3) {
				object->OnSquirterHit(&contact, act->mOwner->mSquirterContent, act->mOwner->mItemTintWord);
				found = TRUE;
				if (result)
					*result = contact;
			}
		}
	}
	return found;
}

/**
 * @note Address: 0x800FCE60
 * @note Size: 0x70
 */
void fn_800FCE60(XActLift* act)
{
	if (IsState(act, 38) || IsState(act, 39)) {
		act->mOwner->SetAnim(0x11, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
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
		act->mOwner->SetAnim(0x894, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
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
		act->mOwner->SetAnim(0x894, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
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
 * @note Address: 0x800F53C4
 * @note Size: 0x824
 */
void HandleLiftActionInput(XActLift* act, const CPadOne* pad, f32* weight)
{
	if (act->mState == 4) {
		WiiPlayerState* chr = act->mOwner;
		if (chr->mJump.mState != 0) {
			return;
		}
		if (chr->mHang.mState != 0) {
			return;
		}
		if (chr->mTumble.mState != 0) {
			return;
		}
		if (chr->mSuits.mChangeStep != 0) {
			return;
		}
		if (chr->mMenu.IsOpen()) {
			return;
		}
		if (!gGame.IsRunning()) {
			return;
		}
		if (gGame.GetStatusFlags(0x100)) {
			return;
		}
		if (gGame.mEvt.mData.mScript != NULL) {
			return;
		}
		if (act->mOwner->GetAnimId() == 0x895 || act->mOwner->GetAnimId() == 0x152 || act->mOwner->GetAnimId() == 0x153
		    || act->mOwner->GetAnimId() == 0x154 || act->mOwner->GetAnimId() == 0x8AE || act->mOwner->GetAnimId() == 0x8AF
		    || act->mOwner->GetAnimId() == 0x155 || act->mOwner->GetAnimId() == 0x156 || act->mOwner->GetAnimId() == 0x157) {
			*weight = 0.0f;
			return;
		}
		// a sets it down (6), b puts an item away (20) and sets anything else down
		if (gWiiPointer.Check(1)) {
			if (act->mTarget == NULL) {
				return;
			}
			if (act->mTarget->GetPool() == 8) {
				return;
			}
			if (act->mOwner->mFloorContactObjects[4] >= 0
			    && (gGame.mXObjs.mObjects[act->mOwner->mFloorContactObjects[4]]->mStatus.mFlags & 0x20)) {
				*weight = 0.0f;
				return;
			}
			if (act->mTarget->GetPool() == 0x18) {
				return;
			}
			act->mState = 6;
			*weight     = 0.0f;
		} else if (gWiiPointer.Check(2)) {
			if (act->mTarget != NULL) {
				if (act->mTarget->GetPool() == 8) {
					if (act->mOwner->GetAttachment() == 0) {
						act->mState = 20;
					}
				} else if (act->mTarget->GetPool() != 0x18 || !act->mOwner->mMenu.IsOpen()) {
					act->mState = 6;
				}
			}
			*weight = 0.0f;
		}
	} else if (IsState(act, 13) || IsState(act, 14)) {
		// a or b stops giving power and pulls the plug (15)
		if (gGame.IsRunning() && gGame.mEvt.mData.mScript == NULL && (gWiiPointer.Check(1) || gWiiPointer.Check(2))) {
			act->mState = 15;
			act->mTimer = 0.0f;
		}
		*weight = 0.0f;
	} else if (act->mState == 16) {
		if (gGame.IsRunning()) {
			if (act->_84 == HALF_PI) {
				act->mState = 17;
				if (act->mPlugTarget != NULL) {
					InfoScrn_ApplyObjectGaugeCommand(&gGame.mInfoScrn, NULL, 0, 1, 0);
				}
				// a plug that is connected costs ten watts
				if (((WiiPlayerPlug*)act->mTarget)->mConnection != -1) {
					f32 battery = act->mOwner->mParams.mBattery - 10.0f;
					if (battery < 0.0f) {
						battery = 0.0f;
					}
					act->mOwner->mParams.SetBattery(battery);
				}
				act->mTimer = 0.0f;
			} else if (gWiiPointer.Check(2)) {
				act->mState = 18;
				if (act->mPlugTarget != NULL) {
					InfoScrn_ApplyObjectGaugeCommand(&gGame.mInfoScrn, NULL, 0, 1, 0);
				}
				act->mTimer = 0.0f;
			}
		}
		*weight = 0.0f;
	} else if (act->mState == 29) {
		// the first frames of a toothbrush stroke leave the stick alone
	} else if (act->mState == 31) {
	} else if (act->mState == 33) {
		*weight = 0.0f;
	} else if (IsState(act, 34) || IsState(act, 35)) {
		// he uses the mug for as long as a is held
		if (!(pad->mButton & ((1ULL << RemoteA) | (1ULL << GameCubeA))) || gGame.mEvt.mData.mScript != NULL
		    || IsPlayerBlockingEvents(act->mOwner)) {
			act->mState = 36;
		}
		*weight = 0.0f;
	} else if (act->mState != 0) {
		*weight = 0.0f;
	}
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
		act->mOwner->SetAnim(0x8B2, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
	} else if (anim == 11) {
		act->mOwner->SetAnim(8, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
	}
	act->mState              = 0;
	act->mOwner->mAttachment = 0;
	act->mTarget->mStatus.mFlags &= ~0x0200000000000000ULL;
	act->mTarget->SetAnim(7, TRUE, 0.0f, 1.0f, 1.0f, 15.0f);
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
	act->_80 = act->_84 = act->_88 = 0.0f;
	act->_AC                       = 0;
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
