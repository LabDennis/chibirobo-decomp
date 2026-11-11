#include "game/ccameraactioneye.h"
#include "game/eyecameraparameters.h"

extern "C" {
extern const float sEyeCameraWipeCenterX             = 320.0f;
extern const float sEyeCameraWipeCenterY             = 224.0f;
extern const float sEyeCameraWipeWidth               = 430.0f;
extern const float sEyeCameraWipeHeight              = 400.0f;
extern const float sEyeCameraZero                    = 0.0f;
extern const float sEyeCameraStickMax                = 0.8f;
extern const float sEyeCameraStickMin                = -0.8f;
extern const float sEyeCameraYawScale                = -1.5707964f;
extern const float sEyeCameraPitchScale              = -0.87266463f;
extern const float sEyeCameraPitchStep               = 0.06981317f;
extern const float sEyeCameraPitchMax                = 0.87266463f;
extern const float sEyeCameraTurnStep                = 0.034906585f;
extern const float sEyeCameraZoomScale               = -30.0f;
extern const float sEyeCameraLookSpeedDivisor        = 30.0f;
extern const float sEyeCameraZoomSpeedDivisor        = 20.0f;
extern const float sEyeCameraRadiansToDegrees        = 57.295776f;
extern const float sEyeCameraTurnSoundThreshold      = 0.6f;
extern const float sEyeCameraBaseFov                 = 40.0f;
extern const float sEyeCameraBlurSpeedThreshold      = 0.02f;
extern const float sEyeCameraBlurFadeSpeed           = 0.1f;
extern const float sEyeCameraBlurOffsetScale         = 2.0f;
extern const double sEyeCameraUnsignedConversionBias = 4503599627370496.0;
extern const float sEyeCameraInitialLookSpeed        = 0.05f;
extern const float sEyeCameraInitialDistance         = 100.0f;
extern const float sEyeCameraInitialPitch            = 0.05235988f;
}
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
void CCameraActionEye::vf14()
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
	if (EYE_PLAYER->mLift.mState == 7 || EYE_PLAYER->mLift.mState == 8 || EYE_PLAYER->mLift.mState == 9 || EYE_PLAYER->mLift.mState == 10
	    || EYE_PLAYER->mLift.mState == 11 || EYE_PLAYER->mLift.mState == 12 || EYE_PLAYER->mLift.mState == 13
	    || EYE_PLAYER->mLift.mState == 14 || EYE_PLAYER->mLift.mState == 15 || EYE_PLAYER->mLift.mState == 16
	    || EYE_PLAYER->mLift.mState == 17 || EYE_PLAYER->mLift.mState == 18) {
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
	} else if (value < sEyeCameraStickMin) {
		value = sEyeCameraStickMin;
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
					StartScreenWipeColorTransition(&gGame.mRoboeyes, 0x80, 0x80, 0xFF, 0xFF, 0x80, 0x80, 0xFF, sEyeCameraWipeCenterX,
					                               sEyeCameraWipeCenterY, sEyeCameraWipeWidth, 400.0f, sEyeCameraWipeWidth, 400.0f, 0.0f,
					                               0xFF, 0);
				} else {
					StartScreenWipeColorTransition(&gGame.mRoboeyes, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, sEyeCameraWipeCenterX,
					                               sEyeCameraWipeCenterY, sEyeCameraWipeWidth, 400.0f, sEyeCameraWipeWidth, 400.0f, 0.0f,
					                               0xFF, 0);
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
				mLook.mTarget.y = sEyeCameraYawScale * x;
				mLook.mTarget.x = sEyeCameraPitchScale * y;
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
					pitch = mLook.x + sEyeCameraPitchStep * Abs(tilt);
					if (pitch > sEyeCameraPitchMax) {
						pitch = sEyeCameraPitchMax;
					}
				} else if (tilt < 0.0f) {
					pitch = mLook.x - sEyeCameraPitchStep * Abs(tilt);
					if (pitch < sEyeCameraPitchScale) {
						pitch = sEyeCameraPitchScale;
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
				gGame.GetPlayer()->SetRotationY(EYE_PLAYER->mMoveRotate.y - sEyeCameraTurnStep * turn * step, 0);

				zoom = pad.mPad->mSubStickY;
				if (gGame.mEvt.mData.mScript != NULL || !PlayerCanLook()) {
					zoom = 0.0f;
				}
				if (mOverride.z == 0.0f) {
					mLook.mTarget.z = sEyeCameraZoomScale * zoom;
				} else {
					mLook.mTarget.z = mOverride.z;
				}
			}

			f32 speedY     = Abs(mLook.mTarget.y - mLook.y) / 30.0f;
			f32 speedX     = Abs(mLook.mTarget.x - mLook.x) / 30.0f;
			f32 speedZ     = Abs(mLook.mTarget.z - mLook.z) / sEyeCameraZoomSpeedDivisor;
			mLook.mSpeed.x = speedX;
			mLook.mSpeed.y = speedY;
			mLook.mSpeed.z = speedZ;
			if (sEyeCameraRadiansToDegrees * speedY > sEyeCameraTurnSoundThreshold
			    || sEyeCameraRadiansToDegrees * speedX > sEyeCameraTurnSoundThreshold) {
				PlayEyeCameraTurnSound();
			}
			mLook.Step(step);

			Vec look = { mLook.x, mLook.y, 0.0f };
			SetAngle(EYE_PLAYER->mMoveRotate + look);
			SetFov(sEyeCameraBaseFov + mLook.z);

			static f32 sPreviousZoomSpeed = 0.0f;
			f32 stick                     = pad.mPad->mSubStickY;
			if (Abs(speedZ - sPreviousZoomSpeed) < sEyeCameraBlurSpeedThreshold) {
				gGame.mBlur.mTarget  = 0.0f;
				gGame.mBlur.mSpeed   = sEyeCameraBlurFadeSpeed;
				gGame.mBlur.mOffsetY = gGame.mBlur.mOffsetX = sEyeCameraBlurOffsetScale * -stick;
			} else {
				gGame.mBlur.mAlpha   = 0.8f;
				gGame.mBlur.mTarget  = 0.0f;
				gGame.mBlur.mSpeed   = sEyeCameraBlurFadeSpeed;
				gGame.mBlur.mOffsetY = gGame.mBlur.mOffsetX = sEyeCameraBlurOffsetScale * -stick;
			}
			sPreviousZoomSpeed = speedZ;
		}
	}
	gGame.mRoboeyes._5C = mAngle.y;
}
