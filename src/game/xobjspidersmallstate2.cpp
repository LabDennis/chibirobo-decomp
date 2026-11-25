#include "game/xobjspidersmall.h"

/**
 * @note Address: 0x800FC930
 * @note Size: 0x40
 */
// state 2, dead
void XObjSpiderSmall::State2(u32)
{
	mStatus.mFlags &= ~0x10000ULL;
	mSpiderStatus.mFlags |= 0x80000;
	mSpiderStatus.mFlags &= ~0x100000;
}
