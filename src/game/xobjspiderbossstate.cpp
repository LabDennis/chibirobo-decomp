#include "game/xobjspiderboss.h"

/**
 * @note Address: 0x80103EC0
 * @note Size: 0x94
 */
extern "C" void State0__14XObjSpiderBossFUl(XObjSpiderBoss* spider)
{
	spider->mSpiderStatus.mFlags &= ~0x80000U;
	u32 flags = spider->mSpiderStatus.mFlags;
	if ((flags >> 25) & 1) {
		return;
	}
	if (flags & 0x40000000) {
		spider->mStep      = 0;
		spider->mTimer     = 0;
		spider->mPrevState = spider->mState;
		spider->mState     = 4;
		spider->mAreaPos.x = -200.0f;
		spider->mAreaPos.y = 0.0f;
		spider->mAreaPos.z = -200.0f;
		spider->mAreaWidth = 400.0f;
		spider->mAreaDepth = 400.0f;
	} else {
		spider->mStep      = 0;
		spider->mTimer     = 0;
		spider->mPrevState = spider->mState;
		spider->mState     = 3;
		spider->mStep      = 100;
	}
	spider->mSpiderStatus.mFlags &= ~0x8000U;
}
