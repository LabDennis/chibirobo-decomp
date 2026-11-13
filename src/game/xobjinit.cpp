#include "game/xobj.h"
#include "game/cjobjset.h"
#include "game/cgame.h"
#include "game/hsd_dobj.h"
#include "game/globals.h"

#include "game/spiderplacement.h"
#include "game/mm.h"
#include "game/arcache.h"
#include "game/xobjmap.h"
#include <stdio.h>

extern "C" {

CObjArchive* FindFreeObjectArchive(CObjArchive*);
u32 ARGetSize(void);
u8* GetGlobalTextEntry(s32);
u8* GetStageScriptByIndex(s32);
BOOL IsStageDataLoaded();
}
static inline bool HasSource(const AttrInfo& attr)
{
	return attr.mObjData && attr.mObjData->mSource;
}
static inline const char* ModelName(const AttrInfo& attr)
{
	return HasSource(attr) ? attr.mObjData->mSource->mName : NULL;
}
static inline u32 PaddedSize(CObjArchive* archive)
{
	return (archive->mFile.mSize + 31) & ~31u;
}
static inline bool FitsInAram(CObjArchive* archive)
{
	u32 size     = PaddedSize(archive);
	u32 capacity = ARGetSize();
	return gARAMCacheInfo.mBaseAddress + ((size + 31) & ~31u) < capacity;
}

BOOL XObj::LoadFromPlacement(const CoordInfo& info)
{
	BOOL ready = FALSE;
	if ((u64)info.mPlacement->mFlags & 0x80000000ULL) // the placement opts out of loading
		return TRUE;

	AttrInfo& attr = mAttr;
	if (mStatus.mFlags & 1)
		ready = TRUE;
	else {
		if (!attr.mObjData) {
			mCoord.mPlacement = info.mPlacement;
			attr.mObjData     = GetObjData(&info);
			attr.mPlacement   = info.mPlacement;
			if (!(HasSource(attr) ? attr.mObjData->mSource->mArchive : NULL) && ModelName(attr)) {
				CObjArchive* archive = FindFreeObjectArchive(gGame.mObjArchives.mArchives);
				if (attr.mObjData && attr.mObjData->mSource)
					attr.mObjData->mSource->mArchive = archive;
			}
		}
		if (!mFloors) {
			mFloors        = (XObjFloor*)GetGlobalTextEntry(info.mPlacement->mObjectId);
			mFloorMasks[1] = mFloorMasks[0] = 0;
			BOOL empty                      = mFloors ? mFloors->mId == -1 : FALSE;
			if (empty) // a -1 id means no floor list
				mFloors = NULL;
		}
		if (!ModelName(attr))
			ready = TRUE;
		else if (attr.mObjData ? attr.mObjData->mSource : NULL) {
			CObjArchive* archive = HasSource(attr) ? attr.mObjData->mSource->mArchive : NULL;
			const char* name     = ModelName(attr);
			u32 model            = attr.mObjData ? attr.mObjData->mModel : 0;
			u32 flags            = 0;
			if ((u64)(attr.mObjData ? attr.mObjData->mFlags : 0) & 0x80) // obj flag 0x80 becomes load flag 0x10000
				flags = 0x10000;
			XObjSource::Cache* cache = HasSource(attr) ? &attr.mObjData->mSource->mCache : NULL;
			u32 address = cache->address, size = cache->size;
			if (gGame.mStatus.mFlags & 0x20000000000ULL) { // ARAM caching on
				if (address)
					flags |= 4; // an ARAM address is already known
				else if (size == 1) {
					flags |= 2; // load into ARAM
					address = gARAMCacheInfo.mBaseAddress;
					size    = archive->mFile.mSize;
				}
			}
			char path[64];
			sprintf(path, "%s.dat", name);
			ready = Load(archive, path, model, flags, address, size);
			if (ready) {
				if ((flags & 2) && FitsInAram(archive) && attr.mObjData && attr.mObjData->mSource) {
					attr.mObjData->mSource->mCache.address = gARAMCacheInfo.mBaseAddress; // remember the ARAM copy
					attr.mObjData->mSource->mCache.size    = archive->mFile.mSize;
				}
				if (mm::GetName(archive->mFile.mBuffer)[1] != ' ') { // untagged archives get a state tag
					char label[64];
					sprintf(label, "  %s", mm::GetName(archive->mFile.mBuffer));
					if (flags & 2) {
						if (FitsInAram(archive)) {
							label[0] = 'o'; // cached into ARAM
							mm::SetName(archive->mFile.mBuffer, label);
						} else {
							label[0] = 'x'; // wouldn't fit
							mm::SetName(archive->mFile.mBuffer, label);
						}
					} else if (flags & 4)
						label[0] = '>'; // reused an ARAM address
					mm::SetName(archive->mFile.mBuffer, label);
				}
				if ((flags & 2) && FitsInAram(archive)) {
					const char* cachedName = ModelName(attr);
					s32 id                 = attr.mPlacement ? attr.mPlacement->mObjectId : -1;
					u32 padded             = PaddedSize(archive);

					gARAMCacheInfo.mEntries[gARAMCacheInfo.mCount].address  = gARAMCacheInfo.mBaseAddress;
					gARAMCacheInfo.mEntries[gARAMCacheInfo.mCount].size     = padded;
					gARAMCacheInfo.mEntries[gARAMCacheInfo.mCount].objectId = id;
					gARAMCacheInfo.mEntries[gARAMCacheInfo.mCount].name     = cachedName;
					++gARAMCacheInfo.mCount;
					gARAMCacheInfo.mBaseAddress += padded;
				}
			}
		}
	}
	if (ready) {
		static Vec initial[3];
		SetPos(PlacementPosition(info, initial[0]), 1);
		SetRot(PlacementRotation(info, initial[2]), 1);
		SetScale(PlacementScale(info, initial[1]), 1);
		mMoveTranslate.mSpeed.Set(0, 0, 0);
		mMoveRotate.mSpeed.Set(0, 0, 0);
		mStatus.mFlags = 0;
		mStatus.mFlags |= (attr.mObjData ? attr.mObjData->mFlags : 0) | (1ULL | ((u64)info.mPlacement->mFlags << 16));
		if (mFloors)
			mStatus.mFlags |= 0x0200000000000000ULL;
		else
			mStatus.mFlags &= ~0x0200000000000000ULL;
		mPlacementParam0 = mCoord.mPlacement->mParam0;
		mPlacementParam1 = mCoord.mPlacement->mParam1;
		if (mJObj)
			SetAnim(mCoord.mPlacement->mStartAnim, TRUE, NULL, NULL, NULL, 0, 1, 1, 10);
		ResetObjectState();
		mLastPos.x = mMoveTranslate.x;
		mLastPos.y = mMoveTranslate.y;
		mLastPos.z = mMoveTranslate.z;
		mColors[1] = 0xffffff00; // times white: untinted
		mColors[0] = 0;
		if (mJObj) {
			u32 material[5] = { 0, 0, 0, 0, 0 };
			material[0]     = mStatus.mFlags & 0x800000ULL;
			material[1]     = mStatus.mFlags & 0x200000ULL;
			material[2]     = mStatus.mFlags & 0x40ULL;
			material[3]     = mStatus.mFlags & 0x20000ULL;
			material[4]     = mStatus.mFlags & 0x400000ULL;
			ApplyMaterialPresets(material); // the status bits pick the material presets
			if ((mStatus.mFlags & 0x1000000) && IsStageDataLoaded())
				EnableLighting(GetEnvData()[9] / 100.f); // env slot 9 is the light level in percent
			if ((mStatus.mFlags & 2) && mJObj)
				HSD_JObjWalkTree(mJObj, JObjSetMObjFlags, (f32**)0x04000000);
			UnkStageObj* place = mCoord.mPlacement;
			s32 index          = place - GetStagePlacementByIndex(0); // the placement's index in the stage
			if (index > 0)
				mEventScript = (u32)GetStageScriptByIndex(index);
		}
		UpdateObjectFloorHeights(this);
		UpdateObjectFloorXZExtent(this);
		if ((__fpclassifyf(mFloorXZExtent) != FP_NAN && mFloorXZExtent < 40.0f) || (mStatus.mFlags & 0x400)) {
			s32 kind = GetPool();
			if ((u16)(kind - 7) <= 1 || (u16)(kind - 20) <= 1 || (s16)kind == 22) // pools 7/8, 20/21 and 22
				mStatus.mFlags |= 0x80000000ULL;
		}
		if (mStatus.mFlags & 0x2000) // flag 0x2000 overrides the small-extent flag
			mStatus.mFlags &= ~0x80000000ULL;
		if (mStatus.mFlags & 0x8100000000ULL) // either high bit maps to flag 0x4000
			mStatus.mFlags |= 0x4000;
		mSelectedFloorHeight = NAN;
		mSelectedFloorIndex  = 0;
	}
	return mStatus.mFlags & 1ULL;
}

/**
 * @note Address: 0x800928E8
 * @note Size: 0xDC
 */
void XObj::ResetObjectState()
{
	mEventScript        = 0;
	mFloorHit.mTriangle = -1;
	mFloorHit.mTrigon   = 0;
	mFloorHit.mHeight   = 0.0f;
	mToppleOffset.x     = 0.0f;
	mToppleOffset.y     = 0.0f;
	mToppleOffset.z     = 0.0f;
	mAnmQueue.Clear();
	mAnchorX = mAnchorZ = NAN;
	mMarkerOverride     = 0;
	mMarkerOverride |= 0x80000000; // no marker override
	mColorStep[0].mValue.x = mColorStep[0].mValue.y = mColorStep[0].mValue.z = 255.0f;
	mColorStep[0].mTarget.x = mColorStep[0].mTarget.y = mColorStep[0].mTarget.z = 255.0f;
	mColorStep[0].mSpeed.x = mColorStep[0].mSpeed.y = mColorStep[0].mSpeed.z = 0.0f;
	mColorStep[1].mValue.x = mColorStep[1].mValue.y = mColorStep[1].mValue.z = 0.0f;
	mColorStep[1].mTarget.x = mColorStep[1].mTarget.y = mColorStep[1].mTarget.z = 0.0f;
	mColorStep[1].mSpeed.x = mColorStep[1].mSpeed.y = mColorStep[1].mSpeed.z = 0.0f;
}

// XObj::vf90
