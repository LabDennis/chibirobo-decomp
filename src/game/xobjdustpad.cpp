#include "game/xobjflags.h"
#include "game/ccameraaction.h"
#include "game/gmath.h"
#include <stdlib.h>

// A stick flick sends the dust hopping.
/**
 * @note Address: 0x800DE608
 * @note Size: 0x1EC
 */
s32 XObjDust::OnPlayerContact(const CVec&, s32 flags)
{
	if ((flags & 3) == 1)
		return flags;
	if (GetObjFlags() & 1) {
		if (0.0f == mBouncePhase) {
			CPad pad(0);
			mBouncePhase  = DEG2RAD;
			f32 y         = pad.mPad->mStickY;
			f32 x         = pad.mPad->mStickX;
			f32 magnitude = sqrtf(x * x + y * y);
			mBounceHeight = ((rand() % 400 + 400) / 100.0f) * magnitude;
		}
	}
	return 0;
}
