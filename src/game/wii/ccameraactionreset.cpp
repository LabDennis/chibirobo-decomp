#include "game/ccameraaction.h"
#include "game/wii/cgame.h"
#include "game/ut.h"

#include <math.h>

extern "C" {
#include "Dolphin/os.h"
}

// the Wii build times the interpolators off its own refresh counter
extern s32 gRefreshRate;
extern "C" s32 ClassifyFloatBits(f32 value); // the Wii MSL's out-of-line __fpclassifyf

#undef FRAME_TIME
#define FRAME_TIME (1.0f / gRefreshRate)

class CCameraActionArea;
class CCameraActionMap;

extern "C" {
// the area camera's state, ccameraactionareaex.cpp
s32 GetCameraAreaMode(CCameraActionAreaEx* state, s32 index);
f32 GetAreaCameraFovForHeightLevel(CamAreaState* state, s32 level);
void UpdateCameraAreaIndexAndBaseFov(CamAreaState* state, s32 unused);
void InitCameraAreaEntries(CamAreaState* state);

// the area camera, ccameraactionarea.cpp
void SetAreaCameraHeightIndices(CCameraActionArea* area, s32 index);
void SelectAreaCameraEntryByViewDirection(CCameraActionArea* area, s32 mode, f32 time, s32 arg);

// the map camera, ccameraactionmap2.cpp
void StartMapCameraTargetHeightInterpolation(CCameraActionMap* map, f32 from, f32 to, f32 time);
void RefreshMapCameraArea(CCameraActionMap* map);
void GetMapCameraEyeBehindTarget(CCameraActionMap* map, Vec& out, s32 mode);
void SetMapCameraFovStep(CCameraActionMap* map, s32 index, f32 time);
}

// the Wii build resolves the current area's action in a second, C++ step that falls back to
// the map camera while the player stands outside every area
s32 GetCameraAreaMode(CamAreaState* state);

/**
 * @note Address: 0x801665C4
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

/**
 * @note Address: 0x80166640
 * @note Size: 0xCC
 */
void CCameraActionReset::SetCamera(CCamera* camera)
{
	SetEye(camera->mView.mEye);
	SetInterest(camera->mView.mInterest);
	SetDistance(camera->mView.mDistance);
	SetFov(camera->mFov);
	SetAngle(camera->mView.mAngle);
	mTarget       = (XObj*)gGame.GetPlayer();
	mState        = 100; // hold until the first update
	mInterestMode = 2;
}

/**
 * @note Address: 0x8016670C
 * @note Size: 0x494
 */
void CCameraActionReset::PreviousUpdate()
{
	if (mState == 100) {
		if (mAngle.z) { // drop any roll it came in with
			SetAngleZ(0.0f, 0x40010);
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
		gGame.mMotionControlCamera.ChangeAction(mNextCameraAction);
		if (mNextCameraAction == 1) {
			SetAreaCameraHeightIndices((CCameraActionArea*)gGame.mCameraActions.mTable[1], gCameraAreaState.mZoomLevel);
		} else {
			SetMapCameraFovStep((CCameraActionMap*)gGame.mCameraActions.mTable[7], gCameraAreaState.mZoomLevel, 15.0f);
			RefreshMapCameraArea((CCameraActionMap*)gGame.mCameraActions.mTable[7]);
			f32 dy = gGame.mMotionControlCamera.mView.mInterest.y - ((WiiPlayerState*)mTarget)->GetCameraTarget().y;
			if (0.0f != dy) {
				StartMapCameraTargetHeightInterpolation((CCameraActionMap*)gGame.mCameraActions.mTable[7], dy, 0.0f, 8.0f);
			}
		}
		mImmediate = FALSE;
		return;
	}

	// the action ids are the slots in mTable: 1 is the area camera, 7 the map camera
	if (mNextCameraAction == 1) {
		gGame.mMotionControlCamera.ChangeAction(mNextCameraAction);
		SetAreaCameraHeightIndices((CCameraActionArea*)gGame.mCameraActions.mTable[1], gCameraAreaState.mZoomLevel);
		SelectAreaCameraEntryByViewDirection((CCameraActionArea*)gGame.mCameraActions.mTable[1], mMode, mTime, arg);
		if (mMode == 3) { // mode 3 snaps instead of interpolating
			gGame.mMotionControlCamera.ApplyAction();
		}
		return;
	}

	if (mNextCameraAction == 7) {
		Vec eye;
		GetMapCameraEyeBehindTarget((CCameraActionMap*)gGame.mCameraActions.mTable[7], eye, 1);
		if (ClassifyFloatBits(eye.x) == FP_NAN || ClassifyFloatBits(eye.y) == FP_NAN
		    || ClassifyFloatBits(eye.z) == FP_NAN) { // a NaN eye is fatal

			CVec cameraEye      = gGame.mMotionControlCamera.mView.mEye;
			CVec cameraInterest = gGame.mMotionControlCamera.mView.mInterest;
			OSPanic(__FILE__, 157,
			        FormatToStaticBuffer("CCameraActionReset ...\nEye(%f, %f, %f)\nInt(%f, %f, %f)\n", cameraEye.x, cameraEye.y,
			                             cameraEye.z, cameraInterest.x, cameraInterest.y, cameraInterest.z));
		}

		f32 time   = mTime;
		SetEye_Interp(mMode, eye, time);

		f32 fov = GetAreaCameraFovForHeightLevel(&gCameraAreaState, gCameraAreaState.mZoomLevel);
		SetFov_Interp(mMode, fov, mTime);

		SetAreaCameraHeightIndices((CCameraActionArea*)gGame.mCameraActions.mTable[1], gCameraAreaState.mZoomLevel);
		if (mMode == 3 || 0.0f == mTime) { // no time to blend the interest
			mInterestMode = 0;
		} else {
			mInterestMode = 2;
		}
	}
	mState = 1;
}

/**
 * @note Address: 0x80166BA0
 * @note Size: 0x198
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

	SetInterest(((WiiPlayerState*)mTarget)->GetCameraTarget());
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
 * @note Address: 0x80166D38
 * @note Size: 0x8
 */
XObj* CCameraActionReset::GetTarget()
{
	return mTarget;
}

/**
 * @note Address: 0x80166D40
 * @note Size: 0x8
 */
f32 CCameraActionReset::GetFollowDivisor()
{
	return 4.0f;
}

/**
 * @note Address: 0x80166D48
 * @note Size: 0x8
 */
u32 CCameraActionReset::GetInterestMode()
{
	return mInterestMode;
}
