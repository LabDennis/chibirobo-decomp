#include "game/ccameraaction.h"

/**
 * @note Address: 0x8001160C
 * @note Size: 0x28
 */
// CCameraActionAreaExM's LoadAreaState, resets the shared area camera state
extern "C" void LoadAreaState__20CCameraActionAreaExMFv(CCameraActionAreaExM* action)
{
	LoadStageCameraAreaState(&gCameraAreaState);
}

/**
 * @note Address: 0x80011634
 * @note Size: 0x4
 */
// todo: weak copy
extern "C" void Reset__22CInterfaceCameraActionFv(CInterfaceCameraAction* action)
{
}
