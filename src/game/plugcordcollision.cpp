#include "game/xobjattachment.h"
#include "game/objectfloorquery.h"
#include "game/cgame.h"
#include "game/cvec.h"

/**
 * @note Address: 0x800D9B10
 * @note Size: 0x44C
 */
extern "C" f32 FindPlugCordFloor(XObjPlugCord* cord, Vec* pos, f32 radius, f32 height)
{
	XObjMap* map = (XObjMap*)gGame.mXObjs.mPools[3].mObjects;
	if (!FindMapFloor(map, pos, &cord->mFloorHit, NULL)) {
		cord->mFloorHit.mTriangle = -1;
		cord->mFloorHit.mTrigon   = -1;
		cord->mFloorHit.mHeight   = pos->y;
	}
	f32 highest = cord->mFloorHit.mHeight;
	CVec contact, push;
	for (int i = 0; i < gGame.mXObjs.mObjectCount; ++i) {
		XObj* object = gGame.GetObject(i);
		if (!(object->mStatus.mFlags & 1) || !(object->mStatus.mFlags & 0x10000) || (object->mStatus.mFlags & 0x100000))
			continue;
		f32 dx = object->mMoveTranslate.x - pos->x;
		f32 dz = object->mMoveTranslate.z - pos->z;
		if (sqrtf(dx * dx + dz * dz) > 100.0f + (radius + object->mFloorXZExtent))
			continue;
		s32 floor = 0;
		while (floor != -1) {
			f32 z = pos->z;
			f32 y = pos->y;
			f32 x = pos->x;
			if (ProbeObjectFloor(object, CVec(x, y, z), CVec(x, y, z), radius, height, &push, &contact, &floor) & 1) {
				f32 floorHeight = object->mFloorHeights[object->mHitFloorIndex];
				if (__fpclassifyf(floorHeight) != FP_NAN && pos->y + 1.0f >= floorHeight && floorHeight >= highest)
					highest = floorHeight;
			}
		}
	}
	if (highest > cord->mFloorHit.mHeight)
		cord->mFloorHit.mHeight = highest;
	return cord->mFloorHit.mHeight;
}
