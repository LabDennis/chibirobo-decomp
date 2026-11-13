#ifndef _GAME_OBJECTFLOORQUERY_H
#define _GAME_OBJECTFLOORQUERY_H

#include "game/xobjmap.h"
#include "game/cgame.h"

extern "C" Vec gObjectFloorOffsetTemplate;
extern "C" Vec lbl_80378148;

static inline Vec ReadFloorCoordinates(const s32* coordinates, const Vec& initial)
{
	Vec result = initial;
	result.x   = coordinates[0] / 100.0f;
	result.y   = coordinates[1] / 100.0f;
	result.z   = coordinates[2] / 100.0f;
	return result;
}

static inline Vec ReadObjectFloorOffset(const XObjFloor& floor)
{
	return ReadFloorCoordinates(floor.mCenterOffset, gObjectFloorOffsetTemplate);
}

static inline Vec ReadObjectFloorExtent(const XObjFloor& floor)
{
	return ReadFloorCoordinates(floor.mExtent, lbl_80378148);
}

static inline Vec ReadObjectFloorOffset(const XObj* object, s32 index)
{
	Vec value = gObjectFloorOffsetTemplate;
	value.x   = object->mFloors[index].mCenterOffset[0] / 100.0f;
	value.y   = object->mFloors[index].mCenterOffset[1] / 100.0f;
	value.z   = object->mFloors[index].mCenterOffset[2] / 100.0f;
	return value;
}

static inline Vec ReadObjectFloorExtent(const XObj* object, s32 index)
{
	Vec value = lbl_80378148;
	value.x   = object->mFloors[index].mExtent[0] / 100.0f;
	value.y   = object->mFloors[index].mExtent[1] / 100.0f;
	value.z   = object->mFloors[index].mExtent[2] / 100.0f;
	return value;
}

static inline void MakeFloorTransform(const XObj* object, const Vec& center, const Vec& extent, f32 yaw, Mtx result)
{
	Mtx world, local, rotate, scale;
	PSMTXTrans(world, object->mMoveTranslate.x, object->mMoveTranslate.y, object->mMoveTranslate.z);
	PSMTXTrans(local, center.x, center.y, center.z);
	PSMTXScale(scale, extent.x, extent.y, extent.z);
	PSMTXRotRad(rotate, 'y', yaw);
	PSMTXConcat(world, rotate, result);
	PSMTXConcat(result, local, result);
	PSMTXConcat(result, scale, result);
}

static inline BOOL FindMapFloor(const XObjMap* map, const Vec* pos, XObjMapHit* hit, Vec* tri)
{
	s32 start = FindFloorStart(map, 0.2f + pos->y);
	for (int i = 0; i < map->mTrigonCount; i++) {
		s32 index    = map->mTrigonOrder[(start + i) % map->mTrigonCount];
		int triangle = ((CTrigon*)&map->mTrigon[index])->Check((Vec*)pos, &hit->mHeight, tri);
		if (triangle >= 0) {
			hit->mTrigon   = index;
			hit->mTriangle = triangle;
			return TRUE;
		}
	}
	return FALSE;
}

static inline u32 ProbeObjectFloor(XObj* obj, const CVec& from, const CVec& to, f32 radius, f32 height, Vec* push, Vec* contact, s32* floor)
{
	return obj->TestFloorCollision(&from, &to, radius, height, push, contact, (s32)floor, 0);
}

#endif
