#include "game/ccameraaction.h"
#include "game/cmotioncontrolcamera.h"
#include "Version.h"
#include <math.h>

#include "game/cgame.h"
#include "game/xchr.h"

extern "C" {
#include <dolphin/os.h>
#include "game/video.h"
}

// pal goes by the current refresh rate, 50 or 60
#if defined(VERSION_PAL)
#define FRAME_TIME (1.0f / gVideoFramesPerSecond)
#else
#define FRAME_TIME (1.0f / 60.0f)
#endif

/**
 * @note Address: 0x80147630
 * @note Size: 0x8
 */
u32 CCameraActionReset::GetInterestMode()
{
	return mInterestMode;
}

/**
 * @note Address: 0x80147638
 * @note Size: 0x8
 */
f32 CCameraActionReset::GetFollowDivisor()
{
	return 4.0f;
}

/**
 * @note Address: 0x80147640
 * @note Size: 0x8
 */
XObj* CCameraActionReset::GetTarget()
{
	return mTarget;
}

/**
 * @note Address: 0x80147648
 * @note Size: 0x14C
 */
void CCameraActionReset::Update(u32 step)
{
	if (mTarget == NULL) {
		return;
	}
	if (mState == 100) { // hold for one frame after taking the camera
		mState = 0;
		return;
	}

	SetInterest(mTarget->GetCameraTarget());
	// an interpolator that answers true is done
	if (mEyeInterp != NULL) {
		if (!mEyeInterp->Update(step, FRAME_TIME)) {
			SetEye(mEyeInterp->Get());
		} else {
			mEyeInterp = NULL;
		}
	}
	if (mFovInterp != NULL) {
		if (!mFovInterp->Update(step, FRAME_TIME)) {
			SetFov(mFovInterp->Get());
		} else {
			mFovInterp = NULL;
		}
	}
}

/**
 * @note Address: 0x80147794
 * @note Size: 0x5DC
 */
void CCameraActionReset::PreviousUpdate()
{
	Vec eye;

	if (mState == 100) {
		if (mAngle.z) { // drop any roll it came in with
			mCameraFlags = 0x40010;
			Vec angle    = mAngle;
			angle.z      = 0.0f;
			SetAngle(angle);
		}
		return;
	}
	if (mState == 1) { // hand over once the interpolators ran out
		if (mEyeInterp == NULL && mFovInterp == NULL) {
			gGame.mMotionControlCamera.ChangeAction(mNextCameraAction);
		}
		return;
	}

	mCameraFlags = 0xB;
	UpdateCameraAreaIndexAndBaseFov(&gCameraAreaState, 0);
	s32 arg           = gCameraAreaState.mActiveArea;
	mNextCameraAction = GetCameraAreaMode(&gCameraAreaState);
	if (mImmediate == TRUE) {
		CMotionControlCamera* camera = &gGame.mMotionControlCamera;
		camera->ChangeAction(mNextCameraAction);
		if (mNextCameraAction == CAMACTION_AREA) {
			SetAreaCameraHeightIndices(static_cast<CCameraActionArea*>(gGame.mCameraActions.mTable[CAMACTION_AREA]),
			                           gCameraAreaState.mZoomLevel);
		} else {
			SetMapCameraFovStep(static_cast<CCameraActionMap*>(gGame.mCameraActions.mTable[CAMACTION_MAP]), gCameraAreaState.mZoomLevel,
			                    15.0f);
			RefreshMapCameraArea(static_cast<CCameraActionMap*>(gGame.mCameraActions.mTable[CAMACTION_MAP]));
			f32 dy = camera->mCamera.mInterest.y - mTarget->GetCameraTarget().y;
			if (0.0f != dy) {
				StartMapCameraTargetHeightInterpolation(static_cast<CCameraActionMap*>(gGame.mCameraActions.mTable[CAMACTION_MAP]), dy,
				                                        0.0f, 8.0f);
			}
		}
		mImmediate = FALSE;
		return;
	}

	if (mNextCameraAction == CAMACTION_AREA) {
		CMotionControlCamera* camera = &gGame.mMotionControlCamera;
		camera->ChangeAction(mNextCameraAction);
		SetAreaCameraHeightIndices(static_cast<CCameraActionArea*>(gGame.mCameraActions.mTable[CAMACTION_AREA]),
		                           gCameraAreaState.mZoomLevel);
		SelectAreaCameraEntryByViewDirection(static_cast<CCameraActionArea*>(gGame.mCameraActions.mTable[CAMACTION_AREA]), mMode, mTime,
		                                     arg);
		if (mMode == 3) { // mode 3 snaps instead of interpolating
			camera->ApplyAction();
		}
		return;
	}

	if (mNextCameraAction == CAMACTION_MAP) {
		GetMapCameraEyeBehindTarget(static_cast<CCameraActionMap*>(gGame.mCameraActions.mTable[CAMACTION_MAP]), eye, 1);
		if (fpclassify(eye.x) == FP_NAN || fpclassify(eye.y) == FP_NAN || fpclassify(eye.z) == FP_NAN) { // a NaN eye is fatal

			CVec cameraEye      = gGame.mMotionControlCamera.mCamera.mEye;
			CVec cameraInterest = gGame.mMotionControlCamera.mCamera.mInterest;
			OSPanic(__FILE__, 157, "%s%s",
			        FormatToStaticBuffer("CCameraActionReset ...\nEye(%f, %f, %f)\nInt(%f, %f, %f)\n", cameraEye.x, cameraEye.y,
			                             cameraEye.z, cameraInterest.x, cameraInterest.y, cameraInterest.z),
			        __FUNCTION__);
		}

		SetEye_Interp(mMode, eye, mTime);

		f32 fov = GetAreaCameraFovForHeightLevel(&gCameraAreaState, gCameraAreaState.mZoomLevel);
		SetFov_Interp(mMode, fov, mTime);

		SetAreaCameraHeightIndices(static_cast<CCameraActionArea*>(gGame.mCameraActions.mTable[CAMACTION_AREA]),
		                           gCameraAreaState.mZoomLevel);
		if (mMode == 3 || 0.0f == mTime) { // no time to blend the interest
			mInterestMode = 0;
		} else {
			mInterestMode = 2;
		}
	}
	mState = 1;
}
/**
 * @note Address: 0x80147D70
 * @note Size: 0xCC
 */
void CCameraActionReset::SetCamera(CCamera* camera)
{
	SetEye(camera->mEye);
	SetInterest(camera->mInterest);
	SetDistance(camera->mDistance);
	SetFov(camera->mFov);
	SetAngle(camera->mAngle);
	mTarget       = gGame.GetPlayer();
	mState        = 100; // hold until the first update
	mInterestMode = 2;
}

/**
 * @note Address: 0x80147E3C
 * @note Size: 0x7C
 */
void CCameraActionReset::Init()
{
	mMode      = 1; // ease in-out, like the tables below
	mTime      = 2.0f;
	mImmediate = FALSE;
	InitCameraAreaEntries(&gCameraAreaState);
	mEyeInterps[0] = &mEyeLinear;
	mEyeInterps[1] = &mEyeEaseInOut;
	mEyeInterps[2] = &mEyeEaseOut;
	mFovInterps[0] = &mFovLinear;
	mFovInterps[1] = &mFovEaseInOut;
	mFovInterps[2] = &mFovEaseOut;
}
