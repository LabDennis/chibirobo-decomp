#include "game/xobjflags.h"
#include "game/ccameraaction.h"
#include "game/gmath.h"
#include <stdlib.h>

// A stick flick sends loose dust hopping.
/**
 * @note Address: 0x800DE608
 * @note Size: 0x1EC
 */
s32 XObjDust::OnPlayerContact(s32, s32 flags)
{
	if ((flags & 3u) == 1)
		return flags;
	if (GetObjFlags() & 1) {
		if (0.0f == mBouncePhase) {
			CPad pad(0);
			mBouncePhase  = DEG2RAD;
			f32 y         = pad.mPad->mStickY;
			f32 x         = pad.mPad->mStickX;
			f32 magnitude = sqrtf(x * x + y * y);
			f32 height    = (rand() % 400 + 400) / 100.0f;
			mBounceHeight = height * magnitude;
		}
	}
	return 0;
}
