#include "game/xobjspiderboss.h"

/**
 * @note Address: 0x80103B28
 * @note Size: 0x58
 */
void XObjSpiderBoss::State2(u32 frames)
{
	switch (mStep) {
	case 0:
		mSpiderStatus.mFlags |= 0x80000;
		mSpiderStatus.mFlags |= 0x10;
		mSpiderStatus.mFlags &= ~0x200U;
		mSpiderStatus.mFlags &= ~0x100000U;
		++mStep;
		break;
	case 1:
		break;
	}
}
