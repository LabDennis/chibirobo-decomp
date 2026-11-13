#include "game/xobjflags.h"
#include "game/ccameraaction.h"
#include "game/gmath.h"
#include <stdlib.h>

// XObjDust::OnPlayerContact, a stick flick sends it hopping
/**
 * @note Address: 0x800DE608
 * @note Size: 0x1EC
 */
extern "C" u32 OnPlayerContact__8XObjDustFll(XObjDust* object, void* unused, u32 flags)
{
	if ((flags & 3) == 1)
		return flags;
	if (object->GetObjFlags() & 1) {
		if (0.0f == object->mBouncePhase) {
			CPad pad(0);
			object->mBouncePhase  = DEG2RAD;
			f32 y                 = pad.mPad->mStickY;
			f32 x                 = pad.mPad->mStickX;
			f32 magnitude         = sqrtf(x * x + y * y);
			object->mBounceHeight = ((rand() % 400 + 400) / 100.0f) * magnitude;
		}
	}
	return 0;
}
