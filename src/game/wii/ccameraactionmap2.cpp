#include "game/ccameraaction.h"
#if defined(VERSION_R24J01)
#include "game/wii/cgame.h"
#endif
#include "game/gmath.h"
extern "C" {
#include <dolphin/os.h>
}

// the Wii build merged the area and map camera code into this file; the retail
// rodata carries "ccameraactionmap2.cpp" as the __FILE__ string for both halves.
// CamAreaSlot's constructor is the file's first emitted function.

/**
 * @note Address: 0x80154310
 * @note Size: 0x10
 */
CamAreaSlot::CamAreaSlot()
{
}

// the area camera's half

u32 CCameraActionArea::GetCameraFlags()
{
	return CAMFLAG_EYE | CAMFLAG_INTEREST | CAMFLAG_FOV;
}

u32 CCameraActionArea::GetInterestMode()
{
	return 2; // CMotionControlCamera eases its interest towards ours with Follow instead of taking it at once
}

XObj* CCameraActionArea::GetTarget()
{
	return mTarget;
}

extern "C" void SetAreaCameraHeightIndices(CCameraActionArea*, s32 zoomLevel)
{
	gCameraAreaState.mZoomLevel     = zoomLevel;
	gCameraAreaState.mPrevZoomLevel = zoomLevel;
}

extern "C" void ResumeAreaCameraControl(CCameraActionArea* area)
{
	area->mControlEnabled           = 1;
	gCameraAreaState.mPrevZoomLevel = gCameraAreaState.mZoomLevel;
}

// the map camera's half

extern "C" void SetMapCameraKeepPreviousEye(CCameraActionMap* map)
{
	map->mKeepPreviousEye = TRUE;
}

extern "C" s32 SelectMapCameraArea(CCameraActionMap* map);
extern "C" void StartMapCameraAreaHeightInterpolation(CCameraActionMap* map);

extern "C" void RefreshMapCameraArea(CCameraActionMap* map)
{
	map->mPrevMapArea = map->mMapArea;
	SelectMapCameraArea(map);
	StartMapCameraAreaHeightInterpolation(map);
	map->mPrevMapArea = map->mMapArea;
}

extern "C" void StartMapCameraTargetHeightInterpolation(CCameraActionMap* map, f32 from, f32 to, f32 time)
{
	if (0.0f != time) {
		map->mTargetHeightOffsetInterp = &map->mTargetHeightOffsetEase;
		map->mTargetHeightOffsetInterp->Start(from, to, time);
	}
}

f32 CCameraActionMap::GetFollowDivisor()
{
	// CMotionControlCamera::Follow closes 1/6 of the remaining distance per call
	return 6.0f;
}

u32 CCameraActionMap::GetInterestMode()
{
	return mInterestMode;
}

u32 CCameraActionMap::GetCameraFlags()
{
	return CAMFLAG_EYE | CAMFLAG_INTEREST | CAMFLAG_FOV;
}
