#include "game/cgame.h"
#include "game/objectfloorquery.h"
#include "game/xact.h"
#include "game/xchr.h"

#include <math.h>

extern "C" {
#include <sysdolphin/aobj.h>
}

extern "C" {
/**
 * @note Address: 0x800C5270
 * @note Size: 0x464
 */
void UpdatePushActionAnimation(XActPush* act, s32 anim, HSD_AObj* aobj, u32 frames, f32 frame, f32 end)
{
	switch (act->mState) {
	case 1:
		if (anim != 0x3E) {
			if (act->mTarget != NULL) {
				f32 angle = GetFloorFacingAngle(act->mTarget, &act->mOwner->mMoveTranslate, act->mTargetFloor, 1, 0, 0);
				if (fpclassify(angle) != FP_NAN) {
					act->mOwner->SetAnim(0x3E, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
					act->mOwner->SetRotationY(angle, 0);
				} else {
					ReleasePushAction(act);
					act->mState = 0;
				}
			}
		} else if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
			act->mOwner->SetAnim(0x3F, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
			act->mState       = 2;
			act->mTargetState = 1;
		}
		break;
	case 2:
		if (act->mMoveDirection == 1) {
			if (anim == 0x4B) {
				if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
					act->mOwner->SetAnim(0x40, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
					act->mTargetState = 2;
				}
			} else if (anim != 0x40) {
				act->mOwner->SetAnim(0x4B, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				act->mTargetState = 0;
			}
		} else if (act->mMoveDirection == -1) {
			if (anim == 0x4C) {
				if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
					act->mOwner->SetAnim(0x41, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
					act->mTargetState = 2;
				}
			} else if (anim != 0x41) {
				act->mOwner->SetAnim(0x4C, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				act->mTargetState = 0;
			}
		} else if (anim != 0x3F) {
			act->mOwner->SetAnim(0x3F, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			act->mTargetState = 0;
		}
		break;
	case 3:
		if (anim != 0x42) {
			act->mOwner->SetAnim(0x42, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
		} else if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
			XObjChr* chr = act->mOwner;
			chr->SetAnim(0x11, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
			chr->mIdleLoops = 3;
			act->mState     = 0;
		}
		break;
	}
}

// lets go of what he's pushing once a is let go, japan uses b while the game flag is up
/**
 * @note Address: 0x800C56D4
 * @note Size: 0x248
 */
void XActPush::HandleInput(const CPadOne& pad, f32& moveWeight)
{
	if (mState == 2) {
		if (gGame.IsRunning() && gGame.mEvt.mData.mScript == NULL) {
#if defined(VERSION_JP)
			if (gGame.mStatus.mFlags & 0x0000080000000000ULL) {
				if (pad.mTrigger & PAD_BUTTON_B) {
					ReleasePushAction(this);
					mState     = 3;
					moveWeight = 0.0f;
				}
			} else
#endif
			    if (!(pad.mButton & PAD_BUTTON_A)) {
				ReleasePushAction(this);
				mState     = 3;
				moveWeight = 0.0f;
			}
		}
	} else if (mState != 0) {
		if (mState != 3 && !(pad.mButton & PAD_BUTTON_A)
#if defined(VERSION_JP)
		    && !(gGame.mStatus.mFlags & 0x0000080000000000ULL)
#endif
		) {
			ReleasePushAction(this);
			XObjChr* chr = mOwner;
			chr->SetAnim(17, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			chr->mIdleLoops = 3;
			mState          = 0;
		}
		moveWeight = 0.0f;
	}
}

/**
 * @note Address: 0x800C591C
 * @note Size: 0xE4
 */
// NONMATCHING: retail inlines ReleasePushAction, including the target flag clear and virtual Reset; this build emits a call.
void CancelPlayerPushAction(XActPush* act)
{
	if (act->mState != 0) {
		ReleasePushAction(act);
		act->mState  = 0;
		XObjChr* chr = act->mOwner;
		chr->SetAnim(0x11, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
		chr->mIdleLoops = 3;
	}
}

/**
 * @note Address: 0x800C5A00
 * @note Size: 0x7C
 */
void ReleasePushAction(XActPush* act)
{
	XObj* target = act->mTarget;
	if (target != NULL && (target->mStatus.mFlags & 0x0800000000000000ULL)) {
		target->mStatus.mFlags &= ~0x0800000000000000ULL;
	}
	act->Reset();
}

} // extern "C"
