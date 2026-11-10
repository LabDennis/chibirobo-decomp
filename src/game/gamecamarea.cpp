#include "game/ccameraaction.h"

/**
 * @note Address: 0x8001160C
 * @note Size: 0x28
 */
// CCameraActionAreaExM's vf64, resets the shared area camera state
extern "C" void vf64__20CCameraActionAreaExMFv(CCameraActionAreaExM* action)
{
	LoadStageCameraAreaState(&gCameraAreaState);
}

/**
 * @note Address: 0x80011634
 * @note Size: 0x4
 */
// todo: weak copy
extern "C" void vf14__22CInterfaceCameraActionFv(CInterfaceCameraAction* action)
{
}
