#include "game/cgame.h"
#include "game/cjobjinlineattach.h"
#include "game/cjobjset.h"
#include "game/cparticle.h"
#include "game/cscrn.h"
#include "game/csound.h"
#include "game/cvec.h"
#include "game/gmath.h"
#include "game/stagedata.h"
#include "game/xact.h"
#include "game/xchr.h"

#include <math.h>

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
void ApplyPlayerJumpStrength(XActJump* act, f32 strength);
u32 CheckStageFlag(s32 stage, u32 mask);

void PositionPlayerAroundRopeTarget(XActRope*, f32);

static inline void RopeAnim(XActRope* act, s32 id, f32 loop, BOOL force = FALSE, f32 start = 0)
{
	act->mOwner->SetAnim(id, force, NULL, NULL, NULL, start, 1, loop, 10);
}
static inline void LocateRope(XActRope* act, bool grab)
{
	CVec player(act->mOwner->mMoveTranslate);
	player.y = 0;
	CVec rope(act->mTarget->mMoveTranslate);
	rope.y = 0;
	CVec offset;
	if (grab) {
		offset.Set(act->mTargetOffset);
		offset.y = 0;
	} else
		offset.Set(0, 0, -.5f);
	Mtx matrix;
	PSMTXRotRad(matrix, 'y', grab ? act->mOwner->mMoveRotate.y : act->mTarget->mMoveRotate.y);
	PSMTXMultVec(matrix, &offset, &offset);
	if (grab) {
		PSVECSubtract(&rope, &offset, &rope);
	} else
		rope += offset;
	act->mPos   = rope;
	act->mDist  = PSVECDistance(&rope, &player);
	act->mAngle = atan2(player.x - rope.x, player.z - rope.z);
}
static inline void ApproachRope(XActRope* act, u32 frames, bool move, bool snap, f32 speed)
{
	if (!act->mTarget)
		return;
	Vec delta = act->mTarget->mMoveTranslate - act->mTarget->mLastPos;
	act->mPos.x += delta.x;
	act->mPos.z += delta.z;
	if (act->mDist > 0 && move) {
		if ((act->mDist -= speed * frames) < 0)
			act->mDist = 0;
		act->mOwner->SetTranslateX(act->mDist * (f32)sin(act->mAngle) + act->mPos.x, 0);
		act->mOwner->SetTranslateZ(act->mDist * (f32)cos(act->mAngle) + act->mPos.z, 0);
	} else if (snap) {
		act->mOwner->SetTranslateX(act->mPos.x, 0);
		act->mOwner->SetTranslateZ(act->mPos.z, 0);
	}
}
static inline void ReleaseRopePlug(XActRope* act, f32 frame, f32 threshold)
{
	if (CheckStageFlag(gGame.mStage, 1)) {
		XObjPlug* plug = &act->mOwner->mPlug;
		if (plug->mConnection && frame >= threshold) {
			ResetPlayerPlugState(plug, TRUE);
			PlayGameSound(&gGame.mSound, 0x37, 0xff);
		}
	}
}
static inline void SyncRopePosition(XObjChr* owner)
{
	if (owner->mMoveTranslate.y != owner->mLastPos.y)
		owner->mStatus.mFlags |= 0x0010000000000000ULL;
	owner->mLastPos.Set(owner->mMoveTranslate);
}

/**
 * @note Address: 0x800C5B98
 * @note Size: 0x1C78 (US)
 */
void UpdatePlayerRopeAction(XActRope* act, s32 anim, HSD_AObj* aobj, u32 frames, f32 frame, f32)
{
	switch (act->mState) {
	case 1:
		if (anim != 0x21) {
			RopeAnim(act, 0x21, 0);
			if (act->mTarget) {
				act->mOwner->SetRotationY(WrapAngle(atan2(act->mTarget->mMoveTranslate.x - act->mOwner->mMoveTranslate.x,
				                                          act->mTarget->mMoveTranslate.z - act->mOwner->mMoveTranslate.z)),
				                          0);
				LocateRope(act, true);
			}
		} else if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
			RopeAnim(act, 0x8c, 0);
			act->mState = 5;
		} else {
			ApproachRope(act, frames, frame > 20, true, .2f);
			ReleaseRopePlug(act, frame, 28);
		}
		break;
	case 2:
		if (anim != 0x98) {
			RopeAnim(act, 0x98, 0);
			if (act->mTarget) {
				act->mOwner->SetRotationY(WrapAngle(act->mTarget->mMoveRotate.y), 0);
				LocateRope(act, false);
			}
		} else if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
			s32 path[] = { 0 };
			CVec pos   = GetEventJointWorldPosition(act->mOwner, 1, path);
			Vec offset = act->mTargetOffset;
			Mtx matrix;
			PSMTXRotRad(matrix, 'y', act->mTarget->mMoveRotate.y);
			PSMTXMultVec(matrix, &offset, &offset);
			act->mOwner->SetPos(pos.x + offset.x, pos.y - offset.y, pos.z + offset.z, 0);
			SyncRopePosition(act->mOwner);
			act->mOwner->SetRotationY(PI + act->mOwner->mMoveRotate.y, 0);
			s32 p2[] = { 0 };
			SetJointYawByPath(act->mOwner, 1, p2, 0);
			s32 p3[] = { 0 };
			SetJointTranslationFromVectorByPath(act->mOwner, 1, p3, act->mTargetOffset);
			RopeAnim(act, 0x8c, 0);
			act->mState = 5;
		} else {
			ApproachRope(act, frames, frame > 80, false, .2f);
			ReleaseRopePlug(act, frame, 80);
		}
		break;
	case 3:
		if (anim != 0x85) {
			if (act->mTarget) {
				act->mOwner->SetRotationY(act->mTarget->mMoveRotate.y, 0);
				RopeAnim(act, 0x8a, 1, TRUE);
				act->mOwner->UpdateAnim(0);
				s32 p1[] = { 0 };
				// Keep the local joint lookup's missing-joint assertion.
				GetPlayerJointLocalTranslationByPath(act->mOwner, 1, p1);
				s32 p2[]   = { 0 };
				CVec delta = GetEventJointWorldPosition(act->mOwner, 1, p2) - act->mOwner->mMoveTranslate;
				act->mOwner->SetTranslateY(act->mOwner->mMoveTranslate.y - delta.y, 0);
				RopeAnim(act, 0x85, 1);
				LocateRope(act, false);
			}
			ReleaseRopePlug(act, 0, 0);
		} else if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
			s32 p1[]        = { 0 };
			HSD_JObj* child = GetJointByPath(act->mOwner->mJObj, p1, 1);
			f32 turn        = HSD_JObjGetRotationY(child);
			act->mOwner->SetRotationY(HSD_JObjGetRotationY(act->mOwner->mJObj) + turn, 0);
			HSD_JObjSetRotationY(child, 0);
			s32 p2[]   = { 0 };
			CVec delta = GetEventJointWorldPosition(act->mOwner, 1, p2) - act->mOwner->mMoveTranslate;
			delta.y    = 0;
			CVec offset(act->mTargetOffset);
			Mtx matrix;
			PSMTXRotRad(matrix, 'y', HSD_JObjGetRotationY(act->mOwner->mJObj));
			PSMTXMultVec(matrix, &offset, &offset);
			offset.y = 0;
			act->mOwner->SetPos(act->mOwner->mMoveTranslate - (delta + offset), 0);
			SyncRopePosition(act->mOwner);
			s32 p3[] = { 0 };
			SetJointTranslationFromVectorByPath(act->mOwner, 1, p3, act->mTargetOffset);
			RopeAnim(act, 0x8c, 0);
			act->mState = 5;
		} else
			ApproachRope(act, frames, true, true, .4f);
		break;
	case 4:
		RopeAnim(act, 0x8c, 1);
		act->mState = 5;
		break;
	case 5:
		if (anim == 0x8b) {
			if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
				if (act->mMoveDirection == 1)
					RopeAnim(act, 0x8d, 1);
				else
					RopeAnim(act, 0x8c, 1);
			}
		} else if (anim == 0x8d) {
			if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
				if (act->mMoveDirection == 1)
					RopeAnim(act, 0x8b, 1);
				else
					RopeAnim(act, 0x8a, 1);
			}
		} else if (act->mMoveDirection == 1 || act->mMoveDirection == 2) {
			if (anim == 0x8a)
				RopeAnim(act, 0x8b, 1);
			else if (anim == 0x8c)
				RopeAnim(act, 0x8d, 1);
#if defined(VERSION_PAL)
			else if (anim == 0x23)
				RopeAnim(act, 0x8c, 0);
#endif
		} else if (act->mMoveDirection == -1) {
			if (act->mOwner->GetHeight() > 0) {
				if (anim != 0x23)
					RopeAnim(act, 0x23, 0);
			} else {
				act->mState = 8;
				RopeAnim(act, 0x25, 0);
			}
		} else if (anim == 0x23)
			RopeAnim(act, 0x8c, 0);
		if (act->mMoveDirection == 1 || act->mMoveDirection == 2) {
			s32 path[] = { 0, 0 };
			if (act->mTarget && GetEventJointWorldPosition(act->mOwner, 2, path).y >= act->mTarget->mFloorHeights[0]) {
				act->mOwner->SetTranslateY(act->mTarget->mFloorHeights[0] - act->mTargetOffset.y, 0);
				RopeAnim(act, 0x24, 0);
				act->mState = 6;
				break;
			}
			PlayGameSound(&gGame.mSound, 0x11, 0xff);
		} else if (act->mMoveDirection == -1)
			PlayGameSound(&gGame.mSound, 0x15, 0xff);
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
			if (anim == 0x34 || anim == 0x8b || anim == 0x8d || anim == 0x23)
				act->mOwner->SetRotationY(angle, 0);
			PositionPlayerAroundRopeTarget(act, angle);
		}
		break;
	case 6:
		if (anim != 0x24)
			RopeAnim(act, 0x24, 0);
		else if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
			s32 p[] = { 0 };
			act->mOwner->SetPos(GetEventJointWorldPosition(act->mOwner, 1, p), 0);
			SyncRopePosition(act->mOwner);
			s32 p2[] = { 0 };
			SetObjectJointTranslationByPath(act->mOwner, 1, p2, 0, 0, 0);
			XObjChr* owner = act->mOwner;
			owner->SetAnim(0x11, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
			owner->mIdleLoops = 3;
			act->mState       = 0;
			act->mTarget      = NULL;
		}
		break;
	case 7:
		RopeAnim(act, 0x25, 1, TRUE, 20);
		act->mState = 8;
	// fall through
	case 8:
		if (anim != 0x25)
			RopeAnim(act, 0x25, 0);
		else if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
			if (act->mOwner->GetAttachment() == AttachmentCopter) {
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
				ApplyPlayerJumpStrength(&act->mOwner->mJump, act->mOwner->mFallDistance);
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
		if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
			XObjChr* owner = act->mOwner;
			owner->SetAnim(0x11, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
			owner->mIdleLoops = 3;
			act->mState       = 0;
		}
		break;
	case 11:
		if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
			act->mState = 0;
			RopeAnim(act, 0x8f, 1);
			act->mOwner->mJump.mState = 6;
			gGame.mParticle.Create(0x10015, 0, act->mOwner->mJObj, TRUE);
			PlayGameSound(&gGame.mSound, 0x1a, 255);
			PlayGameSound(&gGame.mSound, 0x36, 255);
		}
		break;
	}
	if (act->mState == 5) {
		s32 p[]         = { 0 };
		HSD_JObj* joint = GetJointByPath(act->mOwner->mJObj, p, 1);
		if (joint)
			act->mOwner->Push(joint, 2, act->mTargetOffset.y);
	}
}

// walks him around the target at the same distance
/**
 * @note Address: 0x800C5A7C
 * @note Size: 0x11C
 */
void PositionPlayerAroundRopeTarget(XActRope* act, f32 angle)
{
	if (act->mTarget != NULL) {
		const CVec dir(act->mTargetOffset.x, 0.0f, act->mTargetOffset.z);
		f32 dist = dir.Length();
		act->mOwner->SetTranslateX(act->mTarget->mMoveTranslate.x - dist * (f32)sin(angle), 0);
		act->mOwner->SetTranslateZ(act->mTarget->mMoveTranslate.z - dist * (f32)cos(angle), 0);
	}
}

/**
 * @note Address: 0x800C7AC4
 * @note Size: 0x100
 */
void XActRope::HandleInput(const CPadOne& pad, f32& moveWeight)
{
	if (mState == 5) {
		if (gGame.IsRunning() && gGame.mEvt.mData.mScript == NULL) {
			// b lets go of the rope
			if (pad.mTrigger & PAD_BUTTON_B) {
				mState = 8;
			}
			if (mOwner->mAnimId == 0x8A || mOwner->mAnimId == 0x8C) {
				moveWeight = 0.0f;
			}
		}
	} else if (mState != 0) {
		moveWeight = 0.0f;
	}
}

/**
 * @note Address: 0x800C7810
 * @note Size: 0x2B4
 */
void SetJointTranslationFromVectorByPath(CJObj* obj, s32 depth, s32* path, const Vec& pos)
{
	CJObj joint(GetJointByPath(obj->mJObj, path, depth));
	joint.SetTranslate(pos.x, pos.y, pos.z, TRUE);
}

} // extern "C"
