#include "game/ccameraactioneye.h"
#include "game/eyecameraparameters.h"

#include "game/gmath.h"
#include "Version.h"
#include "game/csound.h"
#include "game/cscrn.h"
#include "game/cgame.h"
#include "game/xchr.h"

#define EYE_PLAYER gGame.GetPlayer()

u32 CCameraActionEye::GetCameraFlags()
{
	return 0x20019;
}

void CCameraActionEye::Init()
{
}

/**
 * @note Address: 0x80147358
 * @note Size: 0x108
 */
void CCameraActionEye::Reset()
{
	mLook.x         = 0.0f;
	mLook.y         = 0.0f;
	mLook.z         = 0.0f;
	mLook.mTarget.x = 0.0f;
	mLook.mTarget.y = 0.0f;
	mLook.mTarget.z = 0.0f;
	mLook.mSpeed.x  = 0.0f;
	mLook.mSpeed.y  = 0.0f;
	mLook.mSpeed.z  = 0.0f;
	mOverride.x     = 0.0f;
	mOverride.y     = 0.0f;
	mOverride.z     = 0.0f;
	mLook.mSpeed.x  = 0.05f;
	mLook.mSpeed.y  = 0.05f;
	mLook.mSpeed.z  = 0.05f;

	Vec angle = { 0.05235988f, EYE_PLAYER->mMoveRotate.y, 0.0f };
	SetAngle(angle);
	SetDistance(100.0f);
	SetFov(40.0f);
	mLook.x         = 0.05235988f;
	mLook.y         = 0.0f;
	mLook.z         = 0.0f;
	mLook.mTarget.x = 0.05235988f;
	mLook.mTarget.y = 0.0f;
	mLook.mTarget.z = 0.0f;
}

// todo: cleanup
/**
 * @note Address: N/A
 * @note Size: ?
 */
inline bool PlayerCanLook()
{
	bool result = false;
	bool busy   = false;
	if (!(EYE_PLAYER->mLift.mState != 7 && EYE_PLAYER->mLift.mState != 8 && EYE_PLAYER->mLift.mState != 9 && EYE_PLAYER->mLift.mState != 10
	      && EYE_PLAYER->mLift.mState != 11 && EYE_PLAYER->mLift.mState != 12 && EYE_PLAYER->mLift.mState != 13
	      && EYE_PLAYER->mLift.mState != 14 && EYE_PLAYER->mLift.mState != 15 && EYE_PLAYER->mLift.mState != 16
	      && EYE_PLAYER->mLift.mState != 17 && EYE_PLAYER->mLift.mState != 18)) {
		busy = true;
	}
	if (!busy && EYE_PLAYER->mHang.mState == 0 && EYE_PLAYER->mJump.mState == 0 && EYE_PLAYER->mLadder.mState == 0
	    && EYE_PLAYER->mRope.mState == 0 && EYE_PLAYER->mPush.mState == 0 && EYE_PLAYER->mTumble.mState == 0) {
		result = true;
	}
	return result;
}

static inline void ClampStick(f32& value)
{
	if (value > 0.8f) {
		value = 0.8f;
	} else if (value < -0.8f) {
		value = -0.8f;
	}
}

// todo: cleanup
/**
 * @note Address: N/A
 * @note Size: ?
 */
inline void PlayEyeCameraTurnSound()
{
	if (!IsGameSoundPlaying(&gGame.mSound, 3)) {
		PlayGameSound(&gGame.mSound, 3, 0xFF);
	}
}

/**
 * @note Address: 0x801469EC
 * @note Size: 0x96C
 */
void CCameraActionEye::Update(u32 step)
{
	SetEye(GetPlayerFirstPersonEyePosition(gGame.GetPlayer()));
	CPad pad(0);

	if (!(gGame.mStatus.mFlags & 0xC00)) {
		if (gGame.IsRunning()) {
			if ((pad.mPad->mTrigger & 0x40) && (pad.mPad->mButton & 0x20) && (gGame.mStatus.mFlags & 0x80000000)) {
				gGame.mStatus.mFlags ^= 0x200;
				if (gGame.mStatus.mFlags & 0x200) {
					StartScreenWipeColorTransition(&gGame.mRoboeyes, 0x80, 0x80, 0xFF, 0xFF, 0x80, 0x80, 0xFF, 320.0f, 224.0f, 430.0f,
					                               400.0f, 430.0f, 400.0f, 0.0f, 0xFF, 0);
				} else {
					StartScreenWipeColorTransition(&gGame.mRoboeyes, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 320.0f, 224.0f, 430.0f,
					                               400.0f, 430.0f, 400.0f, 0.0f, 0xFF, 0);
				}
			}

			f32 zoom;
			if (gGame.mStatus.mFlags & 0x200) {
				f32 x = pad.mPad->mSubStickX;
				ClampStick(x);
				f32 y = pad.mPad->mSubStickY;
				ClampStick(y);
				if (gGame.mEvt.mData.mScript != NULL || !PlayerCanLook()) {
					y = 0.0f;
					x = y;
				}
				mLook.mTarget.y = -1.5707964f * x;
				mLook.mTarget.x = -0.87266463f * y;
			} else {
				f32 turn = pad.mPad->mStickX;
				ClampStick(turn);
				f32 tilt = pad.mPad->mStickY;
				ClampStick(tilt);
				if (gGame.mEvt.mData.mScript != NULL || !PlayerCanLook()) {
					tilt = 0.0f;
					turn = tilt;
				}
				f32 pitch       = 0.0f;
				mLook.mTarget.y = 0.0f;
				if (tilt > 0.0f) {
					pitch = mLook.x + 0.06981317f * Abs(tilt);
					if (pitch > 0.87266463f) {
						pitch = 0.87266463f;
					}
				} else if (tilt < 0.0f) {
					pitch = mLook.x - 0.06981317f * Abs(tilt);
					if (pitch < -0.87266463f) {
						pitch = -0.87266463f;
					}
				}
				if (mOverride.y != 0.0f) {
					mLook.mTarget.y = mOverride.y;
				}
				if (mOverride.x == 0.0f) {
					if (pitch != 0.0f) {
						mLook.mTarget.x = pitch;
						mLook.x         = pitch;
						PlayEyeCameraTurnSound();
					}
				} else {
					mLook.mTarget.x = mOverride.x;
				}
				gGame.GetPlayer()->SetRotationY(EYE_PLAYER->mMoveRotate.y - 0.034906585f * turn * step, 0);

				zoom = pad.mPad->mSubStickY;
				if (gGame.mEvt.mData.mScript != NULL || !PlayerCanLook()) {
					zoom = 0.0f;
				}
				if (mOverride.z == 0.0f) {
					mLook.mTarget.z = -30.0f * zoom;
				} else {
					mLook.mTarget.z = mOverride.z;
				}
			}

			f32 speedY     = Abs(mLook.mTarget.y - mLook.y) / 30.0f;
			f32 speedX     = Abs(mLook.mTarget.x - mLook.x) / 30.0f;
			f32 speedZ     = Abs(mLook.mTarget.z - mLook.z) / 20.0f;
			mLook.mSpeed.x = speedX;
			mLook.mSpeed.y = speedY;
			mLook.mSpeed.z = speedZ;
			if (57.295776f * speedY > 0.6f || 57.295776f * speedX > 0.6f) {
				PlayEyeCameraTurnSound();
			}
			mLook.Step(step);

			Vec look = { mLook.x, mLook.y, 0.0f };
			SetAngle(EYE_PLAYER->mMoveRotate + look);
			SetFov(40.0f + mLook.z);

			static f32 sPreviousZoomSpeed = 0.0f;
			f32 stick                     = pad.mPad->mSubStickY;
			if (Abs(speedZ - sPreviousZoomSpeed) < 0.02f) {
				gGame.mBlur.mTarget  = 0.0f;
				gGame.mBlur.mSpeed   = 0.1f;
				gGame.mBlur.mOffsetY = gGame.mBlur.mOffsetX = 2.0f * -stick;
			} else {
				gGame.mBlur.mAlpha   = 0.8f;
				gGame.mBlur.mTarget  = 0.0f;
				gGame.mBlur.mSpeed   = 0.1f;
				gGame.mBlur.mOffsetY = gGame.mBlur.mOffsetX = 2.0f * -stick;
			}
			sPreviousZoomSpeed = speedZ;
		}
	}
	gGame.mRoboeyes.mRotation = mAngle.y;
}
