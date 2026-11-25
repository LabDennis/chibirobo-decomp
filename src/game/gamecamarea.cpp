#include "game/ccameraaction.h"

/**
 * @note Address: 0x8001160C
 * @note Size: 0x28
 */
// CCameraActionAreaExM's LoadAreaState, resets the shared area camera state
void CCameraActionAreaExM::LoadAreaState()
{
	LoadStageCameraAreaState(&gCameraAreaState);
}

/**
 * @note Address: 0x80011634
 * @note Size: 0x4
 */
void CInterfaceCameraAction::Reset()
{
}
