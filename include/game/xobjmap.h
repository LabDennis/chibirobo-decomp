#ifndef _GAME_XOBJMAP_H
#define _GAME_XOBJMAP_H

#include "game/xobj.h"
#include "game/ctrigon.h"
#include "game/mapfloorhit.h"
#include "game/clight.h"
#include "game/xobjlight.h"
#include "game/stagedata.h"

// todo: cleanup, EnvInfo lives in stage.h which breaks next to the hsd headers
// its data pointer is the first field, data[4] picks who owns the stage lights, 1 the map
// and 2 the light objects, data[5] has a bit per light
static inline u8* GetEnvData()
{
	return *(u8**)GetStageEnvironment();
}

// a stage's map model, class name from RTTI and the vtable at 0x80225F74. its floors are
// up to 16 CTrigons, sorted by height. offsets are usa, japan is 0x10 lower
class XObjMap : public XObj {
public:
	// _00-_980 = XObj
	s32 mTrigonCount;     // _980
	s32 mTrigonOrder[16]; // _984, lowest mMinY first
	CTrigon mTrigon[16];  // _9C4
	s32 mLightNum;        // _B84
	CLight mLights[8];    // _B88, from desc lists 1 to 8
	s32 mLightAnims[8];   // _2648

	XObjMap();

	virtual ~XObjMap();                                    // _08
	virtual void Release();                                // _54
	virtual void InitializeObject(u32 id);                 // _90
	virtual void ResetObjectState();                       // _98
	virtual BOOL LoadFromPlacement(const CoordInfo& info); // _9C
	virtual void UpdateObject(u32 frames);                 // _A4

	// todo: cleanup
	XObjLightDesc* GetLightDesc(s32 i) { return (XObjLightDesc*)mArchive.mDesc[i + 1]; }
	s32 GetLightAnimNum(s32 i) { return mArchive.mNum[i + 2]; }
	CLight* GetLights() { return mLightNum > 0 ? mLights : NULL; }
};
#if defined(VERSION_JP)
STATIC_ASSERT(sizeof(XObjMap) == 0x2658);
#else
STATIC_ASSERT(sizeof(XObjMap) == 0x2668);
#endif

// the first floor at or under a height, the floors are sorted from the highest down
static inline s32 FindFloorStart(const XObjMap* map, f32 y)
{
	for (int i = 0; i < map->mTrigonCount; i++) {
		if (map->mTrigon[map->mTrigonOrder[i]].mMinY <= y) {
			return i;
		}
	}
	return 0;
}

extern "C" {
// the floor color of one of the map's triangles, 1 to 12, 0 for none
s32 GetMapTriangleFloorType(XObjMap* map, s32 trigon, s32 tri);
}

#endif
