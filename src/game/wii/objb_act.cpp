// chibi robo's actions, the GameCube's objb_act.cpp: .text 0x800F48E0-0x801067C0 (wii_units.md Nr. 67). The
// functions come in the reverse of the GameCube's order, GetTargetFloorPosition first and UpdatePushedObject
// last; ResetTumbleAction (the GameCube's objb_act.cpp, once xacttumble.cpp) follows the file.
// NONMATCHING: all action updates have source now; the remaining differences are in inline boundaries,
// aggregate temporaries and scheduling. The GameCube's src/game/objb_act.cpp was a template, not a copy. Of the
// ported functions, the IsCarrying* checks and IsUsingMug differ only in which of r4-r7 holds each nested inline's
// result; GetStagePlacementScale, GetTargetFloorPosition, GetLiftTargetContactOffset and UpdatePushedObject in
// scheduling and stack slots. UpdatePlayerRopeAction has the retail calls, but its sin/cos and vector temporaries differ.
#include "rvl_msl/PowerPC_EABI_Support/MSL_C/MSL_Common/math_api.h" // the Wii MSL's out-of-line __fpclassifyf
#include "nw4r/ef/ef_resource.h"
#include "nw4r/ef/ef_res_emitter.h"
#include "game/wii/xobjflags.h" // first: its AnimSfx needs the real nw4r::math, ahead of xobj.h's shim
#include "game/wii/xobj.h"
#include "game/xobjattachment.h"
#include "game/wii/xobjleticker.h"
#include "game/wii/cgame.h"
#include "game/wii/csound.h"
#include "game/wii/xact.h"
#include "game/wii/cpadact.h"
#include "game/wii/menuutil.h"
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
StagePlacement* GetStagePlacementByIndex(s32 index);
StagePlacement* GetStageObjectPlacementByIndex(s32 index);
u16 GetItemFlagsMasked(s16 item, u16 mask);
s16 GetItemPickupHoldTime(s16 item);
s8 GetItemSoundIndex(s16 item, s32 kind);
u32 GetItemReleaseSoundId(s8 index);
void SetPlugCordSpeed(WiiPlayerPlug* plug, f32 speed);
u32 GetPlayerActionParameter(s32 index);
s32 GetStageCollectionFlagBase(s32 group);
XObj* SpawnObjectFromPlacement(XOBJS* list, CoordInfo* info, s32 direct, BOOL async);
s32 RemoveInventoryItemCount(Inventory* inventory, s16 id, s32 amount);
void StartOrQueueEventScript(CEvt* evt, u8* script, XObj* target, s16 useValue, s16 hitValue);
void ConfigureSubwindowCamera(FbSubwin* subwin, const Vec& interest, f32 angleY, f32 angleX, f32 distance, f32 fov);
void ResetPlayerPlugState(XObjPlug* plug, BOOL resetJoint);
void fn_800E3AE4(WiiPlayerState* chr, f32 frame);
s32 FindPlayerSupportingFloor(WiiPlayerState* chr, s32 group, s32 preferred);
BOOL IsLiftTargetPlug(XActLift* act);
void ApplyPlayerItemTintToTarget(XAction* act);
extern s32 gRefreshRate;
Vec GetLiftTargetContactOffset(const XActLift* act, XObj* target);
Vec GetTargetFloorPosition(XAction* act);
BOOL IsCarryingToothbrush(XActLift* act);
BOOL IsCarryingSpoon(XActLift* act);
BOOL IsCarryingMug(XActLift* act);
BOOL IsCarryingSquirter(XActLift* act);
s32 GetOwnedInventoryItemCount(Params* stats, s16 id);
u32 GetMaterialSoundId(s32 material);
void HSD_MtxGetScale(Mtx mat, Vec* vec);
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

static const s32 sSquirterChargeLevels[11] = { 0, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3 };

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

static inline Vec GetPlacementScale(const StagePlacement* placement)
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

static inline void StandAfterLift(WiiPlayerState* owner, f32 blend = 0.0f)
{
	owner->SetAnim(0x11, FALSE, 0.0f, 1.0f, blend, 15.0f);
	owner->mIdleLoops = 3;
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

static inline void StandAfterLift(WiiPlayerState* owner, f32 blend);

/**
 * @note Address: 0x801016DC
 * @note Size: 0x229C
 */
// NONMATCHING: the calls, virtual ones included, follow retail's except that state 5 reads the end frame before the
// frame; aggregate return homes and register/schedule allocation still differ.
void UpdateLadderActionAnimation(XActLadder* act, s32 anim, CObj* model, u32 frames, f32 frame, f32 end)
{
	switch (act->mState) {
	case 1:
		if (anim != 0x66) {
			act->mOwner->SetAnim(0x66, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			if (act->mTarget != NULL) {
				act->mOwner->SetRotationY(WrapAngle(PI + act->mTarget->mMoveRotate.y));
				CVec chrPos(*act->mOwner->GetPos());
				chrPos.y = 0.0f;
				CVec ladPos(*act->mTarget->GetPos());
				ladPos.y = 0.0f;
				CVec off(0.0f, 0.0f, 4.0f);
				Mtx matrix;
				PSMTXRotRad(matrix, 'y', act->mTarget->mMoveRotate.y);
				PSMTXMultVec(matrix, &off, &off);
				ladPos += off;
				act->mPos   = ladPos;
				act->mDist  = PSVECDistance(&ladPos, &chrPos);
				act->mAngle = atan2(chrPos.x - ladPos.x, chrPos.z - ladPos.z);
			}
		} else if (model->IsAnimDone()) {
			act->mOwner->SetPos(act->mOwner->GetJointWorldPos("second_null"));
			SyncRopePosition(act->mOwner);
			act->mOwner->GetResNode("second_null").SetTranslate(0.0f, 0.0f, 0.0f);
			act->mOwner->SetAnim(0x67, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
			act->mState = 5;
		} else {
			if (act->mTarget != NULL) {
				Vec move = *act->mTarget->GetPos() - act->mTarget->mLastPos;
				act->mPos.x += move.x;
				act->mPos.z += move.z;
				if (act->mDist > 0.0f) {
					if ((act->mDist -= 0.2f * frames) < 0.0f) {
						act->mDist = 0.0f;
					}
					act->mOwner->SetTranslateX(act->mDist * (f32)sin(act->mAngle) + act->mPos.x);
					act->mOwner->SetTranslateZ(act->mDist * (f32)cos(act->mAngle) + act->mPos.z);
				} else {
					act->mOwner->SetTranslateX(act->mPos.x);
					act->mOwner->SetTranslateZ(act->mPos.z);
				}
			}
			if (CheckStageFlag(gGame.mStage, 1) && act->mOwner->mPlug.mConnection && frame >= 25.0f) {
				act->mOwner->mPlug.mConnection = 0;
				PlayWiiSoundEffect(&gGame.mSound, 0xA4, 1.0f, 0.0f);
			}
		}
		break;
	case 2:
		if (anim != 0x6D) {
			act->mOwner->SetAnim(0x6D, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			if (act->mTarget != NULL) {
				act->mOwner->SetRotationY(WrapAngle(act->mTarget->mMoveRotate.y));
				CVec chrPos(*act->mOwner->GetPos());
				chrPos.y = 0.0f;
				CVec ladPos(*act->mTarget->GetPos());
				ladPos.y = 0.0f;
				CVec off(0.0f, 0.0f, -1.5f);
				Mtx matrix;
				PSMTXRotRad(matrix, 'y', act->mTarget->mMoveRotate.y);
				PSMTXMultVec(matrix, &off, &off);
				ladPos += off;
				act->mPos   = ladPos;
				act->mDist  = PSVECDistance(&ladPos, &chrPos);
				act->mAngle = atan2(chrPos.x - ladPos.x, chrPos.z - ladPos.z);
			}
		} else if (model->IsAnimDone()) {
			act->mOwner->SetPos(act->mOwner->GetJointWorldPos("second_null"));
			SyncRopePosition(act->mOwner);
			act->mOwner->GetResNode("second_null").SetTranslate(0.0f, 0.0f, 0.0f);
			act->mOwner->SetRotationY(PI + act->mOwner->mMoveRotate.y);
			act->mOwner->SetAnim(0x68, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
			act->mState = 5;
		} else {
			if (act->mTarget != NULL) {
				Vec move = *act->mTarget->GetPos() - act->mTarget->mLastPos;
				act->mPos.x += move.x;
				act->mPos.z += move.z;
				if (act->mDist > 0.0f && frame > 20.0f) {
					if ((act->mDist -= 0.2f * frames) < 0.0f) {
						act->mDist = 0.0f;
					}
					act->mOwner->SetTranslateX(act->mDist * (f32)sin(act->mAngle) + act->mPos.x);
					act->mOwner->SetTranslateZ(act->mDist * (f32)cos(act->mAngle) + act->mPos.z);
				}
			}
			if (CheckStageFlag(gGame.mStage, 1) && act->mOwner->mPlug.mConnection && frame >= 50.0f) {
				act->mOwner->mPlug.mConnection = 0;
				PlayWiiSoundEffect(&gGame.mSound, 0xA4, 1.0f, 0.0f);
			}
		}
		break;
	case 3:
		if (anim != 0x87) {
			if (act->mTarget != NULL) {
				if (act->mTarget->mFloors[act->mTargetFloor].mId == 0) {
					CVec chrPos(*act->mOwner->GetPos());
					chrPos.y = 0.0f;
					CVec point(GetTargetFloorPosition(act));
					point.y = 0.0f;
					act->mOwner->SetRotationY(PI + (f32)atan2(point.x - chrPos.x, point.z - chrPos.z));
				} else {
					act->mOwner->SetRotationY(act->mTarget->mMoveRotate.y);
				}
				act->mOwner->SetAnim(0x87, TRUE, 0.0f, 1.0f, 1.0f, 15.0f);
				act->mOwner->SetAnim(0x87, TRUE, act->mOwner->GetAnimEndFrame(), 1.0f, 1.0f, 15.0f);
				act->mOwner->UpdateAnim(0.0f);
				CVec delta = act->mOwner->GetJointWorldPos("second_null") - *act->mOwner->GetPos();
				act->mOwner->SetTranslateX(act->mOwner->GetPos()->x - delta.x);
				act->mOwner->SetTranslateZ(act->mOwner->GetPos()->z - delta.z);
				act->mOwner->SetTranslateY(act->mTarget->mMaxFloorHeight);
				act->mOwner->SetAnim(0x87, TRUE, 0.0f, 1.0f, 1.0f, 15.0f);
				CVec chrPos(*act->mOwner->GetPos());
				chrPos.y = 0.0f;
				CVec ladPos(*act->mTarget->GetPos());
				ladPos.y = 0.0f;
				CVec off(0.0f, 0.0f, -1.0f);
				Mtx matrix;
				PSMTXRotRad(matrix, 'y', act->mTarget->mMoveRotate.y);
				PSMTXMultVec(matrix, &off, &off);
				ladPos += off;
				act->mPos   = ladPos;
				act->mDist  = PSVECDistance(&ladPos, &chrPos);
				act->mAngle = atan2(chrPos.x - ladPos.x, chrPos.z - ladPos.z);
			}
			if (CheckStageFlag(gGame.mStage, 1) && act->mOwner->mPlug.mConnection) {
				act->mOwner->mPlug.mConnection = 0;
				PlayWiiSoundEffect(&gGame.mSound, 0xA4, 1.0f, 0.0f);
			}
		} else if (model->IsAnimDone()) {
			nw4r::g3d::ResNode node = act->mOwner->FindResNode("second_null"); // looked up but unused, as in retail
			act->mOwner->SetRotationY(PI + act->mOwner->mMoveRotate.y);
			CVec delta = act->mOwner->GetWorldPos("second_null") - *act->mOwner->GetPos();
			delta.x    = -delta.x;
			delta.z    = -delta.z;
			act->mOwner->SetPos(*act->mOwner->GetPos() + delta);
			SyncRopePosition(act->mOwner);
			act->mOwner->GetResNode("second_null").SetTranslate(0.0f, 0.0f, 0.0f);
			act->mOwner->SetAnim(0x68, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
			act->mState         = 5;
			act->mMoveDirection = 0;
		} else if (act->mTarget != NULL) {
			Vec move = *act->mTarget->GetPos() - act->mTarget->mLastPos;
			act->mPos.x += move.x;
			act->mPos.z += move.z;
			if (act->mDist > 0.0f) {
				if ((act->mDist -= 0.4f * frames) < 0.0f) {
					act->mDist = 0.0f;
				}
				act->mOwner->SetTranslateX(act->mDist * (f32)sin(act->mAngle) + act->mPos.x);
				act->mOwner->SetTranslateZ(act->mDist * (f32)cos(act->mAngle) + act->mPos.z);
			} else {
				act->mOwner->SetTranslateX(act->mPos.x);
				act->mOwner->SetTranslateZ(act->mPos.z);
			}
			act->mOwner->SetTranslateY(act->mTarget->mMaxFloorHeight);
		}
		break;
	case 4:
		act->mOwner->SetAnim(0x68, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
		act->mState = 5;
		break;
	case 5:
		if (anim == 0x69 || anim == 0x6A || anim == 0x6B || anim == 0x6C) {
			if (model->GetAnimFrame() >= model->GetAnimEndFrame()) {
				CVec hand = act->mOwner->GetNodeTranslate("second_null");
				if (hand.y < 0.0f) {
					hand.y = (s32)(hand.y - 0.5f);
				}
				act->mOwner->GetResNode("second_null").SetTranslate(hand.x, hand.y, hand.z);
				act->mOwner->SetPos(act->mOwner->GetJointWorldPos("second_null"));
				SyncRopePosition(act->mOwner);
				act->mOwner->GetResNode("second_null").SetTranslate(0.0f, 0.0f, 0.0f);
				if (act->mMoveDirection == 1) {
					if (act->mTarget != NULL && 14.0f + act->mOwner->GetJointWorldPos("second_null").y >= act->mTarget->mFloorHeights[0]) {
						act->mState = 6;
						if (anim == 0x69 || anim == 0x6B) {
							act->mOwner->SetAnim(0x6F, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
						} else if (anim == 0x6A || anim == 0x6C) {
							act->mOwner->SetAnim(0x6E, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
						}
					} else if (anim == 0x69 || anim == 0x6B) {
						act->mOwner->SetAnim(0x6A, TRUE, 0.0f, 1.0f, 1.0f, 15.0f);
					} else if (anim == 0x6A || anim == 0x6C) {
						act->mOwner->SetAnim(0x69, TRUE, 0.0f, 1.0f, 1.0f, 15.0f);
					}
				} else if (act->mMoveDirection == -1) {
					if (act->mOwner->GetHeight() <= 2.0f) {
						act->mState = 7;
						if (anim == 0x69 || anim == 0x6B) {
							act->mOwner->SetAnim(0x71, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
						} else if (anim == 0x6A || anim == 0x6C) {
							act->mOwner->SetAnim(0x70, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
						}
					} else if (anim == 0x69 || anim == 0x6B) {
						act->mOwner->SetAnim(0x6C, TRUE, 0.0f, 1.0f, 1.0f, 15.0f);
					} else if (anim == 0x6A || anim == 0x6C) {
						act->mOwner->SetAnim(0x6B, TRUE, 0.0f, 1.0f, 1.0f, 15.0f);
					}
				} else if (anim == 0x69 || anim == 0x6B) {
					act->mOwner->SetAnim(0x68, TRUE, 0.0f, 1.0f, 1.0f, 15.0f);
				} else if (anim == 0x6A || anim == 0x6C) {
					act->mOwner->SetAnim(0x67, TRUE, 0.0f, 1.0f, 1.0f, 15.0f);
				}
				if (act->mBPressed && act->mState == 5) {
					act->mState = 9;
				}
			}
		} else {
			if (act->mMoveDirection == 1) {
				if (act->mTarget != NULL && 14.0f + act->mOwner->GetJointWorldPos("second_null").y >= act->mTarget->mFloorHeights[0]) {
					act->mState = 6;
					if (anim == 0x67) {
						act->mOwner->SetAnim(0x6E, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
					} else if (anim == 0x68) {
						act->mOwner->SetAnim(0x6F, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
					}
				} else if (anim == 0x67) {
					act->mOwner->SetAnim(0x69, TRUE, 0.0f, 1.0f, 1.0f, 15.0f);
				} else if (anim == 0x68) {
					act->mOwner->SetAnim(0x6A, TRUE, 0.0f, 1.0f, 1.0f, 15.0f);
				}
			} else if (act->mMoveDirection == -1) {
				if (act->mOwner->GetHeight() <= 2.0f) {
					act->mState = 7;
					if (anim == 0x67) {
						act->mOwner->SetAnim(0x70, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
					} else if (anim == 0x68) {
						act->mOwner->SetAnim(0x71, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
					}
				} else if (anim == 0x67) {
					act->mOwner->SetAnim(0x6B, TRUE, 0.0f, 1.0f, 1.0f, 15.0f);
				} else if (anim == 0x68) {
					act->mOwner->SetAnim(0x6C, TRUE, 0.0f, 1.0f, 1.0f, 15.0f);
				}
			}
			if (act->mBPressed && act->mState == 5) {
				act->mState = 9;
			}
		}
		break;
	case 6:
		if (anim != 0x6E && anim != 0x6F) {
			if (anim == 0x69 || anim == 0x6B) {
				act->mOwner->SetAnim(0x6F, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			} else if (anim == 0x6A || anim == 0x6C) {
				act->mOwner->SetAnim(0x6E, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			} else if (anim == 0x67) {
				act->mOwner->SetAnim(0x6E, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			} else if (anim == 0x68) {
				act->mOwner->SetAnim(0x6F, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			}
		} else if (model->IsAnimDone()) {
			act->mOwner->SetPos(act->mOwner->GetJointWorldPos("second_null"));
			SyncRopePosition(act->mOwner);
			act->mOwner->GetResNode("second_null").SetTranslate(0.0f, 0.0f, 0.0f);
			StandAfterLift(act->mOwner, 1.0f);
			act->mState  = 0;
			act->mTarget = NULL;
		}
		break;
	case 7:
		if (anim != 0x70 && anim != 0x71) {
			if (anim == 0x69 || anim == 0x6B) {
				act->mOwner->SetAnim(0x71, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			} else if (anim == 0x6A || anim == 0x6C) {
				act->mOwner->SetAnim(0x70, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			} else if (anim == 0x67) {
				act->mOwner->SetAnim(0x70, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			} else if (anim == 0x68) {
				act->mOwner->SetAnim(0x71, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			}
		} else if (model->IsAnimDone()) {
			act->mOwner->SetPos(act->mOwner->GetJointWorldPos("second_null"));
			SyncRopePosition(act->mOwner);
			act->mOwner->GetResNode("second_null").SetTranslate(0.0f, 0.0f, 0.0f);
			StandAfterLift(act->mOwner, 1.0f);
			act->mState  = 0;
			act->mTarget = NULL;
		}
		break;
	case 8:
		if (anim == 0x69 || anim == 0x6B) {
			act->mOwner->SetAnim(0x73, TRUE, 10.0f, 1.0f, 1.0f, 15.0f);
		} else if (anim == 0x6A || anim == 0x6C) {
			act->mOwner->SetAnim(0x72, TRUE, 10.0f, 1.0f, 1.0f, 15.0f);
		} else if (anim == 0x67) {
			act->mOwner->SetAnim(0x72, TRUE, 10.0f, 1.0f, 1.0f, 15.0f);
		} else if (anim == 0x68) {
			act->mOwner->SetAnim(0x73, TRUE, 10.0f, 1.0f, 1.0f, 15.0f);
		}
		act->mState = 9;
	case 9:
		if (anim != 0x72 && anim != 0x73) {
			if (anim == 0x69 || anim == 0x6B) {
				act->mOwner->SetAnim(0x73, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			} else if (anim == 0x6A || anim == 0x6C) {
				act->mOwner->SetAnim(0x72, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			} else if (anim == 0x67) {
				act->mOwner->SetAnim(0x72, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			} else if (anim == 0x68) {
				act->mOwner->SetAnim(0x73, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			}
		} else if (model->IsAnimDone()) {
			act->mOwner->SetPos(act->mOwner->GetJointWorldPos("second_null"));
			SyncRopePosition(act->mOwner);
			act->mOwner->GetResNode("second_null").SetTranslate(0.0f, 0.0f, 0.0f);
			if (act->mOwner->GetAttachment() == 1) {
				act->mState                 = 0;
				act->mOwner->mCopter.mState = 4;
			} else {
				act->mOwner->SetAnim(0x74, TRUE, 0.0f, 1.0f, 1.0f, 15.0f);
				act->mState = 10;
			}
			act->mTarget = NULL;
		}
		break;
	case 10:
		if (act->mOwner->GetHeight() <= 0.0f) {
			if (act->mOwner->mFallDistance > 30.0f) {
				act->mState = 0;
				act->mOwner->SetAnim(0x8E, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
				act->mOwner->mJump.mState = 5;
				ApplyPlayerJumpStrength((XActJump*)&act->mOwner->mJump, act->mOwner->mFallDistance);
				act->mOwner->SetColor(1, 255, 0, 0, 0.0f);
				act->mOwner->SetColor(1, 0, 0, 0, 0.3f);
				StartScreenFade(&gGame.mScrnTex.mColor, 255, 0, 0, 0, 100, 100, 200, 0, 0.3f);
			} else {
				act->mOwner->SetAnim(0x75, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
				act->mState = 11;
			}
		}
		break;
	case 11:
		if (model->IsAnimDone()) {
			StandAfterLift(act->mOwner);
			act->mState = 0;
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
				f32 radius    = act->mOwner->GetScale()->x * GetFloorExtent(act->mOwner, 0).x;
				f32 height    = 0.5f * (act->mOwner->GetScale()->x * (GetFloorOffset(act->mOwner, 0).y + GetFloorExtent(act->mOwner, 0).y));
				Mtx rot;
				PSMTXRotRad(rot, 'y', act->mOwner->mMoveRotate.y);
				Vec offset = *act->mOwner->GetScale() * GetFloorOffset(act->mOwner, 0);
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
			act->mContactEffect = NULL;
		}
		InfoScrn_ApplyObjectGaugeCommand(&gGame.mInfoScrn, NULL, 0, 1, 3);
	}
}

/**
 * @note Address: 0x800FCB14
 * @note Size: 0xB8
 */
// NONMATCHING: five return-type/lifetime forms tried; retail loads the placement and CVec vtable
// before the prologue, while this build schedules them after it; the six conversions otherwise match
CVec GetStagePlacementScale(const XObj* obj)
{
	return CVec(GetPlacementScale(obj->mCoord.mPlacement));
}

static inline f32 GetHangEdgeOutwardYaw(const XObj* object, const Vec* point, s32 index)
{
	Vec first  = PushEdgePoint(object, index, true, point->y);
	Vec second = PushEdgePoint(object, index, false, point->y);
	CVec outward;
	GetOutwardPerpendicularToEdge((Vec*)point, &first, &second, &outward);
	return WrapAngle(atan2(outward.x, outward.z));
}

static inline Vec GetScaledTargetFloorExtent(const XAction* act)
{
	return *act->mTarget->GetScale() * GetFloorExtent(act->mTarget, act->mTargetFloor);
}

/**
 * @note Address: 0x800FDA78
 * @note Size: 0x20E0
 */
// NONMATCHING: one CVec addition and its aggregate temporaries still differ; the state logic and all other calls are
// reconstructed, including the three static contact objects and their registration calls.
// NONMATCHING: the Wii call list and state transitions match; CVec temporary lifetimes and floating-point scheduling
// around the floor-contact calculations make this build 188 bytes larger
void UpdatePlayerHangAction(XActHang* act, s32 anim, CObj* model, u32 frames, f32 frame, f32 end)
{
	switch (act->mState) {
	case 1: {
		static CVec contact, normal, start;
		if (anim != 0x13) {
			if (act->mTarget != NULL) {
				f32 angle = GetFloorFacingAngle(act->mTarget, act->mOwner->GetPos(), act->mTargetFloor, 1, &contact, &normal);
				if (__fpclassifyf(angle) != FP_NAN) {
					StopPlayerMotion(act->mOwner);
					act->mOwner->SetAnim(0x13, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
					act->mOwner->SetRotationY(angle);
					start     = *act->mOwner->GetPos();
					contact.y = start.y;
				} else {
					act->mState  = 0;
					act->mTarget = NULL;
					return;
				}
			}
			if (CheckStageFlag(gGame.mStage, 1) && act->mOwner->mPlug.mConnection) {
				act->mOwner->mPlug.mConnection = 0;
				PlayWiiSoundEffect(&gGame.mSound, 0xA4, 1.0f, 0.0f);
			}
		} else if (model->IsAnimDone()) {
			act->mOwner->SetAnim(0x14, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
			act->mState         = 5;
			act->mMoveDirection = 0;
			if (act->mTarget != NULL) {
				if (act->mTarget->mFloors[act->mTargetFloor].mId == 0) {
					CVec owner(*act->mOwner->GetPos());
					owner.y = 0.0f;
					CVec floor(GetTargetFloorPosition(act));
					floor.y           = 0.0f;
					act->mFloorCenter = floor;
					act->mDistance    = PSVECDistance(&owner, &floor);
				} else {
					act->mOwner->SetPos(contact - normal * 4.0f);
				}
			}
		} else if (act->mTarget != NULL) {
			if (act->mTarget->mFloors[act->mTargetFloor].mId == 0) {
				f32 radius = act->mOwner->GetScale()->x * GetScaledTargetFloorExtent(act).x;
				CVec owner(*act->mOwner->GetPos());
				owner.y = 0.0f;
				CVec floor(GetTargetFloorPosition(act));
				floor.y = 0.0f;
				if (PSVECDistance(&owner, &floor) < 4.0f + radius) {
					f32 yaw = act->mOwner->mMoveRotate.y;
					act->mOwner->SetTranslateX(act->mOwner->GetPos()->x - 0.2f * (f32)sin(yaw));
					act->mOwner->SetTranslateZ(act->mOwner->GetPos()->z - 0.2f * (f32)cos(yaw));
				}
			} else {
				CVec delta = (contact - normal * 4.0f) - start;
				act->mOwner->SetPos((contact - normal * 4.0f) - delta * (1.0f - frame / end));
			}
		}
		break;
	}
	case 4:
		if (anim != 0x7A && anim != 0xCA) {
			if (act->mTarget != NULL) {
				s32 index = FindPlayerSupportingFloor(act->mOwner, 1, act->mTargetFloor);
				if (index >= 0) {
					act->mTargetFloor = index;
				}
				f32 yaw;
				if (act->mTarget->mFloors[act->mTargetFloor].mId == 0) {
					CVec owner(*act->mOwner->GetPos());
					owner.y = 0.0f;
					CVec floor(GetTargetFloorPosition(act));
					floor.y = 0.0f;
					yaw     = PI + (f32)atan2(floor.x - owner.x, floor.z - owner.z);
				} else {
					yaw = PI + GetHangEdgeOutwardYaw(act->mTarget, act->mOwner->GetPos(), act->mTargetFloor);
				}
				yaw                   = WrapAngle(yaw);
				act->mHangStartOffset = *act->mOwner->GetPos() - *act->mTarget->GetPos();
				Vec pos               = *act->mOwner->GetPos();
				pos.x += 1.5f * (f32)sin(PI + yaw);
				pos.z += 1.5f * (f32)cos(PI + yaw);
				act->mOwner->SetPos(pos);
				SyncRopePosition(act->mOwner);
			}
			act->mOwner->SetAnim(0x7A, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			return;
		}
		if (model->IsAnimDone()) {
			act->mOwner->SetPos(*act->mTarget->GetPos() + act->mHangStartOffset);
			SyncRopePosition(act->mOwner);
			act->mState               = 0;
			act->mTarget              = NULL;
			act->mOwner->mJump.mState = 3;
			if (CheckStageFlag(gGame.mStage, 1) && act->mOwner->mPlug.mConnection) {
				act->mOwner->mPlug.mConnection = 0;
				PlayWiiSoundEffect(&gGame.mSound, 0xA4, 1.0f, 0.0f);
			}
			return;
		}
		if (!act->mStickAway) {
			return;
		}
		if (act->mOwner->GetAnimFrame() / act->mOwner->GetAnimEndFrame() <= 0.6f) {
			StandAfterLift(act->mOwner);
			act->mState  = 0;
			act->mTarget = NULL;
			return;
		}
		act->mOwner->SetPos(*act->mTarget->GetPos() + act->mHangStartOffset);
		SyncRopePosition(act->mOwner);
		if (act->mOwner->GetAttachment() == 2 || act->mOwner->mLift.mState) {
			act->mOwner->mJump.mState = 2;
			act->mState               = 0;
			act->mTarget              = NULL;
			if (CheckStageFlag(gGame.mStage, 1) && act->mOwner->mPlug.mConnection) {
				act->mOwner->mPlug.mConnection = 0;
				PlayWiiSoundEffect(&gGame.mSound, 0xA4, 1.0f, 0.0f);
			}
		} else if (act->mTarget != NULL && (act->mTarget->mStatus.mFlags & 0x800000000ULL)) {
			act->mOwner->mLadder.mState       = 3;
			act->mOwner->mLadder.mTarget      = act->mTarget;
			act->mOwner->mLadder.mTargetFloor = act->mTarget->mSelectedFloorIndex;
			act->mState                       = 0;
			act->mTarget                      = NULL;
		} else if (act->mTarget != NULL && (act->mTarget->mStatus.mFlags & 0x1000000000ULL)) {
			act->mOwner->mRope.mState       = 3;
			act->mOwner->mRope.mTarget      = act->mTarget;
			act->mOwner->mRope.mTargetFloor = act->mTarget->mSelectedFloorIndex;
			act->mState                     = 0;
			act->mTarget                    = NULL;
			return;
		} else {
			act->mState = 3;
			return;
		}
	case 3:
		if (anim != 0x76) {
			if (act->mTarget != NULL) {
				s32 index = FindPlayerSupportingFloor(act->mOwner, 1, act->mTargetFloor);
				if (index >= 0) {
					act->mTargetFloor = index;
				}
				CVec contact(0.0f, 0.0f, 0.0f);
				f32 yaw;
				if (act->mTarget->mFloors[act->mTargetFloor].mId == 0) {
					CVec owner(*act->mOwner->GetPos());
					owner.y = 0.0f;
					CVec floor(GetTargetFloorPosition(act));
					floor.y = 0.0f;
					yaw     = PI + (f32)atan2(floor.x - owner.x, floor.z - owner.z);
				} else {
					yaw = PI + GetFloorFacingAngle(act->mTarget, act->mOwner->GetPos(), act->mTargetFloor, 0, &contact, NULL);
					if (__fpclassifyf(yaw) == FP_NAN) {
						act->mOwner->mJump.mState = 2;
						act->mState               = 0;
						act->mTarget              = NULL;
						return;
					}
				}
				yaw = WrapAngle(yaw);
				act->mOwner->SetRotationY(yaw);
				act->mOwner->SetAnim(0x13, TRUE, 0.0f, 1.0f, 1.0f, 15.0f);
				act->mOwner->SetAnim(0x13, TRUE, act->mOwner->GetAnimEndFrame(), 1.0f, 1.0f, 15.0f);
				act->mOwner->UpdateAnim(0.0f);
				CVec local = act->mOwner->GetNodeTranslate("second_null");
				CVec delta = act->mOwner->GetWorldPos("second_null") - *act->mOwner->GetPos();
				delta.y    = -delta.y;
				act->mOwner->SetPos(*act->mOwner->GetPos() + delta);
				SyncRopePosition(act->mOwner);
				act->mOwner->SetAnim(0x76, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
				local.x = -local.x;
				local.z = -local.z;
				act->mOwner->GetResNode("second_null").SetTranslate(local.x, local.y, local.z);
				Vec pos = *act->mOwner->GetPos();
				f32 distance;
				if (act->mTarget->mFloors[act->mTargetFloor].mId == 0) {
					CVec owner(*act->mOwner->GetPos());
					owner.y = 0.0f;
					CVec floor(GetTargetFloorPosition(act));
					floor.y  = 0.0f;
					yaw      = PI + (f32)atan2(floor.x - owner.x, floor.z - owner.z);
					distance = 1.0f + (PSVECDistance(&owner, &floor) - act->mOwner->GetScale()->x * GetScaledTargetFloorExtent(act).x);
				} else {
					pos.x    = contact.x;
					pos.z    = contact.z;
					distance = 1.0f;
				}
				yaw = WrapAngle(yaw);
				pos.x += distance * (f32)sin(PI + yaw);
				pos.z += distance * (f32)cos(PI + yaw);
				Vec offset = act->mTargetOffset;
				Mtx rotate;
				PSMTXRotRad(rotate, 'y', yaw);
				PSMTXMultVec(rotate, &offset, &offset);
				pos.x += offset.x;
				pos.z += offset.z;
				act->mOwner->SetPos(pos);
				SyncRopePosition(act->mOwner);
			}
			if (CheckStageFlag(gGame.mStage, 1) && act->mOwner->mPlug.mConnection) {
				act->mOwner->mPlug.mConnection = 0;
				PlayWiiSoundEffect(&gGame.mSound, 0xA4, 1.0f, 0.0f);
			}
		} else if (model->IsAnimDone()) {
			nw4r::g3d::ResNode node = act->mOwner->FindResNode("second_null"); // looked up but unused, as in retail
			act->mOwner->SetRotationY(PI + act->mOwner->mMoveRotate.y);
			act->mOwner->SetAnim(0x13, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
			act->mOwner->SetAnim(0x13, TRUE, act->mOwner->GetAnimEndFrame(), 1.0f, 1.0f, 15.0f);
			act->mOwner->UpdateAnim(0.0f);
			CVec local = act->mOwner->GetNodeTranslate("second_null");
			CVec delta = act->mOwner->GetWorldPos("second_null") - *act->mOwner->GetPos(); // computed but unused, as in retail
			act->mOwner->GetResNode("second_null").SetTranslate(local.x, local.y, local.z);
			act->mOwner->SetAnim(0x14, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
			act->mState         = 5;
			act->mMoveDirection = 0;
			if (act->mTarget != NULL && act->mTarget->mFloors[act->mTargetFloor].mId == 0) {
				CVec owner(*act->mOwner->GetPos());
				owner.y = 0.0f;
				CVec floor(GetTargetFloorPosition(act));
				floor.y           = 0.0f;
				act->mFloorCenter = floor;
				act->mDistance    = PSVECDistance(&owner, &floor);
			}
		}
		break;
	case 2:
		if (anim != 0x18) {
			act->mOwner->SetAnim(0x18, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
			if (act->mTarget != NULL) {
				act->mOwner->SetRotationY(GetHangEdgeOutwardYaw(act->mTarget, act->mOwner->GetPos(), act->mTargetFloor));
			}
		} else if (model->IsAnimDone()) {
			act->mOwner->SetPos(act->mOwner->GetJointWorldPos("second_null"));
			SyncRopePosition(act->mOwner);
			act->mOwner->GetResNode("second_null").SetTranslate(0.0f, 0.0f, 0.0f);
			act->mOwner->SetRotationY(PI + act->mOwner->mMoveRotate.y);
			act->mOwner->SetAnim(0x14, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
			act->mOwner->UpdateAnim(0.0f);
			act->mOwner->SetPos(*act->mOwner->GetPos() + (*act->mOwner->GetPos() - act->mOwner->GetWorldPos("second_null")));
			SyncRopePosition(act->mOwner);
			act->mState         = 5;
			act->mMoveDirection = 0;
		}
		break;
	case 5:
		if (act->mMoveDirection == -1) {
			if (anim != 0x16) {
				act->mOwner->SetAnim(0x16, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
			}
		} else if (act->mMoveDirection == 1) {
			if (anim != 0x15) {
				act->mOwner->SetAnim(0x15, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
			}
		} else if (anim != 0x14) {
			act->mOwner->SetAnim(0x14, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
		}
		break;
	case 6:
	case 7: {
		s32 next = act->mState == 7 ? 0x7D : 0x17;
		if (anim != next) {
			act->mOwner->SetAnim(next, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
		} else if (model->IsAnimDone()) {
			act->mOwner->SetPos(act->mOwner->GetJointWorldPos("second_null"));
			SyncRopePosition(act->mOwner);
			act->mOwner->GetResNode("second_null").SetTranslate(0.0f, 0.0f, 0.0f);
			StandAfterLift(act->mOwner, 1.0f);
			act->mState  = 0;
			act->mTarget = NULL;
		}
		break;
	}
	case 8:
		act->mOwner->SetAnim(0x19, TRUE, 15.0f, 1.0f, 1.0f, 15.0f);
		act->mState = 9;
	case 9:
		if (anim != 0x19) {
			act->mOwner->SetAnim(0x19, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
			return;
		}
		if (!model->IsAnimDone()) {
			return;
		}
		act->mOwner->SetAnim(0x1A, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
		act->mState  = 10;
		act->mTarget = NULL;
	case 10:
		if (act->mOwner->GetHeight() <= 0.0f) {
			if (act->mOwner->mFallDistance > 30.0f) {
				act->mState = 0;
				act->mOwner->SetAnim(0x8E, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
				act->mOwner->mJump.mState = 5;
				ApplyPlayerJumpStrength((XActJump*)&act->mOwner->mJump, act->mOwner->mFallDistance);
				act->mOwner->SetColor(1, 255, 0, 0, 0.0f);
				act->mOwner->SetColor(1, 0, 0, 0, 0.3f);
				StartScreenFade(&gGame.mScrnTex.mColor, 255, 0, 0, 0, 100, 100, 200, 0, 0.3f);
			} else {
				act->mOwner->SetAnim(0x1B, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
				act->mState = 11;
			}
		} else if (act->mOwner->GetAttachment() == 1) {
			act->mState                 = 0;
			act->mOwner->mCopter.mState = 8;
		}
		break;
	case 11:
		if (model->IsAnimDone()) {
			StandAfterLift(act->mOwner, 1.0f);
			act->mState = 0;
		}
		break;
	}
}

static inline EffectInstance* StartLiftEffect(u32 id, MtxP mtx)
{
	EffectInstance* effect = fn_800A11A4((EffectInstance*)&gGame.mEvtPtcl, id, 1);
	if (effect != NULL) {
		effect->mtx = mtx;
		UpdateEffectMtx(effect);
	}
	return effect;
}

/**
 * @note Address: 0x800F5BE8
 * @note Size: 0x42D8
 */
// NONMATCHING: the calls and their SetAnim arguments follow retail's order. State 33's 0x156 release block is laid out
// before the swing test rather than after it, and vector temporaries, stack homes and register scheduling still differ.
void UpdateLiftActionAnimation(XActLift* act, s32 anim, CObj* model, u32 frames, f32 frame, f32 end)
{
	s32 placement
	    = act->mTarget != NULL ? (act->mTarget->mCoord.mPlacement - GetStagePlacementByIndex(0)) / (s32)sizeof(StagePlacement) : -1;
	act->mBatteryCost = 0.0f;
	switch (act->mState) {
	case 1: {
		BOOL special = placement >= 0 && act->mTarget->mAttr.mObjData != NULL && act->mTarget->mAttr.mObjData->GetCategory() == 8
		            && GetItemFlagsMasked(act->mTarget->mAttr.mObjData->GetItem(), 0x8000) != 0;
		s32 liftAnim = special ? 0x158 : 5;
		if (anim != liftAnim) {
			act->mOwner->SetAnim(liftAnim, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			if (act->mTarget != NULL) {
				Vec point           = GetFloorContactPosition(act->mTarget, act->mTargetFloor, NULL);
				const Vec* ownerPos = act->mOwner->GetPos();
				act->mOwner->SetRotateY(atan2(point.x - ownerPos->x, point.z - ownerPos->z), 12.0f * DEG2RAD);
			}
			if (placement < 0 && PSVECDistance(act->mOwner->GetPos(), act->mTarget->GetPos()) >= 3.0f) {
				SetPlugCordSpeed(&act->mOwner->mPlug, 0.2f);
			}
			act->_AC
			    = act->mTarget != NULL && act->mTarget->GetEventScript() != NULL && (act->mTarget->mStatus.mFlags & 0x10000000000ULL) != 0;
			break;
		}
		if (!model->IsAnimDone()) {
			break;
		}
		special = placement >= 0 && act->mTarget->mAttr.mObjData != NULL && act->mTarget->mAttr.mObjData->GetCategory() == 8
		       && GetItemFlagsMasked(act->mTarget->mAttr.mObjData->GetItem(), 0x8000) != 0;
		if (special) {
			act->mOwner->SetAnim(0x159, TRUE, 0.0f, 1.0f, 1.0f, 15.0f);
			InfoScrn_ApplyObjectGaugeCommand(&gGame.mInfoScrn, NULL, 1, 1, 2);
		} else {
			act->mOwner->SetAnim(6, TRUE, 0.0f, 1.0f, 1.0f, 15.0f);
		}
		act->mState = 2;
		if (placement < 0) {
			SetPlugCordSpeed(&act->mOwner->mPlug, 0.5f);
			act->mTimer = 0.0f;
			PlayWiiSoundEffect(&gGame.mSound, 0xA7, 1.0f, 0.0f);
		} else if (act->mTarget->mAttr.mObjData != NULL && act->mTarget->mAttr.mObjData->GetCategory() == 8) {
			act->mTimer = gRefreshRate * (GetItemPickupHoldTime(act->mTarget->mAttr.mObjData->GetItem()) / 100.0f);
		} else {
			act->mTimer = 0.5f * gRefreshRate;
		}
	}
	case 2:
		if (placement >= 0 && act->mTarget->mAttr.mObjData != NULL && act->mTarget->mAttr.mObjData->GetCategory() == 8
		    && GetItemFlagsMasked(act->mTarget->mAttr.mObjData->GetItem(), 0x8000) != 0) {
			if ((gWiiPointer.mTrigger & 0x0C01000000000000ULL)
			    || (nw4r::math::FAbs(gWiiPointer.mRemoteAcceleration[0].y - gWiiPointer.mRemoteAcceleration[1].y) > 0.6f
			        && gWiiPointer.mRemoteAcceleration[0].y < 0.0f && gWiiPointer.mRemoteAcceleration[1].y < 0.0f
			        && gWiiPointer.mRemoteAcceleration[2].y < 0.0f && gWiiPointer.mRemoteAcceleration[3].y < 0.0f
			        && gWiiPointer.mRemoteAcceleration[4].y < 0.0f)) {
				if (gGame.mEvt.mData.mScript == NULL) {
					act->mState = 3;
					InfoScrn_ApplyObjectGaugeCommand(&gGame.mInfoScrn, NULL, 0, 1, 2);
				}
			} else {
				if ((gWiiPointer.mTrigger & 0x0002000000000020ULL) || gGame.mEvt.mData.mScript != NULL
				    || IsPlayerBlockingEvents(act->mOwner)) {
					act->mState = 0;
					StandAfterLift(act->mOwner);
					act->mTarget = NULL;
					act->_AC     = 0;
					InfoScrn_ApplyObjectGaugeCommand(&gGame.mInfoScrn, NULL, 0, 1, 2);
				}
				break;
			}
		} else {
			if ((act->mTimer -= frames) > 0.0f) {
				break;
			}
			act->mState = 3;
		}
	case 3:
		if (anim != 7) {
			BOOL room = TRUE;
			if (act->mTarget != NULL && act->mTarget->mAttr.mObjData != NULL && act->mTarget->mAttr.mObjData->GetCategory() == 8) {
				s32 slot
				    = FindOwnedInventoryItemIndex(gGame.GetPlayer()->mParams.mInventory.mItems, act->mTarget->mAttr.mObjData->GetItem());
				if (slot >= 0 && gGame.GetPlayer()->mParams.mInventory.mItems[slot].mCount >= 10) {
					room = FALSE;
				}
			}
			if (room) {
				act->mTargetState = 1;
				act->mOwner->SetAnim(7, TRUE, 0.0f, 1.0f, 1.0f, 15.0f);
			} else {
				act->mState = 5;
			}
			if (act->mTarget != NULL && act->mTarget->GetEventScript() != NULL && (act->mTarget->mStatus.mFlags & 0x10000000000ULL)) {
				XObj* target       = act->mTarget;
				const void* script = gGame.mXObjs.mObjects[placement]->GetEventScript();
				if (!IsPlayerBlockingEvents(gGame.GetPlayer())) {
					StartOrQueueEventScript(&gGame.mEvt, (u8*)script, target, -1, -1);
				}
			}
			if (act->mTarget != NULL && (act->mTarget->mStatus.mFlags & 0x800000)) {
				act->mTarget->mStatus.mFlags &= ~0x800000ULL;
				u32 materialFlags[5];
				materialFlags[0] = act->mTarget->mStatus.mFlags & 0x800000;
				materialFlags[1] = act->mTarget->mStatus.mFlags & 0x200000;
				materialFlags[2] = act->mTarget->mStatus.mFlags & 0x40;
				materialFlags[3] = act->mTarget->mStatus.mFlags & 0x20000;
				materialFlags[4] = act->mTarget->mStatus.mFlags & 0x400000;
				act->mTarget->ApplyMaterialPresets(materialFlags);
				act->mTarget->mScreenBoundsValid = 0;
			}
			act->_AC = 0;
		} else if (model->IsAnimDone()) {
			act->mOwner->SetAnim(8, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			s32 last = act->mOwner->GetAnimEndFrame();
			if (last >= frames) {
				fn_800E3AE4(act->mOwner, last - frames);
			}
			if (act->mTarget != NULL) {
				if (act->mTarget->GetEventScript() != NULL) {
					act->mState = 5;
				} else if (act->mTarget->mAttr.mObjData != NULL && act->mTarget->mAttr.mObjData->GetCategory() == 8
				           && !(act->mTarget->mStatus.mFlags & 0x400000000000ULL)) {
					act->mState = 20;
				} else {
					act->mState = 4;
				}
				act->mTargetState = 4;
			}
		}
		break;
	case 4:
		if (anim == 0x895) {
			if (model->IsAnimDone()) {
				act->mOwner->SetAnim(0x894, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
				if (act->mContactEffect) {
					fn_800A12F8(&gGame.mEvtPtcl, act->mContactEffectId, FALSE);
					act->mContactEffect = NULL;
				}
			} else {
				if (act->mCheckContact && frame > 6.0f) {
					Vec position = act->mTarget->GetJointPos("second_null");
					if (CheckCarriedObjectContacts(act, &position, 3.0f, 1.0f, TRUE, FALSE, FALSE, NULL)) {
						act->mCheckContact = FALSE;
					}
				}
				if (!act->mContactEffect) {
					act->mContactEffectId = 0x1D;
					act->mContactEffect   = StartLiftEffect(act->mContactEffectId, act->mTarget->GetWorldMtx("second_null"));
				}
			}
		} else if ((anim == 0x8AE || anim == 0x8AF) && model->IsAnimDone()) {
			act->mOwner->SetAnim(0x894, TRUE, 0.0f, 1.0f, 1.0f, 15.0f);
			act->mTarget->SetAnim(1, TRUE, 0.0f, 1.0f, 1.0f, 15.0f);
		}
		if (IsCarryingToothbrush(act)) {
			if (anim != 0x895 && act->mContactEffect) {
				fn_800A12F8(&gGame.mEvtPtcl, act->mContactEffectId, FALSE);
				act->mContactEffect = NULL;
			}
		} else if (IsCarryingSquirter(act) && anim != 0x8C2 && anim != 0x8C3 && anim != 0x8C4 && act->mToolEffect) {
			fn_800A12F8(&gGame.mEvtPtcl, act->mToolEffectId, FALSE);
			act->mToolEffect = NULL;
		}
		break;
	case 5:
		if (gGame.mEvt.mData.mScript == NULL) {
			if (act->mTarget->mAttr.mObjData != NULL && act->mTarget->mAttr.mObjData->GetCategory() == 8
			    && !(act->mTarget->mStatus.mFlags & 0x400000000000ULL)) {
				BOOL room = TRUE;
				s32 slot
				    = FindOwnedInventoryItemIndex(gGame.GetPlayer()->mParams.mInventory.mItems, act->mTarget->mAttr.mObjData->GetItem());
				if (slot >= 0 && gGame.GetPlayer()->mParams.mInventory.mItems[slot].mCount >= 10) {
					room = FALSE;
				}
				if (room) {
					act->mState = 20;
				} else {
					act->mState = 0;
					StandAfterLift(act->mOwner);
					if (act->mTarget->mCoord.mPlacement->mFlags & 0x80) {
						act->mTarget->mStatus.mFlags |= 0x800000ULL;
						u32 materialFlags[5];
						materialFlags[0] = act->mTarget->mStatus.mFlags & 0x800000;
						materialFlags[1] = act->mTarget->mStatus.mFlags & 0x200000;
						materialFlags[2] = act->mTarget->mStatus.mFlags & 0x40;
						materialFlags[3] = act->mTarget->mStatus.mFlags & 0x20000;
						materialFlags[4] = act->mTarget->mStatus.mFlags & 0x400000;
						act->mTarget->ApplyMaterialPresets(materialFlags);
					}
					act->mTarget->mScreenBoundsValid = 1;
					act->mTarget                     = NULL;
				}
			} else {
				act->mState = 4;
			}
		}
		break;
	case 6:
		if (anim != 9) {
			act->mOwner->SetAnim(9, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			act->mTargetState = 5;
		} else if (model->IsAnimDone()) {
			StandAfterLift(act->mOwner);
			act->mState = 0;
		}
		break;
	case 7:
	case 8:
	case 9:
		if (anim != 0x9F && anim != 0xA0 && anim != 0x14D && anim != 0x15C && anim != 0x15F && anim != 0x15A && anim != 0x15D
		    && anim != 0x15B && anim != 0x15E) {
			act->SetTarget(&act->mOwner->mPlug);
			WiiPlayerPlug* plug = (WiiPlayerPlug*)act->mTarget;
			plug->mStatus.mFlags |= 0x0400000000000000ULL;
			plug->mConnection = 0;
			act->mYawOffset   = 0.0f;
			plug->SetRotationY(act->mOwner->mMoveRotate.y);
			plug->SetRotationX(0.0f);
		}
	case 11:
		if (anim != 0x9F && anim != 0xA0 && anim != 0x14D && anim != 0x15C && anim != 0x15F) {
			act->mOwner->ClearAnimQueue();
			if (act->mState == 7) {
				act->mOwner->SetAnim(0x9F, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
				act->mTargetState = 6;
			} else if (act->mState == 8) {
				act->mOwner->SetAnim(0xA0, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
				act->mTargetState = 8;
			} else {
				act->mOwner->SetAnim(0x14D, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
				act->mTargetState = 11;
			}
		} else if (model->IsAnimDone()) {
			if (act->mState == 7) {
				StandAfterLift(act->mOwner, 1.0f);
				s32 last = act->mOwner->GetAnimEndFrame();
				if (last >= frames) {
					fn_800E3AE4(act->mOwner, last - frames);
				}
				act->mState = 0;
				if (act->mPlugTarget != NULL) {
					act->mPlugTarget->ReceivePlugPower(0.0f);
				}
			} else if (act->mState == 8) {
				act->mState = 13;
				act->mTimer = 0.2f * gRefreshRate;
			} else {
				act->mOwner->SetAnim(0x14F, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
				if (act->mPlugTarget != NULL) {
					InfoScrn_ApplyObjectGaugeCommand(&gGame.mInfoScrn, (WiiTitleObject*)act->mPlugTarget, 1, 1, 0);
				}
				act->_84 = act->_88 = 0.0f;
				act->mState         = 16;
			}
		}
		break;
	case 10:
		break;
	case 12:
		if (anim != 0xAD) {
			act->mOwner->SetAnim(0xAD, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
			act->mOwner->UpdateAnim(0.0f);
			act->SetTarget(&act->mOwner->mPlug);
			act->mTargetState = 7;
			act->mYawOffset   = 0.0f;
		} else if (model->IsAnimDone()) {
			act->mOwner->SetPos(act->mOwner->GetJointWorldPos("second_null"));
			SyncRopePosition(act->mOwner);
			act->mOwner->GetResNode("second_null").SetTranslate(0.0f, 0.0f, 0.0f);
			act->mOwner->SetAnim(8, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
			s32 last = act->mOwner->GetAnimEndFrame();
			if (last >= frames) {
				fn_800E3AE4(act->mOwner, last - frames);
			}
			act->mState      = 4;
			act->mPlugTarget = NULL;
		}
		break;
	case 13:
		if ((act->mTimer -= frames) <= 0.0f) {
			act->mTimer = 0.0f;
			if (act->mPlugTarget != NULL && !act->mPlugTarget->ReceivePlugPower(0.0f)) {
				act->mState = 15;
				break;
			}
			act->mOwner->SetAnim(0xAB, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			act->mState = 14;
			if (act->mPlugTarget != NULL && act->mPlugTarget->GetItem() == 0) {
				gGame.mSubwin.mRectX = gGame.mSubwin.mRectY = gGame.mSubwin.mRectWidth = gGame.mSubwin.mRectHeight = NAN;
				nw4r::g3d::ResNode node = act->mOwner->mPlug.FindResNode("plug_model");
				Vec at                  = *act->mOwner->GetPos() + (*act->mPlugTarget->GetPos() - *act->mOwner->GetPos()) * 0.3f;
				at.y = act->mOwner->GetPos()->y + 0.8f * (act->mOwner->mPlug.GetWorldPos(node).y - act->mOwner->GetPos()->y);
				ConfigureSubwindowCamera(&gGame.mSubwin, at, act->mOwner->mMoveRotate.y - 45.0f * DEG2RAD, 24.0f * DEG2RAD, 100.0f, 8.0f);
				gGame.mSubwin.mEnabled      = TRUE;
				gGame.mSubwin.mFlags.mFlags = 0;
			}
		}
		break;
	case 14: {
		Params& params = act->mOwner->mParams;
		if (params.mBattery > 0.0f && params.mBattery / params.mMaxBattery > 0.2f) {
			if (anim != 0xAB) {
				act->mOwner->SetAnim(0xAB, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			}
		} else if (anim != 0xAE) {
			act->mOwner->SetAnim(0xAE, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
		}
		if (act->mPlugTarget != NULL) {
			act->mBatteryCost = frames * (((s32)GetPlayerActionParameter(9) / 100.0f) / gRefreshRate);
			if (!act->mPlugTarget->ReceivePlugPower(act->mBatteryCost)) {
				act->mBatteryCost = 0.0f;
				act->mState       = 15;
			}
		}
		break;
	}
	case 15:
		if (anim != 0xAC) {
			act->mOwner->SetAnim(0xAC, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			act->mOwner->UpdateAnim(0.0f);
			act->mTargetState      = 10;
			gGame.mSubwin.mEnabled = FALSE;
			if (act->mPlugTarget != NULL && GetObjData(&act->mPlugTarget->mCoord)->mCategory == 9) {
				((XObjLeticker*)act->mPlugTarget)->OnUnplugged();
			}
		} else if (model->IsAnimDone()) {
			act->mOwner->SetAnim(8, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			act->mState      = 4;
			act->mPlugTarget = NULL;
		}
		break;
	case 16: {
		f32 angle = atan2(-gWiiPads[0].mStick.y, -gWiiPads[0].mStick.x);
		if (angle < act->_88) {
			act->_88 = angle;
		} else if (angle > act->_88 && act->_88 < 0.0f) {
			act->_88 += DEG2RAD * frames;
			if (act->_88 > 0.0f) {
				act->_88 = 0.0f;
			}
		}
		if (angle < act->_88) {
			angle = act->_88;
		}
		if (angle > act->_88 + HALF_PI) {
			angle = act->_88 + HALF_PI;
		}
		act->_80  = angle;
		f32 delta = angle - act->_88;
		if (act->_84 < delta) {
			act->_84 += 2.0f * DEG2RAD * frames;
			if (act->_84 > delta) {
				act->_84 = delta;
			}
		} else if (act->_84 > delta) {
			act->_84 -= 3.0f * DEG2RAD * frames;
			if (act->_84 < delta) {
				act->_84 = delta;
			}
		}
		f32 progress = act->_84 / HALF_PI;
		if (progress > 0.0f) {
			if (anim != 0x150) {
				act->mOwner->SetAnim(0x150, FALSE, 0.0f, 0.0f, 0.0f, 15.0f);
				if (act->mPlugTarget != NULL) {
					act->mPlugTarget->SetAnim(8, FALSE, 0.0f, 0.0f, 0.0f, 15.0f);
				}
			}
			f32 animFrame = progress * act->mOwner->GetAnimEndFrame();
			fn_800E3AE4(act->mOwner, animFrame);
			if (act->mPlugTarget != NULL) {
				for (s32 i = 0; i < 6; ++i) {
					nw4r::g3d::AnmObj* anm = act->mPlugTarget->mObject.m_pScnMdl->GetAnmObj((nw4r::g3d::ScnMdlSimple::AnmObjType)i);
					if (anm != NULL) {
						anm->SetFrame(animFrame);
					}
				}
			}
		} else if (anim != 0x14F) {
			act->mOwner->SetAnim(0x14F, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			if (act->mPlugTarget != NULL) {
				act->mPlugTarget->SetAnim(8, FALSE, 0.0f, 0.0f, 0.0f, 15.0f);
			}
		}
		break;
	}
	case 17:
		if (anim != 0x151) {
			act->mOwner->SetAnim(0x151, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
			if (act->mPlugTarget != NULL) {
				act->mPlugTarget->SetAnim(9, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
			}
		} else if (model->IsAnimDone()) {
			act->mOwner->SetAnim(8, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			act->mState = act->mTargetState             = 4;
			act->mPlugTarget                            = NULL;
			((WiiPlayerPlug*)act->mTarget)->mConnection = 0;
		}
		break;
	case 18:
		if (anim != 0x14E) {
			act->mOwner->SetAnim(0x14E, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
			act->mOwner->UpdateAnim(0.0f);
			act->mTargetState = 13;
			if (act->mPlugTarget != NULL) {
				act->mPlugTarget->SetAnim(0, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
			}
		} else if (model->IsAnimDone()) {
			act->mOwner->SetAnim(8, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			act->mState      = 4;
			act->mPlugTarget = NULL;
		}
		break;
	case 19:
		if (anim != 0xAC) {
			act->mOwner->SetAnim(0xAC, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
			act->mOwner->UpdateAnim(0.0f);
			act->mTargetState = 13;
		} else if (model->IsAnimDone()) {
			act->mOwner->SetAnim(8, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			act->mState      = 4;
			act->mPlugTarget = NULL;
		}
		break;
	case 20:
		if (anim != 10) {
			act->mOwner->SetAnim(10, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			act->mTargetState = 14;
			if (act->mContactEffect) {
				fn_800A12F8(&gGame.mEvtPtcl, act->mContactEffectId, FALSE);
				act->mContactEffect = NULL;
			}
			if (act->mToolEffect) {
				fn_800A12F8(&gGame.mEvtPtcl, act->mToolEffectId, FALSE);
				act->mContactEffect = NULL;
			}
		} else if (model->IsAnimDone()) {
			act->mState       = 0;
			act->mTargetState = 15;
			StandAfterLift(act->mOwner);
			if (act->mTarget != NULL && act->mTarget->mAttr.mObjData != NULL && act->mTarget->mAttr.mObjData->GetCategory() == 8) {
				s8 sound = GetItemSoundIndex(act->mTarget->mAttr.mObjData->GetItem(), 1);
				if (sound >= 0) {
					PlayWiiSoundEffect(&gGame.mSound, GetItemReleaseSoundId(sound), 1.0f, 0.0f);
				}
			}
		}
		break;
	case 21:
		if (act->mPendingItemId >= 0) {
			act->mPendingItem
			    = SpawnObjectFromPlacement(&gGame.mXObjs, &CoordInfo(GetStageObjectPlacementByIndex(act->mPendingItemId)), 0, TRUE);
		} else {
			act->mPendingItem = NULL;
		}
		act->mState = 22;
	case 22:
		if (act->mPendingItem != NULL) {
			if (act->mPendingItem->mStatus.mFlags & 0x0040000000000000ULL) {
				break;
			}
			act->mPendingItem->mStatus.mFlags |= 0x0001000000000000ULL;
			gGame.SetStatusFlags(0x1000);
		}
		act->mState = 23;
	case 23:
		if (anim != 11) {
			act->mOwner->SetAnim(11, TRUE, 0.0f, 1.0f, 1.0f, 15.0f);
			act->mTargetState = 3;
			if (IsLiftTargetPlug(act)) {
				act->mTarget->mStatus.mFlags &= ~0x0400000000000000ULL;
				act->mTarget = NULL;
			}
			if (act->mPendingItemId >= 0 && act->mPendingItem != NULL) {
				act->mPendingItem->mStatus.mFlags |= 0x0400000000000000ULL;
				act->mPendingItem->SetScale(0.0f);
				act->SetTarget(act->mPendingItem);
				s32 count      = RemoveInventoryItemCount(&act->mOwner->mParams.mInventory, act->mTarget->GetItem(), 1);
				s16 difference = count - GetStageCollectionFlagBase(0);
				if (difference > 0) {
					act->mPendingItem->mPlacementParam1 = difference;
				}
				act->mPendingItemId = -1;
				if (IsCarryingSquirter(act)) {
					ApplyPlayerItemTintToTarget(act);
				}
			}
		} else if (model->IsAnimDone()) {
			act->mOwner->SetAnim(8, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			act->mState = act->mTargetState = 4;
		}
		break;
	case 24:
		if (anim != 0xDD) {
			act->mOwner->SetAnim(0xDD, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			act->mTargetState = 16;
		} else if (model->IsAnimDone()) {
			StandAfterLift(act->mOwner);
			act->mState       = 0;
			act->mTargetState = 17;
		}
		break;
	case 25:
		if (act->mPendingItemId >= 0) {
			act->mPendingItem
			    = SpawnObjectFromPlacement(&gGame.mXObjs, &CoordInfo(GetStageObjectPlacementByIndex(act->mPendingItemId)), 0, TRUE);
		} else {
			act->mPendingItem = NULL;
		}
		act->mState = 26;
	case 26:
		if (act->mPendingItem != NULL) {
			if (act->mPendingItem->mStatus.mFlags & 0x0040000000000000ULL) {
				break;
			}
			act->mPendingItem->mStatus.mFlags |= 0x0001000000000000ULL;
			gGame.SetStatusFlags(0x1000);
		}
		act->mState = 27;
	case 27:
		if (anim != 0xEC) {
			act->mOwner->SetAnim(0xEC, TRUE, 0.0f, 1.0f, 1.0f, 15.0f);
			act->mTargetState = 18;
			if (IsLiftTargetPlug(act)) {
				act->mTarget->mStatus.mFlags &= ~0x0400000000000000ULL;
				act->mTarget = NULL;
			}
			if (act->mPendingItemId >= 0 && act->mPendingItem != NULL) {
				act->mPendingItem->mStatus.mFlags |= 0x0400000000000000ULL;
				act->mPendingItem->SetScale(0.0f);
				act->SetTarget(act->mPendingItem);
				act->mPendingItemId = -1;
			}
		} else if (model->IsAnimDone()) {
			act->mOwner->SetAnim(8, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			act->mState       = 4;
			act->mTargetState = 19;
		}
		break;
	case 28:
		if (act->mOwner->mTumble.mState == 9) {
			act->mState = 4;
		} else if (anim != 0x152) {
			act->mOwner->SetAnim(0x152, TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
		} else if (act->mOwner->GetAnimFrame() >= 10.0f && !(gWiiPointer.mButton & 0x0001000000000010ULL)) {
			act->mOwner->SetAnim(0x894, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			act->mState = 4;
		} else if (act->mOwner->IsAnimDone()) {
			act->mOwner->SetAnim(0x153, TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
			act->mState = 29;
			InfoScrn_ApplyObjectGaugeCommand(&gGame.mInfoScrn, NULL, 1, 1, 3);
		}
		break;
	case 29:
		if (act->mOwner->mTumble.mState == 9) {
			act->mState = 4;
		} else if (nw4r::math::FAbs(gWiiPointer.mRemoteAcceleration[0].z - gWiiPointer.mRemoteAcceleration[1].z) >= 0.2f
		           && gWiiPointer.mRemoteAcceleration[0].z * gWiiPointer.mRemoteAcceleration[1].z < 0.0f) {
			act->mOwner->SetAnim(0x895, TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
			act->mTarget->SetAnim(1, TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
			act->mState        = 31;
			act->mCheckContact = TRUE;
			InfoScrn_ApplyObjectGaugeCommand(&gGame.mInfoScrn, NULL, 0, 1, 3);
		} else {
			if (anim == 0x152) {
				if (act->mOwner->IsAnimDone()) {
					act->mOwner->SetAnim(0x153, TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
				}
			} else if (anim == 0x895) {
				if (act->mOwner->IsAnimDone()) {
					act->mOwner->SetAnim(0x153, TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
					if (act->mContactEffect) {
						fn_800A12F8(&gGame.mEvtPtcl, act->mContactEffectId, FALSE);
						act->mContactEffect = NULL;
					}
				}
				if (act->mCheckContact && frame > 6.0f) {
					Vec position = act->mTarget->GetJointPos("second_null");
					if (CheckCarriedObjectContacts(act, &position, 3.0f, 1.0f, TRUE, FALSE, FALSE, NULL)) {
						act->mCheckContact = FALSE;
					}
				}
				if (!act->mContactEffect) {
					act->mContactEffectId = 0x1D;
					act->mContactEffect   = StartLiftEffect(act->mContactEffectId, act->mTarget->GetWorldMtx("second_null"));
				}
			} else if (act->mContactEffect) {
				fn_800A12F8(&gGame.mEvtPtcl, act->mContactEffectId, FALSE);
				act->mContactEffect = NULL;
				InfoScrn_ApplyObjectGaugeCommand(&gGame.mInfoScrn, NULL, 1, 1, 3);
			}
			if (!(gWiiPointer.mButton & 0x0001000000000010ULL) || gGame.mEvt.mData.mScript != NULL || IsPlayerBlockingEvents(act->mOwner)) {
				act->mState = 30;
				InfoScrn_ApplyObjectGaugeCommand(&gGame.mInfoScrn, NULL, 0, 1, 3);
			} else if (act->mOwner->mHang.mState != 0 || act->mOwner->mJump.mState != 0 || act->mOwner->mTumble.mState != 0) {
				act->mState = 4;
				InfoScrn_ApplyObjectGaugeCommand(&gGame.mInfoScrn, NULL, 0, 1, 3);
			}
		}
		break;
	case 30:
		if (act->mOwner->mTumble.mState == 9) {
			act->mState = 4;
		} else if (anim != 0x154) {
			act->mOwner->SetAnim(0x154, TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
		} else if (act->mOwner->IsAnimDone()) {
			act->mOwner->SetAnim(0x894, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			act->mState = 4;
		}
		break;
	case 31:
		if (act->mOwner->mTumble.mState == 9) {
			act->mState = 4;
		} else if (frame > 4.0f) {
			act->mState  = 29;
			Vec position = act->mTarget->GetJointPos("second_null");
			if (CheckCarriedObjectContacts(act, &position, 3.0f, 1.0f, TRUE, FALSE, FALSE, NULL)) {
				act->mCheckContact = FALSE;
			}
		}
		break;
	case 32:
		act->mOwner->SetAnim(0x8B7, TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
		act->mTarget->SetAnim(5, TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
		act->mState = 33;
		break;
	case 33:
		switch (act->mOwner->GetAnimId()) {
		case 0x8B7:
			if (model->IsAnimDone()) {
				Vec position = *act->mOwner->GetPos();
				position.x += 3.0f * (f32)sin(act->mOwner->mMoveRotate.y);
				position.z += 3.0f * (f32)cos(act->mOwner->mMoveRotate.y);
				BOOL hit = FALSE;
				for (s32 i = 0; i < gGame.mXObjs.mObjectCount; ++i) {
					XObj* object = gGame.mXObjs.mObjects[i];
					if (!(object->mStatus.mFlags & 1) || !(object->mStatus.mFlags & 0x10000)) {
						continue;
					}
					const Vec* objectPos = object->GetPos();
					f32 dx               = objectPos->x - position.x;
					f32 dz               = objectPos->z - position.z;
					if (FSqrt(dx * dx + dz * dz) > 100.0f + (1.0f + object->mFloorXZExtent[0])) {
						continue;
					}
					if (!(object->mStatus.mFlags & 0x40000000000ULL)) {
						continue;
					}
					CVec push, contact;
					if ((object->TestFloorCollision(&position, &position, 1.0f, 1.0f, &push, &contact, NULL, 0) & 3) == 3) {
						hit = TRUE;
						break;
					}
				}
				if (hit) {
					act->mOwner->SetAnim(0x156, TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
					act->mTarget->SetAnim(9, TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
					InfoScrn_ApplyObjectGaugeCommand(&gGame.mInfoScrn, NULL, 1, 1, 2);
				} else {
					CheckCarriedObjectContacts(act, &position, 1.0f, 1.0f, TRUE, FALSE, FALSE, NULL);
					act->mOwner->SetAnim(0x8AF, TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
					act->mTarget->SetAnim(3, TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
					act->mState = 4;
					PlayWiiSoundEffect(&gGame.mSound, 0x17D, 1.0f, 0.0f);
					CVec pos               = act->mTarget->GetJointPos("second_null");
					CVec scale             = act->mTarget->GetJointScale(act->mTarget->GetRootResNode());
					Vec rot                = { 0.0f, 0.0f, 0.0f };
					EffectInstance* effect = fn_800A11A4((EffectInstance*)&gGame.mEvtPtcl, 0x2E, 1);
					if (effect != NULL) {
						effect->pos   = pos;
						effect->rot   = rot;
						effect->scale = scale;
						UpdateEffectMtx(effect);
					}
				}
			}
			break;
		case 0x155:
			if (gWiiPointer.mButton & 0x0001000000000010ULL) {
				if (model->IsAnimDone()) {
					act->mOwner->SetAnim(0x156, TRUE, 0.0f, 1.0f, 1.0f, 15.0f);
					act->mTarget->SetAnim(9, TRUE, 0.0f, 1.0f, 1.0f, 15.0f);
				}
			} else {
				act->mOwner->SetAnim(0x894, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
				act->mState = 4;
			}
			break;
		case 0x156:
			if (!(gWiiPointer.mButton & 0x0001000000000010ULL) || gGame.mEvt.mData.mScript != NULL || IsPlayerBlockingEvents(act->mOwner)) {
				act->mOwner->SetAnim(0x894, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
				act->mState = 4;
				InfoScrn_ApplyObjectGaugeCommand(&gGame.mInfoScrn, NULL, 0, 1, 2);
			} else if ((gWiiPointer.mButton & 0x0C00000000000000ULL)
			           || (nw4r::math::FAbs(gWiiPointer.mRemoteAcceleration[0].y - gWiiPointer.mRemoteAcceleration[1].y) > 0.6f
			               && gWiiPointer.mRemoteAcceleration[0].y < 0.0f && gWiiPointer.mRemoteAcceleration[1].y < 0.0f
			               && gWiiPointer.mRemoteAcceleration[2].y < 0.0f && gWiiPointer.mRemoteAcceleration[3].y < 0.0f
			               && gWiiPointer.mRemoteAcceleration[4].y < 0.0f)) {
				act->mOwner->SetAnim(0x157, TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
				act->mTarget->SetAnim(10, TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
				Vec position = *act->mOwner->GetPos();
				position.x += 3.0f * (f32)sin(act->mOwner->mMoveRotate.y);
				position.z += 3.0f * (f32)cos(act->mOwner->mMoveRotate.y);
				CheckCarriedObjectContacts(act, &position, 1.0f, 1.0f, TRUE, FALSE, FALSE, NULL);
				InfoScrn_ApplyObjectGaugeCommand(&gGame.mInfoScrn, NULL, 0, 1, 2);
			}
			break;
		case 0x157:
			if (model->IsAnimDone()) {
				act->mOwner->SetAnim(0x894, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
				act->mTarget->SetAnim(1, TRUE, 0.0f, 1.0f, 1.0f, 15.0f);
				act->mState = 4;
			}
			break;
		default:
			act->mState = 4;
			break;
		}
		break;
	case 34:
		if (anim != 0x8B1) {
			act->mOwner->SetAnim(0x8B1, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			act->mTarget->SetAnim(4, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			act->mUseEffect = NULL;
		} else if (model->IsAnimDone()) {
			act->mOwner->SetAnim(0x8B2, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			act->mTarget->SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			act->mState = 35;
		} else if (act->mOwner->GetAnimFrame() > 10.0f && !act->mUseEffect) {
			act->mUseEffectId = 0x2F;
			act->mUseEffect   = StartLiftEffect(act->mUseEffectId, act->mOwner->GetWorldMtx());
		}
		break;
	case 35:
		break;
	case 36:
		if (anim != 0x8B3) {
			act->mOwner->SetAnim(0x8B3, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			act->mTarget->SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
		} else if (model->IsAnimDone()) {
			act->mOwner->SetAnim(8, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			act->mState = 4;
		}
		break;
	case 37:
		PutAwayLiftedObject(act);
		break;
	case 38: {
		f32 angle = act->mOwner->mMoveRotate.y;
		Vec point = *act->mOwner->GetPos();
		point.x += 2.0f * (f32)sin(angle);
		point.z += 2.0f * (f32)cos(angle);
		if (CheckCarriedObjectContacts(act, &point, 2.0f, 1.0f, TRUE, FALSE, FALSE, NULL)) {
			act->mOwner->mSquirterContent = 0;
			act->mOwner->mItemTintWord    = -1;
			act->mOwner->SetAnim(0x8D2, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			act->mTarget->SetAnim(17, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
		} else {
			point.x += 2.0f * (f32)sin(angle);
			point.z += 2.0f * (f32)cos(angle);
			point.y += 7.0f;
			if (CheckCarriedObjectContacts(act, &point, 2.0f, 3.0f, TRUE, FALSE, FALSE, NULL)) {
				act->mOwner->mSquirterContent = 0;
				act->mOwner->mItemTintWord    = -1;
				act->mOwner->SetAnim(0x8C1, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
				act->mTarget->SetAnim(10, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			}
		}
		s32 squirtAnim = act->mOwner->GetAnimId();
		if (squirtAnim != 0x8D2 && squirtAnim != 0x8C1) {
			s16 count       = (s16)GetOwnedInventoryItemCount(&act->mOwner->mParams, 0x38);
			s32 chargeLevel = sSquirterChargeLevels[count];
			if (chargeLevel == 3) {
				act->mOwner->SetAnim(0x8C2, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
				act->mTarget->SetAnim(11, TRUE, 0.0f, 1.0f, 1.0f, 15.0f);
			} else if (chargeLevel == 2) {
				act->mOwner->SetAnim(0x8C3, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
				act->mTarget->SetAnim(12, TRUE, 0.0f, 1.0f, 1.0f, 15.0f);
			} else if (chargeLevel == 1) {
				act->mOwner->SetAnim(0x8C4, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
				act->mTarget->SetAnim(13, TRUE, 0.0f, 1.0f, 1.0f, 15.0f);
			} else if (count <= 1) {
				act->mOwner->mSquirterContent = 0;
				act->mOwner->mItemTintWord    = -1;
				act->mOwner->SetAnim(0x8C5, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
				act->mTarget->SetAnim(14, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			}
		}
		act->mTarget->UpdateAnim(frames);
		act->mState        = 39;
		act->mCheckToolHit = TRUE;
		break;
	}
	case 39:
		if (model->IsAnimDone()) {
			BOOL finished = FALSE;
			s16 count     = (s16)(GetOwnedInventoryItemCount(&act->mOwner->mParams, 0x38) - 1);
			if (anim == 0x8C2) {
				if (sSquirterChargeLevels[count] == 3) {
					act->mTarget->SetAnim(4, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
				} else {
					act->mTarget->SetAnim(3, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
				}
				SetOrAddInventoryItemCount(&act->mOwner->mParams.mInventory, 0x38, count, FALSE);
				finished = TRUE;
			} else if (anim == 0x8C3) {
				if (sSquirterChargeLevels[count] == 2) {
					act->mTarget->SetAnim(3, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
				} else {
					act->mTarget->SetAnim(2, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
				}
				SetOrAddInventoryItemCount(&act->mOwner->mParams.mInventory, 0x38, count, FALSE);
				finished = TRUE;
			} else if (anim == 0x8C4) {
				if (sSquirterChargeLevels[count] == 1) {
					act->mTarget->SetAnim(2, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
				} else {
					act->mTarget->SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
				}
				SetOrAddInventoryItemCount(&act->mOwner->mParams.mInventory, 0x38, count, FALSE);
				finished = TRUE;
			} else if (anim == 0x8C5) {
				act->mTarget->SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
				finished = TRUE;
			} else if (anim == 0x8D2) {
				act->mOwner->SetAnim(0x8C0, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
				act->mTarget->SetAnim(9, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			} else if (anim == 0x8C0) {
				act->mOwner->SetAnim(0x8D3, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
				act->mTarget->SetAnim(18, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			} else if (anim == 0x8D3) {
				act->mTarget->SetAnim(4, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
				SetOrAddInventoryItemCount(&act->mOwner->mParams.mInventory, 0x38, 11, FALSE);
				finished = TRUE;
			} else if (anim == 0x8C1) {
				act->mTarget->SetAnim(4, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
				SetOrAddInventoryItemCount(&act->mOwner->mParams.mInventory, 0x38, 11, FALSE);
				finished = TRUE;
			}
			act->mTarget->UpdateAnim(frames);
			if (finished) {
				act->mOwner->SetAnim(0x11, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
				act->mState = 4;
				if (act->mToolEffect) {
					fn_800A12F8(&gGame.mEvtPtcl, act->mToolEffectId, FALSE);
					act->mToolEffect = NULL;
				}
			}
		} else if (anim == 0x8C2 || anim == 0x8C3 || anim == 0x8C4 || anim == 0x8C5) {
			if (act->mCheckToolHit) {
				nw4r::g3d::ResNode atari = act->mTarget->FindResNode("atari");
				nw4r::g3d::ResNode saki  = act->mTarget->FindResNode("saki");
				CVec pos                 = act->mTarget->GetJointPos(atari);
				f32 distance             = (pos - act->mTarget->GetJointPos(saki)).Length();
				if (distance > 0.1f) {
					CVec scale = act->mTarget->GetJointScale(atari);
					Vec hit;
					if (CheckCarriedObjectContacts(act, &pos, scale.x, 0.5f * scale.y, FALSE, TRUE, TRUE, &hit)) {
						act->mCheckToolHit = FALSE;
						if (act->mToolEffect) {
							fn_800A12F8(&gGame.mEvtPtcl, act->mToolEffectId, FALSE);
							act->mToolEffect = NULL;
						}
						hit.x += scale.x * (f32)sin(act->mOwner->mMoveRotate.y);
						hit.z += scale.x * (f32)cos(act->mOwner->mMoveRotate.y);
						EffectInstance* effect = fn_800A11A4((EffectInstance*)&gGame.mEvtPtcl, 0x5A, 1);
						if (effect != NULL) {
							effect->pos   = hit;
							effect->rot   = (Vec) { 0.0f, 0.0f, 0.0f };
							effect->scale = (Vec) { 1.0f, 1.0f, 1.0f };
							UpdateEffectMtx(effect);
						}
					}
					if (act->mCheckToolHit && !act->mToolEffect) {
						act->mToolEffectId = -1;
						nw4r::g3d::ResNode attachment;
						if (anim == 0x8C5) {
							if (frame > 10.0f) {
								attachment         = saki;
								act->mToolEffectId = 0x42;
							}
						} else {
							attachment         = atari;
							act->mToolEffectId = 0x3C;
						}
						if (act->mToolEffectId >= 0) {
							act->mToolEffect = StartLiftEffect(act->mToolEffectId, act->mTarget->GetWorldMtx(attachment));
						}
						if (act->mToolEffect && anim != 0x8C5) {
							nw4r::ef::ResEmitter emitter = nw4r::ef::Resource::GetInstance()->GetEmitterIndexOf(act->mToolEffectId, NULL);
							if (emitter.IsValid()) {
								nw4r::ef::ResParticleParameterDesc desc = emitter.GetParticleParameterDesc();
								if (desc.IsValid()) {
									desc.SetColor(0, 0, gGame.GetPlayer()->mItemTint);
								}
							}
						}
					}
				}
			}
		} else {
			ApplyPlayerItemTintToTarget(act);
		}
		break;
	}
}

/**
 * @note Address: 0x800F9EC0
 * @note Size: 0x2C54
 */
// NONMATCHING: retail keeps GetLiftTargetContactOffset out of line at all 14 sites; this build expands it
// despite its later definition, adding the floor-contact and CVec calls at every site
void UpdateHeldObject(XActLift* act, u32 frames)
{
	if (!act->mTargetState) {
		return;
	}
	XObj* obj = act->mTarget;
	if (obj == NULL || !(obj->mStatus.mFlags & 1)) {
		return;
	}
	f32 frame = act->mOwner->GetAnimFrame();
	f32 end   = act->mOwner->GetAnimEndFrame();
	switch (act->mTargetState) {
	case 1: {
		obj->mStatus.mFlags |= 0x0400000000000000ULL;
		act->mLiftStartOffset.Set(*obj->GetPos() - fn_800E5A2C(act->mOwner));
		act->mYawOffset = obj->mMoveRotate.y - act->mOwner->mMoveRotate.y;
		obj->SetPos(fn_800E5A2C(act->mOwner) - act->mLiftStartOffset * (1.0f - frame / end));
		if (obj->GetPool() == 24) {
			act->mYawOffset = 0.0f;
			obj->SetRotateY(act->mYawOffset + act->mOwner->mMoveRotate.y, DEG2RAD * 6.0f);
			obj->SetRotationX(0.0f);
			((WiiPlayerPlug*)obj)->mConnection = 0;
		}
		if (obj->GetPool() == 8 && obj->GetItem() == 22) {
			act->mYawOffset = 0.0f;
			obj->SetRotateY(act->mYawOffset + act->mOwner->mMoveRotate.y, DEG2RAD * 6.0f);
		}
		act->mTargetState = 2;
	}
	case 2: {
		Vec offset   = GetLiftTargetContactOffset(act, obj);
		f32 progress = 2.0f * (frame / end);
		if (progress > 1.0f) {
			progress = 1.0f;
		}
		obj->SetPos((fn_800E5A2C(act->mOwner) + act->mLiftStartOffset * (1.0f - progress)) - CVec(offset) * progress);
		break;
	}
	case 3:
		if (IsCarryingToothbrush(act) || IsCarryingSpoon(act) || IsCarryingSquirter(act)) {
			obj->SetBaseMtx(act->mOwner->GetWorldMtx(act->mOwner->FindResNode("carry_obj_nll")));
			if (IsCarryingSpoon(act)) {
				obj->SetAnim(4, TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
			} else if (IsCarryingSquirter(act)) {
				obj->SetAnim(sSquirterChargeLevels[(s16)GetOwnedInventoryItemCount(&act->mOwner->mParams, 0x38)] + 5, TRUE, 0.0f, 1.0f,
				             0.0f, 15.0f);
			}
		} else {
			obj->SetPos(fn_800E5A2C(act->mOwner) - GetLiftTargetContactOffset(act, obj));
			CVec scale = act->mOwner->GetJointScale(act->mOwner->FindResNode("carry_obj_nll"));
			obj->SetScale(GetStagePlacementScale(obj) * scale);
			act->mYawOffset = 0.0f;
			obj->SetRotationY(act->mYawOffset + act->mOwner->mMoveRotate.y);
			if (IsCarryingMug(act)) {
				obj->SetAnim(2, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			}
		}
		break;
	case 4:
		if (IsCarryingToothbrush(act) || IsCarryingSpoon(act) || IsCarryingSquirter(act)) {
			obj->SetBaseMtx(act->mOwner->GetWorldMtx(act->mOwner->FindResNode("carry_obj_nll")));
			s32 id;
			switch (act->mOwner->GetAnimId()) {
			case 0x895:
				id = 1;
				break;
			case 0x8B7:
				id = 5;
				break;
			case 0x8AE:
				id = 2;
				break;
			case 0x8AF:
				id = 3;
				break;
			case 0x8C5:
				id = 14;
				break;
			case 0x8C0:
				id = 9;
				break;
			case 0x8D2:
				id = 17;
				break;
			case 0x8D3:
				id = 18;
				break;
			case 0x8C1:
				id = 10;
				break;
			case 0x8C2:
				id = 11;
				break;
			case 0x8C3:
				id = 12;
				break;
			case 0x8C4:
				id = 13;
				break;
			case 0x155:
				id = 8;
				break;
			case 0x156:
				id = 9;
				break;
			case 0x157:
				id = 10;
				break;
			case 0xE0:
				if (IsCarryingToothbrush(act)) {
					id = 0;
				} else if (IsCarryingSpoon(act)) {
					id = 1;
				} else {
					id = sSquirterChargeLevels[(s16)GetOwnedInventoryItemCount(&act->mOwner->mParams, 0x38)] + 1;
				}
				break;
			case 0xCA:
				if (IsCarryingToothbrush(act)) {
					id = 3;
				} else if (IsCarryingSpoon(act)) {
					id = 4;
				} else {
					id = sSquirterChargeLevels[(s16)GetOwnedInventoryItemCount(&act->mOwner->mParams, 0x38)] + 5;
				}
				break;
			default:
				if (IsCarryingToothbrush(act)) {
					id = 0;
				} else if (IsCarryingSpoon(act)) {
					id = 1;
				} else {
					id = sSquirterChargeLevels[(s16)GetOwnedInventoryItemCount(&act->mOwner->mParams, 0x38)] + 1;
				}
				break;
			}
			obj->SetAnim(id, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			break;
		}
		obj->SetRotationY(act->mYawOffset + act->mOwner->mMoveRotate.y);
		obj->SetPos(fn_800E5A2C(act->mOwner) - GetLiftTargetContactOffset(act, obj));
		if (IsCarryingMug(act)) {
			s32 id;
			if (act->mState == 34) {
				id = 4;
			} else {
				switch (act->mOwner->GetAnimId()) {
				case 12:
					id = 8;
					break;
				case 0x88:
					id = 9;
					break;
				case 13:
					id = 10;
					break;
				default:
					id = 1;
					break;
				}
			}
			obj->SetAnim(id, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
		}
		break;
	case 5: {
		obj->SetPos(fn_800E5A2C(act->mOwner) - GetLiftTargetContactOffset(act, obj));
		if (act->mOwner->GetAnimFrame() < 24.0f) {
			break;
		}
		obj->SetTranslateY(act->mOwner->GetPos()->y);
		f32 radius = act->mOwner->GetScale()->x * GetFloorExtent(act->mOwner, 0).x;
		f32 height = act->mOwner->GetCollisionHeight();
		CVec push, contact;
		if (obj->TestFloorCollision(act->mOwner->GetPos(), act->mOwner->GetPos(), radius, height, &push, &contact, NULL, 0) & 1) {
			f32 distance = PSVECDistance(act->mOwner->GetPos(), &contact);
			obj->SetTranslateX(distance * (f32)sin(act->mOwner->mMoveRotate.y) + obj->GetPos()->x);
			obj->SetTranslateZ(distance * (f32)cos(act->mOwner->mMoveRotate.y) + obj->GetPos()->z);
		}
		if (obj->GetPool() == 24) {
			PlayWiiSoundEffect(&gGame.mSound, 0xA6, 1.0f, 0.0f);
		} else if (obj->GetPool() != 8) {
			PlayWiiSoundEffect(&gGame.mSound, GetMaterialSoundId(0), 1.0f, 0.0f);
		}
		obj->mStatus.mFlags &= ~0x0400000000000000ULL;
		act->mTarget = NULL;
		break;
	}
	case 6:
		obj->SetPos(fn_800E5A2C(act->mOwner) - GetLiftTargetContactOffset(act, obj));
		if (obj->GetPool() == 24) {
			if (act->mOwner->IsAnimDone()) {
				act->mTarget = NULL;
			} else if (!((WiiPlayerPlug*)obj)->mConnection && act->mOwner->GetAnimFrame() >= 90.0f) {
				((WiiPlayerPlug*)obj)->mConnection = act->mPlugConnection | 0x10000;
				obj->mStatus.mFlags &= ~0x0400000000000000ULL;
			} else if (!((WiiPlayerPlug*)obj)->mConnection && act->mOwner->GetAnimFrame() >= 34.0f && act->mPlugConnection == 0x200) {
				((WiiPlayerPlug*)obj)->mEndRotationZ = -40.0f * DEG2RAD;
			}
		}
		break;
	case 7:
		obj->SetPos(fn_800E5A2C(act->mOwner) - GetLiftTargetContactOffset(act, obj));
		if (obj->GetPool() == 24) {
			if (act->mOwner->IsAnimDone()) {
				act->mTargetState = 4;
			} else if (act->mOwner->GetAnimFrame() >= 80.0f) {
				((WiiPlayerPlug*)obj)->mEndRotationZ = 0.0f;
			} else if (((WiiPlayerPlug*)obj)->mConnection && act->mOwner->GetAnimFrame() >= 76.0f) {
				((WiiPlayerPlug*)obj)->mConnection = 0;
				obj->mStatus.mFlags |= 0x0400000000000000ULL;
			}
		}
		break;
	case 8:
		obj->SetPos(fn_800E5A2C(act->mOwner) - GetLiftTargetContactOffset(act, obj));
		if (obj->GetPool() == 24) {
			if (act->mOwner->IsAnimDone()) {
				act->mTargetState = 9;
				if (!((WiiPlayerPlug*)obj)->mConnection) {
					((WiiPlayerPlug*)obj)->mConnection = -1;
				}
			} else if (!((WiiPlayerPlug*)obj)->mConnection && act->mOwner->GetAnimFrame() >= 24.0f) {
				((WiiPlayerPlug*)obj)->mConnection = -1;
			}
		}
		break;
	case 10:
		obj->SetPos(fn_800E5A2C(act->mOwner) - GetLiftTargetContactOffset(act, obj));
		if (obj->GetPool() == 24) {
			if (act->mOwner->IsAnimDone()) {
				act->mTargetState = 4;
			} else if (((WiiPlayerPlug*)obj)->mConnection && act->mOwner->GetAnimFrame() >= 0.0f) {
				((WiiPlayerPlug*)obj)->mConnection = 0;
			}
		}
		break;
	case 11:
		obj->SetPos(fn_800E5A2C(act->mOwner) - GetLiftTargetContactOffset(act, obj));
		if (obj->GetPool() == 24) {
			if (act->mOwner->IsAnimDone()) {
				act->mTargetState = 12;
			} else if (!((WiiPlayerPlug*)obj)->mConnection && act->mOwner->GetAnimFrame() >= 24.0f) {
				((WiiPlayerPlug*)obj)->mConnection = 0x10400;
			}
		}
		break;
	case 12:
		obj->SetPos(fn_800E5A2C(act->mOwner) - GetLiftTargetContactOffset(act, obj));
		break;
	case 13:
		obj->SetPos(fn_800E5A2C(act->mOwner) - GetLiftTargetContactOffset(act, obj));
		if (obj->GetPool() == 24) {
			if (act->mOwner->IsAnimDone()) {
				act->mTargetState = 4;
			} else if (((WiiPlayerPlug*)obj)->mConnection && act->mOwner->GetAnimFrame() >= 0.0f) {
				((WiiPlayerPlug*)obj)->mConnection = 0;
			}
		}
		break;
	case 14:
		if (IsCarryingToothbrush(act) || IsCarryingSpoon(act) || IsCarryingSquirter(act)) {
			obj->SetBaseMtx(act->mOwner->GetWorldMtx(act->mOwner->FindResNode("carry_obj_nll")));
			if (IsCarryingSpoon(act)) {
				obj->SetAnim(4, TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
			} else if (IsCarryingSquirter(act)) {
				obj->SetAnim(sSquirterChargeLevels[(s16)GetOwnedInventoryItemCount(&act->mOwner->mParams, 0x38)] + 5, TRUE, 0.0f, 1.0f,
				             0.0f, 15.0f);
			}
		} else {
			obj->SetPos(fn_800E5A2C(act->mOwner) - GetLiftTargetContactOffset(act, obj));
			CVec scale = act->mOwner->GetJointScale(act->mOwner->FindResNode("carry_obj_nll"));
			obj->SetScale(GetStagePlacementScale(obj) * scale);
			if (IsCarryingMug(act)) {
				obj->SetAnim(3, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			}
		}
		break;
	case 15:
		if ((obj->mStatus.mFlags & 0x0001000100000000ULL) == 0x0001000100000000ULL && obj->GetPool() == 8) {
			SetOrAddInventoryItemCount(&gGame.GetPlayer()->mParams.mInventory, obj->GetItem(), 1, TRUE);
			((XObjItem*)obj)->SetFlag();
			obj->mStatus.mFlags &= ~1ULL;
		} else {
			obj->Release();
			act->mTarget = NULL;
		}
		break;
	case 16:
	case 18: {
		Vec offset = GetLiftTargetContactOffset(act, obj);
		Mtx trans, world;
		PSMTXTrans(trans, offset.x, -offset.y, offset.z);
		PSMTXConcat(act->mOwner->GetWorldMtx("carry_obj_nll"), trans, world);
		obj->SetBaseMtx(world);
		Vec value = { world[0][3], world[1][3], world[2][3] };
		obj->SetPos(value);
		HSD_MtxGetScale(world, &value);
		obj->SetScale(value);
		break;
	}
	case 17:
		if ((obj->mStatus.mFlags & 0x0001000100000000ULL) == 0x0001000100000000ULL && obj->GetPool() == 8) {
			((XObjItem*)obj)->SetFlag();
			obj->mStatus.mFlags &= ~1ULL;
		} else {
			obj->Release();
			act->mTarget = NULL;
		}
		break;
	case 19: {
		obj->ClearBaseMtx();
		act->mYawOffset = 0.0f;
		obj->SetRotationY(act->mYawOffset + act->mOwner->mMoveRotate.y);
		obj->SetPos(fn_800E5A2C(act->mOwner) - GetLiftTargetContactOffset(act, obj));
		CVec scale = act->mOwner->GetJointScale(act->mOwner->FindResNode("carry_obj_nll"));
		obj->SetScale(GetStagePlacementScale(obj) * scale);
		act->mTargetState = 4;
		break;
	}
	case 20:
		if ((obj->mStatus.mFlags & 0x0001000100000000ULL) == 0x0001000100000000ULL) {
			if (obj->IsAnimDone()) {
				obj->mStatus.mFlags &= ~1ULL;
			}
		} else {
			obj->Release();
			act->mTarget = NULL;
		}
		break;
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
			if (act->mOwner->mPreviousContactObject >= 0
			    && (gGame.mXObjs.mObjects[act->mOwner->mPreviousContactObject]->mStatus.mFlags & 0x20)) {
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
	act->mContactEffect = NULL;
	act->mUseEffect     = NULL;
	act->mToolEffect    = NULL;
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
