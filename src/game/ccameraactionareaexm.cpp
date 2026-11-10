#include "game/ccameraaction.h"

// CCameraActionAreaExM's _68 to _74, they all go to the shared area state

/**
 * @note Address: 0x80015C54
 * @note Size: 0x10
 */
extern "C" f32 vf68__20CCameraActionAreaExMFv(CCameraActionAreaExM* action)
{
	return gCameraAreaState.mBaseFov;
}

/**
 * @note Address: 0x80015C64
 * @note Size: 0x10
 */
extern "C" s32 vf6C__20CCameraActionAreaExMFv(CCameraActionAreaExM* action)
{
	return gCameraAreaState._14;
}

/**
 * @note Address: 0x80015C74
 * @note Size: 0x10
 */
extern "C" void vf70__20CCameraActionAreaExMFl(CCameraActionAreaExM* action, s32 mode)
{
	gCameraAreaState._14 = mode;
}

/**
 * @note Address: 0x80015C84
 * @note Size: 0x44
 */
extern "C" s32 vf74__20CCameraActionAreaExMF3Vec(CCameraActionAreaExM* action, const Vec& pos)
{
	return FindFirstCameraAreaContainingPoint(&gCameraAreaState, pos);
}
