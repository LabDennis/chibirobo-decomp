#include "game/xobjattachment.h"

/**
 * @note Address: 0x800DBA7C
 * @note Size: 0x150
 */
extern "C" void InitPlugCord(XObjPlugCord* cord, s32 count, f32 x, f32 y, f32 z, f32 speed)
{
	cord->mCount = count;
	static XObjPlugCordPoint gPlugCordPoints[128];
	cord->mPoints = gPlugCordPoints;
	ResetPlugCordPoints(cord, x, y, z);
	cord->mSpeed        = speed;
	cord->mEndSource[0] = NULL;
	cord->mEndSource[1] = NULL;
	cord->mEndSource[2] = NULL;
	cord->_3C           = sPlugCordZero;
	cord->_40           = sPlugCordZero;
	cord->mEndHeight[0] = 0;
	cord->mEndHeight[1] = 0;
	cord->_54           = 6;
	cord->mWidth        = sPlugCordDefaultWidth;
	cord->mColor = cord->_60 = 255;
	cord->mAlpha             = 0;
	cord->mFlags             = 0;
	cord->mFlags |= 1;
	cord->mFlags &= ~8;
}
