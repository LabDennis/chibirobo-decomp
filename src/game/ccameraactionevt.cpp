#include "game/ccameraaction.h"
#include "game/gmath.h"
extern "C" {
#include <dolphin/os.h>
}
#include "Version.h"

// these come out in reverse, deferred inlining

/**
 * @note Address: 0x80143920
 * @note Size: 0x94
 */
void CCameraActionEvt::Init()
{
	mEyeInterps[0]      = &mEyeLinear;
	mEyeInterps[1]      = &mEyeEaseInOut;
	mEyeInterps[2]      = &mEyeEaseOut;
	mInterestInterps[0] = &mInterestLinear;
	mInterestInterps[1] = &mInterestEaseInOut;
	mInterestInterps[2] = &mInterestEaseOut;
	mFovInterps[0]      = &mFovLinear;
	mFovInterps[1]      = &mFovEaseInOut;
	mFovInterps[2]      = &mFovEaseOut;
	mDistanceInterps[0] = &mDistanceLinear;
	mDistanceInterps[1] = &mDistanceEaseInOut;
	mDistanceInterps[2] = &mDistanceEaseOut;
	mAngleInterps[0]    = &mAngleLinear;
	mAngleInterps[1]    = &mAngleEaseInOut;
	mAngleInterps[2]    = &mAngleEaseOut;
	mTarget             = NULL;
}

/**
 * @note Address: 0x8014384C
 * @note Size: 0xD4
 */
void CCameraActionEvt::SetCamera(CCamera* camera)
{
	SetEye(camera->mEye);
	SetInterest(camera->mInterest);
	SetDistance(camera->mDistance);
	SetFov(camera->mFov);
	SetAngle(camera->mAngle);
	_38 = 0;
	mAxes.mFlags &= ~7;
	mTarget         = NULL;
	mEyeInterp      = NULL;
	mInterestInterp = NULL;
	mDistanceInterp = NULL;
	mFovInterp      = NULL;
	mAngleInterp    = NULL;
}

/**
 * @note Address: 0x801435E8
 * @note Size: 0x264
 */
void CCameraActionEvt::Update(u32 step)
{
	if (mEyeInterp != NULL) {
		if (!mEyeInterp->Update(step, FRAME_TIME)) {
			SetEye(mEyeInterp->Get());
		} else {
			mEyeInterp = NULL;
		}
	}
	if (mInterestInterp != NULL) {
		if (!mInterestInterp->Update(step, FRAME_TIME)) {
			SetInterest(mInterestInterp->Get());
		} else {
			mInterestInterp = NULL;
		}
	} else if (mTarget != NULL) {
		SetInterest(mTarget->GetCameraTarget());
	}
	if (mDistanceInterp != NULL) {
		if (!mDistanceInterp->Update(step, FRAME_TIME)) {
			SetDistance(mDistanceInterp->Get());
		} else {
			mDistanceInterp = NULL;
		}
	}
	if (mFovInterp != NULL) {
		if (!mFovInterp->Update(step, FRAME_TIME)) {
			SetFov(mFovInterp->Get());
		} else {
			mFovInterp = NULL;
		}
	}
	if (mAngleInterp != NULL) {
		if (!mAngleInterp->Update(step, FRAME_TIME)) {
			SetAngle(mAngleInterp->Get());
		} else {
			mAngleInterp = NULL;
		}
	}
}

/**
 * @note Address: 0x80143528
 * @note Size: 0xC0
 */
void CCameraActionEvt::SetEye_Interp(s32 mode, const Vec& eye, f32 time)
{
	if (mode == 3 || time <= 0.0f) {
		mEyeInterp = NULL;
		SetEye(eye);
		return;
	}
	mEyeInterp = mEyeInterps[mode];
	if (mEyeInterp != NULL) {
		mEyeInterp->Start(mEye, eye, time);
	}
}

/**
 * @note Address: 0x801433EC
 * @note Size: 0x13C
 */
void CCameraActionEvt::SetInterest_Interp(s32 mode, XObj* target, f32 time)
{
	if (target == NULL) {
#if !defined(VERSION_PAL)
		OSReport("SetInterest_Interp( unknown data )\n");
#endif
		return;
	}
	mTarget      = target;
	Vec interest = mTarget->GetCameraTarget();
	SetInterest_Interp(mode, interest, time);
}

/**
 * @note Address: 0x8014332C
 * @note Size: 0xC0
 */
void CCameraActionEvt::SetInterest_Interp(s32 mode, const Vec& interest, f32 time)
{
	if (mode == 3 || time <= 0.0f) {
		mInterestInterp = NULL;
		SetInterest(interest);
		return;
	}
	mInterestInterp = mInterestInterps[mode];
	if (mInterestInterp != NULL) {
		mInterestInterp->Start(mInterest, interest, time);
	}
}

/**
 * @note Address: 0x8014329C
 * @note Size: 0x90
 */
void CCameraActionEvt::SetFov_Interp(s32 mode, f32 fov, f32 time)
{
	if (mode == 3 || time <= 0.0f) {
		mFovInterp = NULL;
		SetFov(fov);
		return;
	}
	mFovInterp = mFovInterps[mode];
	if (mFovInterp != NULL) {
		mFovInterp->Start(mFov, fov, time);
	}
}

// todo: cleanup
/**
 * @note Address: N/A
 * @note Size: ?
 */
inline void TurnShortWay(f32& to, f32& from)
{
	f32 f = WrapAngle(from);
	f32 t = WrapAngle(to);
	from  = f;
	to    = t;
	if (t != f) {
		f32 diff = t - f;
		if (diff < 0.0f) {
			diff = -diff;
		}
		if (diff >= TWO_PI) {
			diff -= TWO_PI;
		}
		if (diff > PI) {
			if (t < f) {
				to = TWO_PI + t;
			} else {
				to = t - TWO_PI;
			}
		}
	}
}

/**
 * @note Address: 0x80142DE8
 * @note Size: 0x4B4
 */
void CCameraActionEvt::SetAngle_Interp(s32 mode, const Vec& angle, u32 axes, u32 type, BOOL relative, f32 time)
{
	CVec dest;
	CVec from;

	if (type == 1) {
		_38 = 0x20012;
	} else if (type == 2) {
		_38 = 0x20011;
	}
	if (relative) {
		dest = mAngle;
		PSVECAdd(&dest, (Vec*)&angle, &dest);
	}
	if (!relative) {
		dest = CVec(angle);
	}

	mAxes.mFlags &= ~7;
	mAxes.mFlags |= axes;
	Vec to = mAngle;
	if (mAxes.mFlags & 1) {
		to.x = dest.x;
	}
	if (mAxes.mFlags & 2) {
		to.y = dest.y;
	}
	if (mAxes.mFlags & 4) {
		to.z = dest.z;
		_38  = 0x40010;
	}

	from = mAngle;
	TurnShortWay(to.x, from.x);
	TurnShortWay(to.y, from.y);
	TurnShortWay(to.z, from.z);

	if (mode == 3 || time <= 0.0f) {
		mAngleInterp = NULL;
		SetAngle(to);
		return;
	}
	mAngleInterp = mAngleInterps[mode];
	if (mAngleInterp != NULL) {
		mAngleInterp->Start(from, to, time);
	}
}
