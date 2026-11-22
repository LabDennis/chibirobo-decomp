#include "game/xobjflags.h"
#include "game/animframe.h"
#include <math.h>
#include <stdlib.h>
#include "game/gmath.h"
#include "game/stagedata.h"

/**
 * @note Address: 0x800E1E60
 * @note Size: 0xD0
 */
BOOL XObjCoin::LoadFromPlacement(const CoordInfo& info)
{
	mPlacementParam1 = info.mPlacement->mParam1;
	if (IsFlagSet())
		return TRUE;
	if (XObj::LoadFromPlacement(info)) {
		mStatus.mFlags |= 0x10000000ULL;
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x800E1F30
 * @note Size: 0x1C8
 */
void XObjCoin::ResetObjectState()
{
	XObj::ResetObjectState();
	s32 direction = rand() & 1 ? 1 : -1;
	f32 angle     = DEG2RAD * (rand() % 360);
	mSpin         = angle * direction;
	f32 end       = GetAnimEndFrame(mJObj);
	if (end > 0.0f) {
		f32 frame              = fmod((f32)rand(), end);
		UnkStageObj* placement = mCoord.mPlacement;
		SetAnim(placement->mStartAnim, TRUE, NULL, NULL, NULL, frame, 1.0f, 1.0f, 10.0f);
	}
	mFade     = 0.0f;
	mState    = 0;
	mParam0   = 0;
	mBouncing = 0;
}

/**
 * @note Address: 0x800E20F8
 * @note Size: 0x44
 */
s32 XObjCoin::GetFlagIndex()
{
	return GetPlacementFlagIndex(1);
}

/**
 * @note Address: 0x800E213C
 * @note Size: 0xC0
 */
BOOL XObjItem::LoadFromPlacement(const CoordInfo& info)
{
	BOOL result = XObj::LoadFromPlacement(info);
	if (result && IsFlagSet())
		mStatus.mFlags &= ~0x10000ULL;
	return result;
}

/**
 * @note Address: 0x800E21FC
 * @note Size: 0x44
 */
s32 XObjItem::GetFlagIndex()
{
	return GetPlacementFlagIndex(0);
}
