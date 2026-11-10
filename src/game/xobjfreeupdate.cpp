#include "game/cgame.h"
#include "game/gmath.h"
#include "game/xchr.h"
#include "game/xobjfree.h"
#include <math.h>
#include <stdlib.h>

/**
 * @note Address: 0x800DDB8C
 * @note Size: 0x4AC
 */
void XObjFree::UpdateObject(u32 frames)
{
	UpdateObject__4XObjFUl(this, frames);
	if (mStatus.mFlags & 0x2000000000000000ULL) {
		mStatus.mFlags &= ~0x2000000000000000ULL;
		mStatus.mFlags |= 0x4000000000000000ULL;
		SetAnim(1, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
		vf12C(1, 200, 200, 200, 0.0f);
		vf12C(1, 0, 0, 0, 0.4f);
		for (s32 i = 0; i < 10; ++i) {
			f32 angle    = DEG2RAD * (rand() % 360);
			f32 x        = (f32)sin(angle) * (rand() % 100) / 150.0f;
			f32 z        = (f32)cos(angle) * (rand() % 100) / 150.0f;
			f32 y        = (rand() % 100) / 200.0f;
			Vec position = mMoveTranslate;
			position.x += x * (rand() % 100) / 100.0f;
			position.z += z * (rand() % 100) / 100.0f;
			Vec velocity = { x, y, z };
			Vec size     = { 20.0f, 20.0f, 1.0f };
			SpawnPoolParticle(&gGame.GetPlayer()->mDust.mPtcl, &position, &size, &velocity, 0xFFFFFFF0);
		}
		mFlash = 1.0f;
	}
	if (mStatus.mFlags & 0x4000000000000000ULL) {
		HSD_AObj* aobj = GetAObj(mJObj);
		if (aobj && (aobj->flags & 0x44000000))
			ClearAnim();
		if (mFlash > 0.0f) {
			mFlash -= 0.04f;
			if (mFlash <= 0.0f)
				Release();
		}
	}
}
