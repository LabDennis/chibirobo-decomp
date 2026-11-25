#include "game/xobjattachment.h"
#include "game/cvec.h"
#include "game/objectfloorquery.h"

/**
 * @note Address: 0x800D9F5C
 * @note Size: 0x574
 */
// Retail rounds the floor rotation before adding the object rotation.
extern "C" BOOL PushPlugCordPointFromObject(CVec* pos, XObj* object, f32 radius)
{
	const f32 margin = 1.0f;
	if (!(object->mStatus.mFlags & 0x0200000000000000ULL))
		return FALSE;
	if (object->mStatus.mFlags & 0x0c00000000000000ULL)
		return FALSE;
	if (object->mStatus.mFlags & 0x4000000000000000ULL)
		return FALSE;
	CVec point(pos->x, 0.0f, pos->z);
	BOOL pushed = FALSE;
	for (int i = 0; object->mFloors[i].mId != -1; ++i) {
		const XObjFloor& floor = object->mFloors[i];
		Vec centerOffset       = object->mMoveScale * ReadObjectFloorOffset(floor);
		Vec size               = object->mMoveScale * ReadObjectFloorExtent(floor);
		f32 yaw                = DEG2RAD * floor.mYawDegrees;
		f32 angle              = object->mMoveRotate.y + yaw;
		switch (floor.mId) {
		case 0:
		case 1: {
			Mtx world;
			CVec center(0, 0, 0);
			MakeFloorTransform(object, centerOffset, size, angle, world);
			PSMTXMultVec(world, &center, &center);
			f32 centerY = center.y;
			center.y    = 0.0f;
			f32 reach   = radius + size.x;
			if (PSVECDistance(&center, &point) < reach) {
				CVec direction = (point - center).Normalize();
				CVec edge      = center + direction * reach;
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
