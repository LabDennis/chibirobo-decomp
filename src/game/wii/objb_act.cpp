// chibi robo's actions, the GameCube's objb_act.cpp: .text 0x800F48E0-0x801067C0 (wii_units.md Nr. 67). The
// functions come in the reverse of the GameCube's order, GetTargetFloorPosition first and UpdatePushedObject
// last; ResetTumbleAction (the GameCube's objb_act.cpp, once xacttumble.cpp) follows the file.
// NONMATCHING: five per-action updates are still to port: UpdateLiftActionAnimation, UpdateHeldObject,
// UpdatePlayerHangAction, UpdatePlayerJumpAction and UpdateLadderActionAnimation. The Wii rewrote them for the nw4r
// models and the Wii remote, so the GameCube's src/game/objb_act.cpp is a template for them, not a copy. Of the
// ported functions, the IsCarrying* checks and IsUsingMug differ only in which of r4-r7 holds each nested inline's
// result; GetStagePlacementScale, GetTargetFloorPosition, GetLiftTargetContactOffset and UpdatePushedObject in
// scheduling and stack slots. UpdatePlayerRopeAction has the retail calls, but its sin/cos and vector temporaries differ.
#include "rvl_msl/PowerPC_EABI_Support/MSL_C/MSL_Common/math_api.h" // the Wii MSL's out-of-line __fpclassifyf
#include "game/wii/xobjflags.h"                                     // first: its AnimSfx needs the real nw4r::math, ahead of xobj.h's shim
#include "game/wii/xobj.h"
#include "game/wii/cgame.h"
#include "game/wii/csound.h"
#include "game/wii/xact.h"
#include "game/wii/cpadact.h"
#include "game/cvec.h"
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
void StopPlayerMotion(WiiPlayerState* player);
BOOL SetClampedPushPositionXZ(XObj* object, const Vec* direction, u8 axes, f32 x, f32 z);
BOOL QueryMapFloorHeight(XObj* object, f32 minHeight);
f32 QueryObjectFloorHeight(XObj* self, f32 radius, f32 height);
f32 GetFloorFacingAngle(XObj* object, const Vec* pos, s32 index, s32 requireSegment, Vec* contact, Vec* normalOut);
u32 CheckStageFlag(s32 stage, u32 mask);
void PutAwayLiftedObject(XActLift* act);
EffectInstance* fn_800A11A4(EffectInstance* effects, u32 id, s32 group);
void StartScreenFade(CScrnColor* color, u8 r0, u8 g0, u8 b0, u8 a0, u8 r1, u8 g1, u8 b1, u8 a1, f32 time);
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

static inline void RopeAnim(XActRope* act, s32 id, f32 loop, BOOL force = FALSE, f32 start = 0.0f)
{
	act->mOwner->SetAnim(id, force, start, 1.0f, loop, 15.0f);
}

static inline void ReleaseRopePlug(XActRope* act, f32 frame, f32 threshold)
{
	if (CheckStageFlag(gGame.mStage, 1)) {
		WiiPlayerPlug* plug = &act->mOwner->mPlug;
		if (plug->mConnection && frame >= threshold) {
			plug->mConnection = 0;
			PlayWiiSoundEffect(&gGame.mSound, 0xA4, 1.0f, 0.0f);
		}
	}
}

static inline void SyncRopePosition(WiiPlayerState* owner)
{
	if (owner->mMoveTranslate.y != owner->mLastPos.y)
		owner->mStatus.mFlags |= 0x0010000000000000ULL;
	owner->mLastPos = owner->mMoveTranslate;
}

static inline void PositionPlayerAroundRopeTarget(XActRope* act, f32 angle)
{
	if (act->mTarget != NULL) {
		CVec dir((Vec) { act->mTargetOffset.x, 0.0f, act->mTargetOffset.z });
		f32 dist = PSVECMag(&dir);
		act->mOwner->SetTranslateX(act->mTarget->GetPos()->x - dist * (f32)sin(angle));
		act->mOwner->SetTranslateZ(act->mTarget->GetPos()->z - dist * (f32)cos(angle));
	}
}

static inline void ApplyPlayerJumpStrength(XActJump* act, f32 strength)
{
	if (gGame.mEvt.mData.mScript != NULL) {
		act->mTargetStrength = 0.0f;
		act->mStrength       = 0.0f;
	} else {
		act->mTargetStrength = strength;
		act->mStrength       = strength;
		if (act->mTargetStrength >= 50.0f) {
			WiiPlayerState* chr = act->mOwner;
			if (chr->mLift.mState != 0 && IsCarrying(&chr->mLift, 22)) {
				PutAwayLiftedObject((XActLift*)&chr->mLift);
				if (act->mTargetStrength >= 60.0f) {
					act->mState                 = 0;
					act->mOwner->mTumble.mState = 1;
					act->mOwner->SetAnim(0x35, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
				}
			}
		}
	}
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
 * @note Address: 0x80103B1C
 * @note Size: 0x1C34
 */
// NONMATCHING: the calls match, except that in PositionPlayerAroundRopeTarget ours calls GetPos before sin/cos and retail
// after them; CVec return homes and vector temporaries leave a 0x20 smaller frame and different schedules
void UpdatePlayerRopeAction(XActRope* act, s32 anim, CObj* model, u32 frames, f32 frame, f32 end)
{
	switch (act->mState) {
	case 1:
		if (anim != 0x21) {
			RopeAnim(act, 0x21, 0);
			if (act->mTarget) {
				CVec* target = act->mTarget->GetPos();
				CVec* owner  = act->mOwner->GetPos();
				act->mOwner->SetRotationY(WrapAngle(atan2(target->x - owner->x, target->z - owner->z)));
				CVec player(*act->mOwner->GetPos());
				player.y = 0;
				CVec rope(*act->mTarget->GetPos());
				rope.y = 0;
				CVec offset(act->mTargetOffset);
				offset.y = 0;
				Mtx matrix;
				PSMTXRotRad(matrix, 'y', act->mOwner->mMoveRotate.y);
				PSMTXMultVec(matrix, &offset, &offset);
				rope -= offset;
				act->mPos   = rope;
				act->mDist  = PSVECDistance(&rope, &player);
				act->mAngle = atan2(player.x - rope.x, player.z - rope.z);
			}
		} else if (model->IsAnimDone()) {
			RopeAnim(act, 0x8C, 0);
			act->mState = 5;
		} else {
			if (act->mTarget) {
				Vec delta = *act->mTarget->GetPos() - act->mTarget->mLastPos;
				act->mPos.x += delta.x;
				act->mPos.z += delta.z;
				if (act->mDist > 0 && frame > 20) {
					if ((act->mDist -= .2f * frames) < 0)
						act->mDist = 0;
					act->mOwner->SetTranslateX(act->mDist * (f32)sin(act->mAngle) + act->mPos.x);
					act->mOwner->SetTranslateZ(act->mDist * (f32)cos(act->mAngle) + act->mPos.z);
				} else {
					act->mOwner->SetTranslateX(act->mPos.x);
					act->mOwner->SetTranslateZ(act->mPos.z);
				}
			}
			ReleaseRopePlug(act, frame, 28);
		}
		break;
	case 2:
		if (anim != 0x98) {
			RopeAnim(act, 0x98, 0);
			if (act->mTarget) {
				act->mOwner->SetRotationY(WrapAngle(act->mTarget->mMoveRotate.y));
				CVec player(*act->mOwner->GetPos());
				player.y = 0;
				CVec rope(*act->mTarget->GetPos());
				rope.y = 0;
				CVec offset;
				offset.Set(0.0f, 0.0f, -.5f);
				Mtx matrix;
				PSMTXRotRad(matrix, 'y', act->mTarget->mMoveRotate.y);
				PSMTXMultVec(matrix, &offset, &offset);
				rope += offset;
				act->mPos   = rope;
				act->mDist  = PSVECDistance(&rope, &player);
				act->mAngle = atan2(player.x - rope.x, player.z - rope.z);
			}
		} else if (model->IsAnimDone()) {
			CVec pos   = act->mOwner->GetJointWorldPos("second_null");
			Vec offset = act->mTargetOffset;
			Mtx matrix;
			PSMTXRotRad(matrix, 'y', act->mTarget->mMoveRotate.y);
			PSMTXMultVec(matrix, &offset, &offset);
			act->mOwner->SetPos(pos.x + offset.x, pos.y - offset.y, pos.z + offset.z);
			SyncRopePosition(act->mOwner);
			act->mOwner->SetRotationY(PI + act->mOwner->mMoveRotate.y);
			act->mOwner->GetResNode("second_null").SetTranslate(act->mTargetOffset.x, act->mTargetOffset.y, act->mTargetOffset.z);
			RopeAnim(act, 0x8C, 1);
			act->mState = 5;
		} else {
			if (act->mTarget) {
				CVec delta = *act->mTarget->GetPos() - act->mTarget->mLastPos;
				act->mPos.x += delta.x;
				act->mPos.z += delta.z;
				if (act->mDist > 0 && frame > 80) {
					if ((act->mDist -= .2f * frames) < 0)
						act->mDist = 0;
					act->mOwner->SetTranslateX(act->mDist * (f32)sin(act->mAngle) + act->mPos.x);
					act->mOwner->SetTranslateZ(act->mDist * (f32)cos(act->mAngle) + act->mPos.z);
				}
			}
			ReleaseRopePlug(act, frame, 80);
		}
		break;
	case 3:
		if (anim != 0x85) {
			if (act->mTarget) {
				act->mOwner->SetRotationY(act->mTarget->mMoveRotate.y);
				RopeAnim(act, 0x8A, 1, TRUE);
				act->mOwner->UpdateAnim(0);
				CVec local = act->mOwner->GetNodeTranslate("second_null");
				CVec delta = act->mOwner->GetWorldPos("second_null") - *act->mOwner->GetPos();
				delta.y    = -delta.y;
				act->mOwner->SetTranslateY(delta.y + act->mOwner->GetPos()->y);
				RopeAnim(act, 0x85, 1);
				CVec player(*act->mOwner->GetPos());
				player.y = 0;
				CVec rope(*act->mTarget->GetPos());
				rope.y = 0;
				CVec offset;
				offset.Set(0.0f, 0.0f, -.5f);
				Mtx matrix;
				PSMTXRotRad(matrix, 'y', act->mTarget->mMoveRotate.y);
				PSMTXMultVec(matrix, &offset, &offset);
				rope += offset;
				act->mPos   = rope;
				act->mDist  = PSVECDistance(&rope, &player);
				act->mAngle = atan2(player.x - rope.x, player.z - rope.z);
			}
			ReleaseRopePlug(act, 0, 0);
		} else if (model->IsAnimDone()) {
			act->mOwner->FindResNode("second_null");
			act->mOwner->SetRotationY(PI + act->mOwner->mMoveRotate.y);
			CVec delta = act->mOwner->GetJointWorldPos("second_null") - *act->mOwner->GetPos();
			delta.y    = 0;
			CVec offset(act->mTargetOffset);
			Mtx matrix;
			PSMTXRotRad(matrix, 'y', act->mOwner->mMoveRotate.y);
			PSMTXMultVec(matrix, &offset, &offset);
			offset.y = 0;
			act->mOwner->SetPos(*act->mOwner->GetPos() - (delta + offset));
			SyncRopePosition(act->mOwner);
			act->mOwner->GetResNode("second_null").SetTranslate(act->mTargetOffset.x, act->mTargetOffset.y, act->mTargetOffset.z);
			RopeAnim(act, 0x8C, 1);
			act->mState = 5;
		} else if (act->mTarget) {
			Vec delta = *act->mTarget->GetPos() - act->mTarget->mLastPos;
			act->mPos.x += delta.x;
			act->mPos.z += delta.z;
			if (act->mDist > 0) {
				if ((act->mDist -= .4f * frames) < 0)
					act->mDist = 0;
				act->mOwner->SetTranslateX(act->mDist * (f32)sin(act->mAngle) + act->mPos.x);
				act->mOwner->SetTranslateZ(act->mDist * (f32)cos(act->mAngle) + act->mPos.z);
			} else {
				act->mOwner->SetTranslateX(act->mPos.x);
				act->mOwner->SetTranslateZ(act->mPos.z);
			}
		}
		break;
	case 4:
		RopeAnim(act, 0x8C, 1);
		act->mState = 5;
		break;
	case 5:
		if (anim == 0x8B) {
			if (model->IsAnimDone()) {
				if (act->mMoveDirection == 1)
					RopeAnim(act, 0x8D, 1);
				else
					RopeAnim(act, 0x8C, 1);
			}
		} else if (anim == 0x8D) {
			if (model->IsAnimDone()) {
				if (act->mMoveDirection == 1)
					RopeAnim(act, 0x8B, 1);
				else
					RopeAnim(act, 0x8A, 1);
			}
		} else if (act->mMoveDirection == 1 || act->mMoveDirection == 2) {
			if (anim == 0x8A)
				RopeAnim(act, 0x8B, 1);
			else if (anim == 0x8C)
				RopeAnim(act, 0x8D, 1);
			else if (anim == 0x23)
				RopeAnim(act, 0x8C, 0);
		} else if (act->mMoveDirection == -1) {
			if (act->mOwner->GetHeight() > 0) {
				if (anim != 0x23)
					RopeAnim(act, 0x23, 0);
			} else {
				act->mState = 8;
				RopeAnim(act, 0x25, 0);
			}
		} else if (anim == 0x23)
			RopeAnim(act, 0x8C, 0);
		if (act->mMoveDirection == 1 || act->mMoveDirection == 2) {
			if (act->mTarget && act->mOwner->GetJointWorldPos("carry_obj").y >= act->mTarget->mFloorHeights[0]) {
				act->mOwner->SetTranslateY(act->mTarget->mFloorHeights[0] - act->mTargetOffset.y);
				RopeAnim(act, 0x24, 0);
				act->mState = 6;
				break;
			}
			PlayWiiSoundEffect(&gGame.mSound, 0x7E, 1.0f, 0.0f);
		} else if (act->mMoveDirection == -1)
			PlayWiiSoundEffect(&gGame.mSound, 0x82, 1.0f, 0.0f);
		if (act->mTarget) {
			f32 target = WrapAngle(PI + act->mTarget->mMoveRotate.y);
			f32 angle  = act->mOwner->mMoveRotate.y;
			if (WrapAngle(target - angle) >= PI) {
				angle -= DEG2RAD * 2.f * frames;
				if (WrapAngle(target - angle) < PI)
					angle = target;
			} else {
				angle += DEG2RAD * 2.f * frames;
				if (WrapAngle(target - angle) > PI)
					angle = target;
			}
			if (anim == 0x34 || anim == 0x8B || anim == 0x8D || anim == 0x23)
				act->mOwner->SetRotationY(angle);
			PositionPlayerAroundRopeTarget(act, angle);
		}
		break;
	case 6:
		if (anim != 0x24)
			RopeAnim(act, 0x24, 0);
		else if (model->IsAnimDone()) {
			act->mOwner->SetPos(act->mOwner->GetJointWorldPos("second_null"));
			SyncRopePosition(act->mOwner);
			act->mOwner->GetResNode("second_null").SetTranslate(0.0f, 0.0f, 0.0f);
			WiiPlayerState* owner = act->mOwner;
			owner->SetAnim(0x11, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
			owner->mIdleLoops = 3;
			act->mState       = 0;
			act->mTarget      = NULL;
		}
		break;
	case 7:
		RopeAnim(act, 0x25, 1, TRUE, 20);
		act->mState = 8;
	case 8:
		if (anim != 0x25)
			RopeAnim(act, 0x25, 0);
		else if (model->IsAnimDone()) {
			if (act->mOwner->GetAttachment() == 1) {
				act->mState                 = 0;
				act->mOwner->mCopter.mState = 7;
			} else {
				RopeAnim(act, 0x38, 0);
				act->mState = 9;
			}
			act->mTarget = NULL;
		}
		break;
	case 9:
		if (act->mOwner->GetHeight() <= 0) {
			if (act->mOwner->mFallDistance > 30) {
				RopeAnim(act, 0x91, 1);
				act->mState = 11;
				ApplyPlayerJumpStrength((XActJump*)&act->mOwner->mJump, act->mOwner->mFallDistance);
				act->mOwner->SetColor(1, 255, 0, 0, 0);
				act->mOwner->SetColor(1, 0, 0, 0, .3f);
				StartScreenFade(&gGame.mScrnTex.mColor, 255, 0, 0, 0, 100, 100, 200, 0, .3f);
			} else {
				RopeAnim(act, 0x59, 1);
				act->mState = 10;
			}
		}
		break;
	case 10:
		if (model->IsAnimDone()) {
			WiiPlayerState* owner = act->mOwner;
			owner->SetAnim(0x11, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			owner->mIdleLoops = 3;
			act->mState       = 0;
		}
		break;
	case 11:
		if (model->IsAnimDone()) {
			act->mState = 0;
			RopeAnim(act, 0x8F, 1);
			act->mOwner->mJump.mState = 6;
			MtxP mtx                  = act->mOwner->GetWorldMtx();
			EffectInstance* effect    = fn_800A11A4((EffectInstance*)&gGame.mEvtPtcl, 0x15, 1);
			if (effect != NULL) {
				effect->mtx = mtx;
				UpdateEffectMtx(effect);
			}
			PlayWiiSoundEffect(&gGame.mSound, 0x87, 1.0f, 0.0f);
			PlayWiiSoundEffect(&gGame.mSound, 0xA3, 1.0f, 0.0f);
		}
		break;
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

static inline void JumpAnim(XActJump* act, s32 id, f32 loop = 1.0f)
{
	act->mOwner->SetAnim(id, FALSE, 0.0f, 1.0f, loop, 15.0f);
}

static inline void EndJump(XActJump* act)
{
	WiiPlayerState* owner = act->mOwner;
	owner->SetAnim(0x11, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
	owner->mIdleLoops = 3;
	act->mState       = 0;
}

static inline BOOL AdvanceJumpArc(XActJump* act, u32 frames)
{
	if (act->mArcAngle < HALF_PI) {
		if ((act->mArcAngle += act->mArcStep * frames) > HALF_PI) {
			act->mArcAngle = HALF_PI;
		}
		f32 sine = (f32)sin(act->mArcAngle);
		f32 lift = act->mRise * sine;
		act->mOwner->SetTranslateY(act->mArcPosition.y + lift);
		f32 angle = act->mOwner->mMoveRotate.y;
		f32 dx    = act->mForward * (act->mArcStep * (f32)sin(angle));
		f32 dz    = act->mForward * (act->mArcStep * (f32)cos(angle));
		act->mOwner->SetTranslateX(act->mArcPosition.x += dx);
		act->mOwner->SetTranslateZ(act->mArcPosition.z += dz);
		return FALSE;
	}
	return TRUE;
}

static inline void StartJumpDust(XActJump* act, u32 id, s32 group)
{
	MtxP mtx               = act->mOwner->GetWorldMtx();
	EffectInstance* effect = fn_800A11A4((EffectInstance*)&gGame.mEvtPtcl, id, group);
	if (effect != NULL) {
		effect->mtx = mtx;
		UpdateEffectMtx(effect);
	}
}

/**
 * @note Address: 0x800FFBB8
 * @note Size: 0x199C
 */
// NONMATCHING: action/vector lifetimes use extra saved FPRs and a smaller frame; the proximity check has one fewer inlined FSqrt
void UpdatePlayerJumpAction(XActJump* act, s32 anim, CObj* model, u32 frames, f32 frame, f32 end)
{
	BOOL interrupt = FALSE;
	if (!gGame.mEvt.mData.mScript) {
		CPad pad(0);
		if (StickLength(pad.mPad) > 0.0f) {
			switch (act->mState) {
			case 19:
				if (frame > 1.0f) {
					interrupt = TRUE;
				}
				break;
			case 4:
			case 18:
				if (frame > 3.0f) {
					interrupt = TRUE;
				}
				break;
			case 10:
			case 13:
			case 16:
				act->mOwner->SetAnimRate(1.0f + 2.0f * ((frame / end) * StickLength(pad.mPad)));
				break;
			}
		}
	}
	s32 next     = -1;
	BOOL playing = FALSE;
	switch (act->mState) {
	case 1:
		if (anim != 0x1C) {
			JumpAnim(act, 0x1C, 0.0f);
			return;
		}
		if (!model->IsAnimDone()) {
			return;
		}
		act->mState = 2;
	case 2:
	case 3:
	case 17:
		if (act->mOwner->GetHeight() <= 0.0f || (act->mOwner->GetHeight() <= 1.0f && GetPlayerFloorSlope(act->mOwner).Length() > 0.5f)) {
			if (act->mOwner->mLift.mState == 20) {
				act->mState = 0;
				return;
			}
			if (act->mOwner->mFallDistance > 30.0f) {
				JumpAnim(act, 0x8E);
				act->mState = 5;
				ApplyPlayerJumpStrength(act, act->mOwner->mFallDistance);
				act->mOwner->SetColor(1, 255, 0, 0, 0.0f);
				act->mOwner->SetColor(1, 0, 0, 0, 0.3f);
				StartScreenFade(&gGame.mScrnTex.mColor, 255, 0, 0, 0, 100, 100, 200, 0, 0.3f);
			} else if (act->mOwner->mFallDistance > 11.5f) {
				JumpAnim(act, 0x1E);
				act->mState = 4;
				if (act->mOwner->mFallDistance > 12.5f) {
					StartJumpDust(act, 6, 1);
				}
			} else if (act->mOwner->mFallDistance > 7.5f) {
				JumpAnim(act, 0x1F);
				act->mState = 18;
			} else {
				JumpAnim(act, 0x20);
				act->mState = 19;
			}
		} else if (act->mOwner->mLift.mState != 20) {
			if (act->mState == 3) {
				next    = 0x7C;
				playing = anim == 0x7C;
			} else {
				next    = 0x1D;
				playing = anim == 0x1D || anim == 0x93;
			}
			if (!playing) {
				JumpAnim(act, next, 0.0f);
			}
		}
		break;
	case 4:
		if (act->mOwner->mLift.mState == 20) {
			act->mState = 0;
			return;
		}
		if (model->IsAnimDone() || interrupt) {
			EndJump(act);
		}
		break;
	case 5:
		if (act->mOwner->mLift.mState == 20) {
			act->mState = 0;
			return;
		}
		if (model->IsAnimDone()) {
			JumpAnim(act, 0x8F);
			act->mState = 6;
			StartJumpDust(act, 0x15, 1);
			PlayWiiSoundEffect(&gGame.mSound, 0x87, 1.0f, 0.0f);
			PlayWiiSoundEffect(&gGame.mSound, 0xA3, 1.0f, 0.0f);
		}
		break;
	case 6:
		if (act->mOwner->mLift.mState == 20) {
			act->mState = 0;
			return;
		}
		if ((act->mStrength -= frames) <= 0.0f) {
			s32 id;
			if (act->mTargetStrength >= 50.0f) {
				id = 0x9A;
			} else {
				id = 0x90;
				if (act->mTargetStrength >= 45.0f && !gGame.mEvt.mData.mScript) {
					WiiPlayerState* chr = act->mOwner;
					if (chr->mLift.mState && IsCarrying(&chr->mLift, 22)) {
						PutAwayLiftedObject((XActLift*)&chr->mLift);
					}
				}
			}
			JumpAnim(act, id);
			act->mState = 7;
			fn_800A12F8(&gGame.mEvtPtcl, 0x15, FALSE);
			StopWiiSfx(&gGame.mSound, 0xA3, 0.0f);
		} else if (!IsWiiSfxPlaying(&gGame.mSound, 0xA3)) {
			PlayWiiSoundEffect(&gGame.mSound, 0xA3, 1.0f, 0.0f);
		}
		break;
	case 7:
		if (act->mOwner->mLift.mState == 20) {
			act->mState = 0;
			return;
		}
		if (model->GetAnimId() == 0x147) {
			act->mState = 0;
			return;
		}
		if (model->IsAnimDone()) {
			EndJump(act);
		}
		break;
	case 8:
		if (next < 0) {
			next    = 0x5A;
			playing = anim == next || anim == 0x10F || anim == 0x115;
		}
	case 11:
		if (next < 0) {
			next    = 0x60;
			playing = anim == next || anim == 0x112 || anim == 0x118;
		}
	case 14:
		if (next < 0) {
			next    = 0x63;
			playing = anim == next || anim == 0x11B || anim == 0x11E;
		}
		if (!playing) {
			act->mTargetHeight = act->mTarget->mSelectedFloorHeight;
			const Vec* pos     = act->mOwner->GetPos();
			if (pos->y >= act->mTargetHeight) {
				act->mState = 0;
				return;
			}
			if (act->mTarget) {
				f32 yaw = GetFloorFacingAngle(act->mTarget, pos, act->mTargetFloor, 0, NULL, NULL);
				if (__fpclassifyf(yaw) != FP_NAN) {
					StopPlayerMotion(act->mOwner);
					JumpAnim(act, next);
					act->mOwner->SetRotationY(yaw);
				} else {
					act->mState  = 0;
					act->mTarget = NULL;
				}
			}
		} else if (model->IsAnimDone()) {
			s32 id = -1;
			switch (act->mState) {
			case 8:
				if (id < 0) {
					id = 0x5B;
				}
				break;
			case 11:
				if (id < 0) {
					id = 0x61;
				}
				break;
			case 14:
				if (id < 0) {
					id = 0x64;
				}
				break;
			}
			JumpAnim(act, id);
			++act->mState;
			if (CheckStageFlag(gGame.mStage, 1)) {
				WiiPlayerPlug* plug = &act->mOwner->mPlug;
				if (plug->mConnection) {
					plug->mConnection = 0;
					PlayWiiSoundEffect(&gGame.mSound, 0xA4, 1.0f, 0.0f);
				}
			}
			if (act->mTarget) {
				act->mArcPosition = *act->mOwner->GetPos();
				act->mRise        = act->mTargetHeight - act->mOwner->GetPos()->y;
				act->mArcAngle    = 0.0f;
				if (act->mRise > 6.0f) {
					act->mArcStep = 0.8f * (HALF_PI / act->mRise);
				} else {
					act->mArcStep = DEG2RAD * 12.0f;
				}
				act->mForward = 5.0f;
				f32 radius    = act->mOwner->mMoveScale.x * GetFloorExtent(act->mOwner, 0).x;
				f32 height    = 0.5f * (act->mOwner->mMoveScale.x * (GetFloorOffset(act->mOwner, 0).y + GetFloorExtent(act->mOwner, 0).y));
				Mtx rot;
				PSMTXRotRad(rot, 'y', act->mOwner->mMoveRotate.y);
				Vec offset = act->mOwner->mMoveScale * GetFloorOffset(act->mOwner, 0);
				PSMTXMultVec(rot, &offset, &offset);
				f32 yaw = act->mOwner->mMoveRotate.y;
				f32 x   = 3.0f * (f32)sin(yaw) + (offset.x + act->mOwner->GetPos()->x);
				f32 z   = 3.0f * (f32)cos(yaw) + (offset.z + act->mOwner->GetPos()->z);
				f32 y   = 0.5f + act->mTargetHeight;
				for (s32 i = 0; i < gGame.mXObjs.mObjectCount; ++i) {
					XObj* obj = gGame.mXObjs.mObjects[i];
					if (!(obj->mStatus.mFlags & 1) || !(obj->mStatus.mFlags & 0x10000) || (obj->mStatus.mFlags & 0x100000)) {
						continue;
					}
					if (obj->GetCategory() == 7 || obj->GetCategory() == 20 || act->mOwner == obj || obj->GetKind() >= 10000) {
						continue;
					}
					f32 dx = obj->GetPos()->x - x;
					f32 dz = obj->GetPos()->z - z;
					if (FSqrt(dx * dx + dz * dz) > 100.0f + (radius + obj->mFloorXZExtent[0])) {
						continue;
					}
					CVec push, contact;
					u32 touch = ProbeObjectFloor(obj, CVec(x, y, z), CVec(x, y, z), radius, height, &push, &contact, NULL) & 3;
					if (touch == 3) {
						act->mForward = 0.0f;
						act->mRise    = 0.8f * act->mRise;
						break;
					}
				}
				AdvanceJumpArc(act, frames);
			}
			if (!act->mTarget || act->mRise <= 0.0f) {
				EndJump(act);
				act->mTarget = NULL;
			}
		}
		break;
	case 9:
		if (next < 0) {
			next    = 0x5B;
			playing = anim == next || anim == 0x110 || anim == 0x116;
		}
	case 12:
		if (next < 0) {
			next    = 0x61;
			playing = anim == next || anim == 0x113 || anim == 0x119;
		}
	case 15:
		if (next < 0) {
			next    = 0x64;
			playing = anim == next || anim == 0x11C || anim == 0x11F;
		}
		if (!playing) {
			JumpAnim(act, next);
		}
		if (AdvanceJumpArc(act, frames)) {
			if (act->mForward == 0.0f) {
				JumpAnim(act, 0x1D, 0.0f);
				act->mState = 17;
				return;
			}
			s32 id = -1;
			switch (act->mState) {
			case 9:
				if (id < 0) {
					id = 0x5C;
				}
			case 12:
				if (id < 0) {
					id = 0x62;
				}
			case 15:
				if (id < 0) {
					id = 0x65;
				}
			}
			JumpAnim(act, id);
			++act->mState;
		}
		break;
	case 10:
		if (next < 0) {
			next    = 0x5C;
			playing = anim == next || anim == 0x111 || anim == 0x117;
		}
	case 13:
		if (next < 0) {
			next    = 0x62;
			playing = anim == next || anim == 0x114 || anim == 0x11A;
		}
	case 16:
		if (next < 0) {
			next    = 0x65;
			playing = anim == next || anim == 0x11D || anim == 0x120;
		}
		if (!playing) {
			JumpAnim(act, next);
			return;
		}
		if (model->IsAnimDone()) {
			EndJump(act);
			act->mTarget = NULL;
		} else if (interrupt) {
			act->mOwner->SetAnimRate(4.0f);
		}
		break;
	case 18:
		if (model->IsAnimDone() || interrupt) {
			EndJump(act);
		}
		break;
	case 19:
		if (model->IsAnimDone() || interrupt) {
			EndJump(act);
		}
		break;
	}
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
