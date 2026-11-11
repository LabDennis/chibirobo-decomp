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
static inline XObjSource* PlacementSource(const AttrInfo& attr)
{
	return attr._00 ? attr._00->mSource : NULL;
}
static inline bool HasSource(const AttrInfo& attr)
{
	return attr._00 && attr._00->mSource;
}
static inline const char* ModelName(const AttrInfo& attr)
{
	return HasSource(attr) ? attr._00->mSource->mName : NULL;
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

extern "C" BOOL LoadFromPlacement__4XObjFRC9CoordInfo(XObj* object, const CoordInfo* info)
{
	BOOL ready = FALSE;
	if ((u64)info->_00->_34 & 0x80000000ULL)
		return TRUE;

	AttrInfo& attr = object->mAttr;
	if (object->mStatus.mFlags & 1)
		ready = TRUE;
	else {
		if (!attr._00) {
			object->mCoord._00 = info->_00;
			attr._00           = GetObjData(info);
			attr._04           = info->_00;
			if (!(HasSource(attr) ? attr._00->mSource->mArchive : NULL) && ModelName(attr)) {
				CObjArchive* archive = FindFreeObjectArchive(gGame.mObjArchives.mArchives);
				if (attr._00 && attr._00->mSource)
					attr._00->mSource->mArchive = archive;
			}
		}
		if (!object->mFloors) {
			object->mFloors = (XObjFloor*)GetGlobalTextEntry(info->_00->_00);
			object->_250[1] = object->_250[0] = 0;
			BOOL empty                        = object->mFloors ? object->mFloors->mId == -1 : FALSE;
			if (empty)
				object->mFloors = NULL;
		}
		if (!ModelName(attr))
			ready = TRUE;
		else if (PlacementSource(attr)) {
			CObjArchive* archive = HasSource(attr) ? attr._00->mSource->mArchive : NULL;
			const char* name     = ModelName(attr);
			u32 model            = attr._00 ? attr._00->mModel : 0;
			u32 flags            = 0;
			if ((u64)(attr._00 ? attr._00->mFlags : 0) & 0x80)
				flags = 0x10000;
			XObjSource::Cache* cache = HasSource(attr) ? &attr._00->mSource->mCache : NULL;
			u32 address = cache->address, size = cache->size;
			if (gGame.mStatus.mFlags & 0x20000000000ULL) {
				if (address)
					flags |= 4;
				else if (size == 1) {
					flags |= 2;
					address = gARAMCacheInfo.mBaseAddress;
					size    = archive->mFile.mSize;
				}
			}
			char path[64];
			sprintf(path, "%s.dat", name);
			ready = object->Load(archive, path, model, flags, address, size);
			if (ready) {
				if ((flags & 2) && FitsInAram(archive) && attr._00 && attr._00->mSource) {
					attr._00->mSource->mCache.address = gARAMCacheInfo.mBaseAddress;
					attr._00->mSource->mCache.size    = archive->mFile.mSize;
				}
				if (mm::GetName(archive->mFile.mBuffer)[1] != ' ') {
					char label[64];
					sprintf(label, "  %s", mm::GetName(archive->mFile.mBuffer));
					if (flags & 2) {
						if (FitsInAram(archive)) {
							label[0] = 'o';
							mm::SetName(archive->mFile.mBuffer, label);
						} else {
							label[0] = 'x';
							mm::SetName(archive->mFile.mBuffer, label);
						}
					} else if (flags & 4)
						label[0] = '>';
					mm::SetName(archive->mFile.mBuffer, label);
				}
				if ((flags & 2) && FitsInAram(archive)) {
					const char* cachedName = ModelName(attr);
					s32 id                 = attr._04 ? attr._04->_00 : -1;
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
		object->SetPos(PlacementPosition(*info, initial[0]), 1);
		object->SetRot(PlacementRotation(*info, initial[2]), 1);
		object->SetScale(PlacementScale(*info, initial[1]), 1);
		object->mMoveTranslate.mSpeed.Set(0, 0, 0);
		object->mMoveRotate.mSpeed.Set(0, 0, 0);
		object->mStatus.mFlags = 0;
		object->mStatus.mFlags |= (attr._00 ? attr._00->mFlags : 0) | (1ULL | ((u64)info->_00->_34 << 16));
		if (object->mFloors)
			object->mStatus.mFlags |= 0x0200000000000000ULL;
		else
			object->mStatus.mFlags &= ~0x0200000000000000ULL;
		object->_25C = object->mCoord._00->_28;
		object->_260 = object->mCoord._00->_2C;
		if (object->mJObj)
			object->SetAnim(object->mCoord._00->_30, TRUE, NULL, NULL, NULL, 0, 1, 1, 10);
		object->ResetObjectState();
		object->_264.x     = object->mMoveTranslate.x;
		object->_264.y     = object->mMoveTranslate.y;
		object->_264.z     = object->mMoveTranslate.z;
		object->mColors[1] = 0xffffff00;
		object->mColors[0] = 0;
		if (object->mJObj) {
			u32 material[5] = { 0, 0, 0, 0, 0 };
			material[0]     = object->mStatus.mFlags & 0x800000ULL;
			material[1]     = object->mStatus.mFlags & 0x200000ULL;
			material[2]     = object->mStatus.mFlags & 0x40ULL;
			material[3]     = object->mStatus.mFlags & 0x20000ULL;
			material[4]     = object->mStatus.mFlags & 0x400000ULL;
			object->vfD8(material);
			if ((object->mStatus.mFlags & 0x1000000) && IsStageDataLoaded())
				object->vfE0(GetEnvData()[9] / 100.f);
			if ((object->mStatus.mFlags & 2) && object->mJObj)
				HSD_JObjWalkTree(object->mJObj, JObjSetMObjFlags, (f32**)0x04000000);
			UnkStageObj* place = object->mCoord._00;
			s32 index          = place - GetStagePlacementByIndex(0);
			if (index > 0)
				object->_80C = (u32)GetStageScriptByIndex(index);
		}
		UpdateObjectFloorHeights(object);
		UpdateObjectFloorXZExtent(object);
		if ((__fpclassifyf(object->mFloorXZExtent) != FP_NAN && object->mFloorXZExtent < 40.0f) || (object->mStatus.mFlags & 0x400)) {
			s32 kind = object->GetPool();
			if ((u16)(kind - 7) <= 1 || (u16)(kind - 20) <= 1 || (s16)kind == 22)
				object->mStatus.mFlags |= 0x80000000ULL;
		}
		if (object->mStatus.mFlags & 0x2000)
			object->mStatus.mFlags &= ~0x80000000ULL;
		if (object->mStatus.mFlags & 0x8100000000ULL)
			object->mStatus.mFlags |= 0x4000;
		object->mSelectedFloorHeight = NAN;
		object->mSelectedFloorIndex  = 0;
	}
	return object->mStatus.mFlags & 1ULL;
}

/**
 * @note Address: 0x800928E8
 * @note Size: 0xDC
 */
extern "C" void ResetObjectState__4XObjFv(XObj* object)
{
	object->_80C                = 0;
	object->mFloorHit.mTriangle = -1;
	object->mFloorHit.mTrigon   = 0;
	object->mFloorHit.mHeight   = 0.0f;
	object->_794.x              = 0.0f;
	object->_794.y              = 0.0f;
	object->_794.z              = 0.0f;
	object->mAnmQueue.Clear();
	object->mAnchorX = object->mAnchorZ = NAN;
	object->_8B0                        = 0;
	object->_8B0 |= 0x80000000;
	object->mColorStep[0].mValue.x = object->mColorStep[0].mValue.y = object->mColorStep[0].mValue.z = 255.0f;
	object->mColorStep[0].mTarget.x = object->mColorStep[0].mTarget.y = object->mColorStep[0].mTarget.z = 255.0f;
	object->mColorStep[0].mSpeed.x = object->mColorStep[0].mSpeed.y = object->mColorStep[0].mSpeed.z = 0.0f;
	object->mColorStep[1].mValue.x = object->mColorStep[1].mValue.y = object->mColorStep[1].mValue.z = 0.0f;
	object->mColorStep[1].mTarget.x = object->mColorStep[1].mTarget.y = object->mColorStep[1].mTarget.z = 0.0f;
	object->mColorStep[1].mSpeed.x = object->mColorStep[1].mSpeed.y = object->mColorStep[1].mSpeed.z = 0.0f;
}

// XObj::vf90
