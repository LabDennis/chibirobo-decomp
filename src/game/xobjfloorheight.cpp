#include "game/xobj.h"
#include "game/xobjchr.h"

static inline Vec FloorCenterOffset(const XObjFloor* record)
{
	Vec value = gPlayerFloorCenterDefault;
	value.x   = record->mCenterOffset[0] / 100.0f;
	value.y   = record->mCenterOffset[1] / 100.0f;
	value.z   = record->mCenterOffset[2] / 100.0f;
	return value;
}

static inline Vec FloorExtent(const XObjFloor* record)
{
	Vec value = gPlayerFloorExtentDefault;
	value.x   = record->mExtent[0] / 100.0f;
	value.y   = record->mExtent[1] / 100.0f;
	value.z   = record->mExtent[2] / 100.0f;
	return value;
}

/**
 * @note Address: 0x800BD01C
 * @note Size: 0x160
 */
extern "C" f32 GetCollisionHeight__7XObjChrFv(const XObj* object)
{
	return object->mMoveScale.x * (FloorCenterOffset(object->mFloors).y + FloorExtent(object->mFloors).y);
}
