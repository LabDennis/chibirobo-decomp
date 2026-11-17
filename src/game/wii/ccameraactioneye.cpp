// the Wii first person view: the remote's pointer aims the look
#include "game/ccameraactioneye.h"

#include "game/gmath.h"
#include "game/cscrn.h"
#include "game/wii/cgame.h"
#include "game/wii/cpad.h"
#include "game/wii/cpadact.h"
#include "game/wii/csound.h"

class XObjChr;
extern "C" {
// the empty debug-draw hook DrawDebug2d feeds, in cmotioncontrolcamera.cpp's range
void fn_8016AA74(const f32* params);
// the first person eye of the player object; xchr.h is GameCube-only, on the
// Wii the same function sits at 0x800E589C
Vec GetPlayerFirstPersonEyePosition(XObjChr* player);
}

// the band limits the pointer's aim point is clamped into; the free look swaps
// between the two whenever mOverride has a part set
static CVec sEyeLimitA(20.0f, 20.0f, 20.0f);
static CVec sEyeLimitB(30.0f, 30.0f, 20.0f);

// the Wii player object keeps its action states past the named part of
// WiiPlayerState; the offsets are read straight off the retail object:
// +0xAA4 the lift state (7-19 busy), the rest the idle checks the GameCube ran
// on mHang/mJump/mLadder/mRope/mPush/mTumble
static inline bool PlayerCanLook()
{
	const u8* player = (const u8*)gGame.GetPlayer();
	s32 lift         = *(const s32*)(player + 0xAA4);
	if (lift >= 7 && lift <= 19) {
		return false;
	}
	return *(const s32*)(player + 0xB54) == 0 && *(const s32*)(player + 0xBA4) == 0 && *(const s32*)(player + 0xBF8) == 0
	    && *(const s32*)(player + 0xC3C) == 0 && *(const s32*)(player + 0xC7C) == 0 && *(const s32*)(player + 0xCD4) == 0;
}

static inline void ClampStick(f32& value)
{
	if (value > 0.8f) {
		value = 0.8f;
	} else if (value < -0.8f) {
		value = -0.8f;
	}
}

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

// NONMATCHING: same stores and calls as retail, different emission order;
// mLook.Reset folds the value, target and speed writes into the member call
void CCameraActionEye::Reset()
{
	mLook.x         = 0.0f;
	mLook.y         = 0.0f;
	mLook.z         = 0.0f;
	mLook.mTarget.x = 0.0f;
	mLook.mTarget.y = 0.0f;
	mLook.mTarget.z = 0.0f;
	mOverride.x     = 0.0f;
	mOverride.y     = 0.0f;
	mOverride.z     = 0.0f;
	mLook.mSpeed.x  = 0.05f;
	mLook.mSpeed.y  = 0.05f;
	mLook.mSpeed.z  = 0.05f;
	mVec80.x        = 0.0f;
	mVec80.y        = 0.0f;
	mVec80.z        = 0.0f;
	mVec90.x        = 0.0f;
	mVec90.y        = 0.0f;
	mVec90.z        = 0.0f;
	mVecA0.x        = 0.0f;
	mVecA0.y        = 0.0f;
	mVecA0.z        = 0.0f;

	Vec angle = { 0.05235988f, 0.0f, 0.0f };
	angle.y   = gGame.GetPlayer()->mMoveRotate.y;
	SetAngle(angle);
	SetDistance(100.0f);
	SetFov(40.0f);

	mLook.Reset(0.05235988f, gGame.GetPlayer()->mMoveRotate.y, 0.0f);
	mB0         = 0.0f;
	mVecB4.x    = 0.0f;
	mVecB4.y    = 0.0f;
	mVecB4.z    = 0.0f;
	mVecC0      = sEyeLimitA;
	mVec10C.x   = 0.0f;
	mVec10C.y   = 0.0f;
	mVec10C.z   = 0.0f;
	mRange      = 24.0f;
	mFirstFrame = 1;
}

// NONMATCHING: the body is reconstructed from the retail flow - the free-look
// toggle on the remote buttons, the pointer aim through TrackPointer and the
// two response curves, the action-state look gates, the roboeyes swap and the
// blur writes on the zoom - but the instruction order and the uncharted member
// reads do not line up yet
void CCameraActionEye::Update(u32 step)
{
	WiiPlayerState* player = gGame.GetPlayer();
	const CPadOne& pad     = gWiiPads[0];

	SetEye(GetPlayerFirstPersonEyePosition((XObjChr*)player));

	if (!gGame.GetStatusFlags(0xC00)) {
		if ((pad.mTrigger & 0x40) && (pad.mButton & 0x20) && gGame.GetStatusFlags(0x80000000)) {
			if (gGame.GetStatusFlags(0x200)) { // toggle the free-look mode
				gGame.ClearStatusFlags(0x200);
				StartScreenWipeColorTransition(&gGame.mRoboeyes, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 320.0f, 224.0f, 430.0f, 400.0f,
				                               430.0f, 400.0f, 0.0f, 0xFF, FALSE);
			} else {
				gGame.SetStatusFlags(0x200);
				StartScreenWipeColorTransition(&gGame.mRoboeyes, 0x80, 0x80, 0xFF, 0xFF, 0x80, 0x80, 0xFF, 320.0f, 224.0f, 430.0f, 400.0f,
				                               430.0f, 400.0f, 0.0f, 0xFF, FALSE);
			}
		}

		f32 zoom;
		if (gGame.GetStatusFlags(0x200)) { // free-look: the C-stick aims
			f32 x = pad.mSubStick.x;
			ClampStick(x);
			f32 y = pad.mSubStick.y;
			ClampStick(y);
			if (gGame.mEvt.mData.mScript != NULL || !PlayerCanLook()) {
				y = 0.0f;
				x = y;
			}
			mLook.mTarget.y = -1.5707964f * x;
			mLook.mTarget.x = -0.7853982f * y;
		} else {
			// the pointer picks the aim point, run through the response curve
			mVecB4.x = gWiiPointer.mPointer.x;
			mVecB4.y = gWiiPointer.mPointer.y;
			mVecB4.z = gWiiPointer.mPointer.z;
			if (gWiiPointer.mPointerValid) {
				mLook.mTarget.x = TrackPointer(step, 30.0f, 400.0f) * -0.7853982f;
			}

			f32 turn = pad.mStick.x;
			ClampStick(turn);
			f32 tilt = pad.mStick.y;
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
					if (!IsWiiSfxPlaying(&gGame.mSound, 0x70)) {
						PlayWiiSoundEffect(&gGame.mSound, 0x70, 1.0f, 0.0f);
					}
				}
			} else {
				mLook.mTarget.x = mOverride.x;
			}
			player->mVtable->SetRotation(player, player->mMoveRotate.y - 0.034906585f * turn * step);

			zoom = pad.mSubStick.y;
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
		if (57.295776f * speedY > 0.2f || 57.295776f * speedX > 0.2f) { // still turning fast: keep the sound on
			if (!IsWiiSfxPlaying(&gGame.mSound, 0x70)) {
				PlayWiiSoundEffect(&gGame.mSound, 0x70, 1.0f, 0.0f);
			}
		}
		mLook.Step(step);

		// the move vector's y is mRotY at +0x64, so it sits at +0x60
		Vec look = { mLook.x, mLook.y, 0.0f };
		SetAngle(CVec(*(const Vec*)((const u8*)player + 0x60)) + look);
		SetFov(40.0f + mLook.z);

		static f32 sPreviousZoomSpeed = 0.0f;
		f32 stick                     = pad.mStick.y;
		// the Wii blur sits at CGame+0x52E4; the fields are uncharted, the writes
		// retail makes land at +0x190..+0x1A0 with the vfunc at +0x88
		u8* blur = (u8*)&gGame + 0x52E4;
		if (Abs(speedZ - sPreviousZoomSpeed) < 0.02f) { // zoom settled
			*(f32*)(blur + 0x190) = -1.0f;
		} else {
			*(f32*)(blur + 0x190) = 0.8f;
		}
		*(f32*)(blur + 0x194) = 0.0f;
		*(f32*)(blur + 0x198) = 0.1f;
		*(f32*)(blur + 0x19C) = *(f32*)(blur + 0x1A0) = 2.0f * -stick;
		sPreviousZoomSpeed                            = speedZ;
	}
}

// the pointer tracker's dead band: keeps the aim inside a mRange-wide window
// around the screen middle and hands back the scaled offset;
// NONMATCHING: retail folds the band math and the first-frame snap into the
// mVec10C window and a static previous-sample word at lbl_805A1D70
f32 CCameraActionEye::TrackPointer(u32 step, f32 low, f32 high)
{
	f32 y = gWiiPointer.mPointer.y;

	mVec10C.y = y;
	if (mFirstFrame) {
		mFirstFrame = 0;
		if (y > high) {
			y = high;
		} else if (y < low) {
			y = low;
		}
	}
	f32 mid = 0.5f * (low + high);
	if (y > mid + mRange) {
		y = mid + mRange;
	} else if (y < mid - mRange) {
		y = mid - mRange;
	}
	return (y - mid) / mRange;
}

// NONMATCHING: retail passes a four-float debug block to an empty hook
void CCameraActionEye::DrawDebug2d()
{
	static const f32 sEyeDebugParams[4] = { 0.9f, 0.9f, 0.3f, 0.2f };
	fn_8016AA74(sEyeDebugParams);
}

u32 CCameraActionEye::GetCameraFlags()
{
	return 0x20019;
}
