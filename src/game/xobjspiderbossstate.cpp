#include "game/xobjspiderboss.h"

const f32 XObjSpiderBoss::sEscapeInputStrength = 8.0f;
const f32 XObjSpiderBoss::sDefaultPatrolMin    = -200.0f;
const f32 XObjSpiderBoss::sDefaultPatrolSize   = 400.0f;

/**
 * @note Address: 0x80103EC0
 * @note Size: 0x94
 */
void XObjSpiderBoss::State0(u32 frames)
{
	mSpiderStatus.mFlags &= ~0x80000U;
	u32 flags = mSpiderStatus.mFlags;
	if ((flags >> 25) & 1) {
		return;
	}
	if (flags & 0x40000000) {
		mStep      = 0;
		mTimer     = 0;
		mPrevState = mState;
		mState     = 4;
		ResetPatrolArea();
	} else {
		mTimer     = 0;
		mPrevState = mState;
		mState     = 3;
		mStep      = 100;
	}
	mSpiderStatus.mFlags &= ~0x8000U;
}
