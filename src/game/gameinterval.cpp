#include "game/cgame.h"

/**
 * @note Address: 0x8000A4B8
 * @note Size: 0x68
 */
// CGame's FrameInterval, the logo and title run at 60, the game at 30
s32 CGame::FrameInterval()
{
	return IsIntroActive() ? 1 : 2;
}

/**
 * @note Address: 0x8000A520
 * @note Size: 0xC
 */
// CGame's GetCamera
CCamera* CGame::GetCamera()
{
	return &mMotionControlCamera.mCamera;
}

/**
 * @note Address: 0x8000A52C
 * @note Size: 0xC
 */
CLight* CGame::GetLight()
{
	return mLightList;
}

/**
 * @note Address: 0x8000A538
 * @note Size: 0x8
 */
u32 CBase::GetRetraceStep()
{
	return mRetraceStep;
}

/**
 * @note Address: 0x8000A540
 * @note Size: 0x8
 */
CCamera* CBase::GetPerspectiveCamera()
{
	return &mCameras[0];
}

/**
 * @note Address: 0x8000A548
 * @note Size: 0x8
 */
CCamera* CBase::GetScreenCamera()
{
	return &mCameras[1];
}

/**
 * @note Address: 0x8000A550
 * @note Size: 0x8
 */
CLight* CBase::GetDefaultLight()
{
	return &mLights[0];
}

/**
 * @note Address: 0x8000A558
 * @note Size: 0x8
 */
CLight* CBase::GetScreenLight()
{
	return &mLights[4];
}
