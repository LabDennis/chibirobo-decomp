#include "game/xobjattachment.h"

/**
 * @note Address: 0x800DBA7C
 * @note Size: 0x150
 */
extern "C" void InitPlugCord(XObjPlugCord* cord, s32 count, f32 x, f32 y, f32 z, f32 speed)
{
	cord->mCount = count;
	static XObjPlugCordPoint points[128];
	cord->mPoints = points;
	ResetPlugCordPoints(cord, x, y, z);
	cord->mSpeed        = speed;
	cord->mEndSource[0] = NULL;
	cord->mEndSource[1] = NULL;
	cord->mEndSource[2] = NULL;
	cord->_3C           = 0.0f;
	cord->_40           = 0.0f;
	cord->mEndHeight[0] = NULL;
	cord->mEndHeight[1] = NULL;
	cord->_54           = 6;
	cord->mWidth        = 0.2f;
	cord->mColor        = 255;
	cord->_60           = 255;
	cord->mAlpha        = 0;
	cord->mFlags        = XObjPlugCord::UpdateColor;
}
