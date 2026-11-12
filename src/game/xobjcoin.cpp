#include "game/xobjflags.h"
#include "game/hsd_cobj.h"
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
	_260 = info._00->_2C;
	if (IsFlagSet())
		return TRUE;
	if (LoadFromPlacement__4XObjFRC9CoordInfo(this, &info)) {
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
	ResetObjectState__4XObjFv(this);
	s32 sign       = rand() & 1 ? 1 : -1;
	mSpin          = (DEG2RAD * (rand() % 360)) * sign;
	HSD_AObj* aobj = GetAObj(mJObj);
	f32 end        = aobj != NULL ? HSD_AObjGetEndFrame(aobj) : 0.0f;
	if (end > 0.0f) {
		f32 frame              = fmod((f32)rand(), end);
		UnkStageObj* placement = mCoord._00;
		SetAnim(placement->_30, TRUE, NULL, NULL, NULL, frame, 1.0f, 1.0f, 10.0f);
	}
	_98C      = 0.0f;
	mState    = 0;
	mParam0   = 0;
	mBouncing = 0;
}

/**
 * @note Address: 0x800E20F8
 * @note Size: 0x44
 */
extern "C" s32 GetFlagIndex__8XObjCoinFv(XObjCoin* coin)
{
	return coin->GetPlacementFlagIndex(1);
}

/**
 * @note Address: 0x800E213C
 * @note Size: 0xC0
 */
BOOL XObjItem::LoadFromPlacement(const CoordInfo& info)
{
	BOOL result = LoadFromPlacement__4XObjFRC9CoordInfo(this, &info);
	if (result && IsFlagSet())
		mStatus.mFlags &= ~0x10000ULL;
	return result;
}

/**
 * @note Address: 0x800E21FC
 * @note Size: 0x44
 */
extern "C" s32 GetFlagIndex__8XObjItemFv(XObjItem* item)
{
	return item->GetPlacementFlagIndex(0);
}
