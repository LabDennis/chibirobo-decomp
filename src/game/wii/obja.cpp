// The XObj core methods, .text 0x800AC07C-0x800B1558 (wii_units.md Nr. 45): the object's
// init/reset pair, its placement load, release, update, the draw dispatch virtuals, the
// material helpers, the animation set/clear pair and the floor-collision test. The Wii
// source is the GameCube's obja.cpp carried over onto the nw4r model and queue types.
#include "Version.h"
#if defined(VERSION_R24J01)
#include "rvl_msl/PowerPC_EABI_Support/MSL_C/MSL_Common/math_api.h" // the Wii MSL's out-of-line __fpclassifyf
#include "game/wii/xobjflags.h"                                     // first: its AnimSfx needs the real nw4r::math, ahead of xobj.h's shim
#include "game/wii/spiderplacement.h"
#include "game/wii/xobj.h"
#include "game/wii/cgame.h"
#include "game/wii/xobj_ut2.h"
#include "game/cvec.h"
#include "game/evt.h"
#include "game/gmath.h"
#include "game/globals.h"
#include "game/stagedata.h"
#include "arithmetic.h"
#include "Dolphin/mtx.h"
#include "game/cfile.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace nw4r::g3d;

extern "C" void UpdateObjectFloorHeights(XObj* object);
extern "C" void UpdateObjectFloorXZExtent(XObj* object);
extern "C" void PSMTXMultVecArray(const Mtx m, const Vec* srcBase, Vec* dstBase, u32 count);
extern "C" BOOL fn_800AD498(XObj* obj);
extern "C" void fn_800AF51C(XObj* obj);
extern "C" void fn_800B4988(XObj* obj);
extern "C" s32 IsStageDataLoaded(void);
extern "C" u8* GetStageScriptByIndex(s32 index);
extern f32 gScreenWidthScale;
extern s32 gScreenLeft, gScreenRight, gScreenTop, gScreenBottom;
extern CustomShadow gCustomShadow;

static inline u32 ShadowSize(XObj* object)
{
	return object->mAttr.mObjData != NULL ? object->mAttr.mObjData->mShadowSize : 0;
}

static inline XObjSource* Source(const AttrInfo& attr)
{
	return attr.mObjData ? attr.mObjData->mSource : NULL;
}

static inline BOOL HasSource(const AttrInfo& attr)
{
	return attr.mObjData && attr.mObjData->mSource;
}

static inline const char* ModelName(const AttrInfo& attr)
{
	return HasSource(attr) ? attr.mObjData->mSource->mName : NULL;
}

// the head of the stage's info record, as far as this file reads it: the GameCube's
// UnkStageInfo (game/cgameutil.h, which the Wii cannot include); EnvInfo starts with a pointer to it
struct WiiStageInfo {
	u8 _00[0x9];
	u8 _09;
};

static inline WiiStageInfo* GetStageInfo()
{
	return *(WiiStageInfo**)GetStageEnvironment();
}

static inline void StepColor(f32& current, f32& target, f32& speed, u32 frames)
{
	if (current != target) {
		current += speed * frames;
		if (speed > 0.0f && current >= target) {
			current = target;
			speed   = 0.0f;
		}
		if (speed < 0.0f && current <= target) {
			current = target;
			speed   = 0.0f;
		}
	}
}

// 0x800AC07C
void XObj::InitializeObject()
{
	Init();
	mStatus.mFlags    = 0;
	mCoord.mPlacement = NULL;
	mAttr.mObjData    = NULL;
	mAttr.mPlacement  = NULL;
	mFloors           = NULL;
	mPoolIndex        = 0;
	ResetObjectState();
	InitializeEventScriptContext(&mEvt, gGame.GetMsg(1), this);
	ClearAnimQueue();
}

// 0x800AC11C
void XObj::ResetObjectState()
{
	mEventScript        = NULL;
	mFloorHit.mTriangle = -1;
	mFloorHit.mTrigon   = 0;
	mFloorHit.mHeight   = 0.0f;
	mVec714.x = mVec714.y = mVec714.z = 0.0f;
	ClearAnimQueue();
	mLastPos           = mMoveTranslate;
	mLastRot           = mMoveRotate;
	mLastAnimFrame     = -1.0f;
	mAnmQueue.mAnchorX = mAnmQueue.mAnchorZ = NAN;
	mAnmQueue.mMarkerOverride               = 0x80000000;
	mAnmQueue.mColorStep[0].mValue.x = mAnmQueue.mColorStep[0].mValue.y = mAnmQueue.mColorStep[0].mValue.z = 255.0f;
	mAnmQueue.mColorStep[0].mTarget.x = mAnmQueue.mColorStep[0].mTarget.y = mAnmQueue.mColorStep[0].mTarget.z = 255.0f;
	mAnmQueue.mColorStep[0].mSpeed.x = mAnmQueue.mColorStep[0].mSpeed.y = mAnmQueue.mColorStep[0].mSpeed.z = 0.0f;
	mAnmQueue.mColorStep[1].mValue.x = mAnmQueue.mColorStep[1].mValue.y = mAnmQueue.mColorStep[1].mValue.z = 0.0f;
	mAnmQueue.mColorStep[1].mTarget.x = mAnmQueue.mColorStep[1].mTarget.y = mAnmQueue.mColorStep[1].mTarget.z = 0.0f;
	mAnmQueue.mColorStep[1].mSpeed.x = mAnmQueue.mColorStep[1].mSpeed.y = mAnmQueue.mColorStep[1].mSpeed.z = 0.0f;
	ClearBaseMtx();
	_99C = 0;
}

// 0x800AC238, the placement read: copies the stage entry's transform, flags and parameters
// into the object, starts its animation, wires the event script and records its scope
BOOL XObj::LoadFromPlacement(const CoordInfo& info)
{
	BOOL ready = FALSE;
	if ((u64)info.mPlacement->mFlags & 0x80000000ULL) { // the placement opts out of loading
		return TRUE;
	}
	AttrInfo* attr = &mAttr;
	if (mStatus.mFlags & 1) {
		ready = TRUE;
	} else {
		if (attr->mObjData == NULL) {
			mCoord.mPlacement = info.mPlacement;
			attr->Init((CoordInfo*)&info);
		}
		if (mFloors == NULL) {
			mFloors        = GetObjectCollisionList(info.mPlacement->mObjectId);
			mFloorMasks[0] = mFloorMasks[1] = 0;
			BOOL empty                      = mFloors ? mFloors->mId == -1 : FALSE; // a -1 id means no floor list
			if (empty) {
				mFloors = NULL;
			}
		}
		if (!HasSource(*attr) || attr->mObjData->mSource->mName == NULL) {
			ready = TRUE;
		} else if (HasSource(*attr)) {
			ready = fn_800AD498(this);
		}
	}
	if (ready) {
		mStatus.mFlags = (mStatus.mFlags & 0x0040000000000000ULL) | ((u64)info.mPlacement->mFlags << 16)
		               | (attr->mObjData != NULL ? attr->mObjData->mFlags : 0) | 1;
		Vec pos = PlacementPosition(info);
		SetPos(pos);
		if (mStatus.mFlags & 0x800ULL) {
			SetTranslateX(GetPos()->x * gScreenWidthScale);
		}
		Vec rot = PlacementRotation(info);
		SetRot(rot);
		Vec scale = PlacementScale(info);
		SetScale(scale);
		mMoveSpeed0.Set(0.0f, 0.0f, 0.0f);
		mMoveSpeed1.Set(0.0f, 0.0f, 0.0f);
		if (mObject.m_pScnMdl != NULL) {
			SetAnim(mCoord.mPlacement->mStartAnim, TRUE, 0.0f, 1.0f, 1.0f, 15.0f);
			s32 frames = (s32)GetAnimEndFrame();
			if (frames > 1) {
				SetAnim(GetAnimId(), TRUE, (f32)(rand() % frames), 1.0f, 1.0f, 15.0f);
			}
		}
		ResetObjectState();
		if (mColor1.r != 0xFF) {
			mColor1.r = 0xFF;
		}
		if (mColor1.g != 0xFF) {
			mColor1.g = 0xFF;
		}
		if (mColor1.b != 0xFF) {
			mColor1.b = 0xFF;
		}
		if (mColor1.a != 0) {
			mColor1.a = 0;
		}
		mColorsChanged = TRUE;
		if (mColor0.r != 0) {
			mColor0.r = 0;
		}
		if (mColor0.g != 0) {
			mColor0.g = 0;
		}
		if (mColor0.b != 0) {
			mColor0.b = 0;
		}
		if (mColor0.a != 0) {
			mColor0.a = 0;
		}
		mColorsChanged = TRUE;
		if (mStatus.mFlags & 0x2ULL) {
			fn_800AF51C(this);
		}
		s32 index = GetStagePlacementIndex(mCoord.mPlacement);
		if (index >= 0) {
			mEventScript = GetStageScriptByIndex(index);
		} else {
			mEventScript = NULL;
		}
		if (mFloors != NULL) {
			mStatus.mFlags |= 0x0200000000000000ULL;
		} else {
			mStatus.mFlags &= ~0x0200000000000000ULL;
		}
		mPlacementParam0 = mCoord.mPlacement->mParam0;
		mPlacementParam1 = mCoord.mPlacement->mParam1;
		if (mStatus.mFlags & 0x800000ULL) {
			if (!(mStatus.mFlags & 0x100000000ULL)) {
				mStatus.mFlags &= ~0x800000ULL;
			}
		} else {
			s32 kind = GetKind();
			if (kind == 0x345 || kind == 0x27E) {
				mStatus.mFlags |= 0x800000ULL;
			}
		}
		if (HasModel()) {
			u32 flags[5];
			flags[0] = mStatus.mFlags & 0x800000ULL;
			flags[1] = mStatus.mFlags & 0x200000ULL;
			flags[2] = mStatus.mFlags & 0x40ULL;
			flags[3] = mStatus.mFlags & 0x20000ULL;
			flags[4] = mStatus.mFlags & 0x400000ULL;
			vf1A0(flags);
			if ((mStatus.mFlags & 0x1000000ULL) && IsStageDataLoaded() != 0) {
				vf1A8((f32)GetStageInfo()->_09 / 100.0f);
			}
		}
		UpdateObjectFloorHeights(this);
		UpdateObjectFloorXZExtent(this);
		if (__fpclassifyf(mFloorXZExtent[0]) != FP_NAN && mFloorXZExtent[0] < 40.0f || (mStatus.mFlags & 0x400ULL)) {
			s16 cat = GetCategory();
			if (cat == 7 || cat == 8 || (u16)(cat - 0x14) <= 2) {
				mStatus.mFlags |= 0x80000000ULL;
			}
		}
		if (mStatus.mFlags & 0x2000ULL) { // flag 0x2000 overrides the small-extent flag
			mStatus.mFlags &= ~0x80000000ULL;
		}
		if (mStatus.mFlags & 0x8100000000ULL) { // either high bit maps to flag 0x4000
			mStatus.mFlags |= 0x4000ULL;
		}
		mSelectedFloorHeight = NAN;
		mSelectedFloorIndex  = 0;
		_188 = GetCategory() == 2 || GetCategory() == 9 || GetCategory() == 5 || GetCategory() == 0xA || GetCategory() == 0xB
		    || GetCategory() == 0xC || GetCategory() == 0xD || GetCategory() == 0xE || GetCategory() == 0xF || GetCategory() == 0x10
		    || GetCategory() == 0x11 || GetCategory() == 0x12 || GetCategory() == 0x13;
		switch (GetKind()) {
		case 0x0:
			_18C = 0;
			break;
		case 0x6B:
		case 0x3B1:
		case 0x472:
			_18C = 1;
			break;
		case 0xF:
			_18C = 2;
			break;
		case 0xC4:
		case 0x24D:
			_18C = 3;
			break;
		case 0x113:
		case 0x24B:
			_18C = 4;
			break;
		case 0x159:
		case 0x42C:
			_18C = 5;
			break;
		case 0x117:
			_18C = 6;
			break;
		case 0xF8:
			_18C = 7;
			break;
		case 0xF7:
			_18C = 8;
			break;
		case 0x110:
			_18C = 9;
			break;
		case 0xD7:
		case 0x24C:
			_18C = 0xA;
			break;
		case 0x25E:
		case 0x345:
		case 0x459:
			_18C = 0xB;
			break;
		case 0x1D3:
		case 0x38E:
			_18C = 0xC;
			break;
		case 0x1FB:
		case 0x464:
			_18C = 0xD;
			break;
		case 0x26D:
			_18C = 0xE;
			break;
		case 0x1FC:
			_18C = 0xF;
			break;
		case 0xE:
			_18C = 0x10;
			break;
		case 0x6F:
			_18C = 0x11;
			break;
		case 0x26C:
		case 0x485:
			_18C = 0x12;
			break;
		case 0x317:
			_18C = 0x13;
			break;
		case 0x134:
			_18C = 0x14;
			break;
		case 0x10F:
		case 0x376:
			_18C = 0x15;
			break;
		case 0x156:
			_18C = 0x16;
			break;
		case 0x148:
			_18C = 0x17;
			break;
		case 0x2C5:
			_18C = 0x18;
			break;
		case 0x1D2:
			_18C = 0x19;
			break;
		case 0x2C3:
			_18C = 0x1A;
			break;
		case 0x29F:
		case 0x446:
		case 0x465:
			_18C = 0x1B;
			break;
		case 0x429:
		case 0x447:
		case 0x466:
			_18C = 0x1C;
			break;
		default:
			_18C = -1;
			break;
		}
		s16 cat            = GetCategory();
		mScreenBoundsValid = cat == 2 || cat == 7 || (cat == 8 && (mStatus.mFlags & 0x100000000ULL)) || (u16)(cat - 9) <= 0xA || _18C >= 0
		                  || GetKind() == 0xE || GetKind() == 0x3AE || GetKind() == 0x1D0 || GetKind() == 0x260 || GetKind() == 0x24E
		                  || GetKind() == 0x23F || GetKind() == 0x23D || GetKind() == 0x41D || GetKind() == 0x2A0 || GetKind() == 0x30D
		                  || GetKind() == 0x30E || GetKind() == 0x46F || GetKind() == 0x34B || GetKind() == 0x324 || GetKind() == 0x2C9
		                  || (GetKind() == 0x6A && mPlacementParam0 == 1);
		_6DC       = 0;
		_6D8       = 0;
		mScreenTop = mScreenBottom = mScreenLeft = mScreenRight = NAN;
		if (mScreenBoundsValid) {
			mStatus.mFlags |= 0x100ULL;
		}
		fn_800B4988(this);
		AssignAnimEventCallback();
		mReleaseCountdown = 0;
		_9A4              = 0;
	}
	return mStatus.mFlags & 1;
}

// 0x800AD498, the load half of LoadFromPlacement: the kind's archive, model id and a tex-stage
// count out of the object data, "<name>.brres" through CObj::Load. the GameCube keeps this
// inline in xobjinit.cpp (there with "%s.dat" and ARAM flags instead of the stage count)
// NONMATCHING: ~87% fuzzy - retail CSEs GetPool()'s mObjData load across the two &&-range tests
// and schedules the selects differently; the generated logic is otherwise identical
extern "C" BOOL fn_800AD498(XObj* obj)
{
	AttrInfo& attr     = obj->mAttr;
	ResourceFile* file = Source(attr) ? attr.mObjData->mSource->mArchive : NULL;
	const char* name   = ModelName(attr);
	u32 model          = attr.mObjData ? attr.mObjData->mModel : 0;
	u32 flags          = 0;
	if ((u64)(attr.mObjData ? attr.mObjData->mFlags : 0) & 0x80) { // obj flag 0x80 becomes load flag 0x10000
		flags |= 0x10000;
	}
	BOOL isWall2 = FALSE;
	if (HasSource(attr) && ModelName(attr) != NULL && strcmp(ModelName(attr), "ufo/wall2") == 0) {
		isWall2 = TRUE;
	}
	if (isWall2) {
		flags |= 0x40000;
	}
	u64 objFlags = ((u64)obj->mCoord.mPlacement->mFlags << 16) | (attr.mObjData ? attr.mObjData->mFlags : 0);
	u32 texNum   = 3;
	if (objFlags & 4) {
		texNum = 4;
	}
	if (gGame.GetStatusFlags(0x2000000000ULL) != 0) {
		if ((objFlags & 0x20000000ULL) || obj->GetPool() == 2 || (obj->GetPool() >= 4 && obj->GetPool() <= 16)) {
			texNum += 1;
		}
	}
	if (gGame.GetStatusFlags(0x100000000000ULL) != 0) {
		if ((objFlags & 0x200000000000ULL) || obj->GetPool() == 2 || (obj->GetPool() >= 4 && obj->GetPool() <= 16)) {
			texNum += 5;
		}
	}
	if (obj->mStatus.mFlags & 0x200) {
		texNum += 1;
	}
	switch (obj->mCoord.mPlacement->mObjectId) {
	case 0xEE:
		texNum += 10;
		break;
	case 0x12A:
		texNum += 200;
		break;
	case 0x13C:
		texNum += 16;
		break;
	case 0x1BC:
		texNum += 12;
		break;
	}
	flags = (flags & 0xFFFFFF00) | texNum;
	char path[64];
	sprintf(path, "%s.brres", name);
	CFile entry;
	if (entry.ConvertPathToEntrynum(path) < 0) {
		strcpy(path, "test/unitCube.brres");
	}
	if (obj->Load(file, path, model, flags, 0, 0)) {
		return TRUE;
	}
	return FALSE;
}

// 0x800AD870, flags[0] asks for dst alpha 8, flags[1] or nothing for 4, on every material's pix copy
void XObj::vf1A0(const u32* flags)
{
	u32 dstAlpha8 = flags[0];
	u32 dstAlpha4 = flags[1];
	for (u32 i = 0; i < mObject.m_resMdl.GetResMatNumEntries(); i++) {
		ResMat mat = mObject.m_resMdl.GetResMat(i);
		ScnMdl::CopiedMatAccess access(mObject.m_pScnMdl, i);
		ResMatPix pix;
		pix = access.GetResMatPix(false);
		if (!pix.IsValid()) {
			pix = mat.GetResMatPix();
		}
		if (dstAlpha8 != 0) {
			pix.GXSetDstAlpha(GX_TRUE, 8);
		} else if (dstAlpha4 != 0) {
			pix.GXSetDstAlpha(GX_TRUE, 4);
		} else {
			pix.GXSetDstAlpha(GX_TRUE, 4);
		}
		pix.DCStore(false);
		mat.DCStore(false);
	}
}

// 0x800AD9D0
void XObj::SetDestinationAlpha(BOOL enabled, u8 value)
{
}

// 0x800AD9D4
void XObj::vf1A8(f32)
{
}

// 0x800AD9D8
void XObj::SetDiffuseColor(GXColor color)
{
	for (u32 i = 0; i < mObject.m_resMdl.GetResMatNumEntries(); i++) {
		ResMat mat = mObject.m_resMdl.GetResMat(i);
		ScnMdl::CopiedMatAccess access(mObject.m_pScnMdl, i);
		ResMatTevColor tevColor;
		tevColor = access.GetResMatTevColor(false);
		if (!tevColor.IsValid()) {
			tevColor = mat.GetResMatTevColor();
		}
		tevColor.GXSetTevKColor(GX_KCOLOR0, color);
		tevColor.DCStore(false);
		mat.DCStore(false);
	}
}

// 0x800ADB20
void XObj::Release()
{
	if (mArchive.mAnm.IsLoaded()) {
		if (GetAnimId() >= 0) {
			XObjSource* source = NULL;
			if (mAttr.mObjData != NULL) {
				source = mAttr.mObjData->mSource;
			}
			if (source != NULL && source->mAnimNames != NULL
			    && mObject.m_resFile.GetResAnmChr(source->mAnimNames[GetAnimId()]).mpData == NULL) {
				SetAnim(0, TRUE, 0.0f, 15.0f, 15.0f, 1.0f);
			}
			SetAnim(GetAnimId(), TRUE, GetAnimFrame(), 15.0f, 15.0f, 1.0f);
		}
		ClearAnmSource();
		if (mArchive.mAnm.mFile.IsLoaded()) {
			mArchive.mAnm.Release(TRUE);
			mArchive.mAnm.mFile.Close();
		}
	}
	if (mEvt.mData.mScript != NULL) {
		ResetEventScriptState(&mEvt, FALSE, FALSE, FALSE, FALSE);
	}
	if (mStatus.mFlags & 1) {
		CObj::Release();
	}
	mAttr.mObjData   = NULL;
	mAttr.mPlacement = NULL;
	mFloors          = NULL;
	mStatus.mFlags &= ~0x10001ULL;
}

// 0x800ADD28
void XObj::UpdateObject(u32 frames)
{
	if (mObject.m_pScnMdl != NULL) {
		if (mReleaseCountdown > 0 && !(mStatus.mFlags & 0x10000ULL)) {
			if (--mReleaseCountdown == 0) {
				Release();
			}
			return;
		}
		vf168(frames);
		if (mStatus.mFlags & 0x1000000000000000ULL) { // toppling: sink onto the floor hit
			if (mMoveTranslate.y + mVec714.y > mFloorHit.mHeight) {
				mMoveTarget0.y = mFloorHit.mHeight;
				f32 speed      = 0.15f + mMoveSpeed0.y; // fall faster, capped at 10
				if (speed > 10.0f) {
					speed = 10.0f;
				}
				mMoveSpeed0.y = speed;
			} else { // landed: take over the offset and stand straight
				mMoveSpeed0.y = 0.0f;
				mMoveTranslate.x += mVec714.x;
				mMoveTranslate.z += mVec714.z;
				mMoveTranslate.y = mFloorHit.mHeight;
				mVec714.x = mVec714.y = mVec714.z = 0.0f;
				SetRotationX(0.0f);
				SetRotationZ(0.0f);
				mStatus.mFlags &= ~0x1000000000000000ULL;
				if ((s16)GetPool() == 0x15) {
					mStatus.mFlags |= 0x2000000000000000ULL;
				} else if ((s16)GetPool() == 0x16 && (mStatus.mFlags & 0x04000000ULL)) {
					mStatus.mFlags |= 0x2000000000000000ULL;
				}
			}
		}
		// move, scale and turn toward their targets, then animate
		((CObjVec*)&mMoveRotate)->Step((f32)frames);
		((CObjVec*)&mMoveTranslate)->Step((f32)frames);
		if (((CObjVec*)&mMoveScale)->Step((f32)frames)) {
			UpdateObjectFloorHeights(this);
			UpdateObjectFloorXZExtent(this);
		}
		if (mStatus.mFlags & 0x80000000000000ULL) { // the archive anm source's read is still running
			UpdateArchiveAnmSource(this);
		}
		UpdateAnim((f32)frames);
	}
	if ((mStatus.mFlags & 0x0010000000000000ULL) || mMoveTranslate.y != mLastPos.y) { // floor heights track the height
		UpdateObjectFloorHeights(this);
	}
	if (mEvt.mData.mScript != NULL) {
		CEvt_Update(&mEvt, frames);
	}
	if (mAnimEventCallback != NULL && !(mStatus.mFlags & 0x800ULL)) {
		mAnimEventCallback(this, GetAnimId(), mLastAnimFrame, GetAnimFrame());
	}
	if (PSVECMag(&mMoveScale) > 0.0f) {
		WiiObjectRef* ref = &gGame.mDrawLists[1].mRefs[gGame.mDrawLists[1].mCount];
		ref->mObj         = this;
		ref->mIndex       = 0;
		gGame.mDrawLists[1].mCount++;
	}
}

// 0x800AE170, steps the two queued material colors to their targets and flags the change
void XObj::vf168(u32 frames)
{
	if (mObject.m_pScnMdl != NULL) {
		for (s32 i = 0; i < 2; i++) {
			StepColor(mAnmQueue.mColorStep[i].mValue.x, mAnmQueue.mColorStep[i].mTarget.x, mAnmQueue.mColorStep[i].mSpeed.x, frames);
			StepColor(mAnmQueue.mColorStep[i].mValue.y, mAnmQueue.mColorStep[i].mTarget.y, mAnmQueue.mColorStep[i].mSpeed.y, frames);
			StepColor(mAnmQueue.mColorStep[i].mValue.z, mAnmQueue.mColorStep[i].mTarget.z, mAnmQueue.mColorStep[i].mSpeed.z, frames);
		}
		u32 color1 = ((u32)mAnmQueue.mColorStep[0].mValue.x << 24) | ((u32)mAnmQueue.mColorStep[0].mValue.y << 16)
		           | ((u32)mAnmQueue.mColorStep[0].mValue.z << 8);
		mColor1.Set((s32)(color1 >> 24), (s32)((color1 >> 16) & 0xFF), (s32)((color1 >> 8) & 0xFF), (s32)(color1 & 0xFF));
		mColorsChanged = TRUE;
		u32 color0     = ((u32)mAnmQueue.mColorStep[1].mValue.x << 24) | ((u32)mAnmQueue.mColorStep[1].mValue.y << 16)
		           | ((u32)mAnmQueue.mColorStep[1].mValue.z << 8);
		mColor0.Set((s32)(color0 >> 24), (s32)((color0 >> 16) & 0xFF), (s32)((color0 >> 8) & 0xFF), (s32)(color0 & 0xFF));
		mColorsChanged = TRUE;
	}
}

// 0x800AE448, queues the model on the scene's draw list when it has one and a nonzero scale
void XObj::vf170(Mtx view)
{
	if (mObject.m_pScnMdl == NULL) {
		return;
	}
	if (PSVECMag(&mMoveScale) > 0.0f) {
		WiiGsSceneObj* scene = gGame.mGs.mScene.mHead;
		scene->vf034(scene->mPending, mObject.m_pScnMdl);
	}
}

// 0x800AE4B4
void XObj::DrawWithView(Mtx view)
{
	if (mObject.m_pScnMdl == NULL) {
		return;
	}
	if (mStatus.mFlags & 0x800) {
		return;
	}
	vf170(view);
}

// 0x800AE4E0
void XObj::DrawGroundShadow(f32 opacity)
{
	if (opacity == 0.0f) {
		return;
	}
	if (mObject.m_pScnMdl == NULL) {
		return;
	}
	if (mStatus.mFlags & 0x800) {
		return;
	}
	if (mStatus.mFlags & 0x4) {
		vf170(NULL);
	} else {
		DrawGroundShadowAt(&mMoveTranslate, 1.0f);
	}
}

// NONMATCHING: the body matches except two referent/scheduling details: retail loads
// mObjData twice (r6, then r7) where our build keeps the first load live; and our
// literals land on different .sdata2 slots while the unit is incomplete.
// 0x800AE53C, the blob shadow at a given spot: the model's scale times the kind's shadow
// size and a fixed 0.6 sets the blob, 170*opacity its alpha; the draw call takes both
// vectors by value, so the caller keeps its own copies
void XObj::DrawGroundShadowAt(const Vec* origin, f32 opacity)
{
	if (!(mStatus.mFlags & 0x8)) {
		return;
	}
	if (ShadowSize(this) == 0) {
		return;
	}
	Vec pos   = *origin;
	Vec scale = mMoveScale * (f32)ShadowSize(this) * 0.6f;
	gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
	gCustomShadow.Draw(pos, scale, (u32)(170.0f * opacity));
}

// 0x800AE6A8
void XObj::DrawCollisionShapes()
{
}

// 0x800AE6AC, the screen-list twin of vf170, gated on both flag bits being set
void XObj::DrawScreenOverlay()
{
	if (mObject.m_pScnMdl == NULL) {
		return;
	}
	if ((mStatus.mFlags & 0x10001) != 0x10001) {
		return;
	}
	if (!(mStatus.mFlags & 0x800)) {
		return;
	}
	WiiGsSceneObj* scene = gGame.mGs.mScene.mHead;
	scene->vf034(scene->mPending, mObject.m_pScnMdl);
}

// NONMATCHING: same instruction stream (188 each side); retail schedules the flag test's
// lwz/and/xor. between the callee saves and the zero-stores between the render-state setup,
// here both run after. Register assignment and the sdata2 constant labels differ with it.
// 0x800AE700, frames the object's on-screen bounds a margin wider while it is pointer-hovered
void XObj::vf198(f32 margin, f32 alpha, u32 color)
{
	if ((mStatus.mFlags & 0x10001) != 0x10001) {
		return;
	}
	if (mStatus.mFlags & 0x1000000000800) {
		return;
	}
	BOOL onScreen = mScreenBoundsValid && __fpclassifyf(mScreenLeft) != FP_NAN && __fpclassifyf(mScreenTop) != FP_NAN
	             && __fpclassifyf(mScreenRight) != FP_NAN && __fpclassifyf(mScreenBottom) != FP_NAN && mScreenLeft <= gScreenRight
	             && mScreenTop <= gScreenBottom && mScreenRight >= gScreenLeft && mScreenBottom >= gScreenTop;
	if (onScreen && _6D8 != 0) {
		SetGsChannelEnabled(&gGame.mGs, 0, 0);
		gGame.mGs.mCurRenderState->SetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
		f32 l      = mScreenLeft - margin;
		f32 r      = mScreenRight + margin;
		f32 t      = mScreenTop - margin;
		f32 b      = mScreenBottom + margin;
		Vec pos[5] = { { l, t, 0.0f },   // top left
			           { r, t, 0.0f },   // top right
			           { r, b, 0.0f },   // bottom right
			           { l, b, 0.0f },   // bottom left
			           { l, t, 0.0f } }; // back to top left
		gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
		gGame.mGs.mCurRenderState->SetLineWidth(0xC, GX_TO_ONE);
		DrawGsLineStrip(&gGame.mGs, pos, (color & 0xFFFFFF00) | (u8)((color & 0xFF) * alpha), 5);
	}
}

// 0x800AE9F0, a second on-screen-bounds effect: a rotating two-tone ring (colored inner
// edge fading to a transparent rim, then a white copy under a different blend) around the
// object's screen box while it is hovered
// NONMATCHING: the instruction stream is structurally identical (same guards, same
// (h-w)*0.5f ring radii, same matrix chain, same two blend passes over 130 verts) — the
// residue is register allocation (f23-f31/r25-r31 shift), MWCC picking pointer-walk
// stores instead of the retail base+index stfsux/stwux forms in the unrolled pass-2
// loop, and scheduling interleave around the u64 cvt scratch pairs
extern "C" void fn_800AE9F0(XObj* obj, f32 margin, f32 scale, f32 rot, u32 color)
{
	if ((obj->mStatus.mFlags & 0x10001) != 0x10001) {
		return;
	}
	if (obj->mStatus.mFlags & 0x800) {
		return;
	}
	SetGsChannelEnabled(&gGame.mGs, 0, 0);
	gGame.mGs.mCurRenderState->SetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
	Vec out[130];
	Vec vtx[130];
	u32 colors[130];
	Mtx transM, scaleM, rotM, mtx;
	u32 edge1 = color & 0xFFFFFF00;
	edge1 |= (u8)(0.8f * ((color & 0xFF) * scale));
	u32 edge0 = edge1 & 0xFFFFFF00;
	f32 l     = obj->mScreenLeft - margin;
	f32 r     = obj->mScreenRight + margin;
	f32 t     = obj->mScreenTop - margin;
	f32 b     = obj->mScreenBottom + margin;
	f32 w     = r - l;
	f32 h     = b - t;
	f32 cx    = w * 0.5f + l;
	f32 cy    = h * 0.5f + t;
	f32 rIn   = (h - w) * 0.5f - 2.0f;
	f32 rOut  = (h - w) * 0.5f + 6.0f + 4.0f * scale;
	for (s32 i = 0; i < 65; i++) {
		s32 j         = i * 2;
		vtx[j].x      = rIn * std::cosf(i * (PI / 32.0f));
		vtx[j].y      = rIn * std::sinf(i * (PI / 32.0f));
		vtx[j].z      = 0.0f;
		colors[j]     = edge1;
		vtx[j + 1].x  = rOut * std::cosf(i * (PI / 32.0f));
		vtx[j + 1].y  = rOut * std::sinf(i * (PI / 32.0f));
		vtx[j + 1].z  = 0.0f;
		colors[j + 1] = edge0;
	}
	PSMTXTrans(transM, cx, cy, 0.0f);
	f32 size = 1.0f + (1.0f - scale);
	PSMTXScale(scaleM, size, size, size);
	PSMTXConcat(transM, scaleM, mtx);
	PSMTXRotRad(rotM, 'z', rot);
	PSMTXConcat(mtx, rotM, mtx);
	PSMTXMultVecArray(mtx, vtx, out, 130);
	gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
	DrawGsColoredTrianglePrimitive(&gGame.mGs, GX_TRIANGLESTRIP, gGame.mGs.mCamera->mViewMtx, out, colors, 0x26);
	DrawGsColoredTrianglePrimitive(&gGame.mGs, GX_TRIANGLESTRIP, gGame.mGs.mCamera->mViewMtx, out + 42, colors, 0x26);
	DrawGsColoredTrianglePrimitive(&gGame.mGs, GX_TRIANGLESTRIP, gGame.mGs.mCamera->mViewMtx, out + 86, colors, 0x26);
	for (s32 i = 0; i < 65; i++) {
		colors[i * 2]     = 0xFFFFFF00 | (u8)((edge1 & 0xFF) * scale);
		colors[i * 2 + 1] = 0xFFFFFF00;
	}
	gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCCOL, GX_BL_SRCALPHA, GX_LO_SET);
	DrawGsColoredTrianglePrimitive(&gGame.mGs, GX_TRIANGLESTRIP, gGame.mGs.mCamera->mViewMtx, out, colors, 0x26);
	DrawGsColoredTrianglePrimitive(&gGame.mGs, GX_TRIANGLESTRIP, gGame.mGs.mCamera->mViewMtx, out + 42, colors, 0x26);
	DrawGsColoredTrianglePrimitive(&gGame.mGs, GX_TRIANGLESTRIP, gGame.mGs.mCamera->mViewMtx, out + 86, colors, 0x26);
	gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
}

// 0x800AF098, the READ command's start half: kicks the archive's animation-file read and
// arms the flag UpdateObject polls; the panic names the header the helper family lives in
// NONMATCHING: retail pools "" in .sdata (lbl_805A0F80), ours folds it into the
// "xobj_ut2.h" .rodata stringBase (addi r5, r3, 0xb) — a string-pool layout
// difference of the still-incomplete unit; the code is instruction-identical otherwise
extern "C" BOOL fn_800AF098(XObj* obj, const char* path)
{
	s32 result = obj->mArchive.mAnm.mFile.ReadAsync(path, 0, 0, FALSE);
	BOOL ok    = result != 0 ? TRUE : (OSPanic("xobj_ut2.h", 0xe2, ""), FALSE);
	if (ok) {
		obj->mStatus.mFlags |= 0x80000000000000ULL;
		return TRUE;
	}
	return FALSE;
}

// 0x800AF12C, the READ command's empty-path half: rebinds the playing anim to the object's
// own file (resetting to anim 0 when only the archive had it), then drops the source,
// releases the archive and closes the file
extern "C" void fn_800AF12C(XObj* obj)
{
	if (obj->mArchive.mAnm.IsLoaded()) {
		if (obj->GetAnimId() >= 0) {
			XObjSource* source = obj->mAttr.mObjData != NULL ? obj->mAttr.mObjData->mSource : NULL;
			if (source != NULL) {
				if (source->mAnimNames != NULL) {
					if (obj->mObject.m_resFile.GetResAnmChr(source->mAnimNames[obj->GetAnimId()]).mpData == NULL) {
						obj->SetAnim(0, TRUE, 0.0f, 1.0f, 1.0f, 15.0f);
					}
				}
			}
			f32 frame = obj->GetAnimFrame();
			obj->SetAnim(obj->GetAnimId(), TRUE, frame, 1.0f, 1.0f, 15.0f);
		}
		obj->ClearAnmSource();
		if (obj->mArchive.mAnm.mFile.IsLoaded()) {
			obj->mArchive.mAnm.Release(TRUE);
			obj->mArchive.mAnm.mFile.Close();
		}
	}
}

// 0x800AF2C8, the kind's packed anim id: the low halfword names the character animation, the high
// one a material animation; both index the source's own name table, each wrapping at 1000
void XObj::SetAnim(s32 id, BOOL force, f32 frame, f32 rate, f32 blend, f32 blendRate)
{
	if (id != GetAnimId()) {
		mLastAnimFrame = -1.0f;
	} else if (force && GetAnimFrame() != frame) {
		mLastAnimFrame = -1.0f;
	}
	if (mStatus.mFlags & 0x20000000000000ULL) {
		blend = 15.0f;
	}
	s32 animIndex      = (id & 0xFFFF) % 1000;
	XObjSource* source = NULL;
	if (mAttr.mObjData != NULL) {
		source = mAttr.mObjData->mSource;
	}
	if (source != NULL && source->mAnimNames != NULL) {
		if (animIndex >= source->mAnimCount) {
			animIndex = 0;
		}
		CObjAnim anim;
		anim.mId   = id;
		anim.mName = source->mAnimNames[animIndex];
		CObj::SetAnim(&anim, force, frame, rate, blend, blendRate);
		if ((id >> 16) != 0) {
			s32 matIndex = (id >> 16) % 1000;
			if (matIndex < source->mAnimCount) {
				CObjAnim matAnim;
				matAnim.mId   = id;
				matAnim.mName = source->mAnimNames[matIndex];
				SetMatAnim(&matAnim, frame);
			}
		}
	} else {
		CObj::SetAnim(id, force, frame, rate, blend, blendRate);
	}
}

// 0x800AF4CC
void XObj::ClearAnim()
{
	CObj::ClearAnim();
	mStatus.mFlags &= ~0x0008000000000000ULL;
}

// the marker the material pass writes into ResTev's padding, and the "xobj.cpp" assert the
// retail's Panic calls name (the GameCube's obja.cpp is xobj.cpp here)
#ifndef UPDATED_TEV_MAGIC_NUMBER
#define UPDATED_TEV_MAGIC_NUMBER    0xC3
#define XOBJ_ASSERT_LINE(line, exp) ((void)((exp) || (nw4r::db::Panic("xobj.cpp", line, "NW4R:Failed assertion " #exp), 0)))
#endif

// 0x800AF51C, the material pass LoadFromPlacement runs while the placement flag is set: every
// material gains a final TEV stage multiplying the previous colour by the rendered shadow
// texture, unless its "sdw" user-data item's first value is -1
// NONMATCHING: the flow is the retail's (copied-resource fallbacks, the "sdw" item lookup,
// the stage append and its trailing asserts) but the reconstruction is provisional — the
// ResUserDataItem field names are modelled from the retail offsets and the report/assert
// strings are the plausible expressions, so codegen is expected to differ
extern "C" void fn_800AF51C(XObj* obj)
{
	if (obj->mObject.m_pScnMdl == NULL) {
		return;
	}
	u32 numMats = obj->mObject.m_resMdl.GetResMatNumEntries();
	for (u32 i = 0; i < numMats; i++) {
		ResMat mat = obj->mObject.m_resMdl.GetResMat(i);
		ScnMdl::CopiedMatAccess access(obj->mObject.m_pScnMdl, i);

		ResGenMode genMode = access.GetResGenMode(false);
		if (!genMode.IsValid()) {
			genMode = mat.GetResGenMode();
		}
		ResMatTevColor tevColor = access.GetResMatTevColor(false);
		if (!tevColor.IsValid()) {
			tevColor = mat.GetResMatTevColor();
		}
		XOBJ_ASSERT_LINE(0x74F, tevColor.IsValid());
		ResMatTexCoordGen texCoordGen = access.GetResMatTexCoordGen(false);
		if (!texCoordGen.IsValid()) {
			texCoordGen = mat.GetResMatTexCoordGen();
		}
		XOBJ_ASSERT_LINE(0x755, texCoordGen.IsValid());
		ResTexObj texObj = access.GetResTexObj(false);
		if (!texObj.IsValid()) {
			texObj = mat.GetResTexObj();
		}
		XOBJ_ASSERT_LINE(0x75B, texObj.IsValid());
		ResTexSrt texSrt = access.GetResTexSrt(false);
		if (!texSrt.IsValid()) {
			texSrt = mat.GetResTexSrt();
		}
		XOBJ_ASSERT_LINE(0x761, texSrt.IsValid());
		ResTev tev = access.GetResTev(false);
		if (!tev.IsValid()) {
			tev = mat.GetResTev();
		}
		XOBJ_ASSERT_LINE(0x767, tev.IsValid());
		if (i == 0 && tev.ptr()->PADDING_0x18[6] == UPDATED_TEV_MAGIC_NUMBER) {
			return;
		}

		ResUserData userData = mat.GetResUserData();
		if (userData.IsValid()) {
			ResDic dic           = userData.GetResDic();
			ResUserDataItem item = ResUserDataItem(dic["sdw"]);
			if (item.IsValid() && item.ref().numData != 0 && item.GetS32(0) == -1) {
				continue;
			}
		}

		u32 sdwTexMapId     = mat.ref().numResTexPlttInfo;
		u32 numTexGens      = genMode.GXGetNumTexGens();
		u32 numTevStages    = genMode.GXGetNumTevStages();
		u32 numTevStageData = tev.ref().nStages;
		if (sdwTexMapId > 7) {
			OSReport("mat '%s' has more than %d textures.(%d)\n", mat.GetName(), 7, sdwTexMapId);
			continue;
		}
		if (numTexGens > 7) {
			OSReport("mat '%s' has more than %d tex coord gens.(%d)\n", mat.GetName(), 7, numTexGens);
			continue;
		}
		if (numTevStages > 15) {
			OSReport("mat '%s' has more than %d tev stages.(%d)\n", mat.GetName(), 15, numTevStages);
			continue;
		}

		static const GXColor sdwKColor[] = { { 0x00, 0x00, 0x00, 0xFF } };
		GXColor color                    = sdwKColor[numTexGens];
		XOBJ_ASSERT_LINE(0x846, !texObj.IsValidTexObj((GXTexMapID)sdwTexMapId));
		*texObj.GetTexObj((GXTexMapID)sdwTexMapId) = gGame.mShadow.mTex;
		texObj.Validate((GXTexMapID)sdwTexMapId);
		genMode.GXSetNumTevStages((u8)(numTevStages + 1));
		genMode.GXSetNumTexGens((u8)(numTexGens + 1));
		tevColor.GXSetTevKColor(GX_KCOLOR2, color);
		tevColor.DCStore(false);
		// the new tex gen is a position transform on its own Wii texmtx base (0x40 + 3*n)
		texCoordGen.GXSetTexCoordGen2((GXTexCoordID)numTexGens, GX_TG_MTX3x4, GX_TG_POS, GX_FALSE, 0x40 + numTexGens * 3);
		texCoordGen.DCStore(false);
		texSrt.SetEffectMtx(numTexGens, NULL);
		texSrt.SetMapMode(numTexGens, 2, 1, -1);
		texSrt.SetTexSrtFlag(numTexGens, 0xF);

		GXTevStageID stage = (GXTevStageID)numTevStages;
		if (tev.ptr()->PADDING_0x18[6] == 0) {
			tev.SetNumTevStages((u8)(numTevStages + 1));
			tev.GXSetTevDirect(stage);
			tev.GXSetTevKColorSel(stage, GX_TEV_KCSEL_K2);
			tev.GXSetTevOrder(stage, (GXTexCoordID)numTexGens, (GXTexMapID)sdwTexMapId, GX_COLOR_NULL);
			tev.GXSetTevColorIn(stage, GX_CC_CPREV, GX_CC_KONST, GX_CC_TEXA, GX_CC_ZERO);
			tev.GXSetTevColorOp(stage, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
			tev.GXSetTevAlphaIn(stage, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_APREV);
			tev.GXSetTevAlphaOp(stage, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
			tev.ptr()->PADDING_0x18[6] = UPDATED_TEV_MAGIC_NUMBER;
			tev.DCStore(false);
			XOBJ_ASSERT_LINE(0x89C, numTevStages == numTevStageData);
		} else {
			XOBJ_ASSERT_LINE(0x89E, numTevStageData == numTevStages + 1);
		}
		XOBJ_ASSERT_LINE(0x8A0, tev.ptr()->PADDING_0x18[6] == UPDATED_TEV_MAGIC_NUMBER);
	}
}

// 0x800AFF80
void XObj::vf1BC(s32 mode)
{
	if (mObject.m_pScnMdl == NULL) {
		return;
	}
	for (u32 i = 0; i < mObject.m_resMdl.GetResMatNumEntries(); i++) {
		ResMat mat = mObject.m_resMdl.GetResMat(i);
		ScnMdl::CopiedMatAccess access(mObject.m_pScnMdl, i);
		ResGenMode genMode;
		genMode = access.GetResGenMode(false);
		if (!genMode.IsValid()) {
			genMode = mat.GetResGenMode();
		}
		genMode.GXSetCullMode((GXCullMode)mode);
		mat.DCStore(false);
	}
}

// 0x800B008C, keeps each tev's old cull mode in the spare tag byte before forcing mode
void XObj::SetCullMode(s32 mode)
{
	if (mObject.m_pScnMdl == NULL) {
		return;
	}
	for (u32 i = 0; i < mObject.m_resMdl.GetResMatNumEntries(); i++) {
		ResMat mat = mObject.m_resMdl.GetResMat(i);
		ScnMdl::CopiedMatAccess access(mObject.m_pScnMdl, i);
		ResGenMode genMode;
		genMode = access.GetResGenMode(false);
		if (!genMode.IsValid()) {
			genMode = mat.GetResGenMode();
		}
		ResTev tev;
		tev = access.GetResTev(false);
		if (!tev.IsValid()) {
			tev = mat.GetResTev();
		}
		if (tev.ptr()->PADDING_0x18[5] == 0) {
			tev.ptr()->PADDING_0x18[5] = genMode.GXGetCullMode() + 1;
		}
		genMode.GXSetCullMode((GXCullMode)mode);
		mat.DCStore(false);
	}
}

// 0x800B0210
void XObj::ResetCullMode()
{
	if (mObject.m_pScnMdl == NULL) {
		return;
	}
	for (u32 i = 0; i < mObject.m_resMdl.GetResMatNumEntries(); i++) {
		ResMat mat = mObject.m_resMdl.GetResMat(i);
		ScnMdl::CopiedMatAccess access(mObject.m_pScnMdl, i);
		ResTev tev;
		tev = access.GetResTev(false);
		if (!tev.IsValid()) {
			tev = mat.GetResTev();
		}
		if (tev.ptr()->PADDING_0x18[5] == 0) {
			continue;
		}
		ResGenMode genMode;
		genMode = access.GetResGenMode(false);
		if (!genMode.IsValid()) {
			genMode = mat.GetResGenMode();
		}
		genMode.GXSetCullMode((GXCullMode)(tev.ptr()->PADDING_0x18[5] - 1));
		mat.DCStore(false);
	}
}

extern "C" CVec ReflectVectorAboutNormal(const Vec* source, const Vec* normal);
extern "C" BOOL IsPlayerStopped(const WiiPlayerState* chr);

static inline void StoreFloorEdge(XObjEdge& edge, const Vec& origin, const Vec& a, const Vec& b)
{
	edge.mContactOffset.x -= origin.x;
	edge.mContactOffset.y -= origin.y;
	edge.mContactOffset.z -= origin.z;
	edge.mFrom.x = a.x - origin.x;
	edge.mFrom.y = a.y - origin.y;
	edge.mFrom.z = a.z - origin.z;
	edge.mTo.x   = b.x - origin.x;
	edge.mTo.y   = b.y - origin.y;
	edge.mTo.z   = b.z - origin.z;
}

static inline void StoreSweepContact(const Vec* from, const Vec* to, Vec* contact, Vec& dest, const Vec& hit, f32 top)
{
	if (to->y == top) {
		contact->x = to->x;
		// The original copies the destination height into Z on an exact top hit.
		contact->z = to->y;
		contact->y = from->y;
	} else {
		contact->x = hit.x;
		contact->z = hit.z;
		contact->y = from->y;
		dest.x     = hit.x;
		dest.z     = hit.z;
	}
}

// the floor coordinate readers this unit expands inline (same reads as cjanim.cpp's
// GetFloorCenterOffset/GetFloorExtent; the GameCube's objectfloorquery.h version
// initialized from template globals the Wii no longer has)
static inline Vec ReadObjectFloorOffset(const XObj* object, s32 index)
{
	Vec offset = { object->mFloors[index].mCenterOffset[0] / 100.0f, object->mFloors[index].mCenterOffset[1] / 100.0f,
		           object->mFloors[index].mCenterOffset[2] / 100.0f };
	return offset;
}

static inline Vec ReadObjectFloorExtent(const XObj* object, s32 index)
{
	Vec extent = { object->mFloors[index].mExtent[0] / 100.0f, object->mFloors[index].mExtent[1] / 100.0f,
		           object->mFloors[index].mExtent[2] / 100.0f };
	return extent;
}

// 0x800B034C
// NONMATCHING: retail hands OnPlayerContact the contact delta as an unnamed temporary (lowest stack slot) and calls
// PSVECMag between building it and the call, as if the virtual took (const Vec&, f32, s32); the overrides live in
// the other Wii units, so the signature stays (s32, s32) for now
u32 XObj::TestFloorCollision(const Vec* from, const Vec* to, f32 radius, f32 height, Vec* push, Vec* contact, s32* cursor, s32 flags)
{
	if (!(mStatus.mFlags & 0x0200000000000000ULL)) {
		if (cursor)
			*cursor = -1;
		return 0;
	}
	if (mStatus.mFlags & 0x0C00000000000000ULL) {
		if (cursor)
			*cursor = -1;
		return 0;
	}
	if (mStatus.mFlags & 0x4000000000000000ULL) {
		if (cursor)
			*cursor = -1;
		return 0;
	}
	u32 result = 0;
	CVec start(*(const CVec*)from); // the callers pass CVecs
	start.y = 0;
	CVec dest(*(const CVec*)to);
	dest.y = 0;
	CVec hit;
	s32 i = cursor ? *cursor : 0; // a cursor lets a caller resume after a hit
	for (; mFloors[i].mId != -1; ++i) {
		if (!(mFloors[i].mFlags & 1)) // the Wii skips floors this flag clears
			continue;
		u32 collision = 0;
		Vec center    = mMoveScale * (mVec714 + ReadObjectFloorOffset(this, i));
		Vec extent    = mMoveScale * ReadObjectFloorExtent(this, i);
		f32 objYaw    = mMoveRotate.y;
		f32 angle     = DEG2RAD * mFloors[i].mYawDegrees;
		f32 yaw       = objYaw + angle;
		switch (mFloors[i].mId) {
		case 0:
		case 1: { // a disc
			CVec middle(0, 0, 0);
			Mtx world, local, rotate, scale, transform;
			PSMTXTrans(world, mMoveTranslate.x, mMoveTranslate.y, mMoveTranslate.z);
			PSMTXTrans(local, center.x, center.y, center.z);
			PSMTXScale(scale, extent.x, extent.y, extent.z);
			PSMTXRotRad(rotate, 'y', yaw);
			PSMTXConcat(world, rotate, transform);
			PSMTXConcat(transform, local, transform);
			PSMTXConcat(transform, scale, transform);
			PSMTXMultVec(transform, &middle, &middle);
			const f32 y = middle.y;
			middle.y    = 0;
			f32 reach   = radius + extent.x;
			if (PSVECDistance(&middle, &dest) < reach) { // inside the disc's radius
				CVec travel              = (dest - start).Normalize();
				CVec normal              = (dest - middle).Normalize();
				CVec reflect             = ReflectVectorAboutNormal(&travel, &normal);
				*push                    = reflect * (dest - start).Length() * 0.0f;
				hit                      = middle + normal * reach;
				mEdges[i].mContactOffset = (middle + normal * extent.x) - mMoveTranslate;
				if (to->y <= mFloorHeights[i] && to->y >= ((y - extent.y) - height)) {
					StoreSweepContact(from, to, contact, dest, hit, mFloorHeights[i]);
					collision = 3; // touching it within its height span
				} else
					collision = to->y < ((y - extent.y) - height) ? 0 : 1; // above it
			}
			break;
		}
		case 2: { // a box
			Mtx base, local, rotate, scale, transform;
			PSMTXTrans(base, mMoveTranslate.x, mMoveTranslate.y, mMoveTranslate.z);
			PSMTXTrans(local, center.x, center.y, center.z);
			PSMTXScale(scale, extent.x, extent.y, extent.z);
			PSMTXRotRad(rotate, 'y', yaw);
			PSMTXConcat(base, rotate, transform);
			PSMTXConcat(transform, local, transform);
			PSMTXConcat(transform, scale, transform);
			f32 h = (2.0f / (3.0f * nw4r::math::FrSqrt(3.0f))) * 0.5f;
			static Vec corners[8]
			    = { { -h, h, -h }, { h, h, -h }, { h, h, -h }, { h, h, h }, { h, h, h }, { -h, h, h }, { -h, h, h }, { -h, h, -h } };
			Vec world[8];
			PSMTXMultVecArray(transform, corners, world, 8);
			for (s32 side = 0; side < 4; ++side) {
				collision = ResolveCircleWallCollisionXZ(&start, &dest, &world[side * 2], &world[side * 2 + 1], &hit,
				                                         &mEdges[i].mContactOffset, push, 0, radius);
				if (collision) {
					collision = 8; // a wall hit
					StoreFloorEdge(mEdges[i], mMoveTranslate, world[side * 2], world[side * 2 + 1]);
					break;
				}
			}
			if (!collision && IsPointInsideQuadXZ(&dest, &world[0], &world[2], &world[4], &world[6])) {
				collision = 4; // inside it
				hit.x     = to->x;
				hit.z     = to->z;
			}
			if (collision) {
				if (to->y <= mFloorHeights[i] && to->y >= (world[0].y - 2.f * (extent.y * h) - height)) {
					StoreSweepContact(from, to, contact, dest, hit, mFloorHeights[i]);
					collision = 3;
				} else
					collision = to->y < (world[0].y - 2.f * (extent.y * h) - height) ? 0 : 1; // above it
			}
			break;
		}
		case 3: { // a wall segment
			Mtx base, local, rot, extra, scale, transform;
			PSMTXTrans(base, mMoveTranslate.x, mMoveTranslate.y, mMoveTranslate.z);
			PSMTXTrans(local, center.x, center.y, center.z);
			PSMTXScale(scale, extent.x, extent.y, extent.z);
			PSMTXRotRad(rot, 'y', objYaw);
			PSMTXRotRad(extra, 'y', angle);
			PSMTXConcat(base, rot, transform);
			PSMTXConcat(transform, local, transform);
			PSMTXConcat(transform, extra, transform);
			PSMTXConcat(transform, scale, transform);
			Vec wall[2];
			static Vec ends[2] = { { -.5f, 0, 0 }, { .5f, 0, 0 } };
			PSMTXMultVecArray(transform, ends, wall, 2);
			u32 touched
			    = ResolveCircleWallCollisionXZ(&start, &dest, &wall[0], &wall[1], contact, &mEdges[i].mContactOffset, push, 1, radius);
			if (touched) {
				collision  = 3; // the wall stops the sweep
				contact->y = from->y;
				StoreFloorEdge(mEdges[i], mMoveTranslate, wall[0], wall[1]);
			}
			break;
		}
		case 5: { // a quad from this and the next three records
			Mtx translate, rotate, scale, transform;
			Vec quad[4];
			for (s32 j = 0; j < 4; ++j)
				quad[j] = mVec714 + ReadObjectFloorOffset(this, i + j);
			Vec size       = mMoveScale * ReadObjectFloorExtent(this, i);
			f32 floorAngle = DEG2RAD * mFloors[i].mYawDegrees;
			f32 rotY       = mMoveRotate.y + floorAngle;
			PSMTXTrans(translate, mMoveTranslate.x, mMoveTranslate.y, mMoveTranslate.z);
			PSMTXScale(scale, size.x, size.y, size.z);
			PSMTXRotRad(rotate, 'y', rotY);
			PSMTXConcat(translate, rotate, transform);
			PSMTXConcat(transform, scale, transform);
			PSMTXMultVecArray(transform, quad, quad, 4);
			for (s32 j = 0; j < 4; ++j) {
				collision = ResolveCircleWallCollisionXZ(&start, &dest, &quad[j], &quad[(j + 1) % 4], &hit, &mEdges[i + 3].mContactOffset,
				                                         push, 1, radius);
				if (collision) {
					collision = 8;
					StoreFloorEdge(mEdges[i + 3], mMoveTranslate, quad[j], quad[(j + 1) % 4]);
					break;
				}
			}
			if (!collision && IsPointInsideQuadXZ(&dest, &quad[0], &quad[1], &quad[2], &quad[3])) {
				collision = 4;
				hit.x     = to->x;
				hit.z     = to->z;
			}
			if (collision) {
				if (to->y <= mFloorHeights[i] && to->y >= (quad[0].y - extent.y / 2.f - height)) {
					StoreSweepContact(from, to, contact, dest, hit, mFloorHeights[i]);
					collision = 3;
				} else
					collision = to->y < (quad[0].y - extent.y / 2.f - height) ? 0 : 1; // above it
			}
			i += 3;
			break;
		}
		}
		if (collision) {
			mFloorMasks[0] |= 1u << i; // mark the floor touched
			result |= collision;
			mHitFloorIndex = i;
			if (cursor) {
				++i;
				break;
			}
		}
	}
	if (cursor) {
		*cursor = i;
		if (mFloors[i].mId == -1)
			*cursor = -1;
	}
	if (result) {
		mStatus.mFlags |= 0x0100000000000000ULL;
		if ((flags & 2) && (result & 3) == 3) { // any contact (3) can start its event script
			WiiPlayerState* player = gGame.GetPlayer();
			if ((mStatus.mFlags & 0x20000000000ULL) && !(mStatus.mFlags & 0x8000000000000000ULL) && !gGame.GetStatusFlags(0x400)
			    && player->mAttachmentRequest < 0
			    && (!gGame.GetStatusFlags(0x40000000) || !IsPlayerStopped(player) || !player->mBatteryDrains
			        || (mStatus.mFlags & 0x8000))) {
				mStatus.mFlags |= 0x8000000000000000ULL;
				u8* script = (u8*)GetEventScript();
				StartOrQueueEventScript(&gGame.mEvt, script, this, -1, -1);
			}
		}
		if (flags & 1) { // let the object's own handler react and adjust the result
			const CVec& delta = *(CVec*)contact - *from;
			PSVECMag(push);
			result = OnPlayerContact((s32)&delta, result);
		}
	} else
		mStatus.mFlags &= ~0x0100000000000000ULL;
	return result;
}

#endif
