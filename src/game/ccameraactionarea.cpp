#include "game/ccameraaction.h"
#include "game/cmotioncontrolcamera.h"
#include <math.h>
#include "game/cvec.h"
#include "game/hsd_jobj.h"

#include "game/cgame.h"
#include "game/xchr.h"

extern "C" {
#include <dolphin/os.h>
#include "game/cgs.h"
}

inline void CCameraActionArea::SetEye_I(s32 mode, Vec from, Vec to, f32 time)
{
	if (mode == 5) {
		OSPanic("ccameraactionarea.cpp", 660, "%s%s", "error ! : CCameraActionArea::SetEye_I()\n", "SetEye_I");
		return;
	}
	SetEyeInterpolation(mode, from, to, time);
}

// a flat unit direction from an angle's y
inline void YawDir(CVec& dir, const Vec& angle)
{
	f32 x = sin(angle.y);
	f32 z = cos(angle.y);
	dir.Set(x, 0.0f, z);
	PSVECNormalize(&dir, &dir);
}

// the entry in a slot facing closest to dir
inline s32 FindFacing(const Vec& dir, s32 slotIndex)
{
	CVec look(dir);
	CamAreaSlot* slot   = &gCameraAreaState.mGroups[slotIndex];
	f32 best            = -1.0f;
	s32 bestIndex       = -1;
	CamAreaEntry* entry = slot->mEntries;
	for (s32 i = 0; i < slot->mEntryCount; i++, entry++) {
		CVec facing;
		YawDir(facing, entry->mAngle);
		f32 dot = PSVECDotProduct(&look, &facing);
		if (dot > best) {
			bestIndex = i;
			best      = dot;
		}
	}
	if (best == -1.0f) {
		bestIndex = 0;
	}
	return bestIndex;
}

extern "C" void SelectAreaCameraEntry(CCameraActionArea* area, s32 slotIndex, s32 entryIndex, s32 mode, f32 time);
extern "C" void ApplyAreaCameraEntry(CCameraActionArea* area, CamAreaEntry* entry, s32 mode, s32 adjustFov, f32 time);

/**
 * @note Address: 0x80138294
 * @note Size: 0x8
 */
u32 CCameraActionArea::GetCameraFlags()
{
	return CAMFLAG_EYE | CAMFLAG_INTEREST | CAMFLAG_FOV;
}

/**
 * @note Address: 0x8013829C
 * @note Size: 0x8
 */
u32 CCameraActionArea::GetInterestMode()
{
	return 2; // CMotionControlCamera eases its interest towards ours with Follow instead of taking it at once
}

/**
 * @note Address: 0x801382A4
 * @note Size: 0x8
 */
XObj* CCameraActionArea::GetTarget()
{
	return mTarget;
}

/**
 * @note Address: 0x801382AC
 * @note Size: 0x28
 */
void CCameraActionArea::DrawDebug3d()
{
	gCameraAreaState.DrawDebug3d();
}

/**
 * @note Address: 0x801382D4
 * @note Size: 0x14
 */
extern "C" void SetAreaCameraHeightIndices(CCameraActionArea*, s32 zoomLevel)
{
	gCameraAreaState.mZoomLevel     = zoomLevel;
	gCameraAreaState.mPrevZoomLevel = zoomLevel;
}

// moves the camera to entry's eye by mode over time, and its fov to the area's base fov, scaled by the zoom level when adjustFov is set
/**
 * @note Address: 0x801382E8
 * @note Size: 0x7B4
 */
extern "C" void ApplyAreaCameraEntry(CCameraActionArea* area, CamAreaEntry* entry, s32 mode, s32 adjustFov, f32 time)
{
	switch (mode) {
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
		area->SetEye_I(mode, area->mEye, entry->mEye, time);
		gGame.GetPlayer()->ResetCameraValue();
		break;
	case 5:
		// SetEye_I panics on mode 5; SetEyeInterpolation takes it as 4
		area->SetEyeInterpolation(mode, area->mEye, entry->mEye, time);
		gGame.GetPlayer()->ResetCameraValue();
		break;
	}
	f32 fov = gCameraAreaState.mBaseFov;
	if (adjustFov) {
		switch (gCameraAreaState.mZoomLevel) {
		case 0:
			fov *= 0.8f;
			break;
		case 2:
			fov *= 1.5f;
			break;
		case 3:
			fov *= 2.0f;
			break;
		}
	}
	area->SetFovInterpolation(mode, area->mFov, fov, time);
	gGame.GetPlayer()->ResetCameraValue();
}
/**
 * @note Address: 0x80138A9C
 * @note Size: 0x90
 */
extern "C" void SelectAreaCameraEntry(CCameraActionArea* area, s32 slotIndex, s32 entryIndex, s32 mode, f32 time)
{
	CamAreaEntry* entry   = &gCameraAreaState.mGroups[slotIndex].mEntries[entryIndex];
	area->mEntry          = entry;
	area->mPrevEntry      = entry;
	area->mEntryIndex     = entryIndex;
	area->mPrevEntryIndex = entryIndex;
	SetCameraAreaIndexAndBaseFov(&gCameraAreaState, slotIndex);
	ApplyAreaCameraEntry(area, entry, mode, 1, time);
}

/**
 * @note Address: 0x80138B2C
 * @note Size: 0x24
 */
extern "C" void SelectAreaCameraEntryWithControl(CCameraActionArea* area, s32 slotIndex, s32 entryIndex, s32 mode, s32 control, f32 time)
{
	area->mControlEnabled = control;
	SelectAreaCameraEntry(area, slotIndex, entryIndex, mode, time);
}

/**
 * @note Address: 0x80138B50
 * @note Size: 0x154
 */
extern "C" void SetAreaCameraInterestTarget(CCameraActionArea* area, s32 mode, XObj* target, f32 time)
{
	if (target == NULL) {
#if !defined(VERSION_PAL)
		OSReport("SetInte_I( unknown data )\n");
#endif
		return;
	}
	area->mTarget = target;
	Vec interest  = target->GetCameraTarget();
	Vec current   = area->mInterest;
	area->SetInte_I(mode, current, interest, time);
}

/**
 * @note Address: 0x80138CA4
 * @note Size: 0x1EC
 */
extern "C" void SelectAreaCameraEntryByViewDirection(CCameraActionArea* area, s32 mode, f32 time, s32 slotIndex)
{
	area->mControlEnabled = 1;
	CVec view             = area->mInterest - area->mEye;
	PSVECNormalize(&view, &view);
	SelectAreaCameraEntry(area, slotIndex, FindFacing(view, slotIndex), mode, time);
}
/**
 * @note Address: 0x80138E90
 * @note Size: 0x2C
 */
extern "C" void SelectCurrentCameraAreaByViewDirection(CCameraActionArea* area, s32 mode, f32 time)
{
	SelectAreaCameraEntryByViewDirection(area, mode, time, gCameraAreaState.mActiveArea);
}

/**
 * @note Address: 0x80138EBC
 * @note Size: 0x1C
 */
extern "C" void ResumeAreaCameraControl(CCameraActionArea* area)
{
	area->mControlEnabled           = 1;
	gCameraAreaState.mPrevZoomLevel = gCameraAreaState.mZoomLevel;
}

/**
 * @note Address: 0x80138ED8
 * @note Size: 0x6E0
 */
extern "C" s32 UpdateAreaCameraEntrySelection(CCameraActionArea* area)
{
	CPad pad(0);
	s32 slotIndex    = gCameraAreaState.mActiveArea;
	s32 currentArea  = gCameraAreaState.mCurrentArea;
	s32 previousArea = gCameraAreaState.mPrevArea;
	s32 zoomLevel    = gCameraAreaState.mZoomLevel;
	if (gGame.mStatus.mFlags & 0x20) {
		return area->mEntry->mId;
	}

	// with status bit 0x80000000 the d-pad steers the camera, otherwise the c-stick
	u32 up       = (gGame.mStatus.mFlags & 0x80000000) ? HSD_PAD_DPADUP : HSD_PAD_CSTICK_UP;
	u32 down     = (gGame.mStatus.mFlags & 0x80000000) ? HSD_PAD_DPADDOWN : HSD_PAD_CSTICK_DOWN;
	u32 next     = (gGame.mStatus.mFlags & 0x80000000) ? HSD_PAD_DPADLEFT : HSD_PAD_CSTICK_LEFT;
	u32 previous = (gGame.mStatus.mFlags & 0x80000000) ? HSD_PAD_DPADRIGHT : HSD_PAD_CSTICK_RIGHT;
	u32 buttons  = pad.mPad->mTrigger;
	if (buttons & up) {
		if (zoomLevel > 1) {
			gCameraAreaState.mZoomLevel = zoomLevel - 1;
			return area->mEntry->mId;
		}
	} else if (buttons & down) {
		if (zoomLevel < 2) {
			gCameraAreaState.mZoomLevel = zoomLevel + 1;
			return area->mEntry->mId;
		}
	} else if (area->mKeepViewDirection == 1 && (buttons & (next | previous))) {
		// after SetCamera, go by the view instead of mEntryIndex: the entry whose eye bearing is nearest at or below the camera's
		f32 yaw             = WrapAngle(atan2(area->mEye.x - area->mInterest.x, area->mEye.z - area->mInterest.z));
		f32 best            = TWO_PI;
		s32 index           = 0;
		CamAreaSlot* slot   = &gCameraAreaState.mGroups[gCameraAreaState.mActiveArea];
		CamAreaEntry* entry = slot->mEntries;
		for (s32 i = 0; i < slot->mEntryCount; i++, entry++) {
			f32 facing = WrapAngle(atan2(entry->mEye.x - area->mInterest.x, entry->mEye.z - area->mInterest.z));
			f32 turn   = WrapAngle(yaw - facing);
			if (turn <= best) {
				best  = turn;
				index = i;
			}
		}
		if (!(buttons & next) && --index < 0) {
			index = slot->mEntryCount - 1;
		}
		area->mEntryIndex = index;
		area->mForceApply = TRUE;
	} else if (buttons & next) {
		if (++area->mEntryIndex >= gCameraAreaState.mGroups[slotIndex].mEntryCount) {
			area->mEntryIndex = 0;
		}
	} else if (buttons & previous) {
		if (--area->mEntryIndex < 0) {
			area->mEntryIndex = gCameraAreaState.mGroups[slotIndex].mEntryCount - 1;
		}
	} else if (currentArea != previousArea && currentArea != -1) {
		// entered another area: the entry facing nearest the current view (after SetCamera) or the current entry's angle
		CVec dir;
		if (area->mKeepViewDirection != 0) {
			CVec delta = area->mInterest - area->mEye;
			delta.Normalize();
			dir.Set(delta);
		} else {
			YawDir(dir, area->mEntry->mAngle);
		}
		area->mEntryIndex = FindFacing(dir, slotIndex);
	}
	area->mEntry = &gCameraAreaState.mGroups[slotIndex].mEntries[area->mEntryIndex];
	return area->mEntry->mId;
}
/**
 * @note Address: 0x801395B8
 * @note Size: 0x94
 */
void CCameraActionArea::DrawDebug2d()
{
	DrawGsStrokeTextFormat(&gGame.mGs, -1, "\\B000000A0AREA(%3d,%3d)(%3d)\n", 324.0f, 298.0f, 0.0f, 12.0f, 12.0f,
	                       gCameraAreaState.mActiveArea, mEntryIndex, gCameraAreaState.mCurrentArea);
	DrawGsStrokeTextFormat(&gGame.mGs, -1, "\\B000000A0ACAREA\n", 324.0f, 310.0f, 0.0f, 12.0f, 12.0f);
}

/**
 * @note Address: 0x8013964C
 * @note Size: 0x544
 */
void CCameraActionArea::Update(u32 step)
{
	mPadControlEnabled = (gGame.mStatus.mFlags & 0xC00) == 0;
	if (mTarget != NULL && mTarget->mJObj != NULL) {
		HSD_JObjSetupMatrix(mTarget->mJObj);
	}
	if (mControlEnabled == 1 && mPadControlEnabled) {
		UpdateAreaCameraEntrySelection(this);
	}
	s32 currentArea  = gCameraAreaState.mCurrentArea;
	s32 previousArea = gCameraAreaState.mPrevArea;
	if (mEntry->mId != mPrevEntry->mId || mForceApply) {
		if (currentArea != previousArea && currentArea != -1) {
			// the player entered another area: mode 1 eases in and out
			mTransitionTime = 2.0f;
			ApplyAreaCameraEntry(this, mEntry, 1, 1, mTransitionTime);
		} else if (mForceApply) {
			// a pick from the SetCamera view, which may keep the same entry id
			mTransitionTime = 15.0f;
			ApplyAreaCameraEntry(this, mEntry, 5, 1, mTransitionTime);
			mForceApply = FALSE;
		} else if (currentArea == previousArea) {
			mTransitionTime = 15.0f;
			ApplyAreaCameraEntry(this, mEntry, 5, 1, mTransitionTime);
		}
		gGame.GetPlayer()->ResetCameraValue();
		mPrevEntryIndex = mEntryIndex;
		mPrevEntry      = mEntry;
		if (mKeepViewDirection != 0) {
			mKeepViewDirection = 0;
		}
	}
	s32 zoomLevel = gCameraAreaState.mZoomLevel;
	if (zoomLevel != gCameraAreaState.mPrevZoomLevel) {
		f32 fov = GetAreaCameraFovForHeightLevel(&gCameraAreaState, zoomLevel);
		SetFovInterpolation(4, mFov, fov, 15.0f);
		gGame.GetPlayer()->ResetCameraValue();
		gCameraAreaState.mZoomLevel     = zoomLevel;
		gCameraAreaState.mPrevZoomLevel = zoomLevel;
	}
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
	if (mInterestInterp != NULL) {
		if (!mInterestInterp->Update(step, FRAME_TIME)) {
			SetInterest(mInterestInterp->Get());
		} else {
			mInterestInterp = NULL;
		}
	}
	if (mInterestInterp == NULL && mTarget != NULL) {
		SetInterest(mTarget->GetCameraTarget());
	}
}
/**
 * @note Address: 0x80139B90
 * @note Size: 0x108
 */
void CCameraActionArea::PreviousUpdate()
{
	if (mControlEnabled == 1) {
		UpdateCameraAreaIndexAndBaseFov(&gCameraAreaState, 0);
	}
	// entering an area run by the map camera hands the camera over to it
	if (gCameraAreaState.mCurrentArea != gCameraAreaState.mPrevArea && gCameraAreaState.mCurrentArea != -1
	    && GetCameraAreaMode(&gCameraAreaState) == CAMACTION_MAP) {
		CMotionControlCamera* camera = &gGame.mMotionControlCamera;
		camera->ChangeAction(CAMACTION_MAP);
		gCameraAreaState.mPrevZoomLevel = gCameraAreaState.mZoomLevel;
		SetMapCameraFovStep(static_cast<CCameraActionMap*>(gGame.mCameraActions.mTable[CAMACTION_MAP]), gCameraAreaState.mZoomLevel, 15.0f);
		RefreshMapCameraArea(static_cast<CCameraActionMap*>(gGame.mCameraActions.mTable[CAMACTION_MAP]));
		f32 dy = camera->mCamera.mInterest.y - mTarget->GetCameraTarget().y;
		if (0.0f != dy) {
			StartMapCameraTargetHeightInterpolation(static_cast<CCameraActionMap*>(gGame.mCameraActions.mTable[CAMACTION_MAP]), dy, 0.0f,
			                                        8.0f);
		}
	}
}

/**
 * @note Address: 0x80139C98
 * @note Size: 0x78
 */
void CCameraActionArea::Reset()
{
	LoadStageCameraAreaState(&gCameraAreaState);
	mControlEnabled     = 1;
	mPadControlEnabled  = TRUE;
	mEntryIndex         = 0;
	CamAreaEntry* entry = gCameraAreaState.mGroups[0].mEntries;
	mEntry              = entry;
	mPrevEntry          = entry;
	mTarget             = gGame.GetPlayer();
	mKeepViewDirection  = 0;
	mForceApply         = FALSE;
}

/**
 * @note Address: 0x80139D10
 * @note Size: 0x98
 */
void CCameraActionArea::SetCamera(CCamera* camera)
{
	SetEye(camera->mEye);
	SetInterest(camera->mInterest);
	SetDistance(camera->mDistance);
	SetFov(camera->mFov);
	mKeepViewDirection = 1;
}

/**
 * @note Address: 0x80139DA8
 * @note Size: 0x98
 */
void CCameraActionArea::Init()
{
	InitCameraAreaEntries(&gCameraAreaState);
	// by interpolation mode; mode 3 or a time <= 0 sets the value at once, and the eye and fov take mode 5 as 4
	mEyeInterps[0]      = &mEyeLinear;
	mEyeInterps[1]      = &mEyeEaseInOut;
	mEyeInterps[2]      = &mEyeEaseOut;
	mEyeInterps[3]      = NULL;
	mEyeInterps[4]      = &mEyeEaseOutDivide;
	mFovInterps[0]      = &mFovLinear;
	mFovInterps[1]      = &mFovEaseInOut;
	mFovInterps[2]      = &mFovEaseOut;
	mFovInterps[3]      = NULL;
	mFovInterps[4]      = &mFovEaseOutDivide;
	mInterestInterps[0] = NULL;
	mInterestInterps[1] = &mInterestEaseInOut;
	mInterestInterps[2] = NULL;
	mInterestInterps[3] = NULL;
	mInterestInterps[4] = NULL;
}
