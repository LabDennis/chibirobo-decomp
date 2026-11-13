#ifndef _GAME_COORDINFO_H
#define _GAME_COORDINFO_H

#include "types.h"

// one 0x38 byte entry of the stage's object placement table
struct UnkStageObj {
	s32 mObjectId;  // _00, the object id, negative ends the table
	s32 mPos[3];    // _04, in hundredths
	s32 _10[3];     // _10
	s32 mScale[3];  // _1C, in hundredths
	s32 _28;        // _28
	s32 _2C;        // _2C
	s32 mStartAnim; // _30, starting anim
	u32 mFlags;     // _34, flags, 0x80000000 skips the read
};
STATIC_ASSERT(sizeof(UnkStageObj) == 0x38);

// name from __vt__9CoordInfo
class CoordInfo {
public:
	UnkStageObj* _00; // _00
	// _04 vtable

	CoordInfo() { _00 = NULL; }
	CoordInfo(void* value)
	    : _00((UnkStageObj*)value)
	{
	}
	virtual ~CoordInfo() { } // _08
};

// an entry of objTable or objTableEx, 0x18 bytes. the kind record an object points at
struct XObjSource;
struct UnkObjData {
	union {
		u32 _00;
		XObjSource* mSource;
	}; // _00
	u32 mModel;        // _04
	u32 mFlags;        // _08
	u32 mShadowSize;   // _0C, shadow size
	s16 mLookPriority; // _10
	s16 mLookRange;    // _12
	s16 mCategory;     // _14, category, also which pool in XOBJS
	s16 mItemIndex;    // _16, item index, some callers test it as flags

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
	UnkObjData* _00;         // _00
	UnkStageObj* mPlacement; // _04, the placement CoordInfo points at
	// _08 vtable

	AttrInfo() { _00 = NULL; }
	AttrInfo(CoordInfo* info)
	    : _00(GetObjData(info))
	    , mPlacement(info->_00)
	{
	}
	AttrInfo(const CoordInfo& info)
	    : _00(GetObjData(&info))
	    , mPlacement(info._00)
	{
	}
	virtual ~AttrInfo() { } // _08
};

extern "C" UnkStageObj* GetStagePlacementByIndex(s32 i);

#endif
