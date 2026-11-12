#include "game/xobjactor.h"

/**
 * @note Address: 0x80109004
 * @note Size: 0x4
 */
void XObjActor3::DrawDebugStatus()
{
}

/**
 * @note Address: 0x80109008
 * @note Size: 0x44
 */
void XObjActor3::DrawObjectEffects()
{
	if (GetKind() == 0x31B) {
		DrawParticlePool(&mDust.mPtcl);
	}
}
