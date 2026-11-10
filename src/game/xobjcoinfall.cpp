#include "game/xobjflags.h"

static inline void SetPosAndTarget(CObjVec* pos, const Vec& value)
{
	pos->Set(value);
	pos->SetTarget(value);
}

/**
 * @note Address: 0x800E11B8
 * @note Size: 0x20C
 */
extern "C" void UpdateFallingCoin(XObjCoin* coin, u32 frames)
{
	CVec step;
	switch (coin->mState) {
	case 1:
		++coin->mFallFrames;
		coin->mFallSpeed -= 0.09f * frames;
		coin->mMoveTranslate.y += coin->mFallSpeed;
		if (coin->mMoveTranslate.y <= 0.0f) {
			coin->mFallSpeed       = 0.0f;
			coin->mMoveTranslate.y = 0.0f;
			coin->mState           = 0;
			coin->mStatus.mFlags &= ~0x100000ULL;
		}
		if (coin->mFallSpeed >= -2.3f) {
			coin->mFallSpeed -= 0.36f * frames;
		} else {
			coin->mFallSpeed = -2.3f;
		}
		step.x = 0.4f * (f32)sin(coin->mHeading);
		step.z = 0.4f * (f32)cos(coin->mHeading);
		step.y = 0.0f;
		SetPosAndTarget(&coin->mMoveTranslate, coin->mMoveTranslate + step * (f32)frames);
		coin->SetPos(coin->mMoveTranslate, 0);
		break;
	}
}
