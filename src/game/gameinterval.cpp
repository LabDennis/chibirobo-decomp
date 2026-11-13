#include "game/cgame.h"

// todo: cleanup
static inline u8 LogoActive(CGame* game)
{
	s32 state = game->mLogo.mState;
	return (u8)(state >= 1 && state < 15);
}

// todo: cleanup
static inline u8 TitleActive(CGame* game)
{
	s32 state = game->mTitle.mState;
	return (u8)(state >= 1 && state < 23);
}

// todo: cleanup
static inline u8 IntroActive(CGame* game)
{
	u8 active = 0;
	if (LogoActive(game) || TitleActive(game))
		active = 1;
	return active;
}

/**
 * @note Address: 0x8000A4B8
 * @note Size: 0x68
 */
// CGame's FrameInterval, the logo and title run at 60, the game at 30
s32 CGame::FrameInterval()
{
	return IntroActive(this) ? 1 : 2;
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
u32 CBase::vf2C()
{
	return mRetraceStep;
}

/**
 * @note Address: 0x8000A540
 * @note Size: 0x8
 */
extern "C" void* vf30__5CBaseFv(CBase* base)
{
	return &base->mCameras[0];
}

/**
 * @note Address: 0x8000A548
 * @note Size: 0x8
 */
extern "C" void* vf34__5CBaseFv(CBase* base)
{
	return &base->mCameras[1];
}

/**
 * @note Address: 0x8000A550
 * @note Size: 0x8
 */
extern "C" void* vf3C__5CBaseFv(CBase* base)
{
	return &base->mLights[0];
}

/**
 * @note Address: 0x8000A558
 * @note Size: 0x8
 */
extern "C" void* vf40__5CBaseFv(CBase* base)
{
	return &base->mLights[4];
}
