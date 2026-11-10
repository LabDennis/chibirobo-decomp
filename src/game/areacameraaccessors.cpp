#include "game/ccameraaction.h"

/**
 * @note Address: 0x80138294
 * @note Size: 0x8
 */
extern "C" s32 GetCameraFlags__17CCameraActionAreaFv(CCameraActionArea*)
{
	return 11;
}

/**
 * @note Address: 0x8013829C
 * @note Size: 0x8
 */
extern "C" s32 vf5C__17CCameraActionAreaFv(CCameraActionArea*)
{
	return 2;
}

/**
 * @note Address: 0x801382A4
 * @note Size: 0x8
 */
extern "C" XObj* GetTarget__17CCameraActionAreaFv(CCameraActionArea* area)
{
	return area->mTarget;
}

/**
 * @note Address: 0x801382AC
 * @note Size: 0x28
 */
extern "C" void vf28__17CCameraActionAreaFv(CCameraActionArea*)
{
	DrawDebug3d__19CCameraActionAreaExFv(&gCameraAreaState);
}

/**
 * @note Address: 0x801382D4
 * @note Size: 0x14
 */
extern "C" void SetAreaCameraHeightIndices(CCameraActionArea*, s32 index)
{
	gCameraAreaState._14 = index;
	gCameraAreaState._18 = index;
}
