#include "game/ccameraaction.h"
#include <string.h>

extern void* gSavedScrollCameraInfo;

u32 CCameraActionScroll2::GetCameraFlags()
{
	return 11;
}

void CCameraActionScroll2::vf28()
{
}

/**
 * @note Address: 0x801439C0
 * @note Size: 0x40
 */
void CCameraActionScroll2::SaveCamInfo(const void* info)
{
	if (info == NULL) {
		gSavedScrollCameraInfo = NULL;
	} else {
		gSavedScrollCameraInfo = mSavedInfo;
		memmove(mSavedInfo, info, sizeof(mSavedInfo));
	}
}
