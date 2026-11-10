#include "game/ccameraaction.h"

extern "C" {
f32 gChaseCameraSpringSettings[3] = { 0.5f, 2.5f, 4.5f };
}

u32 CCameraActionChase::GetCameraFlags()
{
	return mMode;
}
