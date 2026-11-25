#include "game/cgame.h"
#include "game/ccamera.h"
#include "game/cscrn.h"
#include "game/csound.h"
#include "game/gmath.h"
#include "game/objectfloorquery.h"
#include "game/stagedata.h"
#include "game/xact.h"
#include "game/xchr.h"

#include <math.h>
#include <stdlib.h>

extern "C" {
#include <dolphin/mtx/mtx.h>
#include <dolphin/mtx/vec.h>
#include <sysdolphin/aobj.h>
}

extern "C" {
void SetObjectJointTranslationByPath(CJObj* object, s32 depth, s32* path, f32 x, f32 y, f32 z);
CVec GetPlayerJointLocalTranslationByPath(CJObj* obj, s32 depth, s32* path);
void SetJointYawByPath(CXObj* obj, s32 depth, s32* path, f32 angle);
void SetJointTranslationFromVectorByPath(CJObj* obj, s32 depth, s32* path, const Vec& pos);
void RecordObjectPositionAndHeightChange(XObj* obj);
void ApplyPlayerJumpStrength(XActJump* act, f32 strength);
void TryLeaveHang(XActHang* act);
Vec GetTargetFloorPosition(XAction* act);
u32 CheckStageFlag(s32 stage, u32 mask);

static f32 GetScaledTargetFloorExtent(const XAction* act)
{
	const XObj* target = act->mTarget;
	return target->mMoveScale.x * ReadObjectFloorExtent(target->mFloors[act->mTargetFloor]).x;
}

s32 FindPlayerSupportingFloor(XObjChr*, s32, s32);
f32 GetObjectEdgeOutwardYaw(const XObj*, Vec*, s32);

static inline void HangAnim(XActHang* act, s32 id, f32 loop = 1, BOOL force = FALSE, f32 start = 0)
{
	act->mOwner->SetAnim(id, force, NULL, NULL, NULL, start, 1, loop, 10);
}
static inline void ReleaseHangPlug(XActHang* act)
{
	if (CheckStageFlag(gGame.mStage, 1)) {
		XObjPlug* plug = &act->mOwner->mPlug;
		if (plug->mConnection) {
			ResetPlayerPlugState(plug, TRUE);
			PlayGameSound(&gGame.mSound, 0x37, 255);
		}
	}
}
static inline bool RoundHangFloor(XActHang* act)
{
	return act->mTarget->mFloors[act->mTargetFloor].mId == 0;
}
static inline void SaveHangDistance(XActHang* act)
{
	CVec owner(act->mOwner->mMoveTranslate);
	owner.y = 0;
	CVec floor(GetTargetFloorPosition(act));
	floor.y           = 0;
	act->mFloorCenter = floor;
	act->mDistance    = PSVECDistance(&owner, &floor);
}
static inline void RefreshHangFloor(XActHang* act)
{
	s32 floor = FindPlayerSupportingFloor(act->mOwner, 1, act->mTargetFloor);
	if (floor >= 0)
		act->mTargetFloor = floor;
}
static inline f32 RoundHangAngle(XActHang* act)
{
	CVec owner(act->mOwner->mMoveTranslate);
	owner.y = 0;
	CVec floor(GetTargetFloorPosition(act));
	floor.y = 0;
	return PI + (f32)atan2(floor.x - owner.x, floor.z - owner.z);
}
static inline void HangRootPosition(XActHang* act)
{
	s32 path[] = { 0 };
	act->mOwner->SetPos(GetEventJointWorldPosition(act->mOwner, 1, path), 0);
	RecordObjectPositionAndHeightChange(act->mOwner);
	s32 reset[] = { 0 };
	SetObjectJointTranslationByPath(act->mOwner, 1, reset, 0, 0, 0);
}
static inline void FinishHang(XActHang* act, f32 loop)
{
	XObjChr* owner = act->mOwner;
	owner->SetAnim(0x11, FALSE, NULL, NULL, NULL, 0, 1, loop, 10);
	owner->mIdleLoops = 3;
	act->mState       = 0;
}

/**
 * @note Address: 0x800CBDCC
 * @note Size: 0x1CE8
 */
void UpdatePlayerHangAction(XActHang* act, s32 anim, HSD_AObj* aobj, u32, f32 frame, f32 end)
{
	switch (act->mState) {
	case 1: {
		static CVec contact, normal, start;
		if (anim != 0x13) {
			if (act->mTarget) {
				f32 angle = GetFloorFacingAngle(act->mTarget, &act->mOwner->mMoveTranslate, act->mTargetFloor, 1, &contact, &normal);
				if (__fpclassifyf(angle) == FP_NAN) {
					act->mState  = 0;
					act->mTarget = NULL;
					return;
				}
				StopPlayerMotion(act->mOwner);
				HangAnim(act, 0x13, 0);
				act->mOwner->SetRotationY(angle, 0);
				start.Set(act->mOwner->mMoveTranslate);
				contact.y = start.y;
			}
			ReleaseHangPlug(act);
		} else if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
			HangAnim(act, 0x14);
			act->mState         = 5;
			act->mMoveDirection = 0;
			if (act->mTarget) {
				if (RoundHangFloor(act))
					SaveHangDistance(act);
				else
					act->mOwner->SetPos(contact - normal * 4.f, 0);
			}
		} else if (act->mTarget) {
			if (RoundHangFloor(act)) {
				XObjChr* chr = act->mOwner;
				f32 radius   = chr->mMoveScale.x * GetScaledTargetFloorExtent(act);
				CVec owner(act->mOwner->mMoveTranslate);
				owner.y = 0;
				CVec floor(GetTargetFloorPosition(act));
				floor.y = 0;
				if (PSVECDistance(&owner, &floor) < 4.f + radius) {
					f32 yaw = act->mOwner->mMoveRotate.y;
					act->mOwner->SetTranslateX(act->mOwner->mMoveTranslate.x - .2f * (f32)sin(yaw), 0);
					act->mOwner->SetTranslateZ(act->mOwner->mMoveTranslate.z - .2f * (f32)cos(yaw), 0);
				}
			} else {
				CVec delta = (contact - normal * 4.f) - start;
				act->mOwner->SetPos((contact - normal * 4.f) - delta * (1.f - frame / end), 0);
			}
		}
		break;
	}
	case 4:
		if (anim != 0x7a && anim != 0xca) {
			if (act->mTarget) {
				RefreshHangFloor(act);
				f32 yaw               = RoundHangFloor(act) ? RoundHangAngle(act)
				                                            : PI + GetObjectEdgeOutwardYaw(act->mTarget, &act->mOwner->mMoveTranslate, act->mTargetFloor);
				yaw                   = WrapAngle(yaw);
				act->mHangStartOffset = act->mOwner->mMoveTranslate - act->mTarget->mMoveTranslate;
				CVec pos(act->mOwner->mMoveTranslate);
				pos.x += 1.5f * (f32)sin(PI + yaw);
				pos.z += 1.5f * (f32)cos(PI + yaw);
				act->mOwner->SetPos(pos, 0);
				RecordObjectPositionAndHeightChange(act->mOwner);
			}
			HangAnim(act, 0x7a, 0);
			return;
		}
		if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
			act->mOwner->SetPos(act->mTarget->mMoveTranslate + act->mHangStartOffset, 0);
			RecordObjectPositionAndHeightChange(act->mOwner);
			act->mState               = 0;
			act->mTarget              = NULL;
			act->mOwner->mJump.mState = 3;
			ReleaseHangPlug(act);
			return;
		}
		if (!act->mStickAway)
			return;
		{
			f32 last = GetJointAnimationEndFrame(act->mOwner);
			if (GetJointAnimationCurrentFrame(act->mOwner) / last <= .6f) {
				FinishHang(act, 0);
				act->mTarget = NULL;
				return;
			}
		}
		act->mOwner->SetPos(act->mTarget->mMoveTranslate + act->mHangStartOffset, 0);
		RecordObjectPositionAndHeightChange(act->mOwner);
		if (act->mOwner->GetAttachment() == AttachmentPopper || act->mOwner->mLift.mState) {
			act->mOwner->mJump.mState = 2;
			act->mState               = 0;
			act->mTarget              = NULL;
			ReleaseHangPlug(act);
		} else if (act->mTarget && act->mTarget->mStatus.mFlags & 0x800000000ULL) {
			act->mOwner->mLadder.SetState(3, act->mTarget);
			act->mState  = 0;
			act->mTarget = NULL;
		} else if (act->mTarget && act->mTarget->mStatus.mFlags & 0x1000000000ULL) {
			act->mOwner->mRope.SetState(3, act->mTarget);
			act->mState  = 0;
			act->mTarget = NULL;
			return;
		} else {
			act->mState = 3;
			return;
		}
		// The jump and ladder transitions also run the stage plug check below.
		// fall through
	case 3:
		if (anim != 0x76) {
			if (act->mTarget) {
				RefreshHangFloor(act);
				CVec contact(0, 0, 0);
				f32 yaw;
				if (RoundHangFloor(act))
					yaw = RoundHangAngle(act);
				else {
					yaw = PI + GetFloorFacingAngle(act->mTarget, &act->mOwner->mMoveTranslate, act->mTargetFloor, 0, &contact, NULL);
					if (__fpclassifyf(yaw) == FP_NAN) {
						act->mOwner->mJump.mState = 2;
						act->mState               = 0;
						act->mTarget              = NULL;
						return;
					}
				}
				yaw = WrapAngle(yaw);
				act->mOwner->SetRotationY(yaw, 0);
				HangAnim(act, 0x13, 1, TRUE);
				HangAnim(act, 0x13, 1, TRUE, GetJointAnimationEndFrame(act->mOwner));
				act->mOwner->UpdateAnim(0);
				s32 p1[]   = { 0 };
				CVec local = GetPlayerJointLocalTranslationByPath(act->mOwner, 1, p1);
				s32 p2[]   = { 0 };
				CVec delta = GetEventJointWorldPosition(act->mOwner, 1, p2) - act->mOwner->mMoveTranslate;
				delta.y    = -delta.y;
				act->mOwner->SetPos(act->mOwner->mMoveTranslate + delta, 0);
				RecordObjectPositionAndHeightChange(act->mOwner);
				HangAnim(act, 0x76);
				local.x  = -local.x;
				local.z  = -local.z;
				s32 p3[] = { 0 };
				SetJointTranslationFromVectorByPath(act->mOwner, 1, p3, local);
				CVec pos(act->mOwner->mMoveTranslate);
				f32 distance;
				if (RoundHangFloor(act)) {
					CVec owner(act->mOwner->mMoveTranslate);
					owner.y = 0;
					CVec floor(GetTargetFloorPosition(act));
					floor.y    = 0;
					yaw        = PI + (f32)atan2(floor.x - owner.x, floor.z - owner.z);
					f32 span   = PSVECDistance(&owner, &floor);
					XObjChr* p = act->mOwner;
					distance   = 1.f + (span - p->mMoveScale.x * GetScaledTargetFloorExtent(act));
				} else {
					pos.x    = contact.x;
					pos.z    = contact.z;
					distance = 1;
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
				act->mOwner->SetPos(pos, 0);
				RecordObjectPositionAndHeightChange(act->mOwner);
			}
			ReleaseHangPlug(act);
		} else if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
			s32 childPath[] = { 0 };
			HSD_JObj* child = GetObjectJointByPath(act->mOwner, 1, childPath);
			f32 turn        = HSD_JObjGetRotationY(child);
			act->mOwner->SetRotationY(HSD_JObjGetRotationY(act->mOwner->mJObj) + turn, 0);
			HSD_JObjSetRotationY(child, 0);
			HangAnim(act, 0x13);
			HangAnim(act, 0x13, 1, TRUE, GetJointAnimationEndFrame(act->mOwner));
			act->mOwner->UpdateAnim(0);
			s32 p1[]   = { 0 };
			CVec local = GetPlayerJointLocalTranslationByPath(act->mOwner, 1, p1);
			s32 p2[]   = { 0 };
			// Update the joint's world matrix before restoring the local translation.
			GetEventJointWorldPosition(act->mOwner, 1, p2);
			s32 p3[] = { 0 };
			SetJointTranslationFromVectorByPath(act->mOwner, 1, p3, local);
			HangAnim(act, 0x14);
			act->mState         = 5;
			act->mMoveDirection = 0;
			if (act->mTarget && RoundHangFloor(act))
				SaveHangDistance(act);
		}
		break;
	case 2:
		if (anim != 0x18) {
			HangAnim(act, 0x18);
			if (act->mTarget)
				act->mOwner->SetRotationY(GetObjectEdgeOutwardYaw(act->mTarget, &act->mOwner->mMoveTranslate, act->mTargetFloor), 0);
		} else if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
			HangRootPosition(act);
			act->mOwner->SetRotationY(PI + act->mOwner->mMoveRotate.y, 0);
			s32 p[] = { 0 };
			SetJointYawByPath(act->mOwner, 1, p, 0);
			HangAnim(act, 0x14);
			act->mOwner->UpdateAnim(0);
			s32 child[] = { 0 };
			act->mOwner->SetPos(
			    act->mOwner->mMoveTranslate + (act->mOwner->mMoveTranslate - GetEventJointWorldPosition(act->mOwner, 1, child)), 1);
			RecordObjectPositionAndHeightChange(act->mOwner);
			act->mState         = 5;
			act->mMoveDirection = 0;
		}
		break;
	case 5:
		if (act->mMoveDirection == -1) {
			if (anim != 0x16)
				HangAnim(act, 0x16);
		} else if (act->mMoveDirection == 1) {
			if (anim != 0x15)
				HangAnim(act, 0x15);
		} else if (anim != 0x14)
			HangAnim(act, 0x14);
		break;
	// climbing up, TryLeaveHang picks the other climb (7) one time in eight
	case 6:
	case 7: {
		int next = act->mState == 7 ? 0x7d : 0x17;
		if (anim != next)
			HangAnim(act, next);
		else if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
			HangRootPosition(act);
			FinishHang(act, 0);
			act->mTarget = NULL;
		}
		break;
	}
	case 8:
		HangAnim(act, 0x19, 1, TRUE, 15);
		act->mState = 9;
	// fall through
	case 9:
		if (anim != 0x19) {
			HangAnim(act, 0x19);
			return;
		}
		if (!(aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)))
			return;
		HangAnim(act, 0x1a);
		act->mState  = 10;
		act->mTarget = NULL;
	// fall through
	case 10:
		if (act->mOwner->GetHeight() <= 0) {
			if (act->mOwner->mFallDistance > 30) {
				act->mState = 0;
				HangAnim(act, 0x8e);
				act->mOwner->mJump.mState = 5;
				ApplyPlayerJumpStrength(&act->mOwner->mJump, act->mOwner->mFallDistance);
				act->mOwner->SetColor(1, 255, 0, 0, 0);
				act->mOwner->SetColor(1, 0, 0, 0, .3f);
				StartScreenFade(&gGame.mScrnTex.mColor, 255, 0, 0, 0, 100, 100, 200, 0, .3f);
			} else {
				HangAnim(act, 0x1b);
				act->mState = 11;
			}
		} else if (act->mOwner->GetAttachment() == AttachmentCopter) {
			act->mState                 = 0;
			act->mOwner->mCopter.mState = 8;
		}
		break;
	case 11:
		if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM))
			FinishHang(act, 1);
		break;
	}
}

/**
 * @note Address: 0x800CDBA8
 * @note Size: 0x578
 */
void TryLeaveHang(XActHang* act)
{
	const XObjFloor& floor = act->mOwner->mFloors[0];
	Vec extent             = ReadObjectFloorExtent(floor);
	Vec offset             = ReadObjectFloorOffset(floor);
	f32 radius             = act->mOwner->mMoveScale.x * extent.x;
	f32 halfHeight         = 0.5f * (act->mOwner->mMoveScale.x * (offset.y + extent.y));
	s32 path[]             = { 0 };
	Vec position           = GetEventJointWorldPosition(act->mOwner, 1, path);
	position.y += 0.5f;

	for (s32 i = 0; i < gGame.mXObjs.mObjectCount; i++) {
		XObj* obj = gGame.GetObject(i);
		if (!(obj->mStatus.mFlags & 1) || !(obj->mStatus.mFlags & 0x10000) || (obj->mStatus.mFlags & 0x100000))
			continue;
		if (obj->GetCategory() == 7 || obj->GetCategory() == 20 || (s32)act->mOwner->mId == (s32)obj->mId || obj->GetKind() >= 10000)
			continue;
		f32 dx = obj->mMoveTranslate.x - position.x;
		f32 dz = obj->mMoveTranslate.z - position.z;
		if (sqrtf(dx * dx + dz * dz) > 100.0f + (radius + obj->mFloorXZExtent))
			continue;

		CVec push, contact;
		if ((obj->TestFloorCollision(&position, &position, radius, halfHeight, &push, &contact, 0, 0) & 3) == 3) {
			return;
		}
	}
	if ((rand() & 7) == 0)
		act->mState = 7;
	else
		act->mState = 6;
}

/**
 * @note Address: 0x800CE120
 * @note Size: 0x214
 */
void XActHang::HandleInput(const CPadOne& pad, f32& moveWeight)
{
	mStickAway = FALSE;
	if (mState == 4) {
		f32 stick = -(f32)atan2(pad.mStickX, pad.mStickY);
		f32 angle = WrapAngle(stick + gGame.mMotionControlCamera.mCamera.mAngle.y - mOwner->mMoveRotate.y);
		// the stick points anywhere but within 30 degrees of where he faces
		if (!(angle < 30 * DEG2RAD) && !(angle > 330 * DEG2RAD)) {
			mStickAway = TRUE;
		}
		moveWeight = 0.0f;
	} else if (mState == 5) {
		if (gGame.IsRunning() && gGame.mEvt.mData.mScript == NULL) {
			// a climbs up if there is room, b lets go (9)
			if (pad.mTrigger & PAD_BUTTON_A) {
				if (mTarget == NULL || (mTarget->mStatus.mFlags & 0x400000000ULL)) {
					TryLeaveHang(this);
					mMoveDirection = 0;
				}
				moveWeight = 0.0f;
			} else if (pad.mTrigger & PAD_BUTTON_B) {
				mState         = 9;
				mMoveDirection = 0;
				moveWeight     = 0.0f;
			}
		}
	} else if (mState != 0) {
		moveWeight = 0.0f;
	}
}

} // extern "C"
