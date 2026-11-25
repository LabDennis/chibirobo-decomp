#ifndef _GAME_LETICKER_CAMERA_H
#define _GAME_LETICKER_CAMERA_H

#include "game/ccameraaction.h"
#include "game/gmath.h"
#include "Version.h"
#include <dolphin/os.h>

extern "C" CCameraActionLeticker gLetickerCameraAction;

// Checks whether the camera eye stays inside the room at this angle.
inline bool CanTurnLetickerCamera(CCameraActionLeticker& camera, f32 angle)
{
	if (!camera.mBounded) {
		return true;
	}
	Vec eye;
	camera.CalcEye(&eye, angle);
#if !defined(VERSION_PAL)
	OSReport("%4.2f %4.2f\n", eye.x, eye.z);
#endif
	if (eye.x < camera.mMinX || camera.mMaxX < eye.x || eye.z < camera.mMinZ || camera.mMaxZ < eye.z) {
#if !defined(VERSION_PAL)
		OSReport("CCameraActionLeticker::Update %4.2f %4.2f %d\n", eye.x, eye.z, (s32)(RAD2DEG * camera.mTurnTo));
#endif
		return false;
	}
	return true;
}

#endif
