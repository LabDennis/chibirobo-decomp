#include "game/xobj.h"
#include "game/cgame.h"
#include "game/hsd_dobj.h"
#include "game/placement.h"
#include "game/mm.h"
#include "game/arcache.h"
#include "game/stagedata.h"

#include <dolphin/ar.h>
#include <stdio.h>

extern "C" CObjArchive* FindFreeObjectArchive(CObjArchive* archives);

namespace {
enum ArchiveLoadFlags {
	CacheArchiveInAram     = 2,
	ReadArchiveFromAram    = 4,
	LoadInterpolationModel = 0x10000,
};
}

BOOL XObj::LoadFromPlacement(const CoordInfo& info)
{
	const StagePlacement& placement = *info.mPlacement;
	if (placement.mFlags & 0x80000000)
		return TRUE;

	BOOL ready = TRUE;
	if (!(mStatus.mFlags & 1)) {
		if (mAttr.mObjData == NULL) {
			mCoord.mPlacement  = info.mPlacement;
			mAttr.mObjData     = GetObjData(&info);
			mAttr.mPlacement   = info.mPlacement;
			XObjSource* source = mAttr.mObjData != NULL ? mAttr.mObjData->mSource : NULL;
			if (source != NULL && source->mName != NULL && source->mArchive == NULL) {
				source->mArchive = FindFreeObjectArchive(gGame.mObjArchives.mArchives);
			}
		}
		if (mFloors == NULL) {
			mFloors        = reinterpret_cast<XObjFloor*>(GetGlobalTextEntry(placement.mObjectId));
			mFloorMasks[1] = mFloorMasks[0] = 0;
			if (mFloors != NULL && mFloors->mId == -1)
				mFloors = NULL;
		}

		XObjSource* source = mAttr.mObjData != NULL ? mAttr.mObjData->mSource : NULL;
		if (source != NULL && source->mName != NULL) {
			CObjArchive* archive = source->mArchive;
			u32 flags            = (mAttr.mObjData->mFlags & 0x80) ? LoadInterpolationModel : 0;
			u32 address          = source->mCache.address;
			u32 size             = source->mCache.size;
			if (gGame.mStatus.mFlags & (1ULL << 41)) {
				if (address != 0) {
					flags |= ReadArchiveFromAram;
				} else if (size == 1) {
					flags |= CacheArchiveInAram;
					address = gARAMCacheInfo.mBaseAddress;
					size    = archive->mFile.mSize;
				}
			}
			char path[64];
			sprintf(path, "%s.dat", source->mName);
			ready = Load(archive, path, mAttr.mObjData->mModel, flags, address, size);
			if (ready) {
				const u32 fileSize   = archive->mFile.mSize;
				const u32 paddedSize = (fileSize + 31) & ~31u;
				const bool cached    = (flags & CacheArchiveInAram) && gARAMCacheInfo.mBaseAddress + paddedSize < ARGetSize();
				source               = mAttr.mObjData != NULL ? mAttr.mObjData->mSource : NULL;
				if (cached && source != NULL) {
					source->mCache.address = gARAMCacheInfo.mBaseAddress;
					source->mCache.size    = fileSize;
				}

				void* buffer     = archive->mFile.mBuffer;
				const char* name = mm::GetName(buffer);
				if (name[1] != ' ') {
					char label[64];
					sprintf(label, "  %s", name);
					if (flags & CacheArchiveInAram)
						label[0] = cached ? 'o' : 'x';
					else if (flags & ReadArchiveFromAram)
						label[0] = '>';
					mm::SetName(buffer, label);
				}

				if (cached) {
					ARCacheEntry& entry = gARAMCacheInfo.mEntries[gARAMCacheInfo.mCount];
					entry.address       = gARAMCacheInfo.mBaseAddress;
					entry.size          = paddedSize;
					entry.objectId      = mAttr.mPlacement != NULL ? mAttr.mPlacement->mObjectId : -1;
					entry.name          = source != NULL ? source->mName : NULL;
					++gARAMCacheInfo.mCount;
					gARAMCacheInfo.mBaseAddress += paddedSize;
				}
			}
		}
	}
	if (ready) {
		SetPos(PlacementPosition(info), 1);
		SetRot(PlacementRotation(info), 1);
		SetScale(PlacementScale(info), 1);
		mMoveTranslate.mSpeed.Set(0, 0, 0);
		mMoveRotate.mSpeed.Set(0, 0, 0);
		mStatus.mFlags = (mAttr.mObjData != NULL ? mAttr.mObjData->mFlags : 0) | 1ULL | ((u64)info.mPlacement->mFlags << 16);
		if (mFloors != NULL)
			mStatus.mFlags |= 0x0200000000000000ULL;
		mPlacementParam0 = mCoord.mPlacement->mParam0;
		mPlacementParam1 = mCoord.mPlacement->mParam1;
		if (mJObj)
			SetAnim(mCoord.mPlacement->mStartAnim, TRUE, NULL, NULL, NULL, 0, 1, 1, 10);
		ResetObjectState();
		mLastPos.Set(mMoveTranslate);
		mColors[1] = 0xffffff00; // times white: untinted
		mColors[0] = 0;
		if (mJObj) {
			u32 status      = mStatus.mFlags;
			u32 material[5] = {
				status & 0x800000, status & 0x200000, status & 0x40, status & 0x20000, status & 0x400000,
			};
			ApplyMaterialPresets(material); // the status bits pick the material presets
			if ((mStatus.mFlags & 0x1000000) && IsStageDataLoaded())
				EnableLighting(GetEnvData()[9] / 100.f); // env slot 9 is the light level in percent
			if ((mStatus.mFlags & 2) && mJObj)
				HSD_JObjWalkTree(mJObj, JObjSetMObjFlags, reinterpret_cast<void*>(0x04000000));
			StagePlacement* place = mCoord.mPlacement;
			s32 index             = place - GetStagePlacementByIndex(0); // the placement's index in the stage
			if (index > 0)
				mEventScript = GetStageScriptByIndex(index);
		}
		UpdateObjectFloorHeights(this);
		UpdateObjectFloorXZExtent(this);
		if ((__fpclassifyf(mFloorXZExtent) != FP_NAN && mFloorXZExtent < 40.0f) || (mStatus.mFlags & 0x400)) {
			s32 kind = GetPool();
			if (kind == 7 || kind == 8 || kind == 20 || kind == 21 || kind == 22) // pools 7/8, 20/21 and 22
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
	mEventScript        = NULL;
	mFloorHit.mTriangle = -1;
	mFloorHit.mTrigon   = 0;
	mFloorHit.mHeight   = 0.0f;
	mToppleOffset.x     = 0.0f;
	mToppleOffset.y     = 0.0f;
	mToppleOffset.z     = 0.0f;
	mAnmQueue.Clear();
	mAnchorX = mAnchorZ   = NAN;
	mMarkerOverride       = MarkerUseAvailable;
	const Vec white       = { 255.0f, 255.0f, 255.0f };
	const Vec zero        = { 0.0f, 0.0f, 0.0f };
	mColorStep[0].mValue  = white;
	mColorStep[0].mTarget = white;
	mColorStep[0].mSpeed  = zero;
	mColorStep[1].mValue  = zero;
	mColorStep[1].mTarget = zero;
	mColorStep[1].mSpeed  = zero;
}

// XObj::vf90
