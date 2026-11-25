#include "game/xobjactor.h"

/**
 * @note Address: 0x80115538
 * @note Size: 0x24
 */
void XObjActorToy::ClearStateHandler()
{
	mState = NULL;
}

/**
 * @note Address: 0x8011555C
 * @note Size: 0x58
 */
void XObjActorToy::RunStateHandler(u32 frames)
{
	if (mState != NULL)
		(this->*mState)(frames);
}
