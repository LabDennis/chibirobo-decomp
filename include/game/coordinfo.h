#ifndef _GAME_COORDINFO_H
#define _GAME_COORDINFO_H

#include "types.h"

// one 0x38 byte entry of the stage's object placement table
struct UnkStageObj {
	s32 mObjectId; // _00, the object id, negative ends the table
	s32 mPos[3];   // _04, in hundredths
	s32 mRot[3];   // _10, rotation in whole degrees, x y z
	s32 mScale[3]; // _1C, in hundredths
	s32 mParam0; // _28, first per-object placement parameter, copied to XObj _25C; GetObjectPlacementParam(obj, 0) returns it (spider mode,
	             // custom ledge height)
	s32 mParam1; // _2C, second per-object placement parameter, copied to XObj _260; GetObjectPlacementParam(obj, 1) returns it; coins, dust
	             // and hbox use it as their collection flag id, the spider as a height offset
	s32 mStartAnim; // _30, starting anim
	u32 mFlags;     // _34, flags, 0x80000000 skips the read
};
STATIC_ASSERT(sizeof(UnkStageObj) == 0x38);

// name from __vt__9CoordInfo
class CoordInfo {
public:
	UnkStageObj* mPlacement; // _00, the object's entry in the stage placement table
	// _04 vtable

	CoordInfo() { mPlacement = NULL; }
	CoordInfo(void* value)
	    : mPlacement((UnkStageObj*)value)
	{
	}
	virtual ~CoordInfo() { } // _08
};

// an entry of objTable or objTableEx, 0x18 bytes. the kind record an object points at
struct XObjSource;
struct UnkObjData {
	XObjSource* mSource; // _00
	u32 mModel;          // _04
	u32 mFlags;          // _08
	u32 mShadowSize;     // _0C, shadow size
	s16 mLookPriority;   // _10
	s16 mLookRange;      // _12
	s16 mCategory;       // _14, category, also which pool in XOBJS
	s16 mItemIndex;      // _16, item index, some callers test it as flags

	s16 GetCategory() const { return this != NULL ? mCategory : 0; }
	s16 GetItem() const { return this != NULL ? mItemIndex : 0; }
};
STATIC_ASSERT(sizeof(UnkObjData) == 0x18);
extern "C" UnkObjData* GetObjData(const CoordInfo* info);

// todo: cleanup
inline s32 GetObjDataPool(const UnkObjData* data)
{
	return data != NULL ? data->mCategory : 0;
}
inline s32 GetObjDataFlags(const UnkObjData* data)
{
	return data != NULL ? data->mItemIndex : 0;
}

// name from __vt__8AttrInfo
class AttrInfo {
public:
	UnkObjData* mObjData; // _00, the object's data record from GetObjData (source, shadow size, look range, category, item), NULL until set
	UnkStageObj* mPlacement; // _04, the placement CoordInfo points at
	// _08 vtable

	AttrInfo() { mObjData = NULL; }
	AttrInfo(CoordInfo* info)
	    : mObjData(GetObjData(info))
	    , mPlacement(info->mPlacement)
	{
	}
	AttrInfo(const CoordInfo& info)
	    : mObjData(GetObjData(&info))
	    , mPlacement(info.mPlacement)
	{
	}
	virtual ~AttrInfo() { } // _08
};

extern "C" UnkStageObj* GetStagePlacementByIndex(s32 i);

// Spawned objects can use placements in the executable, outside the loaded stage table.
inline s32 GetStagePlacementIndex(const UnkStageObj* placement)
{
	const UnkStageObj* first = GetStagePlacementByIndex(0);
	s32 byteOffset           = (s32)((u32)placement - (u32)first);
	return byteOffset / (s32)sizeof(UnkStageObj);
}

#endif
