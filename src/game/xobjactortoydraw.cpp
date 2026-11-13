#include "game/xobjactor.h"

/**
 * @note Address: 0x801220A8
 * @note Size: 0xA4
 */
void XObjActorToy::vfC8()
{
	XObjActor::vfC8();
	if (mFirst.mStatus.mFlags & 0x10000ULL)
		mFirst.vfC8();
	if (mSecond.mStatus.mFlags & 0x10000ULL)
		mSecond.vfC8();
}
