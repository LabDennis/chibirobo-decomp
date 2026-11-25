#include "game/cgame.h"
#include "game/cparticle.h"
#include "game/cscrn.h"
#include "game/csound.h"
#include "game/gmath.h"
#include "game/objectfloorquery.h"
#include "game/xact.h"
#include "game/xchr.h"

#include <math.h>

extern "C" {
#include <dolphin/mtx/mtx.h>
#include <dolphin/mtx/vec.h>
}

extern "C" {
void ApplyPlayerJumpStrength(XActJump* act, f32 strength);
u32 CheckStageFlag(s32 stage, u32 mask);
void PutAwayLiftedObject(XActLift*);

/**
 * @note Address: 0x800C9C34
 * @note Size: 0x3C
 */
void RecordObjectPositionAndHeightChange(XObj* obj)
{
	if (obj->mMoveTranslate.y != obj->mLastPos.y) {
		obj->mStatus.mFlags |= 0x0010000000000000ULL;
	}
	obj->mLastPos.x = obj->mMoveTranslate.x;
	obj->mLastPos.y = obj->mMoveTranslate.y;
	obj->mLastPos.z = obj->mMoveTranslate.z;
}

/**
 * @note Address: 0x800C9C70
 * @note Size: 0xE4
 */
void XActLadder::HandleInput(const CPadOne& pad, f32& moveWeight)
{
	mBPressed = FALSE;
	if (mState == 5) {
		if (gGame.IsRunning() && gGame.mEvt.mData.mScript == NULL && (pad.mTrigger & PAD_BUTTON_B)) {
			mBPressed = TRUE;
		}
	} else if (mState != 0) {
		moveWeight = 0.0f;
	}
}

CVec GetPlayerFloorSlope(XObjChr*);

static inline f32 JumpMotionLength(XObjChr* owner)
{
	CVec motion = GetPlayerFloorSlope(owner);
	return PSVECMag(&motion);
}

static inline f32 JumpStickLength(const CPadOne* pad)
{
	return sqrtf(pad->mStickX * pad->mStickX + pad->mStickY * pad->mStickY);
}
static inline void JumpAnim(XActJump* act, s32 id, f32 loop = 1)
{
	act->mOwner->SetAnim(id, FALSE, NULL, NULL, NULL, 0, 1, loop, 10);
}
static inline void EndJump(XActJump* act)
{
	XObjChr* owner = act->mOwner;
	owner->SetAnim(0x11, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
	owner->mIdleLoops = 3;
	act->mState       = 0;
}
static inline bool AdvanceJumpArc(XActJump* act, u32 frames)
{
	if (act->mArcAngle < HALF_PI) {
		if ((act->mArcAngle += act->mArcStep * frames) > HALF_PI)
			act->mArcAngle = HALF_PI;
		f32 lift = act->mRise * (f32)sin(act->mArcAngle);
		act->mOwner->SetTranslateY(act->mArcPosition.y + lift, 0);
		f32 angle = HSD_JObjGetRotationY(act->mOwner->mJObj);
		f32 dx    = act->mForward * (act->mArcStep * (f32)sin(angle));
		f32 dz    = act->mForward * (act->mArcStep * (f32)cos(angle));
		act->mOwner->SetTranslateX(act->mArcPosition.x += dx, 0);
		act->mOwner->SetTranslateZ(act->mArcPosition.z += dz, 0);
		return false;
	}
	return true;
}

static inline bool IsJumpAnim(s32 anim, s32 base, s32 first, s32 second)
{
	return anim == base || anim == first || anim == second;
}

/**
 * @note Address: 0x800CA0B4
 * @note Size: 0x1D00
 */
void UpdatePlayerJumpAction(XActJump* act, s32 anim, HSD_AObj* aobj, u32 frames, f32 frame, f32 end)
{
	bool interrupt = false;
	if (!gGame.mEvt.mData.mScript) {
		CPad pad(0);
		if (JumpStickLength(pad.mPad) > 0) {
			switch (act->mState) {
			case 19:
				if (frame > 1)
					interrupt = true;
				break;
			case 4:
			case 18:
				if (frame > 3)
					interrupt = true;
				break;
			case 10:
			case 13:
			case 16:
				act->mOwner->SetAnimRate(1.f + 2.f * ((frame / end) * JumpStickLength(pad.mPad)), 0x5a0);
				break;
			}
		}
	}
	s32 next     = -1;
	bool playing = false;
	switch (act->mState) {
	case 1:
		if (anim != 0x1c) {
			JumpAnim(act, 0x1c, 0);
			return;
		}
		if (!(aobj->flags & 0x44000000))
			return;
		act->mState = 2;
		// fall through
	case 2:
	case 3:
	case 17:
		if (act->mOwner->GetHeight() <= 0 || (act->mOwner->GetHeight() <= 1.f && JumpMotionLength(act->mOwner) > .5f)) {
			if (act->mOwner->mLift.mState == 19) {
				act->mState = 0;
				return;
			}
			if (act->mOwner->mFallDistance > 30) {
				JumpAnim(act, 0x8e);
				act->mState = 5;
				ApplyPlayerJumpStrength(act, act->mOwner->mFallDistance);
				act->mOwner->SetColor(1, 255, 0, 0, 0);
				act->mOwner->SetColor(1, 0, 0, 0, .3f);
				StartScreenFade(&gGame.mScrnTex.mColor, 255, 0, 0, 0, 100, 100, 200, 0, .3f);
			} else if (act->mOwner->mFallDistance > 11.5f) {
				JumpAnim(act, 0x1e);
				act->mState = 4;
				if (act->mOwner->mFallDistance > 12.5f)
					gGame.mParticle.Create(0x10006, 0, act->mOwner->mJObj, FALSE);
			} else if (act->mOwner->mFallDistance > 7.5f) {
				JumpAnim(act, 0x1f);
				act->mState = 18;
			} else {
				JumpAnim(act, 0x20);
				act->mState = 19;
			}
		} else if (act->mOwner->mLift.mState != 19) {
			if (act->mState == 3) {
				if (anim != 0x7c)
					JumpAnim(act, 0x7c, 0);
			} else if (anim != 0x1d && anim != 0x93)
				JumpAnim(act, 0x1d, 0);
		}
		break;
	case 4:
		if (act->mOwner->mLift.mState == 19) {
			act->mState = 0;
			return;
		}
		if ((aobj->flags & 0x44000000) || interrupt)
			EndJump(act);
		break;
	case 5:
		if (act->mOwner->mLift.mState == 19) {
			act->mState = 0;
			return;
		}
		if (aobj->flags & 0x44000000) {
			JumpAnim(act, 0x8f);
			act->mState = 6;
			gGame.mParticle.Create(0x10015, 0, act->mOwner->mJObj, TRUE);
			PlayGameSound(&gGame.mSound, 0x1a, 255);
			PlayGameSound(&gGame.mSound, 0x36, 255);
		}
		break;
	case 6:
		if (act->mOwner->mLift.mState == 19) {
			act->mState = 0;
			return;
		}
		if ((act->mStrength -= frames) <= 0) {
			s32 id = 0x90;
			if (act->mTargetStrength >= 50)
				id = 0x9a;
			else if (act->mTargetStrength >= 45 && !gGame.mEvt.mData.mScript) {
				// a hard landing makes him put the mug (item 22) away
				XObjChr* chr = act->mOwner;
				if (chr->mLift.mState && IsCarryingMug(&chr->mLift))
					PutAwayLiftedObject(&chr->mLift);
			}
			JumpAnim(act, id);
			act->mState = 7;
			gGame.mParticle.Kill(0x15);
			StopGameSfx(&gGame.mSound, 0x36);
		} else if (!IsGameSoundPlaying(&gGame.mSound, 0x36))
			PlayGameSound(&gGame.mSound, 0x36, 255);
		break;
	case 7:
		if (act->mOwner->mLift.mState == 19) {
			act->mState = 0;
			return;
		}
		if (aobj->flags & 0x44000000)
			EndJump(act);
		break;
	case 8:
		next    = 0x5a;
		playing = IsJumpAnim(anim, next, 0x10f, 0x115);
	// fall through
	case 11:
		if (next < 0) {
			next    = 0x60;
			playing = IsJumpAnim(anim, next, 0x112, 0x118);
		}
	// fall through
	case 14:
		if (next < 0) {
			next    = 0x63;
			playing = IsJumpAnim(anim, next, 0x11b, 0x11e);
		}
		if (!playing) {
			act->mTargetHeight = act->mTarget->mSelectedFloorHeight;
			if (act->mOwner->mMoveTranslate.y >= act->mTargetHeight) {
				act->mState = 0;
				return;
			}
			if (act->mTarget) {
				f32 yaw = GetFloorFacingAngle(act->mTarget, &act->mOwner->mMoveTranslate, act->mTargetFloor, 0, NULL, NULL);
				if (__fpclassifyf(yaw) != FP_NAN) {
					StopPlayerMotion(act->mOwner);
					JumpAnim(act, next);
					act->mOwner->SetRotationY(yaw, 0);
				} else {
					act->mState  = 0;
					act->mTarget = NULL;
				}
			}
		} else if (aobj->flags & 0x44000000) {
			s32 id = act->mState == 8 ? 0x5b : act->mState == 11 ? 0x61 : 0x64;
			JumpAnim(act, id);
			++act->mState;
			if (CheckStageFlag(gGame.mStage, 1)) {
				XObjPlug* plug = &act->mOwner->mPlug;
				if (plug->mConnection) {
					plug->mConnection = 0;
					s32 path[]        = { 0 };
					HSD_JObjSetRotationX(GetJointByPath(plug->mJObj, path, 1), 0);
					PlayGameSound(&gGame.mSound, 0x37, 255);
				}
			}
			if (act->mTarget) {
				act->mArcPosition = act->mOwner->mMoveTranslate;
				act->mRise        = act->mTargetHeight - act->mOwner->mMoveTranslate.y;
				act->mArcAngle    = 0;
				act->mArcStep     = act->mRise > 6 ? .8f * (HALF_PI / act->mRise) : DEG2RAD * 12;
				act->mForward     = 5;
				{
					XObjChr* p = act->mOwner;
					f32 radius = p->mMoveScale.x * ReadObjectFloorExtent(p->mFloors[0]).x;
					f32 height
					    = .5f * (p->mMoveScale.x * (ReadObjectFloorOffset(p->mFloors[0]).y + ReadObjectFloorExtent(p->mFloors[0]).y));
					Mtx rot;
					PSMTXRotRad(rot, 'y', p->mMoveRotate.y);
					Vec offset = act->mOwner->mMoveScale * ReadObjectFloorOffset(act->mOwner->mFloors[0]);
					PSMTXMultVec(rot, &offset, &offset);
					f32 yaw = act->mOwner->mMoveRotate.y;
					f32 x   = 3.f * (f32)sin(yaw) + (offset.x + act->mOwner->mMoveTranslate.x);
					f32 z   = 3.f * (f32)cos(yaw) + (offset.z + act->mOwner->mMoveTranslate.z);
					f32 y   = .5f + act->mTargetHeight;
					for (s32 i = 0; i < gGame.mXObjs.mObjectCount; ++i) {
						XObj* obj = gGame.GetObject(i);
						if (!(obj->mStatus.mFlags & 1) || !(obj->mStatus.mFlags & 0x10000) || obj->mStatus.mFlags & 0x100000)
							continue;
						if (obj->GetCategory() == 7 || obj->GetCategory() == 20 || act->mOwner->mId == obj->mId || obj->GetKind() >= 10000)
							continue;
						f32 dx = obj->mMoveTranslate.x - x, dz = obj->mMoveTranslate.z - z;
						if (sqrtf(dx * dx + dz * dz) > 100.f + (radius + obj->mFloorXZExtent))
							continue;
						CVec push, contact, from(x, y, z), to(x, y, z);
						if ((obj->TestFloorCollision(&from, &to, radius, height, &push, &contact, 0, 0) & 3) == 3) {
							act->mForward = 0;
							act->mRise *= .8f;
							break;
						}
					}
				}
				AdvanceJumpArc(act, frames);
			}
			if (!act->mTarget || act->mRise <= 0) {
				EndJump(act);
				act->mTarget = NULL;
			}
		}
		break;
	case 9:
		next    = 0x5b;
		playing = IsJumpAnim(anim, next, 0x110, 0x116);
	// fall through
	case 12:
		if (next < 0) {
			next    = 0x61;
			playing = IsJumpAnim(anim, next, 0x113, 0x119);
		}
	// fall through
	case 15:
		if (next < 0) {
			next    = 0x64;
			playing = IsJumpAnim(anim, next, 0x11c, 0x11f);
		}
		if (!playing)
			JumpAnim(act, next);
		if (AdvanceJumpArc(act, frames)) {
			if (act->mForward == 0) {
				JumpAnim(act, 0x1d, 0);
				act->mState = 17;
				return;
			}
			s32 id = act->mState == 9 ? 0x5c : act->mState == 12 ? 0x62 : 0x65;
			JumpAnim(act, id);
			++act->mState;
		}
		break;
	case 10:
		next    = 0x5c;
		playing = IsJumpAnim(anim, next, 0x111, 0x117);
	// fall through
	case 13:
		if (next < 0) {
			next    = 0x62;
			playing = IsJumpAnim(anim, next, 0x114, 0x11a);
		}
	// fall through
	case 16:
		if (next < 0) {
			next    = 0x65;
			playing = IsJumpAnim(anim, next, 0x11d, 0x120);
		}
		if (!playing) {
			JumpAnim(act, next);
			return;
		}
		if (aobj->flags & 0x44000000) {
			EndJump(act);
			act->mTarget = NULL;
		} else if (interrupt)
			act->mOwner->SetAnimRate(4, 0x5a0);
		break;
	case 18:
	case 19:
		if ((aobj->flags & 0x44000000) || interrupt)
			EndJump(act);
		break;
	}
}

// the jump's fall: a long one makes him put the mug away, a longer one makes him tumble
/**
 * @note Address: 0x800C9D54
 * @note Size: 0x360
 */
void ApplyPlayerJumpStrength(XActJump* act, f32 strength)
{
	if (gGame.mEvt.mData.mScript != NULL) {
		act->mTargetStrength = 0.0f;
		act->mStrength       = 0.0f;
	} else {
		act->mTargetStrength = strength;
		act->mStrength       = strength;
		if (act->mTargetStrength >= 50.0f) {
			XObjChr* chr = act->mOwner;
			if (chr->mLift.mState != 0 && IsCarryingMug(&chr->mLift)) {
				PutAwayLiftedObject(&chr->mLift);
				if (act->mTargetStrength >= 60.0f) {
					act->mState                 = 0;
					act->mOwner->mTumble.mState = 1;
					act->mOwner->SetAnim(0x35, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				}
			}
		}
	}
}

/**
 * @note Address: 0x800CBDB4
 * @note Size: 0x18
 */
void XActJump::HandleInput(const CPadOne&, f32& moveWeight)
{
	if (mState != 0) {
		moveWeight = 0.0f;
	}
}

} // extern "C"
