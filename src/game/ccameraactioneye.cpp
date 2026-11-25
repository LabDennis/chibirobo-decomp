#include "game/ccameraactioneye.h"

#include "game/gmath.h"
#include "Version.h"
#include "game/csound.h"
#include "game/cscrn.h"
#include "game/cgame.h"
#include "game/xchr.h"

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
	const f32 restPitch = 0.05235988f;
	mLook.Set(0.0f, 0.0f, 0.0f);
	mLook.SetTarget(0.0f, 0.0f, 0.0f);
	mOverride.Set(0.0f, 0.0f, 0.0f);
	mLook.mSpeed.Set(0.05f, 0.05f, 0.05f);

	Vec angle = { restPitch, gGame.GetPlayer()->mMoveRotate.y, 0.0f };
	SetAngle(angle);
	SetDistance(100.0f);
	SetFov(40.0f);

	mLook.Set(restPitch, 0.0f, 0.0f);
	mLook.SetTarget(restPitch, 0.0f, 0.0f);
}

static bool PlayerCanLook(const XObjChr& player)
{
	s32 liftState = player.mLift.mState;
	return (liftState < 7 || liftState > 18) && player.mHang.mState == 0 && player.mJump.mState == 0 && player.mLadder.mState == 0
	    && player.mRope.mState == 0 && player.mPush.mState == 0 && player.mTumble.mState == 0;
}

static inline void ClampStick(f32& value)
{
	if (value > 0.8f) {
		value = 0.8f;
	} else if (value < -0.8f) {
		value = -0.8f;
	}
}

static void PlayEyeCameraTurnSound()
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
				gGame.mStatus.mFlags ^= 0x200; // toggle the free-look mode
				if (gGame.mStatus.mFlags & 0x200) {
					StartScreenWipeColorTransition(&gGame.mRoboeyes, 0x80, 0x80, 0xFF, 0xFF, 0x80, 0x80, 0xFF, 320.0f, 224.0f, 430.0f,
					                               400.0f, 430.0f, 400.0f, 0.0f, 0xFF, 0);
				} else {
					StartScreenWipeColorTransition(&gGame.mRoboeyes, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 320.0f, 224.0f, 430.0f,
					                               400.0f, 430.0f, 400.0f, 0.0f, 0xFF, 0);
				}
			}

			f32 zoom;
			if (gGame.mStatus.mFlags & 0x200) { // free-look: the C-stick aims
				f32 x = pad.mPad->mSubStickX;
				ClampStick(x);
				f32 y = pad.mPad->mSubStickY;
				ClampStick(y);
				if (gGame.mEvt.mData.mScript != NULL || !PlayerCanLook(*gGame.GetPlayer())) {
					x = 0.0f;
					y = 0.0f;
				}
				mLook.mTarget.y = -HALF_PI * x;
				mLook.mTarget.x = -0.87266463f * y;
			} else {
				f32 turn = pad.mPad->mStickX;
				ClampStick(turn);
				f32 tilt = pad.mPad->mStickY;
				ClampStick(tilt);
				if (gGame.mEvt.mData.mScript != NULL || !PlayerCanLook(*gGame.GetPlayer())) {
					turn = 0.0f;
					tilt = 0.0f;
				}
				f32 pitch       = 0.0f;
				mLook.mTarget.y = 0.0f;
				if (tilt > 0.0f) {
					pitch = mLook.x + 0.06981317f * Abs(tilt); // up to 4 degrees per update, scaled by the stick
					if (pitch > 0.87266463f) {
						pitch = 0.87266463f; // 50 degrees up
					}
				} else if (tilt < 0.0f) {
					pitch = mLook.x - 0.06981317f * Abs(tilt);
					if (pitch < -0.87266463f) {
						pitch = -0.87266463f; // 50 degrees down
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
				gGame.GetPlayer()->SetRotationY(gGame.GetPlayer()->mMoveRotate.y - 0.034906585f * turn * step, 0); // 2 degrees per step

				zoom = pad.mPad->mSubStickY;
				if (gGame.mEvt.mData.mScript != NULL || !PlayerCanLook(*gGame.GetPlayer())) {
					zoom = 0.0f;
				}
				if (mOverride.z == 0.0f) {
					mLook.mTarget.z = -30.0f * zoom; // the C-stick zooms the fov
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
			if (57.295776f * speedY > 0.6f || 57.295776f * speedX > 0.6f) { // still turning fast: keep the sound on
				PlayEyeCameraTurnSound();
			}
			mLook.Step(step);

			Vec look = { mLook.x, mLook.y, 0.0f };
			SetAngle(gGame.GetPlayer()->mMoveRotate + look);
			SetFov(40.0f + mLook.z);

			static f32 sPreviousZoomSpeed = 0.0f;
			f32 stick                     = pad.mPad->mSubStickY;
			// Zoom changes kick the blur up so it can decay back to zero.
			if (!(Abs(speedZ - sPreviousZoomSpeed) < 0.02f)) {
				gGame.mBlur.mAlpha = 0.8f;
			}
			gGame.mBlur.mTarget  = 0.0f;
			gGame.mBlur.mSpeed   = 0.1f;
			gGame.mBlur.mOffsetY = gGame.mBlur.mOffsetX = 2.0f * -stick;
			sPreviousZoomSpeed                          = speedZ;
		}
	}
	gGame.mRoboeyes.mRotation = mAngle.y;
}
