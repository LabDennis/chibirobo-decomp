#include "game/xchr.h"

/**
 * @note Address: 0x800BD01C
 * @note Size: 0x160
 */
f32 XObjChr::GetCollisionHeight()
{
	const XObjFloor& floor = mFloors[0];
	f32 centerY            = floor.mCenterOffset[1] / 100.0f;
	f32 extentY            = floor.mExtent[1] / 100.0f;
	return mMoveScale.x * (centerY + extentY);
}
