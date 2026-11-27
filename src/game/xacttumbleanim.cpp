#include "game/xchr.h"
#include "game/cvec.h"
#include "game/cgame.h"
#include "game/stagedata.h"
#include <stdlib.h>

extern "C" {

// the ring on the floor while he tumbles
/**
 * @note Address: 0x800C36E4
 * @note Size: 0x2F0
 */
void DrawTumbleRing(XActTumbleFade* fade, const Vec& pos, f32 angle)
{
	if (fade->mAmount > 0.0f) {
		CVec offset(fade->mOffset);
		Mtx m;
		PSMTXRotRad(m, 'y', angle);
		PSMTXMultVec(m, &offset, &offset);
		Vec center = { pos.x + offset.x, 0.01f + pos.y, pos.z + offset.z };
		f32 size   = 6.0f * fade->mAmount;
		u32 color  = (u32)(160.0f * fade->mAmount) | 0x641E00;
		DrawGsCircle(&gGame.mGs, 0x18, color, color, center.x, center.y, center.z, size, size, -HALF_PI, 0.0f, 0.0f);
		PrimVertex verts[50];
		f32 outerRadius = size * 0.5f + 1.0f;
		for (s32 i = 0; i <= 24; ++i) {
			f32 ringAngle     = (TWO_PI / 24) * i;
			f32 sine          = (f32)sin(ringAngle);
			f32 cosine        = (f32)cos(ringAngle);
			PrimVertex& inner = verts[i * 2];
			PrimVertex& outer = verts[i * 2 + 1];
			inner.pos.x       = center.x - size * sine * 0.5f;
			inner.pos.z       = center.z + size * cosine * 0.5f;
			inner.pos.y       = center.y;
			inner.color       = color;
			outer.pos.x       = center.x - outerRadius * sine;
			outer.pos.z       = center.z + outerRadius * cosine;
			outer.pos.y       = center.y;
			outer.color       = 0;
		}
		DrawGsTriangleVertices(&gGame.mGs, GX_TRIANGLESTRIP, gGame.mGs.mCamera->mViewMtx, verts, 50);
	}
}

/**
 * @note Address: 0x800C39D4
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

static inline void PlayTumbleAnimation(XObjChr* owner, s32 anim, f32 blend)
{
	owner->SetAnim(anim, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, blend, 10.0f);
}

static inline void FinishTumble(XActTumble* act, f32 blend)
{
	XObjChr* owner = act->mOwner;
	PlayTumbleAnimation(owner, 0x11, blend);
	owner->mIdleLoops = 3;
	act->mState       = 0;
}

static inline void StartTumble(XActTumble* act, s32 anim, HSD_AObj* aobj, s32 next)
{
	if (anim != 0x35 && anim != 0x10D) {
		PlayTumbleAnimation(act->mOwner, 0x35, 0.0f);
		gGame.mParticle.Create(0x10006, 0, act->mOwner->mJObj, FALSE);
	} else if (aobj->flags & 0x44000000) {
		PlayTumbleAnimation(act->mOwner, 0x36, 1.0f);
		act->mState = next;
	}
}

static inline void RecoverTumble(XActTumble* act, s32 anim, HSD_AObj* aobj, s32 recovery, f32 blend)
{
	if (anim != recovery)
		PlayTumbleAnimation(act->mOwner, recovery, blend);
	else if (aobj->flags & 0x44000000)
		FinishTumble(act, 1.0f);
}

/**
 * @note Address: 0x800C3A80
 * @note Size: 0x9E8
 */
void UpdatePlayerTumbleAnimation(XActTumble* act, s32 anim, HSD_AObj* aobj, u32, f32, f32)
{
	// 1 and 15 knock him over; 2 and 16 keep him down until a button or the idle timer, then 3 and
	// 17 get him up. 4 is the empty battery, 10 comes after five turns on the spot, 12 turns him to
	// face the camera and 13 back, 14 holds the pose while the idle timer runs
	switch (act->mState) {
	case 1:
		StartTumble(act, anim, aobj, 2);
		break;
	case 3:
		RecoverTumble(act, anim, aobj, 0x37, 1.0f);
		break;
	case 4:
		if (anim != 0x43)
			PlayTumbleAnimation(act->mOwner, 0x43, 0.0f);
		else if (aobj->flags & 0x44000000) {
			// battery empty: the stage's script takes over, otherwise he lies there until a button (5)
			if ((gGame.mStatus.mFlags & 0x40000000) && GetStageBatteryDepletedScript() && gGame.GetPlayer()->mParams.mBattery <= 0.0f) {
				if (gGame.mEvt.mData.mScript == NULL)
					StartOrQueueEventScript(&gGame.mEvt, GetStageBatteryDepletedScript(), 0, -1, -1);
			} else {
				act->mState = 5;
			}
		}
		break;
	case 6:
		RecoverTumble(act, anim, aobj, 0x1E, 1.0f);
		break;
	case 7:
		RecoverTumble(act, anim, aobj, 0xB3, 0.0f);
		break;
	case 8:
		PlayTumbleAnimation(act->mOwner, 0xD4, 0.0f);
		act->mState = 9;
		break;
	case 9:
		if (aobj->flags & 0x44000000)
			FinishTumble(act, 1.0f);
		break;
	case 10: {
		PlayTumbleAnimation(act->mOwner, 0xE6, 0.0f);
		Vec offset = { 0.0f, 12.0f, 0.0f }, rotation = { 0.0f, 0.0f, 0.0f };
		gGame.mParticle.CreateSRT(0x10005, 1, act->mOwner->mMoveTranslate + offset, act->mOwner->mMoveScale, rotation, FALSE);
		act->mState = 11;
		break;
	}
	case 11:
		if (aobj->flags & 0x44000000)
			FinishTumble(act, 0.0f);
		break;
	case 12:
		act->mSavedYaw = act->mOwner->mMoveRotate.y;
		act->mOwner->SetRotateY(PI + gGame.mMotionControlCamera.mCamera.mAngle.y, 12.0f * DEG2RAD);
		PlayTumbleAnimation(act->mOwner, 0x5D, 0.0f);
		act->mState = 14;
		break;
	case 13:
		act->mOwner->SetRotateY(act->mSavedYaw, 12.0f * DEG2RAD);
		PlayTumbleAnimation(act->mOwner, 0x5F, 0.0f);
		act->mState = 14;
		break;
	case 14:
		if (act->mOwner->mIdleModeTimer > 0) {
			if (!act->mIdleParticle && (act->mOwner->mMoveTranslate - (Vec)act->mOwner->mLastPos).Length() == 0.0f) {
				s32 effect = 0x28;
				if (rand() & 1)
					effect = 0x26;
				s32 height = act->mOwner->mLift.mState == 0 ? 2 : 4;
				s32 path[] = { 0, 1, 0, 1, 2, 0, 1 };
				CVec pos   = GetEventJointWorldPosition(act->mOwner, 7, path);
				Vec offset = { 0.0f, 0.0f, 0.0f }, rotation = { 0.0f, 0.0f, 0.0f };
				offset.y           = height;
				act->mIdleParticle = gGame.mParticle.CreateSRT(effect | 0x10000, 1, pos + offset, act->mOwner->mMoveScale, rotation, FALSE);
			}
		} else {
			act->mIdleParticle = NULL;
			act->mState        = 0;
		}
		break;
	case 15:
		StartTumble(act, anim, aobj, 16);
		break;
	case 16:
		if (act->mOwner->mIdleModeTimer <= 0)
			act->mState = 17;
		break;
	case 17:
		RecoverTumble(act, anim, aobj, 0x37, 1.0f);
		break;
	}
}

/**
 * @note Address: 0x800C4468
 * @note Size: 0x4A4
 */
void XActTumble::HandleInput(const CPadOne& pad, f32& moveWeight)
{
	if (mState == 2) {
		if (gGame.IsRunning() && gGame.mEvt.mData.mScript == NULL) {
			if ((pad.mTrigger & PAD_BUTTON_A) || (pad.mTrigger & PAD_BUTTON_B)
			    || sqrtf(pad.mStickX * pad.mStickX + pad.mStickY * pad.mStickY) > 0.0f) {
				mState = 3;
			}
		}
		moveWeight = 0.0f;
	} else if (mState == 16) {
		if (gGame.IsRunning() && gGame.mEvt.mData.mScript == NULL) {
			if ((pad.mTrigger & PAD_BUTTON_A) || (pad.mTrigger & PAD_BUTTON_B)
			    || sqrtf(pad.mStickX * pad.mStickX + pad.mStickY * pad.mStickY) > 0.0f) {
				mState = 17;
			}
		}
		moveWeight = 0.0f;
	} else if (mState == 5) {
		// Recovering from an empty battery takes a button press.
		if (gGame.IsRunning() && gGame.mEvt.mData.mScript == NULL && ((pad.mTrigger & PAD_BUTTON_A) || (pad.mTrigger & PAD_BUTTON_B))) {
			mState = 6;
		}
		moveWeight = 0.0f;
	} else if (mState == 7) {
		moveWeight = 0.8f;
	} else if (mState != 0) {
		moveWeight = 0.0f;
	}
}

} // extern "C"
