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
extern "C" BOOL LoadFromPlacement__8XObjCoinFRC9CoordInfo(XObjCoin* coin, CoordInfo* info)
{
	coin->_260 = info->_00->_2C;
	if (coin->IsFlagSet())
		return TRUE;
	if (LoadFromPlacement__4XObjFRC9CoordInfo(coin, info)) {
		coin->mStatus.mFlags |= 0x10000000ULL;
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x800E1F30
 * @note Size: 0x1C8
 */
extern "C" void ResetObjectState__8XObjCoinFv(XObjCoin* coin)
{
	ResetObjectState__4XObjFv(coin);
	s32 sign       = rand() & 1 ? 1 : -1;
	coin->mSpin    = (DEG2RAD * (rand() % 360)) * sign;
	HSD_AObj* aobj = GetAObj(coin->mJObj);
	f32 end        = aobj != NULL ? HSD_AObjGetEndFrame(aobj) : 0.0f;
	if (end > 0.0f) {
		f32 frame              = fmod((f32)rand(), end);
		UnkStageObj* placement = coin->mCoord._00;
		coin->SetAnim(placement->_30, TRUE, NULL, NULL, NULL, frame, 1.0f, 1.0f, 10.0f);
	}
	coin->_98C      = 0.0f;
	coin->mState    = 0;
	coin->mParam0   = 0;
	coin->mBouncing = 0;
}

/**
 * @note Address: 0x800E20F8
 * @note Size: 0x44
 */
extern "C" s32 GetFlagIndex__8XObjCoinFv(XObjCoin* coin)
{
	s32 id = coin->_260;
	if (id > 0)
		return (s32)((u32)GetStageCollectionFlagBase(1) + (u32)id);
	return 0;
}

/**
 * @note Address: 0x800E213C
 * @note Size: 0xC0
 */
extern "C" BOOL LoadFromPlacement__8XObjItemFRC9CoordInfo(XObjItem* item, CoordInfo* info)
{
	BOOL result = LoadFromPlacement__4XObjFRC9CoordInfo(item, info);
	if (result && item->IsFlagSet())
		item->mStatus.mFlags &= ~0x10000ULL;
	return result;
}

/**
 * @note Address: 0x800E21FC
 * @note Size: 0x44
 */
extern "C" s32 GetFlagIndex__8XObjItemFv(XObjItem* item)
{
	s32 id = item->_260;
	if (id > 0)
		return (s32)((u32)GetStageCollectionFlagBase(0) + (u32)id);
	return 0;
}
