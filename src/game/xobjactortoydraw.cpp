#include "game/xobjactor.h"

/**
 * @note Address: 0x801220A8
 * @note Size: 0xA4
 */
void XObjActorToy::DrawScreenOverlay()
{
	XObjActor::DrawScreenOverlay();
	if (mFirst.mStatus.mFlags & 0x10000ULL)
		mFirst.DrawScreenOverlay();
	if (mSecond.mStatus.mFlags & 0x10000ULL)
		mSecond.DrawScreenOverlay();
}
