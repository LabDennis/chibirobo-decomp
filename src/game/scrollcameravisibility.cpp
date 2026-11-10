#include "game/ccameraaction.h"
#include "game/scrollcameravisibility.h"

// the objects hidden while this camera runs and what they were before
s32 gScrollCameraHiddenObjectIds[256];
s32 gScrollCameraHiddenObjectStates[256];

// in reverse order, same deferred inlining thing

/**
 * @note Address: 0x801469A8
 * @note Size: 0x38
 */
BOOL CCameraActionScroll2::AddHideObj(s32 id)
{
	s32* slot = gScrollCameraHiddenObjectIds;
	for (s32 i = 0; i < 256; i++, slot++) {
		if (*slot == -1) {
			*slot = id;
			return TRUE;
		}
	}
	return FALSE;
}

/**
 * @note Address: 0x801468FC
 * @note Size: 0xAC
 */
void CCameraActionScroll2::ClearHideList()
{
	for (int i = 0; i < 256; i++) {
		gScrollCameraHiddenObjectIds[i]    = -1;
		gScrollCameraHiddenObjectStates[i] = -1;
	}
}
