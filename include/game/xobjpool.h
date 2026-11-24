#ifndef _GAME_XOBJPOOL_H
#define _GAME_XOBJPOOL_H

#include "Version.h"
#include "types.h"

#if defined(VERSION_R24J01)
class XObj;
class CEvtFlags;
#else
#include "game/xobjflags.h"
#endif

// one of XOBJS's object pools, all its objects the same size in one block
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

	XObj* First() const { return (XObj*)mObjects; }
	XObj* Get(s32 i) const { return (XObj*)(mObjects + i * mStride); }

	// the first object whose flags match, -1 when there's none
#if defined(VERSION_R24J01)
	s32 Find(u64 mask, u64 value) const; // xobjs.cpp holds the body
#else
	s32 Find(u64 mask, u64 value) const
	{
		for (s32 i = 0; i < mCount; i++) {
			if ((Get(i)->mStatus.mFlags & mask) == value) {
				return i;
			}
		}
		return -1;
	}
#endif
};
STATIC_ASSERT(sizeof(XObjPool) == 0x14);

// the objects: a pool per kind and the table of the ones in use, name from RTTI
class XOBJS {
public:
	// the collections with a flag bit per placement: the coin, item and dust pools each
	// lead with a flags object that carries the CEvtFlags
	enum CollectionPool {
		COINS = 7,
		ITEMS = 8,
		DUST  = 20,
	};

	XObjPool mPools[0x17]; // _000
	XObj* mObjects[0x400]; // _1CC, the object table
	s32 mObjectCount;      // _11CC
	s32 mLoadObjectIndex;  // _11D0
	s32 mLoadState;        // _11D4, UpdateStageObjectLoading's state, 0 when idle
	// _11D8 vtable

	virtual ~XOBJS() { } // _08

#if defined(VERSION_R24J01)
	// the definition needs XObjFlags, so the units that use it hold it
	CEvtFlags& GetCollectionFlags(CollectionPool pool) const;
#else
	CEvtFlags& GetCollectionFlags(CollectionPool pool) const { return *static_cast<XObjFlags*>(mPools[pool].First())->mFlags; }
#endif

	void ReadAll(); // fills the pools and the object table from the stage placements, then loads them
};
STATIC_ASSERT(sizeof(XOBJS) == 0x11DC);

#endif
