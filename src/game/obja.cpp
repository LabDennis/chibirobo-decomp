// XObj's own file: InitializeObject to CJAnim::Release, written in the reverse of retail's code order
// (-inline auto,deferred).
// NONMATCHING: the data (US offsets; JP and PAL alike except where said). .rodata lacks the 0x48 block of six
// Vecs at 0x00 that CGame's particle inlines make in every file, so the four HSD_PEDesc
// presets at 0x5C stay behind the gObjectPixelEnginePresets stand-in: as the local initialisers retail copies
// they come out at 0x14 and ApplyJointMaterialPEPresets drops to 99.95 %. Its strings after CJArchive are
// retail's jobj.h assert, translate, CXObjIntp; the build emits CXObjIntp, translate, assert. .data lacks the
// 4-byte pad at 0x00 ("0000", PAL zero) and the 26-word header block at 0x04 (0x6C bytes together); the floor
// tables, CJAnim's and XObj's vtables and base lists behind it come out in retail's order. .bss is the
// 0x140-byte (JP 0x134) header-made prefix of zeroed Vecs: the build has LoadFromPlacement's placement
// defaults (initial, 0x24) and cjobjset.h's three static zero Vecs instead, and the floor slots at 0xBC/0xC8
// stay the gObjectFloorOffsetTemplate/lbl_80378148 externs. In .sdata2 retail has 2.0f before 0.5f (0x8065D1E0)
// and 160.0f before 128.0f (0x8065D1F0, DrawGroundShadowAt's); the build has both pairs the other way round.
// The build also emits header constants, jobj.h's "rotate" and "scale" assert strings and weak RTTI records
// that retail's block lacks; the unreferenced and duplicate ones go at link time

#include "game/xobj.h"
#include "game/gmath.h"
#include "game/objectfloorquery.h"
#include "game/cjobjset.h"
#include "game/cgame.h"
#include "game/hsd_dobj.h"
#include "game/globals.h"
#include "game/xobjchr.h"
#include "game/cobj.h"
#include "game/cfile.h"
#include <math.h>
#include "game/xchr.h"
#include "game/cvec.h"
#include "game/spiderplacement.h"
#include "game/mm.h"
#include "game/arcache.h"
#include "game/xobjmap.h"
#include <stdio.h>

extern "C" CObjArchive* FindFreeObjectArchive(CObjArchive* archives);

extern "C" {
void GXProject(f32 x, f32 y, f32 z, const Mtx view, const f32* projection, const f32* viewport, f32* screenX, f32* screenY, f32* screenZ);
}

static inline void TriggerObjectEvent(XObj* object, u32 first, u32 second)
{
	const void* script = object->GetEventScript();
	if (!IsPlayerBlockingEvents(gGame.GetPlayer())) {
		StartOrQueueEventScript(&gGame.mEvt, (u8*)script, object, first, second);
	}
}

extern "C" void PSMTXMultVecArray(const Mtx m, const Vec* src, Vec* dst, u32 count);

static inline void CountCrossedEdge(Vec& from, Vec& to, Vec* a, Vec* b, s32& outside, s32& crossed)
{
	f32 first = SignedTriangleArea2XZ(&from, a, b);
	f32 last  = SignedTriangleArea2XZ(&to, a, b);
	if (first < 0.0f) {
		++outside;
		if (last > 0.0f)
			++crossed;
	}
}

extern "C" {
#include <dolphin/mtx/vec.h>
}

extern "C" {
#include <sysdolphin/class_alloc.h>
}

// XObj, the CXObjIntp every placed object in the house is built on

extern "C" {
void C_VECReflect(const Vec* source, const Vec* normal, Vec* result);
}

#if defined(VERSION_PAL)
#define ANIMATION_RATE(rate) ((rate) * (60.0f / (f32)gVideoFramesPerSecond))

#else
#define ANIMATION_RATE(rate) (rate)
#endif

// the pe presets sit 0x5C into the file's .rodata, behind the header-made six Vecs
struct XObjPEPresets {
	u8 _00[0x5C];          // _00
	HSD_PEDesc mReset;     // _5C
	HSD_PEDesc mPreset[3]; // _68
};

extern "C" {
extern const XObjPEPresets gObjectPixelEnginePresets;
}

static inline u32 ShadowSize(XObj* object)
{
	return object->mAttr.mObjData != NULL ? object->mAttr.mObjData->mShadowSize : 0;
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

static inline void ExpandRadius(f32 x, f32 z, f32& maxX, f32& maxZ)
{
	if (ABS(x) > maxX)
		maxX = ABS(x);
	if (ABS(z) > maxZ)
		maxZ = ABS(z);
}

// the floor readers the Wii build's cjanim.cpp keeps; retail GameCube has GetFloorYawRadians out of line
// in obja_chr.cpp (0x800B247C) with the same signature
static inline Vec GetFloorCenterOffset(XObjFloor* const* floors, s32 index)
{
	Vec offset = gObjectFloorOffsetTemplate;
	offset.x   = (*floors)[index].mCenterOffset[0] / 100.0f;
	offset.y   = (*floors)[index].mCenterOffset[1] / 100.0f;
	offset.z   = (*floors)[index].mCenterOffset[2] / 100.0f;
	return offset;
}

static inline Vec GetFloorExtent(XObjFloor* const* floors, s32 index)
{
	Vec extent = lbl_80378148;
	extent.x   = (*floors)[index].mExtent[0] / 100.0f;
	extent.y   = (*floors)[index].mExtent[1] / 100.0f;
	extent.z   = (*floors)[index].mExtent[2] / 100.0f;
	return extent;
}

static inline f32 GetFloorYawRadians(XObjFloor* const* floors, s32 index)
{
	return DEG2RAD * (*floors)[index].mYawDegrees;
}

static inline s32 IntersectFloorRay(const Vec& start, const Vec& end, const Vec& a, const Vec& b, CVec& hit, s32 segment)
{
	return IntersectLinesXZ(&start, &end, &a, &b, &hit, segment);
}

static inline void FloorNormalAngle(const CVec& normal, Vec* out, f32& angle)
{
	angle = WrapAngle(atan2(normal.x, normal.z));
	if (out)
		*out = normal;
}

extern "C" CVec ReflectVectorAboutNormal(const Vec* source, const Vec* normal);
extern "C" void SetJointMaterialOpaque(HSD_JObj* joint, f32** arguments, s32 type);
extern "C" void SetJointMaterialTranslucent(HSD_JObj* joint, f32** arguments, s32 type);
extern "C" void ScaleJointMaterialDiffuse(HSD_JObj* joint, f32** arguments, s32 type);
extern "C" void SetJointMaterialDestAlpha(HSD_JObj* joint, f32** arguments, s32 type);

extern "C" BOOL IsPlayerStopped(const XObjChr* player);

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

extern "C" {

CObjArchive* FindFreeObjectArchive(CObjArchive*);
u32 ARGetSize(void);
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

// XObj::vf90

/**
 * @note Address: 0x800929C4
 * @note Size: 0xE0
 */
void XObj::InitializeObject(u32 id)
{
	CXObj::Init(id);
	mStatus.mFlags    = 0;
	mCoord.mPlacement = NULL;
	mAttr.mObjData    = NULL;
	mAttr.mPlacement  = NULL;
	mFloors           = NULL;
	mPoolIndex        = 0;
	ResetObjectState();
	InitializeEventScriptContext(&mEvt, gGame.GetMsg(1), this);
	mAnmQueue.Clear();
	mAnim.Clear();
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

// NONMATCHING: retail reads the placement defaults (initial) at 0xD4 into the file's shared .bss block
// (lbl_80378080) through one base register; that prefix is header-made (see the top of the file)
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
			mFloors        = GetObjectCollisionList(info.mPlacement->mObjectId);
			mFloorMasks[0] = mFloorMasks[1] = 0;
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
				flags |= 0x10000;
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
				if ((flags & 2) && FitsInAram(archive)) {
					u32 fileSize = archive->mFile.mSize;
					if (attr.mObjData && attr.mObjData->mSource) {
						attr.mObjData->mSource->mCache.address = gARAMCacheInfo.mBaseAddress; // remember the ARAM copy
						attr.mObjData->mSource->mCache.size    = fileSize;
					}
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
					u32 padded             = (PaddedSize(archive) + 31) & ~31u;

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
			s32 index = GetStagePlacementIndex(mCoord.mPlacement); // the placement's index in the stage
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

// XObj::ApplyMaterialPresets, the flags go to ApplyJointMaterialPEPresets on every joint
/**
 * @note Address: 0x80091C44
 * @note Size: 0x3C
 */
void XObj::ApplyMaterialPresets(u32* flags)
{
	if (mJObj != NULL)
		HSD_JObjWalkTree(mJObj, ApplyJointMaterialPEPresets, (f32**)flags);
}

// joint walk callback for XObj::ApplyMaterialPresets, the args are five on/off flags
/**
 * @note Address: 0x80091888
 * @note Size: 0x3BC
 */
extern "C" void ApplyJointMaterialPEPresets(HSD_JObj* joint, f32** arguments, s32 type)
{
	const XObjPEPresets* presets = &gObjectPixelEnginePresets;
	HSD_DObj* draw;
	u32 reset        = (u32)arguments[0];
	u32 firstPreset  = (u32)arguments[1];
	u32 secondPreset = (u32)arguments[2];
	u32 thirdPreset  = (u32)arguments[3];
	u32 toon         = (u32)arguments[4];
	for (draw = HSD_JObjGetDObj(joint); draw != NULL; draw = HSD_DObjGetNext(draw)) {
		HSD_MObj* material = HSD_DObjGetMObj(draw);
		if (material != NULL) {
			if (reset) {
				if (material->pe == NULL)
					material->pe = (HSD_PEDesc*)hsdAllocMemPiece(sizeof(HSD_PEDesc));
				HSD_PEDesc settings = presets->mReset;
				*material->pe       = settings;
				if (material->rendermode & RENDER_XLU)
					material->pe->type = 1;
				if (material->rendermode & RENDER_NO_ZUPDATE)
					material->pe->flags &= ~0x20;
				if (material->rendermode & RENDER_ZMODE_ALWAYS)
					material->pe->flags &= ~0x10;
			}
			if (firstPreset) {
				if (material->pe == NULL)
					material->pe = (HSD_PEDesc*)hsdAllocMemPiece(sizeof(HSD_PEDesc));
				HSD_PEDesc settings = presets->mPreset[0];
				*material->pe       = settings;
			} else if (secondPreset) {
				if (material->pe == NULL)
					material->pe = (HSD_PEDesc*)hsdAllocMemPiece(sizeof(HSD_PEDesc));
				HSD_PEDesc settings = presets->mPreset[1];
				*material->pe       = settings;
			} else if (thirdPreset) {
				if (material->pe == NULL)
					material->pe = (HSD_PEDesc*)hsdAllocMemPiece(sizeof(HSD_PEDesc));
				HSD_PEDesc settings = presets->mPreset[2];
				*material->pe       = settings;
				if (material->rendermode & RENDER_XLU)
					material->pe->type = 1;
			}
			if (toon) {
				if (!(GetMaterialRenderModeFlags(material) & RENDER_TOON)) {
					HSD_MObjSetFlags(material, RENDER_TOON);
					HSD_MObjCompileTev(material);
				}
			} else {
				if (GetMaterialRenderModeFlags(material) & RENDER_TOON) {
					HSD_MObjClearFlags(material, RENDER_TOON);
					HSD_MObjCompileTev(material);
				}
			}
		}
	}
}

/**
 * @note Address: 0x80091834
 * @note Size: 0x54
 */
void XObj::SetDestinationAlpha(BOOL enabled, u8 value)
{
	void* arguments[2] = { NULL, NULL };
	arguments[0]       = (void*)enabled;
	arguments[1]       = (void*)value;
	if (mJObj != NULL)
		HSD_JObjWalkTree(mJObj, SetJointMaterialDestAlpha, (f32**)arguments);
}

/**
 * @note Address: 0x80091784
 * @note Size: 0xB0
 */
extern "C" void SetJointMaterialDestAlpha(HSD_JObj* joint, f32** arguments, s32 type)
{
	BOOL enabled = (BOOL)arguments[0];
	u8 alpha     = (u32)arguments[1];
	for (HSD_DObj* draw = HSD_JObjGetDObj(joint); draw != NULL; draw = HSD_DObjGetNext(draw)) {
		HSD_MObj* material = HSD_DObjGetMObj(draw);
		if (material != NULL && material->pe != NULL) {
			if (enabled)
				material->pe->flags |= 6;
			else
				material->pe->flags &= ~6;
			material->pe->dst_alpha = alpha;
		}
	}
}

/**
 * @note Address: 0x8009172C
 * @note Size: 0x58
 */
void XObj::EnableLighting(f32 factor)
{
	void* arguments[2] = { NULL, NULL };
	arguments[0]       = this;
	arguments[1]       = &factor;
	if (mJObj != NULL)
		HSD_JObjWalkTree(mJObj, ScaleJointMaterialDiffuse, (f32**)arguments);
}

/**
 * @note Address: 0x80091540
 * @note Size: 0x1EC
 */
extern "C" void ScaleJointMaterialDiffuse(HSD_JObj* joint, f32** arguments, s32 type)
{
	f32 factor = *arguments[1];
	HSD_JObjSetFlags(joint, 0x80);
	for (HSD_DObj* draw = HSD_JObjGetDObj(joint); draw != NULL; draw = HSD_DObjGetNext(draw)) {
		HSD_MObj* material = HSD_DObjGetMObj(draw);
		if (material != NULL) {
			HSD_MObjSetFlags(material, 4);
			if (material->mat != NULL && __fpclassifyf(factor) != FP_NAN) {
				material->mat->diffuse.r = (u8)(material->mat->diffuse.r * factor);
				material->mat->diffuse.g = (u8)(material->mat->diffuse.g * factor);
				material->mat->diffuse.b = (u8)(material->mat->diffuse.b * factor);
			}
			for (HSD_TObj* texture = HSD_MObjGetTObj(material); texture != NULL; texture = HSD_TObjGetNext(texture)) {
				u32 value      = texture->flags & ~0x1F0;
				texture->flags = value;
				texture->flags = value | 0x50;
				value          = texture->flags & ~0xF0000;
				texture->flags = value;
				texture->flags = value | 0x70000;
			}
			HSD_MObjCompileTev(material);
		}
	}
}

/**
 * @note Address: 0x800914CC
 * @note Size: 0x74
 */
void XObj::SetTranslucent(f32 alpha)
{
	void* arguments[2] = { NULL, NULL };
	arguments[0]       = this;
	arguments[1]       = &alpha;
	if (mJObj != NULL)
		HSD_JObjWalkTree(mJObj, SetJointMaterialTranslucent, (f32**)arguments);
	HSD_JObjModifyFlags(mJObj, 0x20000000, 0x70000000);
}

/**
 * @note Address: 0x8009146C
 * @note Size: 0x60
 */
void XObj::SetOpaque()
{
	void* arguments[1] = { NULL };
	arguments[0]       = this;
	if (mJObj != NULL)
		HSD_JObjWalkTree(mJObj, SetJointMaterialOpaque, (f32**)arguments);
	HSD_JObjModifyFlags(mJObj, 0x10000000, 0x70000000);
}

/**
 * @note Address: 0x8009136C
 * @note Size: 0x100
 */
extern "C" void SetJointMaterialTranslucent(HSD_JObj* joint, f32** arguments, s32 type)
{
	CXObj* object = (CXObj*)arguments[0];
	f32 alpha     = *arguments[1];
	HSD_JObjModifyFlags(joint, 0x80000, 0x1C0000);
	for (HSD_DObj* draw = HSD_JObjGetDObj(joint); draw != NULL; draw = HSD_DObjGetNext(draw)) {
		HSD_DObjModifyFlags(draw, 4, 0xE);
		HSD_MObj* material = HSD_DObjGetMObj(draw);
		if (material != NULL) {
			HSD_MObjSetFlags(material, RENDER_XLU | RENDER_NO_ZUPDATE);
			HSD_MObjSetFlags(material, RENDER_ALPHA_MAT);
			HSD_MObjCompileTev(material);
			if (material->mat != NULL)
				HSD_MObjSetAlpha(material, alpha * object->GetMObjAlpha(material));
		}
	}
}

/**
 * @note Address: 0x800912B4
 * @note Size: 0xB8
 */
extern "C" void SetJointMaterialOpaque(HSD_JObj* joint, f32** arguments, s32 type)
{
	HSD_JObjModifyFlags(joint, 0x40000, 0x1C0000);
	for (HSD_DObj* draw = HSD_JObjGetDObj(joint); draw != NULL; draw = HSD_DObjGetNext(draw)) {
		HSD_DObjModifyFlags(draw, 2, 0xE);
		HSD_MObj* material = HSD_DObjGetMObj(draw);
		if (material != NULL) {
			HSD_MObjClearFlags(material, RENDER_XLU | RENDER_NO_ZUPDATE | RENDER_ZMODE_ALWAYS);
			HSD_MObjSetFlags(material, RENDER_ALPHA_MAT);
			HSD_MObjCompileTev(material);
		}
	}
}

/**
 * @note Address: 0x800911C0
 * @note Size: 0xF4
 */
void XObj::Release()
{
	CJArchive* archive = &mAnim;
	archive->Release();
	if (mEvt.mData.mScript != NULL)
		ResetEventScriptState(&mEvt, TRUE, FALSE);
	if (mStatus.mFlags & 1) {
		CXObjIntp::Release();
		UnkObjData* data = mAttr.mObjData;
		if (data != NULL && data->mSource != NULL) {
			XObjSource* source = data->mSource;
			if (source->mArchive != NULL && source->mArchive->mRefCount <= 0) // its last user is gone
				source->mArchive = NULL;
		}
	}
	mAttr.mObjData   = NULL;
	mAttr.mPlacement = NULL;
	mFloors          = NULL;
	mStatus.mFlags &= ~0x10001ULL;
}

/**
 * @note Address: 0x80090ED4
 * @note Size: 0x2EC
 */
void XObj::UpdateObject(u32 frames)
{
	if (mJObj != NULL) {
		StepColors(frames);
		if (mStatus.mFlags & 0x1000000000000000ULL) { // toppling: sink onto the floor hit
			if (mMoveTranslate.y + mToppleOffset.y > mFloorHit.mHeight) {
				mMoveTranslate.mTarget.y = mFloorHit.mHeight;
				f32 speed                = 0.15f + mMoveTranslate.mSpeed.y; // fall faster, capped at 10
				if (speed > 10.0f)
					speed = 10.0f;
				mMoveTranslate.mSpeed.y = speed;
			} else { // landed: take over the offset and stand straight
				mMoveTranslate.mSpeed.y = 0.0f;
				mMoveTranslate.x += mToppleOffset.x;
				mMoveTranslate.z += mToppleOffset.z;
				mMoveTranslate.y = mFloorHit.mHeight;
				mToppleOffset.x  = 0.0f;
				mToppleOffset.y  = 0.0f;
				mToppleOffset.z  = 0.0f;
				SetRotationX(0.0f, 0);
				SetRotationZ(0.0f, 0);
				mStatus.mFlags &= ~0x1000000000000000ULL;
				if ((s16)GetPool() == 21) {
					mStatus.mFlags |= 0x2000000000000000ULL;
				} else if ((s16)GetPool() == 22) {
					if (mStatus.mFlags & 0x04000000ULL)
						mStatus.mFlags |= 0x2000000000000000ULL;
				}
			}
		}
		// move, scale and turn toward their targets, then animate
		CXObjIntp::StepRotate(frames);
		CXObjIntp::StepTranslate(frames);
		if (CXObjIntp::StepScale(frames)) {
			UpdateObjectFloorHeights((XObj*)this);
			UpdateObjectFloorXZExtent((XObj*)this);
		}
		UpdateAnim(frames);
		HSD_JObjSetupMatrix(mJObj);
	}
	if ((mStatus.mFlags & 0x0010000000000000ULL) || mMoveTranslate.y != mLastPos.y) { // floor heights track the height
		UpdateObjectFloorHeights((XObj*)this);
	}
	if (mEvt.mData.mScript != NULL)
		mEvt.Update(frames);
}

/**
 * @note Address: 0x80090C84
 * @note Size: 0x250
 */
void XObj::StepColors(u32 frames)
{
	if (mJObj != NULL) {
		for (s32 i = 0; i < 2; ++i) {
			XObjColorStep& color = mColorStep[i];
			StepColor(color.mValue.x, color.mTarget.x, color.mSpeed.x, frames);
			StepColor(color.mValue.y, color.mTarget.y, color.mSpeed.y, frames);
			StepColor(color.mValue.z, color.mTarget.z, color.mSpeed.z, frames);
		}
		// the stepped channels get packed back into the color words
		mColors[1] = ((u32)mColorStep[0].mValue.x << 24) | ((u32)mColorStep[0].mValue.y << 16) | ((u32)mColorStep[0].mValue.z << 8);
		mColors[0] = ((u32)mColorStep[1].mValue.x << 24) | ((u32)mColorStep[1].mValue.y << 16) | ((u32)mColorStep[1].mValue.z << 8);
	}
}

/**
 * @note Address: 0x800908C0
 * @note Size: 0x3C4
 */
void XObj::DrawWithView(Mtx view)
{
	HSD_AObj* animation = GetAObj(mJObj);
	if (animation != NULL && (animation->flags & (AOBJ_NO_ANIM | AOBJ_REWINDED)) && HasQueuedAnim()) {
		s32 id;
		u16 frame;
		s16 rate;
		PopAnim(id, frame, rate);
		if (id & 0xFFFF0000) { // packed ids: material in the high half, joint in the low
			s32 jointId             = id & 0xFFFF;
			s32 materialId          = id >> 16;
			void* jointAnimation    = GetJointAnimation(jointId);
			void* materialAnimation = GetMaterialAnimation(materialId);
			void* shapeAnimation    = GetShapeAnimation(jointId);
			SetAnim(0, TRUE, jointAnimation, materialAnimation, shapeAnimation, (f32)frame, rate / 100.0f, 0.0f, 10.0f);
			if (jointId >= 1000 || materialId >= 1000)
				mStatus.mFlags |= 0x0008000000000000ULL;
		} else {
			SetAnim(id, FALSE, NULL, NULL, NULL, (f32)frame, rate / 100.0f, 0.0f, 10.0f);
		}
		UpdateAnim(1);
	}
	if (!(mStatus.mFlags & 0x800ULL)) {
		if (mJObj != NULL)
			HSD_JObjDispAll(mJObj, view, 7, 0);
	}
}

/**
 * @note Address: 0x80090864
 * @note Size: 0x5C
 */
void XObj::DrawGroundShadow()
{
	if (mStatus.mFlags & 8ULL) {
		DrawGroundShadowAt(&mMoveTranslate, 1.0f);
	}
}

/**
 * @note Address: 0x8009056C
 * @note Size: 0x2F8
 */
void XObj::DrawGroundShadowAt(const Vec* origin, f32 opacity)
{
	if (!(mStatus.mFlags & 8ULL))
		return;
	if (ShadowSize(this) == 0)
		return;
	Vec position = *origin;
	Vec rotation = { -HALF_PI, 0.0f, 0.0f };
	Vec size     = mMoveScale * (f32)ShadowSize(this);
	SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 4, 5, 15);
	u32 edge   = (u32)(128.0f * opacity);
	u32 center = (u32)(160.0f * opacity);
	DrawGsCircle(&gGame.mGs, 12, center, edge, position.x, position.y, position.z, size.x, size.z, rotation.x, rotation.y, rotation.z);
	PrimVertex vertices[26]; // a rim of transparent vertices fades the edge out
	f32 halfWidth  = size.x / 2.0f;
	f32 halfHeight = size.z / 2.0f;
	for (s32 i = 0; i < 13; i++) {
		f32 angle                 = (PI / 6.0f) * i;
		vertices[i * 2].pos.x     = position.x - halfWidth * (f32)sin(angle);
		vertices[i * 2].pos.z     = position.z + halfHeight * (f32)cos(angle);
		vertices[i * 2].pos.y     = position.y;
		vertices[i * 2].color     = edge;
		vertices[i * 2 + 1].pos.x = position.x - (1.5f * mMoveScale.x + halfWidth) * (f32)sin(angle);
		vertices[i * 2 + 1].pos.z = position.z + (1.5f * mMoveScale.z + halfHeight) * (f32)cos(angle);
		vertices[i * 2 + 1].pos.y = position.y;
		vertices[i * 2 + 1].color = 0;
	}
	DrawGsTriangleVertices(&gGame.mGs, GX_TRIANGLESTRIP, gGame.mGs.mCamera->mViewMtx, vertices, 26);
}

/**
 * @note Address: 0x80090568
 * @note Size: 0x4
 */
void XObj::DrawCollisionShapes()
{
}

/**
 * @note Address: 0x8009047C
 * @note Size: 0xEC
 */
void XObj::DrawScreenOverlay()
{
	if ((mStatus.mFlags & 0x800ULL) && (mStatus.mFlags & 0x10000ULL)) {
		HSD_JObjSetFlags(mJObj, CLASSICAL_SCALE);
		Vec scale = mMoveScale;
		SetScale(scale.x, -scale.y, scale.z, 0); // drawn flipped
		if (mJObj != NULL)
			HSD_JObjDispAll(mJObj, NULL, 7, 0);
		SetScale(scale, 0);
	}
}

/**
 * @note Address: 0x800902A4
 * @note Size: 0x1D8
 */
void XObj::SetAnim(s32 id, BOOL force, void* jointAnim, void* matAnim, void* shapeAnim, f32 frame, f32 rate, f32 blend, f32 blendRate)
{
	if (mStatus.mFlags & 0x0020000000000000ULL)
		blend = 1.0f;
	if (GetAnimSet(id) != 0) {
		s32 index = id % 1000;
		jointAnim = jointAnim != NULL ? jointAnim : mAnim.GetAnimJoint(index);
		matAnim   = matAnim != NULL ? matAnim : mAnim.GetMatAnimJoint(index);
		shapeAnim = shapeAnim != NULL ? shapeAnim : mAnim.GetShapeAnimJoint(index);
		CXObjIntp::SetAnim(id, force, jointAnim, matAnim, shapeAnim, frame, ANIMATION_RATE(rate), blend, blendRate);
		mStatus.mFlags |= 0x0008000000000000ULL; // the anim has a matching set
	} else {
		CXObjIntp::SetAnim(id, force, jointAnim, matAnim, shapeAnim, frame, ANIMATION_RATE(rate), blend, blendRate);
		mStatus.mFlags &= ~0x0008000000000000ULL;
	}
}

/**
 * @note Address: 0x80090254
 * @note Size: 0x50
 */
void XObj::ClearAnim()
{
	CXObj::ClearAnim();
	mStatus.mFlags &= ~0x0008000000000000ULL;
}

/**
 * @note Address: 0x800901E8
 * @note Size: 0x6C
 */
extern "C" CVec ReflectVectorAboutNormal(const Vec* source, const Vec* normal)
{
	CVec result;
	C_VECReflect(source, normal, &result);
	return result;
}

/**
 * @note Address: 0x8008ED50
 * @note Size: 0x1498
 */
// NONMATCHING: retail hands OnPlayerContact the contact delta as an unnamed temporary (lowest stack slot) and calls
// PSVECMag between building it and the call, as if the virtual took (const Vec&, f32, s32); the overrides live in
// xobjdust/hbox/coin and the spider files, so the signature stays (s32, s32) for now (probe: 99.36 -> 99.69)
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
		u32 collision = 0;
		Vec center    = mMoveScale * (mToppleOffset + ReadObjectFloorOffset(this, i));
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
			f32 h = (2.f / sqrtf(3.f)) / 2.f;
			static Vec corners[8]
			    = { { -h, h, -h }, { h, h, -h }, { h, h, -h }, { h, h, h }, { h, h, h }, { -h, h, h }, { -h, h, h }, { -h, h, -h } };
			Vec world[8];
			PSMTXMultVecArray(transform, corners, world, 8);
			for (s32 side = 0; side < 4; ++side) {
				collision = ResolveCircleWallCollisionXZ(&start, &dest, &world[side * 2], &world[side * 2 + 1], push, &hit,
				                                         &mEdges[i].mContactOffset, NULL, radius, 0, 0);
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
			u32 touched = ResolveCircleWallCollisionXZ(&start, &dest, &wall[0], &wall[1], push, contact, &mEdges[i].mContactOffset, NULL,
			                                           radius, 0, 1);
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
				quad[j] = mToppleOffset + ReadObjectFloorOffset(this, i + j);
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
				collision = ResolveCircleWallCollisionXZ(&start, &dest, &quad[j], &quad[(j + 1) % 4], push, &hit,
				                                         &mEdges[i + 3].mContactOffset, NULL, radius, 0, 1);
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
			XObjChr* player = gGame.GetPlayer();
			if ((mStatus.mFlags & 0x20000000000ULL) && !(mStatus.mFlags & 0x8000000000000000ULL) && !(gGame.mStatus.mFlags & 0x400)
			    && player->mAttachmentRequest < 0
			    && (!(gGame.mStatus.mFlags & 0x40000000) || !IsPlayerStopped(player) || !player->mBatteryDrains
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

// NONMATCHING: retail loads floors once after the extent (0x8008E184, r20, never stored to the frame), reads
// the yaw and id before the switch through it, the quad's corners from it with lwzx at floors + (k * 32 + 4)
// (0x8008E784) and the quad's yaw through object->mFloors again (0x8008E900); it keeps index * 32 apart from
// the pointer and allocates registers differently. Tried: yaw and id through object->mFloors,
// GetFloorYawRadians(&object->mFloors, index), the corners through floors with the yaw through
// object->mFloors (89-90 %); GetFloorCenterOffset(&floors, k) gives the lwzx reads but stores floors to the
// frame (90.0 %); ReadObjectFloorOffset(floors[k]) and ReadFloorCoordinates(floors[k].mCenterOffset, ...)
// form floors + k * 32 first (89.3 %). The corner index is index - 3 + j: that keeps retail's add/subi/slwi
// (0x8008E75C), index + j - 3 folds the -3 into the displacement (lwz -0x5c). The lwzx reads come only from
// (*p)[k] through an address-taken local (dead store) or from a member reload; the reads written out in the loop,
// with or without a named k, and a by-value or by-reference pointer parameter all form floors + k * 32 once
extern "C" f32 GetFloorFacingAngle(XObj* object, const Vec* pos, s32 index, s32 requireSegment, Vec* contact, Vec* normalOut)
{
	CVec from(*pos);
	from.y = 0.0f;
	CVec middle;
	f32 angle = NAN;

	Vec center        = object->mMoveScale * (object->mToppleOffset + ReadObjectFloorOffset(object, index));
	Vec extent        = object->mMoveScale * ReadObjectFloorExtent(object, index);
	XObjFloor* floors = object->mFloors;
	f32 baseYaw       = object->mMoveRotate.y;
	f32 floorYaw      = DEG2RAD * floors[index].mYawDegrees;
	f32 yaw           = baseYaw + floorYaw;
	switch (floors[index].mId) {
	case 0:
	case 1: { // a disc: face its center
		Mtx world, local, rotate, scale, transform;
		PSMTXTrans(world, object->mMoveTranslate.x, object->mMoveTranslate.y, object->mMoveTranslate.z);
		PSMTXTrans(local, center.x, center.y, center.z);
		PSMTXScale(scale, extent.x, extent.y, extent.z);
		PSMTXRotRad(rotate, 'y', yaw);
		PSMTXConcat(world, rotate, transform);
		PSMTXConcat(transform, local, transform);
		PSMTXConcat(transform, scale, transform);
		static Vec origin = { 0.0f, 0.0f, 0.0f };
		PSMTXMultVec(transform, &origin, &middle);
		middle.y = 0.0f;
		angle    = atan2(middle.x - from.x, middle.z - from.z);
		break;
	}
	case 2: { // a box: face the wall edge the ray crosses
		Mtx world, local, rotate, scale, transform;
		PSMTXTrans(world, object->mMoveTranslate.x, object->mMoveTranslate.y, object->mMoveTranslate.z);
		PSMTXTrans(local, center.x, center.y, center.z);
		PSMTXScale(scale, extent.x, extent.y, extent.z);
		PSMTXRotRad(rotate, 'y', yaw);
		PSMTXConcat(world, rotate, transform);
		PSMTXConcat(transform, local, transform);
		PSMTXConcat(transform, scale, transform);
		static Vec origin = { 0.0f, 0.0f, 0.0f };
		PSMTXMultVec(transform, &origin, &middle);
		middle.y = 0.0f;
		f32 h    = (2.0f / sqrtf(3.0f)) / 2.0f;
		static Vec corners[8]
		    = { { -h, h, -h }, { h, h, -h }, { h, h, -h }, { h, h, h }, { h, h, h }, { -h, h, h }, { -h, h, h }, { -h, h, -h } };
		Vec points[8];
		PSMTXMultVecArray(transform, corners, points, 8);
		for (s32 i = 0; i < 4; ++i) {
			Vec& a = points[2 * i];
			Vec& b = points[2 * i + 1];
			CVec hit, normal;
			if (IntersectLinesXZ(&from, &middle, &a, &b, &hit, 1)) {

				a.y = b.y = 0.0f;
				GetOutwardPerpendicularToEdge(&from, &a, &b, &normal);
				PSVECNormalize(&normal, &normal);
				if (contact && IntersectFloorRay(from - normal * 100.0f, from + normal * 100.0f, a, b, hit, 0))
					*contact = hit;
				if (requireSegment) {
					if (IntersectFloorRay(from - normal * 100.0f, from + normal * 100.0f, a, b, hit, 1)) {
						FloorNormalAngle(normal, normalOut, angle);
						break;
					}
				} else {
					FloorNormalAngle(normal, normalOut, angle);
					break;
				}
			}
		}
		break;
	}
	case 3:
		break;
	case 5: { // a quad: face the edge the ray crosses
		Mtx translate, rotate, scaling, transform;
		Vec corners[4];
		for (s32 j = 0; j < 4; ++j)
			corners[j] = object->mToppleOffset + ReadObjectFloorOffset(object, index - 3 + j);
		Vec scale = object->mMoveScale * ReadObjectFloorExtent(object, index);

		f32 baseAngle = object->mMoveRotate.y;
		f32 turnAngle = DEG2RAD * floors[index].mYawDegrees;
		f32 angleY    = baseAngle + turnAngle;
		PSMTXTrans(translate, object->mMoveTranslate.x, object->mMoveTranslate.y, object->mMoveTranslate.z);
		PSMTXScale(scaling, scale.x, scale.y, scale.z);
		PSMTXRotRad(rotate, 'y', angleY);
		PSMTXConcat(translate, rotate, transform);
		PSMTXConcat(transform, scaling, transform);
		PSMTXMultVecArray(transform, corners, corners, 4);
		IntersectLinesXZ(&corners[0], &corners[2], &corners[1], &corners[3], &middle, 1);
		for (s32 i = 0; i < 4; ++i) {
			Vec& a = corners[i];
			Vec& b = corners[(i + 1) % 4];
			CVec hit, normal;
			if (IntersectLinesXZ(&from, &middle, &a, &b, &hit, 1)) {
				if (contact)
					*contact = hit;
				a.y = b.y = 0.0f;
				GetOutwardPerpendicularToEdge((Vec*)pos, &a, &b, &normal);
				PSVECNormalize(&normal, &normal);
				if (contact && IntersectFloorRay(from - normal * 100.0f, from + normal * 100.0f, a, b, hit, 0))
					*contact = hit;
				if (requireSegment) {
					if (IntersectFloorRay(from - normal * 100.0f, from + normal * 100.0f, a, b, hit, 1)) {
						FloorNormalAngle(normal, normalOut, angle);
						break;
					}
				} else {
					FloorNormalAngle(normal, normalOut, angle);
					break;
				}
			}
		}
		break;
	}
	}
	return angle;
}

/**
 * @note Address: 0x8008DEC0
 * @note Size: 0x74
 */
extern "C" void ClearActiveObjectFloors(XObj* object)
{
	if (object->mStatus.mFlags & 0x0200000000000000ULL) {
		object->mFloorMasks[1] = object->mFloorMasks[0];
		for (int i = 0; object->mFloors[i].mId != -1; i++) {
			if (object->IsFloorOn(i)) {
				object->ClearFloor(i);
			}
		}
	}
}

extern "C" void UpdateObjectFloorXZExtent(XObj* object)
{
	object->mFloorXZExtent = NAN;
	if (!(object->mStatus.mFlags & 0x0200000000000000ULL))
		return;
	f32 maxX = 0.0f, maxZ = 0.0f;
	for (s32 i = 0; object->mFloors[i].mId != -1; ++i) {
		CVec origin((Vec) { 0, 0, 0 });
		Vec center     = object->mMoveScale * (object->mToppleOffset + ReadObjectFloorOffset(object, i));
		Vec extent     = object->mMoveScale * ReadObjectFloorExtent(object, i);
		f32 baseYaw    = object->mMoveRotate.y;
		f32 floorAngle = DEG2RAD * object->mFloors[i].mYawDegrees;
		f32 yaw        = baseYaw + floorAngle;
		switch (object->mFloors[i].mId) {
		case 0:
		case 1: { // a disc
			CVec point(0, 0, 0);
			Mtx world, local, rotate, scale, result;
			PSMTXTrans(world, origin.x, origin.y, origin.z);
			PSMTXTrans(local, center.x, center.y, center.z);
			PSMTXScale(scale, extent.x, extent.y, extent.z);
			PSMTXRotRad(rotate, 'y', yaw);
			PSMTXConcat(world, rotate, result);
			PSMTXConcat(result, local, result);
			PSMTXConcat(result, scale, result);
			PSMTXMultVec(result, &point, &point);
			f32 x = point.x + extent.x;
			f32 z = point.z + extent.z;
			ExpandRadius(x, z, maxX, maxZ);
			break;
		}
		case 2: { // a box
			Mtx world, local, rotate, scale, result;
			PSMTXTrans(world, origin.x, origin.y, origin.z);
			PSMTXTrans(local, center.x, center.y, center.z);
			PSMTXScale(scale, extent.x, extent.y, extent.z);
			PSMTXRotRad(rotate, 'y', yaw);
			PSMTXConcat(world, rotate, result);
			PSMTXConcat(result, local, result);
			PSMTXConcat(result, scale, result);
			f32 h = (2.0f / sqrtf(3.0f)) / 2.0f;
			static Vec corners[8]
			    = { { -h, h, -h }, { h, h, -h }, { h, h, -h }, { h, h, h }, { h, h, h }, { -h, h, h }, { -h, h, h }, { -h, h, -h } };
			Vec points[8];
			PSMTXMultVecArray(result, corners, points, 8);
			for (int j = 0; j < 4; ++j)
				ExpandRadius(points[j].x, points[j].z, maxX, maxZ);
			break;
		}
		case 3: { // a wall segment
			Mtx world, local, rotate, turn, scale, result;
			PSMTXTrans(world, origin.x, origin.y, origin.z);
			PSMTXTrans(local, center.x, center.y, center.z);
			PSMTXScale(scale, extent.x, extent.y, extent.z);
			PSMTXRotRad(rotate, 'y', baseYaw);
			PSMTXRotRad(turn, 'y', floorAngle);
			PSMTXConcat(world, rotate, result);
			PSMTXConcat(result, local, result);
			PSMTXConcat(result, turn, result);
			PSMTXConcat(result, scale, result);
			static Vec ends[2] = { { -.5f, 0, 0 }, { .5f, 0, 0 } };
			Vec line[2];
			PSMTXMultVecArray(result, ends, line, 2);
			// retail scans four points after transforming two
			for (int j = 0; j < 4; ++j)
				ExpandRadius(line[j].x, line[j].z, maxX, maxZ);
			break;
		}
		case 5: { // a quad from this and the next three records
			Mtx world, rotate, scale, result;
			Vec corners[4];
			for (int j = 0; j < 4; ++j)
				corners[j] = object->mToppleOffset + ReadObjectFloorOffset(object, i + j);
			Vec extent    = object->mMoveScale * ReadObjectFloorExtent(object, i);
			f32 baseAngle = object->mMoveRotate.y;
			f32 turnAngle = DEG2RAD * object->mFloors[i].mYawDegrees;
			f32 angle     = baseAngle + turnAngle;

			PSMTXTrans(world, origin.x, origin.y, origin.z);
			PSMTXScale(scale, extent.x, extent.y, extent.z);
			PSMTXRotRad(rotate, 'y', angle);
			PSMTXConcat(world, rotate, result);
			PSMTXConcat(result, scale, result);
			PSMTXMultVecArray(result, corners, corners, 4);
			for (int j = 0; j < 4; ++j)
				ExpandRadius(corners[j].x, corners[j].z, maxX, maxZ);
			break;
		}
		}
	}
	object->mFloorXZExtent = maxX > maxZ ? maxX : maxZ;
}

extern "C" void UpdateObjectFloorHeights(XObj* object)
{
	for (u32 i = 0; i < ARRAY_SIZE(object->mFloorHeights); ++i)
		object->mFloorHeights[i] = NAN;
	object->mMaxFloorHeight = NAN;
	if (!(object->mStatus.mFlags & 0x0200000000000000ULL))
		return;
	object->mStatus.mFlags &= ~0x0010000000000000ULL;
	for (s32 i = 0; object->mFloors[i].mId != -1; ++i) { // the list ends at id -1
		Vec center  = object->mMoveScale * (object->mToppleOffset + ReadObjectFloorOffset(object, i));
		Vec extent  = object->mMoveScale * ReadObjectFloorExtent(object, i);
		f32 baseYaw = object->mMoveRotate.y;
		f32 turn    = DEG2RAD * object->mFloors[i].mYawDegrees;
		f32 yaw     = baseYaw + turn;
		switch (object->mFloors[i].mId) {
		case 0:
		case 1: { // a disc: the top of its column
			CVec point(0.0f, 0.0f, 0.0f);
			Mtx world, local, rotate, scale, transform;
			PSMTXTrans(world, object->mMoveTranslate.x, object->mMoveTranslate.y, object->mMoveTranslate.z);
			PSMTXTrans(local, center.x, center.y, center.z);
			PSMTXScale(scale, extent.x, extent.y, extent.z);
			PSMTXRotRad(rotate, 'y', yaw);
			PSMTXConcat(world, rotate, transform);
			PSMTXConcat(transform, local, transform);
			PSMTXConcat(transform, scale, transform);
			PSMTXMultVec(transform, &point, &point);
			object->mFloorHeights[i] = point.y + extent.y;
			break;
		}
		case 2: { // a box: the top of its cube
			Mtx world, local, rotate, scale, transform;
			PSMTXTrans(world, object->mMoveTranslate.x, object->mMoveTranslate.y, object->mMoveTranslate.z);
			PSMTXTrans(local, center.x, center.y, center.z);
			PSMTXScale(scale, extent.x, extent.y, extent.z);
			PSMTXRotRad(rotate, 'y', yaw);
			PSMTXConcat(world, rotate, transform);
			PSMTXConcat(transform, local, transform);
			PSMTXConcat(transform, scale, transform);
			f32 h = (2.0f / sqrtf(3.0f)) / 2.0f; // cube corners at unit radius
			static Vec corners[8]
			    = { { -h, h, -h }, { h, h, -h }, { h, h, -h }, { h, h, h }, { h, h, h }, { -h, h, h }, { -h, h, h }, { -h, h, -h } };
			Vec points[8];
			PSMTXMultVecArray(transform, corners, points, 8);
			object->mFloorHeights[i] = points[0].y - 0.01f;
			break;
		}
		case 3: // a wall has no floor height
			break;
		case 5: { // a quad: it and the three records after it share one height
			Mtx translate, rotate, scaling, transform;
			Vec corners[4];
			for (s32 j = 0; j < 4; ++j)
				corners[j] = object->mToppleOffset + ReadObjectFloorOffset(object, i + j);
			Vec scale     = object->mMoveScale * ReadObjectFloorExtent(object, i);
			f32 baseAngle = object->mMoveRotate.y;
			f32 turnAngle = DEG2RAD * object->mFloors[i].mYawDegrees;
			f32 angle     = baseAngle + turnAngle;
			PSMTXTrans(translate, object->mMoveTranslate.x, object->mMoveTranslate.y, object->mMoveTranslate.z);
			PSMTXScale(scaling, scale.x, scale.y, scale.z);
			PSMTXRotRad(rotate, 'y', angle);
			PSMTXConcat(translate, rotate, transform);
			PSMTXConcat(transform, scaling, transform);
			PSMTXMultVecArray(transform, corners, corners, 4);
			object->mFloorHeights[i] = object->mFloorHeights[i + 1] = object->mFloorHeights[i + 2] = object->mFloorHeights[i + 3]
			    = extent.y / 2.0f + corners[0].y - 0.01f;
			i += 3;
			break;
		}
		}
		if (__fpclassifyf(object->mMaxFloorHeight) == FP_NAN || object->mFloorHeights[i] > object->mMaxFloorHeight)
			object->mMaxFloorHeight = object->mFloorHeights[i];
	}
}

/**
 * @note Address: 0x8008C7CC
 * @note Size: 0xD8
 */
extern "C" void SelectHighestActiveFloor(XObj* object)
{
	object->mSelectedFloorHeight = NAN;
	for (s32 i = 0; object->mFloors[i].mId != -1; ++i) {
		if (object->mFloorMasks[0] & (1U << i)) {
			if (__fpclassifyf(object->mSelectedFloorHeight) == FP_NAN || object->mFloorHeights[i] > object->mSelectedFloorHeight) {
				object->mSelectedFloorIndex  = i;
				object->mSelectedFloorHeight = object->mFloorHeights[i];
			}
		}
	}
}

/**
 * @note Address: 0x8008C274
 * @note Size: 0x558
 */
// NONMATCHING: instructions match; retail's zero offset is a named .rodata Vec (0x8020928C) where the
// compound literal here is an anonymous one.
extern "C" CVec GetFloorContactPosition(const XObj* object, s32 index, const Vec* offset)
{
	if (!(object->mStatus.mFlags & 0x0200000000000000ULL))
		return object->mMoveTranslate; // that flag off: no floor contact to compute

	Vec displacement = offset ? *offset : (Vec) { 0.0f, 0.0f, 0.0f };
	Vec position     = object->mMoveTranslate + displacement;
	// retail stores index to the frame on entry (0x8008C2B4) and reloads it before each floor read
	// (0x8008C360, 0x8008C470, 0x8008C55C): a reference is bound to it
	const s32& i = index;
	Vec center   = object->mMoveScale * (object->mToppleOffset + GetFloorCenterOffset(&object->mFloors, i));
	Vec extent   = object->mMoveScale * GetFloorExtent(&object->mFloors, i);
	f32 yaw      = object->mMoveRotate.y + GetFloorYawRadians(&object->mFloors, i);
	f32 height;
	switch (object->mFloors[i].mId) {
	case 0:
	case 1: // a disc touches at its bottom, one unit down
		height = -1.0f;
		break;
	case 2: { // a box touches at its underside, half an edge down
		f32 root = sqrtf(3.0f);
		f32 edge = 2.0f / root;
		height   = 0.5f * -edge;
		break;
	}
	default:
		height = NAN; // other shapes return the object's translation, offset ignored
	}
	if (__fpclassifyf(height) != FP_NAN) {
		Mtx translate, local, rotate, scale, result;
		PSMTXTrans(translate, position.x, position.y, position.z);
		PSMTXTrans(local, center.x, center.y, center.z);
		PSMTXScale(scale, extent.x, extent.y, extent.z);
		PSMTXRotRad(rotate, 'y', yaw);
		PSMTXConcat(translate, rotate, result);
		PSMTXConcat(result, local, result);
		PSMTXConcat(result, scale, result);
		CVec point(0.0f, height, 0.0f);
		PSMTXMultVec(result, &point, &point);
		return point;
	}
	return object->mMoveTranslate;
}

/**
 * @note Address: 0x8008C1F4
 * @note Size: 0x80
 */
CVec CVec::operator*(Vec other) const
{
	CVec result = *this;
	result.x *= other.x;
	result.y *= other.y;
	result.z *= other.z;
	return result;
}

// NONMATCHING: the floor readers take the table by address, as the Wii build's do, which gives retail's
// floors + (i * 0x20 + offset) lwzx reads for the center and extent; passing the local's address also spills
// floors to the frame (stw 0x8(r1)), which retail does not, so the frame is 4 bytes off and registers shift by
// one. The quad case loads mFloors once before its corner loop (0x8008BE50, r15, kept across the loop's
// operator+ call) and reads the corners from it with lwzx; a plain local pointer there forms the element
// address instead (94.8 %). Also ruled out: a by-value XObjFloor* parameter, an XObjFloor* const& parameter and
// a const local, which all give the element address (93.6-93.8 %), and a by-value wrapper around the by-address
// reader, which drops the prologue store but stores both parameter copies inside the loop instead (95.4 %)
BOOL XObj::TestFloorCrossing(const Vec& start, const Vec& end)
{
	XObjFloor* floors = mFloors;
	if (!floors)
		return FALSE;
	if (mStatus.mFlags & 0x0400000000000000ULL)
		return FALSE;
	if (mStatus.mFlags & 0x4000000000000000ULL)
		return FALSE;
	CVec from(*(const CVec*)&start); // retail copies both ends word by word, as CVecs
	from.y = 0;
	CVec to(*(const CVec*)&end);
	to.y = 0;    // floors are tested flat on the ground plane
	CVec extent; // never read: retail builds it here and the loop's own extent shadows it

	f32 rotation = mMoveRotate.y;
	for (s32 i = 0; floors[i].mId != -1; ++i) {
		Vec center   = mMoveScale * (mToppleOffset + GetFloorCenterOffset(&floors, i));
		Vec extent   = mMoveScale * GetFloorExtent(&floors, i);
		f32 localYaw = DEG2RAD * floors[i].mYawDegrees;
		f32 yaw      = rotation + localYaw;
		switch (floors[i].mId) {
		case 0:
		case 1: { // a point: passed if the bearing to it swings through
			CVec point(0, 0, 0);
			Mtx world, local, rotate, scale, transform;
			PSMTXTrans(world, mMoveTranslate.x, mMoveTranslate.y, mMoveTranslate.z);
			PSMTXTrans(local, center.x, center.y, center.z);
			PSMTXScale(scale, extent.x, extent.y, extent.z);
			PSMTXRotRad(rotate, 'y', yaw);
			PSMTXConcat(world, rotate, transform);
			PSMTXConcat(transform, local, transform);
			PSMTXConcat(transform, scale, transform);
			PSMTXMultVec(transform, &point, &point);
			point.y   = 0;
			f32 first = atan2(from.x - point.x, from.z - point.z);
			f32 last  = atan2(to.x - point.x, to.z - point.z);
			f32 diff  = WrapAngle(first - last);
			if (diff > 2 * THIRD_PI && diff < 4 * THIRD_PI)
				return TRUE;
			break;
		}
		case 2: { // a box, tested against its four side edges
			Mtx world, local, rotate, scale, transform;
			PSMTXTrans(world, mMoveTranslate.x, mMoveTranslate.y, mMoveTranslate.z);
			PSMTXTrans(local, center.x, center.y, center.z);
			PSMTXScale(scale, extent.x, extent.y, extent.z);
			PSMTXRotRad(rotate, 'y', yaw);
			PSMTXConcat(world, rotate, transform);
			PSMTXConcat(transform, local, transform);
			PSMTXConcat(transform, scale, transform);
			f32 h = (2.0f / sqrtf(3.0f)) * 0.5f;
			static Vec corners[8]
			    = { { -h, h, -h }, { h, h, -h }, { h, h, -h }, { h, h, h }, { h, h, h }, { -h, h, h }, { -h, h, h }, { -h, h, -h } };
			Vec placed[8];
			PSMTXMultVecArray(transform, corners, placed, 8);
			s32 outside = 0, crossed = 0;
			for (s32 i = 0; i < 4; ++i) {
				Vec* a = &placed[2 * i];
				Vec* b = &placed[2 * i + 1];
				CountCrossedEdge(from, to, a, b, outside, crossed);
			}
			if (outside == crossed)
				return TRUE;
			break;
		}
		case 3: { // a line segment: the path crosses it when the sides swap
			Mtx world, offset, turn, local, scale, transform;
			PSMTXTrans(world, mMoveTranslate.x, mMoveTranslate.y, mMoveTranslate.z);
			PSMTXTrans(offset, center.x, center.y, center.z);
			PSMTXScale(scale, extent.x, extent.y, extent.z);
			PSMTXRotRad(turn, 'y', rotation);
			PSMTXRotRad(local, 'y', localYaw);
			PSMTXConcat(world, turn, transform);
			PSMTXConcat(transform, offset, transform);
			PSMTXConcat(transform, local, transform);
			PSMTXConcat(transform, scale, transform);
			static Vec ends[2] = { { -.5f, 0, 0 }, { .5f, 0, 0 } };
			Vec edge[2];
			PSMTXMultVecArray(transform, ends, edge, 2);
			f32 first = SignedTriangleArea2XZ(&from, &edge[0], &edge[1]);
			if (first * SignedTriangleArea2XZ(&to, &edge[0], &edge[1]) < 0)
				return TRUE;
			break;
		}
		case 5: { // a four-point polygon built from this and the next three floors
			Mtx translate, rotate, scale, transform;
			Vec corners[4];
			for (s32 j = 0; j < 4; ++j)
				corners[j] = mToppleOffset + GetFloorCenterOffset(&mFloors, i + j);
			Vec size  = mMoveScale * GetFloorExtent(&mFloors, i);
			f32 angle = mMoveRotate.y + GetFloorYawRadians(&mFloors, i);
			PSMTXTrans(translate, mMoveTranslate.x, mMoveTranslate.y, mMoveTranslate.z);
			PSMTXScale(scale, size.x, size.y, size.z);
			PSMTXRotRad(rotate, 'y', angle);
			PSMTXConcat(translate, rotate, transform);
			PSMTXConcat(transform, scale, transform);
			PSMTXMultVecArray(transform, corners, corners, 4);
			s32 outside = 0, crossed = 0;
			for (s32 i = 0; i < 4; ++i) {
				Vec* a = &corners[i];
				Vec* b = &corners[(i + 1) % 4];
				CountCrossedEdge(from, to, a, b, outside, crossed);
			}
			if (outside == crossed)
				return TRUE;
			i += 3; // its four points were this and the next three entries
			break;
		}
		}
	}
	return FALSE;
}

/**
 * @note Address: 0x8008B5D4
 * @note Size: 0xCC
 */
void XObj::ProjectOffsetToScreen(const Vec* offset, f32* x, f32* y, f32* z, CCamera* camera)
{
	camera = camera != NULL ? camera : &gGame.mMotionControlCamera.mCamera;
	f32 screenX, screenY, screenZ;
	GXProject(mMoveTranslate.x + offset->x, mMoveTranslate.y + offset->y, mMoveTranslate.z + offset->z, camera->mViewMtx,
	          camera->mProjection, camera->mViewport, &screenX, &screenY, &screenZ);
	if (x != NULL)
		*x = screenX;
	if (y != NULL) {
#if defined(VERSION_PAL)
		*y = 448.0f * screenY / (&gGame.mGs.mScreen)->xfbHeight;
#else
		*y = screenY;
#endif
	}
	if (z != NULL)
		*z = screenZ;
}

/**
 * @note Address: 0x8008B338
 * @note Size: 0x29C
 */
BOOL XObj::ProjectVisibleBounds(f32* width, f32* height, CCamera* camera, f32 radius, f32 objectHeight)
{
	camera          = camera != NULL ? camera : &gGame.mMotionControlCamera.mCamera;
	f32 heading     = camera->mAngle.y;
	MtxPtr view     = camera->mViewMtx;
	f32* projection = camera->mProjection;
	f32* viewport   = camera->mViewport;
	f32 left, right, bottom, top, unusedX, unusedY, depth;
	// three points on the circle and one raised by the height give its screen extent
	f32 angle  = (PI + HALF_PI) + heading;
	f32 cosine = std::cosf(angle);
	f32 sine   = std::sinf(angle);
	GXProject(radius * sine + mMoveTranslate.x, mMoveTranslate.y, radius * cosine + mMoveTranslate.z, view, projection, viewport, &left,
	          &unusedY, &depth);
	angle  = HALF_PI + heading;
	cosine = std::cosf(angle);
	sine   = std::sinf(angle);
	GXProject(radius * sine + mMoveTranslate.x, mMoveTranslate.y, radius * cosine + mMoveTranslate.z, view, projection, viewport, &right,
	          &unusedY, &depth);
	angle  = PI + heading;
	cosine = std::cosf(angle);
	sine   = std::sinf(angle);
	GXProject(radius * sine + mMoveTranslate.x, mMoveTranslate.y, radius * cosine + mMoveTranslate.z, view, projection, viewport, &unusedX,
	          &bottom, &depth);
	cosine = std::cosf(heading);
	sine   = std::sinf(heading);
	GXProject(radius * sine + mMoveTranslate.x, mMoveTranslate.y + objectHeight, radius * cosine + mMoveTranslate.z, view, projection,
	          viewport, &unusedX, &top, &depth);
#if defined(VERSION_PAL)
	bottom *= 448.0f / (&gGame.mGs.mScreen)->xfbHeight;
	top *= 448.0f / (&gGame.mGs.mScreen)->xfbHeight;
#endif
	if (left >= 0.0f && right < 640.0f && bottom >= 0.0f && top < 448.0f && depth > 0.0f && depth < 1.0f) {
		if (width != NULL) {
			f32 value = right - left;
			if (value < 0.0f)
				value = -value;
			*width = value;
		}
		if (height != NULL) {
			f32 value = top - bottom;
			if (value < 0.0f)
				value = -value;
			*height = value;
		}
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x8008B224
 * @note Size: 0x114
 */
extern "C" BOOL IsOutsideCameraDepth__4XObjFP7CCamera(const XObj* object, CCamera* camera)
{
	HSD_JObj* joint = object->mJObj;
	if (joint != NULL) {
		Vec position = GetJointPosition(joint);
		f32 screenX, screenY, screenZ;
		GXProject(position.x, position.y, position.z, camera->mViewMtx, camera->mProjection, camera->mViewport, &screenX, &screenY,
		          &screenZ);
		if (screenZ > 0.0f && screenZ < 1.0f)
			return FALSE; // inside the depth range: visible
		return TRUE;
	}
	return TRUE;
}

/**
 * @note Address: 0x8008B0D8
 * @note Size: 0x14C
 */
void XObj::SetCullMode(s32 mode)
{
	switch (mode) { // the two PObj flags pick which faces to drop
	case 0:
		if (mJObj != NULL)
			HSD_JObjWalkTree(mJObj, JObjClearPObjFlags, (f32**)0x4000);
		if (mJObj != NULL)
			HSD_JObjWalkTree(mJObj, JObjClearPObjFlags, (f32**)0x8000);
		break;
	case 2:
		if (mJObj != NULL)
			HSD_JObjWalkTree(mJObj, JObjClearPObjFlags, (f32**)0x4000);
		if (mJObj != NULL)
			HSD_JObjWalkTree(mJObj, JObjSetPObjFlags, (f32**)0x8000);
		break;
	case 1:
		if (mJObj != NULL)
			HSD_JObjWalkTree(mJObj, JObjSetPObjFlags, (f32**)0x4000);
		if (mJObj != NULL)
			HSD_JObjWalkTree(mJObj, JObjClearPObjFlags, (f32**)0x8000);
		break;
	case 3:
		if (mJObj != NULL)
			HSD_JObjWalkTree(mJObj, JObjSetPObjFlags, (f32**)0x4000);
		if (mJObj != NULL)
			HSD_JObjWalkTree(mJObj, JObjSetPObjFlags, (f32**)0x8000);
		break;
	}
}

/**
 * @note Address: 0x8008ADA0
 * @note Size: 0x338
 */
extern "C" BOOL SetClampedPushPositionXZ(XObj* object, const Vec* direction, u8 axes, f32 x, f32 z)
{
	BOOL changed = FALSE;
	if (__fpclassifyf(object->mAnchorX) == FP_NAN || __fpclassifyf(object->mAnchorZ) == FP_NAN) { // no anchor: place directly
		object->SetTranslateX(x, 0);
		object->SetTranslateZ(z, 0);
	} else if (object->mAnchorX == x && object->mAnchorZ == z) {
		object->SetTranslateX(x, 0);
		object->SetTranslateZ(z, 0);
	} else {
		Mtx rotation;
		Vec displacement;
		PSMTXRotRad(rotation, 'y', -object->mMoveRotate.y);
		displacement.x = object->mAnchorX - x;
		displacement.z = object->mAnchorZ - z;
		displacement.y = 0.0f;
		PSMTXMultVec(rotation, &displacement, &displacement); // into the object's yaw

		if (axes & 1) {
			if (direction->x > 0.0f && displacement.x < object->mLimitPosX) {
				displacement.x = object->mLimitPosX;
				changed        = TRUE;
			} else if (direction->x < 0.0f && displacement.x > object->mLimitNegX) {
				displacement.x = object->mLimitNegX;
				changed        = TRUE;
			}
		}
		if (axes & 2) { // the z axis
			if (direction->z > 0.0f && displacement.z < object->mLimitPosZ) {
				displacement.z = object->mLimitPosZ;
				changed        = TRUE;
			} else if (direction->z < 0.0f && displacement.z > object->mLimitNegZ) {
				displacement.z = object->mLimitNegZ;
				changed        = TRUE;
			}
		}

		PSMTXRotRad(rotation, 'y', object->mMoveRotate.y);
		PSMTXMultVec(rotation, &displacement, &displacement); // and back
		object->SetTranslateX(object->mAnchorX - displacement.x, 0);
		object->SetTranslateZ(object->mAnchorZ - displacement.z, 0);
	}
	return changed;
}

/**
 * @note Address: 0x8008A944
 * @note Size: 0x45C
 */
// NONMATCHING: the map pointer and FindMapFloor's counter get each other's register (r29/r30)
extern "C" f32 QueryObjectFloorHeight(XObj* self, f32 radius, f32 height)
{
	Vec* pos     = &self->mMoveTranslate;
	XObjMap* map = (XObjMap*)gGame.mXObjs.mPools[3].mObjects;
	if (!FindMapFloor(map, pos, &self->mFloorHit, NULL)) {
		self->mFloorHit.mTriangle = -1;
		self->mFloorHit.mTrigon   = -1;
		self->mFloorHit.mHeight   = self->mMoveTranslate.y; // no map floor: use the object's own height
	}
	f32 highest = self->mFloorHit.mHeight;
	CVec push, contact;
	for (int i = 0; i < gGame.mXObjs.mObjectCount; ++i) {
		XObj* object = gGame.GetObject(i);
		if (!(object->mStatus.mFlags & 1) || !(object->mStatus.mFlags & 0x10000) || (object->mStatus.mFlags & 0x100000))
			continue; // inactive, not shown, or skipped
		if (self->mId == object->mId)
			continue;
		f32 dx = object->mMoveTranslate.x - pos->x;
		f32 dz = object->mMoveTranslate.z - pos->z;
		if (sqrtf(dx * dx + dz * dz) > 100.0f + (radius + object->mFloorXZExtent))
			continue;
		s32 floor = 0;
		while (floor != -1) {
			if (ProbeObjectFloor(object, *pos, *pos, radius, height, &push, &contact, &floor) & 1) {
				f32 floorHeight = object->mFloorHeights[object->mHitFloorIndex];
				// only floors at most one unit overhead
				if (__fpclassifyf(floorHeight) != FP_NAN && self->mMoveTranslate.y + 1.0f >= floorHeight && floorHeight >= highest)
					highest = floorHeight;
			}
		}
	}
	if (highest > self->mFloorHit.mHeight)
		self->mFloorHit.mHeight = highest;
	return self->mFloorHit.mHeight;
}

/**
 * @note Address: 0x8008A820
 * @note Size: 0x124
 */
void XObj::SetColor(s32 index, u8 red, u8 green, u8 blue, f32 time)
{
	mColorStep[index].mTarget.x = red;
	mColorStep[index].mTarget.y = green;
	mColorStep[index].mTarget.z = blue;
	if (time == 0.0f) {
		mColorStep[index].mValue.x = red;
		mColorStep[index].mValue.y = green;
		mColorStep[index].mValue.z = blue;
		mColorStep[index].mSpeed.x = mColorStep[index].mSpeed.y = mColorStep[index].mSpeed.z = 0.0f;
	} else {
#if defined(VERSION_PAL)
		// retail reads the refresh rate for each one
		mColorStep[index].mSpeed.x = ((f32)red - mColorStep[index].mValue.x) / (gVideoFramesPerSecond * time);
		mColorStep[index].mSpeed.y = ((f32)green - mColorStep[index].mValue.y) / (gVideoFramesPerSecond * time);
		mColorStep[index].mSpeed.z = ((f32)blue - mColorStep[index].mValue.z) / (gVideoFramesPerSecond * time);
#else
		mColorStep[index].mSpeed.x = ((f32)red - mColorStep[index].mValue.x) / (60.0f * time);
		mColorStep[index].mSpeed.y = ((f32)green - mColorStep[index].mValue.y) / (60.0f * time);
		mColorStep[index].mSpeed.z = ((f32)blue - mColorStep[index].mValue.z) / (60.0f * time);
#endif
	}
}

/**
 * @note Address: 0x8008A818
 * @note Size: 0x8
 */
BOOL XObj::ReceivePlugPower(f32 amount)
{
	return TRUE;
}

/**
 * @note Address: 0x8008A6C0
 * @note Size: 0x158
 */
void XObj::OnHitFromPosition(const Vec& position, f32 strength)
{
	if (mStatus.mFlags & 0x0000080000000000ULL) {
		// a hit of the required force raises event 2001, less raises 2000
		if (strength >= (s32)GetPlayerActionParameter(31) / 100.0f) {
			TriggerObjectEvent(this, -1, 2001);
		} else {
			TriggerObjectEvent(this, -1, 2000);
		}
	}
}

/**
 * @note Address: 0x8008A604
 * @note Size: 0xBC
 */
void XObj::OnSquirterHit(const Vec* contact, s32 value, s32 tint)
{
	if (mStatus.mFlags & 0x0000080000000000ULL) { // flag means: has a hit event
		const void* script = this->GetEventScript();
		if (!IsPlayerBlockingEvents(gGame.GetPlayer())) {
			StartOrQueueEventScript(&gGame.mEvt, (u8*)script, this, -1,
			                        (s16)((u32)value + 3000)); // squirts reach the script as 3000 + value
		}
	}
}

/**
 * @note Address: 0x8008A530
 * @note Size: 0xD4
 */
BOOL XObj::OnItemUsed(const Vec* position, s32 value)
{
	if (mStatus.mFlags & 0x0000040000000000ULL) { // flag means: has an item-use event
		if (value == 56)
			return FALSE; // item 56 never triggers it
		const void* script = this->GetEventScript();
		if (!IsPlayerBlockingEvents(gGame.GetPlayer())) {
			StartOrQueueEventScript(&gGame.mEvt, (u8*)script, this, (s16)value, -1);
		}
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x8008A4FC
 * @note Size: 0x34
 */
// CJAnim::Clear
void CJAnim::Clear()
{
	CJArchive::Clear();
	mLoading = 0;
}

/**
 * @note Address: 0x8008A38C
 * @note Size: 0x170
 */
// starts the read, then sets it up once it's in, TRUE when done
extern "C" BOOL LoadJointAnimationAsync(CJAnim* anim, const char* path)
{
	if (anim->mLoading == 0) {
		anim->Release();
		if (path == NULL || *path == '\0')
			return TRUE; // no animation asked for
		CFile file;
		if (file.ConvertPathToEntrynum(path) < 0)
			return TRUE; // the file isn't there
		CObjArchive* storage = FindFreeObjectArchive(gGame.mObjArchives.mArchives);
		if (storage != NULL) {
			anim->ReadAsync(storage, path, 0, 0, 0);
			anim->mLoading++;
		} else {
			return TRUE; // no free archive slot: treat as done
		}
	} else {
		if (anim->ReadCheck()) {
			anim->SetupScene(0, 0);
			anim->mLoading = 0;
		}
	}
	return anim->mLoading == 0;
}

/**
 * @note Address: 0x8008A360
 * @note Size: 0x2C
 */
// CJAnim::Release
void CJAnim::Release()
{
	if (mArchive != NULL)
		CJArchive::Release();
}
