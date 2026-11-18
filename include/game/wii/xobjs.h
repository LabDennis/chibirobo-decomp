#ifndef GAME_WII_XOBJS_H
#define GAME_WII_XOBJS_H

#include "types.h"

class XObj;
class CoordInfo;

// one of XOBJS's object pools, same layout as the GameCube one
struct XObjPool {
	u8* mObjects;      // _00
	s32 mUsed;         // _04, objects handed out so far
	s32 mCount;        // _08
	const char* mName; // _0C
	s32 mStride;       // _10, size of one object

	void Set(void* objects, s32 count, const char* name, s32 stride)
	{
		mObjects = (u8*)objects;
		mUsed    = 0;
		mCount   = count;
		mName    = name;
		mStride  = stride;
	}

	XObj* Get(s32 i) const { return (XObj*)(mObjects + i * mStride); }

	// first object whose status flags match value under mask, -1 without one; xobjs.cpp holds the body
	s32 Find(u64 mask, u64 value) const;
};
STATIC_ASSERT(sizeof(XObjPool) == 0x14);

// the objects: a pool per kind and the table of the ones in use, same layout as
// the GameCube XOBJS
class XOBJS {
public:
	XObjPool mPools[0x17]; // _000
	XObj* mObjects[0x400]; // _1CC, the object table
	s32 mObjectCount;      // _11CC
	s32 mLoadObjectIndex;  // _11D0
	s32 mLoadState;        // _11D4, UpdateStageObjectLoading's state, 0 when idle
	// _11D8 vtable

	virtual ~XOBJS() { }

	void ReadAll(); // fills the pools and the object table from the stage placements, then loads them
};
STATIC_ASSERT(sizeof(XOBJS) == 0x11DC);

extern "C" {
void ResetPooledObjects(XObjPool* pools);
s32 UpdateStageObjectLoading(XOBJS* list);
void ReleaseStageObjectList(XOBJS* list);
XObj* SpawnObjectFromPlacement(XOBJS* list, CoordInfo* info, s32 direct, BOOL async);
s32 FindStageObjectIndex(XOBJS* list, XObj* obj);
}

#endif
