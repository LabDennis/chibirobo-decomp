// the Wii first person view: the remote's pointer aims the look
#include "game/ccameraactioneye.h"

#include "nw4r/math/math_arithmetic.h"
#include "game/gmath.h"
#include "game/cscrn.h"
#include "game/wii/cgame.h"
#include "game/wii/cpad.h"
#include "game/wii/cpadact.h"
#include "game/wii/csound.h"

#define EYE_PLAYER gGame.GetPlayer()

// the pointer position runs through this on its way to the look, per axis of
// the clamped screen point; shared by Update, TrackPointer and the debug draw
static const f32 sEyePointerLimits[4] = { 0.9f, 0.9f, 0.3f, 0.2f };

// the look angle Reset starts from, word-copied into the SetAngle argument
// the speed limits the look target's merge divides by; the free look swaps to
// the wide one whenever mOverride has a part set
static CVec sEyeLimitA(20.0f, 20.0f, 20.0f);
static CVec sEyeLimitB(30.0f, 30.0f, 20.0f);

// the pitch the last Update merged, Reset reseeds it
static f32 sPrevPitch;

extern "C" {
// the empty debug-draw hook DrawDebug2d feeds, in cmotioncontrolcamera.cpp's range
void fn_8016AA74(const f32* params);
// clamps a value into [min, max], in cmotioncontrolcamera.cpp
f32 fn_8016A81C(f32 value, f32 min, f32 max);
// clamps the screen point into the per-axis limits, in cmotioncontrolcamera.cpp
Vec fn_8016AA78(Vec in, f32* limits);
}

// the first person eye of the player object, xobj_chr.cpp
extern "C" Vec GetPlayerFirstPersonEyePosition(WiiPlayerState* player);

// the pointer's screen resolution, cpadact.cpp
extern s32 gPointerScreenWidth, gPointerScreenHeight;

// the curve evaluator's return, two floats back in r3/r4
struct WiiCameraPoint {
	f32 x;
	f32 y;
};

extern "C" WiiCameraPoint fn_8016A844(const EyeCurve* curve, f32 t);

// whether the player's action states leave the look controls free: the lift
// action's busy states and the idle states of the rest, the GameCube's checks
// with the Wii action offsets
inline bool PlayerCanLook()
{
	WiiPlayerState* player = EYE_PLAYER;
	bool result            = false;
	bool busy              = false;
	if (!(player->mLift.mState != 7 && player->mLift.mState != 8 && player->mLift.mState != 9 && player->mLift.mState != 11
	      && player->mLift.mState != 12 && player->mLift.mState != 13 && player->mLift.mState != 14 && player->mLift.mState != 15
	      && player->mLift.mState != 16 && player->mLift.mState != 17 && player->mLift.mState != 18 && player->mLift.mState != 19)) {
		busy = true;
	}
	if (!busy && player->mHang.mState == 0 && player->mJump.mState == 0 && player->mLadder.mState == 0 && player->mRope.mState == 0
	    && player->mPush.mState == 0 && player->mTumble.mState == 0) {
		result = true;
	}
	return result;
}

/**
 * @note Address: 0x80164BFC
 * @note Size: 0x5C
 */
void CCameraActionEye::Init()
{
	mCurveX.mP0X = 0.0f;
	mCurveX.mP0Y = 0.0f;
	mCurveX.mT0X = 0.0f;
	mCurveX.mT0Y = 0.0f;
	mCurveX.mP1X = 1.0f;
	mCurveX.mP1Y = 1.0f;
	mCurveX.mT1X = 0.0f;
	mCurveX.mT1Y = 3.0f;
	mCurveY.mP0X = 0.0f;
	mCurveY.mP0Y = 0.0f;
	mCurveY.mT0X = 0.0f;
	mCurveY.mT0Y = 0.0f;
	mCurveY.mP1X = 1.0f;
	mCurveY.mP1Y = 1.0f;
	mCurveY.mT1X = 0.0f;
	mCurveY.mT1Y = 1.5f;
	mFirstFrame  = 0;
}

/**
 * @note Address: 0x80164C58
 * @note Size: 0x1A8
 */
void CCameraActionEye::Reset()
{
	mLook.mValue.x   = 0.0f;
	mLook.mValue.y   = 0.0f;
	mLook.mValue.z   = 0.0f;
	mLook.mTarget.x  = 0.0f;
	mLook.mTarget.y  = 0.0f;
	mLook.mTarget.z  = 0.0f;
	mOverride.x      = 0.0f;
	mOverride.y      = 0.0f;
	mOverride.z      = 0.0f;
	mLook.mSpeed.x   = 0.05f;
	mLook.mSpeed.y   = 0.05f;
	mLook.mSpeed.z   = 0.05f;
	mVec80.mValue.x  = 0.0f;
	mVec80.mValue.y  = 0.0f;
	mVec80.mValue.z  = 0.0f;
	mVec80.mTarget.x = 0.0f;
	mVec80.mTarget.y = 0.0f;
	mVec80.mTarget.z = 0.0f;
	mVec80.mSpeed.x  = 0.0f;
	mVec80.mSpeed.y  = 0.0f;
	mVec80.mSpeed.z  = 0.0f;

	Vec angle = { 0.05235988f, 0.0f, 0.0f };
	angle.y   = EYE_PLAYER->mMoveRotate.y;
	SetAngle(angle);
	SetDistance(100.0f);
	SetFov(40.0f);

	mLook.Reset(0.05235988f, EYE_PLAYER->mMoveRotate.y, 0.0f);
	sPrevPitch  = 0.05235988f;
	mB0         = 0.0f;
	Vec zero    = { 0.0f, 0.0f, 0.0f };
	mVecB4      = zero;
	mVecC0      = sEyeLimitA;
	mVec10C.x   = 0.0f;
	mVec10C.y   = 0.0f;
	mVec10C.z   = 0.0f;
	mRange      = 24.0f;
	mFirstFrame = 1;
}

/**
 * @note Address: 0x80164E00
 * @note Size: 0x118C
 */
// NONMATCHING: the direct call targets match; retail uses a 0x160 frame and saves r21, ours 0x130 and r27.
// PlayerCanLook still loses retail's materialized bool temporaries; BOOL types and direct busy initialization
// were flat or worse, and a plain bool `busy = state == 7 || ...` chain scores 67.6. Spelling each flag out
// as a named bool reaches 88 but copies compiler temporaries. Control flow, constants and pad words are checked.
void CCameraActionEye::Update(u32 step)
{
	CPad pad(0);
	SetEye(GetPlayerFirstPersonEyePosition(EYE_PLAYER));

	if (!gGame.GetStatusFlags(0xC00)) {
		if (gGame.IsRunning()) {
			if ((pad.mPad->mTrigger & 0x0040000000008000ULL) && (pad.mPad->mButton & 0x0020000000010000ULL)
			    && gGame.GetStatusFlags(0x80000000)) {
				gGame.ToggleStatusFlags(0x200); // the free-look mode
				if (gGame.GetStatusFlags(0x200)) {
					gGame.mRoboeyes.StartTransition(0x80, 0x80, 0xFF, 0xFF, 0x80, 0x80, 0xFF, gPointerScreenWidth / 2,
					                                gPointerScreenHeight / 2, 430.0f, 400.0f, 430.0f, 400.0f, 0.0f, 0xFF, FALSE);
					mLook.mValue.y  = 0.0f;
					mLook.mValue.x  = 0.05235988f;
					mLook.mValue.z  = 0.0f;
					mLook.mTarget.x = 0.05235988f;
					mLook.mTarget.y = 0.0f;
					mLook.mTarget.z = 0.0f;
				} else {
					gGame.mRoboeyes.StartTransition(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, gPointerScreenWidth / 2,
					                                gPointerScreenHeight / 2, 430.0f, 400.0f, 430.0f, 400.0f, 0.0f, 0xFF, FALSE);
				}
			}

			if (gGame.GetStatusFlags(0x200)) { // free-look: the nunchuk stick aims
				f32 x = pad.mPad->mSubStick.x;
				if (x > 0.8f) {
					x = 0.8f;
				} else if (x < -0.8f) {
					x = -0.8f;
				}
				f32 y = pad.mPad->mSubStick.y;
				if (y > 0.8f) {
					y = 0.8f;
				} else if (y < -0.8f) {
					y = -0.8f;
				}
				if (gGame.mEvt.mData.mScript != NULL || !PlayerCanLook()) {
					y = 0.0f;
					x = y;
				}
				mLook.mTarget.y = -HALF_PI * x;
				mLook.mTarget.x = -0.7853982f * y;
			} else {
				// the pointer picks the aim point
				CVec aim(0.0f, 0.0f, 0.0f);
				if (gWiiPointer.mPointerValid) {
					Vec in     = { 2.0f * (gWiiPointer.mPointer.x / gPointerScreenWidth) - 1.0f,
						           2.0f * (gWiiPointer.mPointer.y / gPointerScreenHeight) - 1.0f, gWiiPointer.mPointer.z };
					Vec out    = fn_8016AA78(in, (f32*)sEyePointerLimits);
					Vec scaled = out;
					aim.Set(scaled);
					mB0              = 0.0f;
					mVecB4           = scaled;
					mVec80.mSpeed.x  = 0.02f;
					mVec80.mSpeed.y  = 0.02f;
					mVec80.mSpeed.z  = 0.02f;
					mVec80.mTarget.x = 1.0f;
					mVec80.mTarget.y = 1.0f;
					mVec80.mTarget.z = 1.0f;
				} else {
					if (!(gGame._41B0 & 8)) {
						aim.Set(mVecB4);
					}
					mVec80.mSpeed.x  = 1.0f;
					mVec80.mSpeed.y  = 1.0f;
					mVec80.mSpeed.z  = 1.0f;
					mVec80.mTarget.x = 0.0f;
					mVec80.mTarget.y = 0.0f;
					mVec80.mTarget.z = 0.0f;
				}
				mVec80.Step(step);

				f32 aimX = aim.x;
				f32 aimY = aim.y;
				if (gGame.mEvt.mData.mScript != NULL || !PlayerCanLook()
				    || !(EYE_PLAYER->mLift.mState != 1 && EYE_PLAYER->mLift.mState != 2 && EYE_PLAYER->mLift.mState != 3)) {
					aimX = 0.0f;
					aimY = aimX;
				}

				f32 track         = TrackPointer(step, -aimX, aimY);
				f32 pitchT        = track * -1.0f;
				WiiCameraPoint pt = fn_8016A844(&mCurveX, nw4r::math::FAbs(aimY));
				f32 pitch         = aimY;
				if (aimY > 0.0f) {
					pitch = 0.8f * pt.y;
				}
				if (pitch < 0.0f) {
					pitch = 0.8f * -pt.y;
				}

				f32 targetY;
				if (aimY == 0.0f && pitch == 0.0f) {
					targetY = (1.0f - 0.3f) * EYE_PLAYER->mMoveRotate.y + 0.3f * mLook.mValue.y;
				} else {
					targetY = EYE_PLAYER->mMoveRotate.y - 0.034906585f * pitch * step;
				}
				if (mOverride.y == 0.0f) {
					mLook.mTarget.y = targetY;
				} else {
					mLook.mTarget.y = mOverride.y + EYE_PLAYER->mMoveRotate.y;
					mVecC0          = sEyeLimitB;
				}
				if (mOverride.x == 0.0f) {
					mLook.mTarget.x = pitchT;
					sPrevPitch      = pitchT;
				} else {
					mLook.mTarget.x = mOverride.x;
					mVecC0          = sEyeLimitB;
				}
				if (mOverride.y == 0.0f) {
					EYE_PLAYER->SetRotationY(mLook.mTarget.y);
				}

				f32 zoom = pad.mPad->mStick.y;
				if (gGame.mEvt.mData.mScript != NULL || !PlayerCanLook()) {
					zoom = 0.0f;
				}
				if (mOverride.z == 0.0f) {
					mLook.mTarget.z = -30.0f * zoom;
				} else {
					mLook.mTarget.z = mOverride.z;
				}
			}

			f32 speedY = nw4r::math::FAbs(mLook.mTarget.y - mLook.mValue.y) / mVecC0.y;
			f32 speedX = nw4r::math::FAbs(mLook.mTarget.x - mLook.mValue.x) / mVecC0.x;
			f32 speedZ = nw4r::math::FAbs(mLook.mTarget.z - mLook.mValue.z) / mVecC0.z;

			// the turn speed may only grow a tenth of a degree per update
			static f32 sPrevSpeedX = speedX;
			f32 diff               = speedX - sPrevSpeedX;
			f32 thresh             = 0.1f * DEG2RAD;
			if (nw4r::math::FAbs(diff) > thresh) {
				speedX = sPrevSpeedX + thresh / nw4r::math::FAbs(diff) * diff;
			}
			sPrevSpeedX = speedX;
			if (gWiiPointer.mPointerValid) {
				speedY *= mVec80.mValue.y;
				speedX *= mVec80.mValue.x;
			}
			mLook.mSpeed.x = speedX;
			mLook.mSpeed.y = speedY;
			mLook.mSpeed.z = speedZ;

			static CVec sPrevTarget;
			if (RAD2DEG * nw4r::math::FAbs(mLook.mTarget.x - sPrevTarget.x) > 0.2f
			    || RAD2DEG * nw4r::math::FAbs(mLook.mTarget.y - sPrevTarget.y) > 0.2f) {
				// still turning fast: keep the sound on
				if (!IsWiiSfxPlaying(&gGame.mSound, 0x70)) {
					PlayWiiSoundEffect(&gGame.mSound, 0x70, 1.0f, 0.0f);
				}
			}
			sPrevTarget.Set(mLook.mTarget);
			mLook.Step(step);

			if (gGame.GetStatusFlags(0x200)) {
				Vec look = { 0.0f, 0.0f, 0.0f };
				look.x   = mLook.mValue.x;
				look.y   = mLook.mValue.y;
				SetAngle(EYE_PLAYER->mMoveRotate + look);
			} else {
				CVec base = EYE_PLAYER->mMoveRotate;
				base.y    = 0.0f;
				Vec look  = { 0.0f, 0.0f, 0.0f };
				look.x    = mLook.mValue.x;
				look.y    = mLook.mValue.y;
				SetAngle(base + look);
			}
			SetFov(40.0f + mLook.mValue.z);

			static f32 sPrevZoomSpeed = 0.0f;
			f32 stick                 = pad.mPad->mStick.y;
			if (nw4r::math::FAbs(speedZ - sPrevZoomSpeed) < 0.02f) { // zoom settled
				gGame.mBlur.Start(-1.0f, 0.0f, 2.0f * -stick, 2.0f * -stick, 0.1f);
			} else { // zooming: kick the blur to 0.8, it decays back to 0
				gGame.mBlur.Start(0.8f, 0.0f, 2.0f * -stick, 2.0f * -stick, 0.1f);
			}
			sPrevZoomSpeed = speedZ;
		}
	}
	gGame.mRoboeyes.mRotation = mAngle.y;
}

// the pointer tracker's dead band: keeps the aim inside a mRange-wide window
// around the tracked point and hands back the scaled scroll offset
/**
 * @note Address: 0x80165F8C
 * @note Size: 0x3F4
 */
f32 CCameraActionEye::TrackPointer(u32 step, f32 low, f32 high)
{
	static f32 sPrev = 0.0f;

	if (mFirstFrame) {
		f32 winLow   = 0.5f * gPointerScreenHeight + 0.5f * gPointerScreenHeight * -sEyePointerLimits[2];
		f32 winHigh  = 0.5f * gPointerScreenHeight + 0.5f * gPointerScreenHeight * sEyePointerLimits[2];
		sPrev        = 0.0f;
		f32 pointerY = gWiiPointer.mPointer.y;
		mVec10C.y    = pointerY;
		f32 lo       = 0.5f * mRange + winLow;
		if (pointerY < lo) {
			mVec10C.y = lo;
		} else {
			f32 hi = -0.5f * mRange + winHigh;
			if (pointerY > hi) {
				mVec10C.y = hi;
			}
		}
		mFirstFrame = 0;
		return 0.7853982f * sPrev + 0.05235988f;
	}

	f32 bandLow  = mVec10C.y - 0.5f * mRange;
	f32 bandHigh = mVec10C.y + 0.5f * mRange;
	if (low == 0.0f) {
		f32 pointerY = gWiiPointer.mPointer.y;
		f32 winLow   = 0.5f * gPointerScreenHeight + 0.5f * gPointerScreenHeight * -sEyePointerLimits[2];
		f32 winHigh  = 0.5f * gPointerScreenHeight + 0.5f * gPointerScreenHeight * sEyePointerLimits[2];
		if (pointerY < bandLow && winHigh < bandHigh && gWiiPointer.mPointerValid) {
			f32 d = pointerY - bandLow;
			mVec10C.y += d;
			sPrev += -d / (0.5f * (gPointerScreenHeight - gPointerScreenHeight * sEyePointerLimits[2]));
		} else if (pointerY > bandHigh && winLow > bandLow && gWiiPointer.mPointerValid) {
			f32 d = pointerY - bandHigh;
			mVec10C.y += d;
			sPrev += -d / (0.5f * (gPointerScreenHeight - gPointerScreenHeight * sEyePointerLimits[2]));
		} else if (!((winLow > bandLow || winHigh < bandHigh) && gWiiPointer.mPointerValid)) {
			mVec10C.y = pointerY;
			sPrev     = 0.0f;
			f32 lo    = 0.5f * mRange + winLow;
			if (pointerY < lo) {
				mVec10C.y = lo;
			} else {
				f32 hi = -0.5f * mRange + winHigh;
				if (pointerY > hi) {
					mVec10C.y = hi;
				}
			}
		}
		return 0.7853982f * sPrev + 0.05235988f;
	} else {
		f32 pointerY = gWiiPointer.mPointer.y;
		if (pointerY < bandLow) {
			f32 d = pointerY - bandLow;
			mVec10C.y += d;
			sPrev = fn_8016A81C(sPrev + -d / (0.5f * (gPointerScreenHeight - gPointerScreenHeight * sEyePointerLimits[2])), -1.0f, 1.0f);
		} else if (pointerY > bandHigh) {
			f32 d = pointerY - bandHigh;
			mVec10C.y += d;
			sPrev = fn_8016A81C(sPrev + -d / (0.5f * (gPointerScreenHeight - gPointerScreenHeight * sEyePointerLimits[2])), -1.0f, 1.0f);
		}
		return 0.7853982f * sPrev + 0.05235988f;
	}
}

/**
 * @note Address: 0x80166380
 * @note Size: 0xC
 */
void CCameraActionEye::DrawDebug2d()
{
	fn_8016AA74(sEyePointerLimits);
}

/**
 * @note Address: 0x8016638C
 * @note Size: 0xC
 */
u32 CCameraActionEye::GetCameraFlags()
{
	return 0x20019;
}
