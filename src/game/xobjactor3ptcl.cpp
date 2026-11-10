#include "game/xobjactor.h"

/**
 * @note Address: 0x80109004
 * @note Size: 0x4
 */
extern "C" void DrawDebugStatus__10XObjActor3Fv(XObjActor3* actor)
{
}

/**
 * @note Address: 0x80109008
 * @note Size: 0x44
 */
extern "C" void DrawObjectEffects__10XObjActor3Fv(XObjActor3* actor)
{
	if (actor->GetKind() == 0x31B) {
		DrawParticlePool(&actor->mDust.mPtcl);
	}
}
