#include "game/xobjattachment.h"
#include "game/cvec.h"
#include "game/cgame.h"

extern "C" {
extern Vec gPlugFloorOffsetTemplate, gPlugFloorExtentTemplate;
}
static inline Vec FloorOffset(XObj* obj, int i)
{
	Vec v = gPlugFloorOffsetTemplate;
	v.x   = obj->mFloors[i].mCenterOffset[0] / 100.0f;
	v.y   = obj->mFloors[i].mCenterOffset[1] / 100.0f;
	v.z   = obj->mFloors[i].mCenterOffset[2] / 100.0f;
	return v;
}
static inline Vec FloorExtent(XObj* obj, int i)
{
	Vec v = gPlugFloorExtentTemplate;
	v.x   = obj->mFloors[i].mExtent[0] / 100.0f;
	v.y   = obj->mFloors[i].mExtent[1] / 100.0f;
	v.z   = obj->mFloors[i].mExtent[2] / 100.0f;
	return v;
}
/**
 * @note Address: 0x800D9F5C
 * @note Size: 0x574
 */
// Retail rounds the floor rotation before adding the object rotation.
extern "C" BOOL PushPlugCordPointFromObject(CVec* pos, XObj* object, f32 radius)
{
	static f32 margin = 1.0f;
	if (!(object->mStatus.mFlags & 0x0200000000000000ULL))
		return FALSE;
	if (object->mStatus.mFlags & 0x0c00000000000000ULL)
		return FALSE;
	if (object->mStatus.mFlags & 0x4000000000000000ULL)
		return FALSE;
	CVec point, edge;
	point.x     = pos->x;
	point.y     = pos->y;
	point.z     = pos->z;
	point.y     = 0.0f;
	BOOL pushed = FALSE;
	for (int i = 0; object->mFloors[i].mId != -1; ++i) {
		CVec origin(object->mMoveTranslate);
		Vec centerOffset = object->mMoveScale * FloorOffset(object, i);
		Vec size         = object->mMoveScale * FloorExtent(object, i);
		f32 yaw          = DEG2RAD * object->mFloors[i].mYawDegrees;
		f32 angle        = object->mMoveRotate.y + yaw;
		switch (object->mFloors[i].mId) {
		case 0:
		case 1: {
			Mtx translate, local, rotate, scale, world;
			CVec center(0, 0, 0);
			PSMTXTrans(translate, origin.x, origin.y, origin.z);
			PSMTXTrans(local, centerOffset.x, centerOffset.y, centerOffset.z);
			PSMTXScale(scale, size.x, size.y, size.z);
			PSMTXRotRad(rotate, 'y', angle);
			PSMTXConcat(translate, rotate, world);
			PSMTXConcat(world, local, world);
			PSMTXConcat(world, scale, world);
			PSMTXMultVec(world, &center, &center);
			f32 centerY = center.y;
			center.y    = 0.0f;
			f32 reach   = radius + size.x;
			if (PSVECDistance(&center, &point) < reach) {
				CVec direction = (point - center).Normalize();
				edge.Set(center + direction * reach);
				if (pos->y <= centerY + size.y + margin && pos->y >= centerY - size.y - margin) {
					pos->x = edge.x;
					pos->z = edge.z;
					pushed = TRUE;
				}
			}
			break;
		}
		}
	}
	return pushed;
}
