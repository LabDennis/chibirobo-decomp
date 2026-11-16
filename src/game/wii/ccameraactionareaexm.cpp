#include "game/ccameraaction.h"

// the Wii emits this file's four functions in reverse source order

/**
 * @note Address: 0x80031FC4
 * @note Size: 0x44
 */
extern "C" s32 FindAreaContainingPoint__20CCameraActionAreaExMF3Vec(CCameraActionAreaExM* action, Vec pos)
{
	return FindFirstCameraAreaContainingPoint(&gCameraAreaState, pos);
}

/**
 * @note Address: 0x80032008
 * @note Size: 0x10
 */
void CCameraActionAreaExM::SetZoomLevel(s32 mode)
{
	gCameraAreaState.mZoomLevel = mode;
}

/**
 * @note Address: 0x80032018
 * @note Size: 0x10
 */
s32 CCameraActionAreaExM::GetZoomLevel()
{
	return gCameraAreaState.mZoomLevel;
}

/**
 * @note Address: 0x80032028
 * @note Size: 0x10
 */
f32 CCameraActionAreaExM::GetBaseFov()
{
	return gCameraAreaState.mBaseFov;
}
