#include "game/xobjspiderboss.h"

extern "C" const f32 sSpiderBossArenaMin;
extern "C" const f32 sSpiderBossArenaExtent;
extern "C" const f32 sSpiderBossZero;

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
		spider->mAreaPos.x = sSpiderBossArenaMin;
		spider->mAreaPos.y = sSpiderBossZero;
		spider->mAreaPos.z = sSpiderBossArenaMin;
		spider->mAreaWidth = sSpiderBossArenaExtent;
		spider->mAreaDepth = sSpiderBossArenaExtent;
	} else {
		spider->mStep      = 0;
		spider->mTimer     = 0;
		spider->mPrevState = spider->mState;
		spider->mState     = 3;
		spider->mStep      = 100;
	}
	spider->mSpiderStatus.mFlags &= ~0x8000U;
}
