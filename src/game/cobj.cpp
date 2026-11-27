#include "game/cobj.h"

#include "game/cjobjset.h"
#include "game/gmath.h"
#include "game/hsd_dobj.h"
extern "C" {
#include <sysdolphin/class.h>
#include <sysdolphin/dobj.h>
#include <sysdolphin/mobj.h>
#include <sysdolphin/pobj.h>
#include <sysdolphin/texp.h>
#include <dolphin/mtx/vec.h>
#include <dolphin/os.h>
}
#include <hsd/sysdolphin/devcom.h>
#include <math.h>
#include "game/xobj.h"

extern "C" {
void OSReport(const char* msg, ...);
u32 ARGetSize(void);
}

// report, then panic with the function name. the line goes in by hand like the asserts
#define HALT(line)              \
	OSReport("\n!!!HALT!!!\n"); \
	OSPanic(__FILE__, line, "%s%s", "", __FUNCTION__)

// the same with a formatted message
#define HALTF(line, ...)        \
	OSReport("\n!!!HALT!!!\n"); \
	OSPanic(__FILE__, line, "%s%s", FormatToStaticBuffer(__VA_ARGS__), __FUNCTION__)

// a material whose tev result is multiplied by one color and has another added, see XCObjMakeTExp
struct XCObj {
	HSD_MObj parent; // _00
	u32* mColors;    // _20
	f32 mAlpha;      // _24, the material's own alpha
};

void XCObjInfoInit();
HSD_TExp* XCObjMakeTExp(HSD_MObj* mobj, HSD_TObj* tobj_top, HSD_TExp** list);
HSD_MObjInfo xCObj = { { { XCObjInfoInit } } };

// todo: cleanup
static inline void ReleaseJObj(CJObj* obj)
{
	if (obj->mJObj != NULL) {
		HSD_JObjRemoveAnimAll(obj->mJObj);
		HSD_JObjUnref(obj->mJObj);
		obj->mJObj = NULL;
	}
}

// todo: cleanup
inline s32 CObjArchive::Parse()
{
	s32 result = HSD_ArchiveParse(this, (u8*)mFile.mBuffer, mFile.mSize);
	if (result < 0) {
		HALTF(51, "ObjArchive::%s() ... error on ArchiveParse - %s\n", __FUNCTION__, mFile.mPath);
		mFile.Close();
	}
	return result;
}

// todo: cleanup
inline CJArchiveScene* CJArchive::GetScene(s32 index)
{
	HSD_Archive* archive = mArchive;
	return (CJArchiveScene*)HSD_ArchiveGetPublicAddress(archive, HSD_ArchiveGetExtern(archive, index));
}

// todo: cleanup
inline HSD_JObj* CJArchive::LoadJObj()
{
	CObjArchive* archive = mArchive;
	if (archive != NULL) {
		HSD_JObj* jobj;
		if (mJoint != NULL) {
			jobj = HSD_JObjLoadJoint(mJoint);
			archive->mRefCount++;
		} else {
			jobj = NULL;
		}
		return jobj;
	}
	return NULL;
}

// todo: cleanup
inline bool CObjVec::IsMoving() const
{
	bool result = false;
	if ((x != mTarget.x && 0.0f != mSpeed.x) || (y != mTarget.y && 0.0f != mSpeed.y) || (z != mTarget.z && 0.0f != mSpeed.z)) {
		result = true;
	}
	return result;
}

// todo: cleanup
inline HSD_JObj* CXObj::FindJoint(s32 depth, s32* path)
{
	HSD_JObj* jobj = mJObj;
	if (jobj == NULL) {
		return NULL;
	}
	for (int i = 0; i < depth; i++) {
		jobj = jobj == NULL ? NULL : jobj->child;
		for (int j = 0; j < path[i]; j++) {
			jobj = jobj == NULL ? NULL : jobj->next;
			if (jobj == NULL) {
				return NULL;
			}
		}
	}
	return jobj;
}

// todo: cleanup
inline void CXObj::ClearQueue()
{
	mQueued = 0;
	for (int i = 0; i < 10; i++) {
		mQueue[i].mJObj  = 0;
		mQueue[i].mType  = 0;
		mQueue[i].mValue = 0.0f;
	}
}

/**
 * @note Address: 0x801A43B4
 * @note Size: 0x4
 */
void CXObj::ApplyTransform()
{
}

/**
 * @note Address: 0x801A4318
 * @note Size: 0x9C
 */
void CJArchive::Clear()
{
	mArchive        = NULL;
	mState          = 0;
	mResult         = 0;
	mJoint          = NULL;
	mAnimJoint      = NULL;
	mMatAnimJoint   = NULL;
	mShapeAnimJoint = NULL;
	for (int i = 0; i < CJARCHIVE_DESC_NUM; i++) {
		mDesc[i] = NULL;
	}
	for (int i = 0; i < CJARCHIVE_DESC_NUM + 1; i++) {
		mNum[i] = 0;
	}
}

/**
 * @note Address: 0x801A4308
 * @note Size: 0x10
 */
// the DevCom callback, moves ReadCheck on to parsing
void ObjectArchiveReadDone(int, void* context, void*, BOOL)
{
	++*static_cast<s32*>(context);
}

/**
 * @note Address: 0x801A415C
 * @note Size: 0x1AC
 */
BOOL CJArchive::ReadAsync(CObjArchive* archive, const char* path, u32 flags, u32 aramAddr, u32 size)
{
	mArchive  = archive;
	mFlags    = flags;
	mResult   = 0;
	mAramAddr = aramAddr;
	mAramSize = size;
	if (mState != 0) {
		return FALSE;
	}
	// the archive is already in memory, nothing to read
	if (mArchive->mFile.mBuffer != NULL) {
		return TRUE;
	}

	s32 result;
	// 4: read the copy kept in aram at aramAddr instead of the file on disc
	if (flags & 4) {
		result = aramAddr != 0 ? mArchive->mFile.ReadAsync(path, aramAddr, size, TRUE) : mArchive->mFile.ReadAsync(path, 0, 0, FALSE);
	} else {
		result = mArchive->mFile.ReadAsync(path, 0, 0, FALSE);
	}
	if (result >= 0) {
		mState++;
	} else {
		mResult = result;
		// 0x100: give up and return FALSE instead of halting
		if (mFlags & 0x100) {
			if (mArchive->mRefCount <= 0) {
				mArchive->mFile.Close();
			}
			return FALSE;
		}
#if defined(VERSION_JP)
		HALTF(273, "%s(%d)\n", path, result);
#else
		HALTF(285, "%s(%d)\n", path, result);
#endif
	}
	return TRUE;
}
// CJArchive::ReadAsync(CObjArchive*, const char*, unsigned long, unsigned long, unsigned long)
/**
 * @note Address: 0x801A3F18
 * @note Size: 0x244
 */
BOOL CJArchive::ReadCheck()
{
	s32 result;
	switch (mState) {
	case 1: {
		CObjArchive* archive = mArchive;
		result               = archive->mFile.GetResult();
		if (result == 0) {
			archive->mFile.CloseHandle();
		}
		if (result == 0) {
			mState++;
		} else if (result < 0) {
			mResult = result;
			if (mFlags & 0x100) {
				if (mArchive->mRefCount <= 0) {
					mArchive->mFile.Close();
				}
				mState = 0;
				break;
			} else {
#if defined(VERSION_JP)
				HALTF(295, "%s(%d)\n", mArchive->mFile.mPath, result);
#else
				HALTF(307, "%s(%d)\n", mArchive->mFile.mPath, result);
#endif
			}
		} else {
			// still reading
			break;
		}
	}
	// read in: with flag 2, copy it out to aram at mAramAddr if it fits
	case 2:
		if (mFlags & 2) {
			u32 size = ((mArchive->mFile.mSize + 31) & ~31);
			if (mAramAddr + size < ARGetSize()) {
				HSD_DevComRequest(0, (u32)mArchive->mFile.mBuffer, mAramAddr, ((mArchive->mFile.mSize + 31) & ~31), HSD_DEVCOM_MRAM_TO_ARAM,
				                  1, ObjectArchiveReadDone, &mState);
				// ObjectArchiveReadDone moves mState on to 3 once the copy is done
				while (*(volatile s32*)&mState == 2) { }
			} else {
				mState++;
			}
		} else {
			mState++;
		}
	case 3: {
		CObjArchive* archive = mArchive;
		archive->Parse();
		mState = 0;
		break;
	}
	}
	return mState == 0;
}
// CJArchive::ReadCheck()
/**
 * @note Address: 0x801A3E14
 * @note Size: 0x104
 */
void CJArchive::Release()
{
	if (mArchive != NULL) {
		if (mArchive->mRefCount > 0) {
			mArchive->mRefCount--;
		}
		if (mArchive->mRefCount <= 0) {
			mArchive->mFile.Close();
		}
		mArchive = NULL;
	}
	mState          = 0;
	mJoint          = NULL;
	mAnimJoint      = NULL;
	mMatAnimJoint   = NULL;
	mShapeAnimJoint = NULL;
	for (int i = 0; i < CJARCHIVE_DESC_NUM; i++) {
		mDesc[i] = NULL;
	}
	for (int i = 0; i < CJARCHIVE_DESC_NUM + 1; i++) {
		mNum[i] = 0;
	}
}

/**
 * @note Address: 0x801A3A84
 * @note Size: 0x390
 */
void CJArchive::SetupScene(s32 model, s32 sceneIndex)
{
	CJArchiveScene* scene;

	scene           = GetScene(sceneIndex);
	mJoint          = scene != NULL ? scene->mModels[model]->mJoint : NULL;
	scene           = GetScene(sceneIndex);
	mAnimJoint      = scene != NULL ? scene->mModels[model]->mAnimJoint : NULL;
	scene           = GetScene(sceneIndex);
	mMatAnimJoint   = scene != NULL ? scene->mModels[model]->mMatAnimJoint : NULL;
	scene           = GetScene(sceneIndex);
	mShapeAnimJoint = scene != NULL ? scene->mModels[model]->mShapeAnimJoint : NULL;

	// mNum[0] is the anim count, taken from the first of the three lists the model has
	if (mAnimJoint != NULL) {
		for (mNum[0] = 0;; mNum[0]++) {
			if (mAnimJoint[mNum[0]] == NULL) {
				break;
			}
		}
	} else if (mMatAnimJoint != NULL) {
		for (mNum[0] = 0;; mNum[0]++) {
			if (mMatAnimJoint[mNum[0]] == NULL) {
				break;
			}
		}
	} else if (mShapeAnimJoint != NULL) {
		for (mNum[0] = 0;; mNum[0]++) {
			if (mShapeAnimJoint[mNum[0]] == NULL) {
				break;
			}
		}
	} else {
		mNum[0] = 0;
	}

	scene    = GetScene(sceneIndex);
	mDesc[0] = scene != NULL ? scene->mCamera : NULL;
	if (mDesc[0] != NULL && mDesc[0]->mAnimations != NULL) {
		for (mNum[1] = 0;; mNum[1]++) {
			if (mDesc[0]->mAnimations[mNum[1]] == NULL) {
				break;
			}
		}
	} else {
		mNum[1] = 0;
	}

	for (int i = 0; i < 8; i++) {
		scene        = GetScene(sceneIndex);
		mDesc[i + 1] = scene != NULL ? scene->mLights[i] : NULL;
		if (mDesc[i + 1] == NULL) {
			break;
		}
		if (mDesc[i + 1]->mAnimations != NULL) {
			for (mNum[i + 2] = 0;; mNum[i + 2]++) {
				if (mDesc[i + 1]->mAnimations[mNum[i + 2]] == NULL) {
					break;
				}
			}
		} else {
			mNum[i + 2] = 0;
		}
	}

#if !defined(VERSION_JP)
	scene    = GetScene(sceneIndex);
	mDesc[9] = scene != NULL ? scene->_0C : NULL;
	if (mDesc[9] != NULL && mDesc[9]->mAnimations != NULL) {
		// checks the last list but counts the first one, retail's like that
		for (mNum[10] = 0;; mNum[10]++) {
			if (mDesc[0]->mAnimations[mNum[10]] == NULL) {
				break;
			}
		}
	} else {
		mNum[10] = 0;
	}
#endif
}
// CJArchive::SetupScene(long, long)
/**
 * @note Address: 0x801A39D0
 * @note Size: 0xB4
 */
// goes depth levels down from anim id's root, skipping path[i] siblings on level i
HSD_AnimJoint* CJArchive::GetAnimJoint(s32 id, s32 depth, const s32* path)
{
	HSD_AnimJoint* joint;
	joint = GetAnimJointRoot(id);
	if (joint == NULL) {
		return NULL;
	}
	for (s32 i = 0; i < depth; i++) {
		joint = joint->child;
		for (s32 j = 0; j < path[i]; j++) {
			joint = joint->next;
			if (joint == NULL) {
				return NULL;
			}
		}
	}
	return joint;
}

/**
 * @note Address: 0x801A391C
 * @note Size: 0xB4
 */
HSD_MatAnimJoint* CJArchive::GetMatAnimJoint(s32 id, s32 depth, const s32* path)
{
	HSD_MatAnimJoint* joint;
	joint = GetMatAnimJointRoot(id);
	if (joint == NULL) {
		return NULL;
	}
	for (s32 i = 0; i < depth; i++) {
		joint = joint->child;
		for (s32 j = 0; j < path[i]; j++) {
			joint = joint->next;
			if (joint == NULL) {
				return NULL;
			}
		}
	}
	return joint;
}

/**
 * @note Address: 0x801A3868
 * @note Size: 0xB4
 */
HSD_ShapeAnimJoint* CJArchive::GetShapeAnimJoint(s32 id, s32 depth, const s32* path)
{
	HSD_ShapeAnimJoint* joint;
	joint = GetShapeAnimJointRoot(id);
	if (joint == NULL) {
		return NULL;
	}
	for (s32 i = 0; i < depth; i++) {
		joint = joint->child;
		for (s32 j = 0; j < path[i]; j++) {
			joint = joint->next;
			if (joint == NULL) {
				return NULL;
			}
		}
	}
	return joint;
}

/**
 * @note Address: 0x801A37E8
 * @note Size: 0x80
 */
// this and the next three are HSD_JObjWalkTree callbacks, the flags come in as the walk's argument
extern "C" void JObjSetMObjFlags(HSD_JObj* jobj, void* flags, s32)
{
	u32 mask       = (u32)flags;
	HSD_DObj* dobj = HSD_JObjGetDObj(jobj);
	while (dobj != NULL) {
		HSD_MObj* mobj = HSD_DObjGetMObj(dobj);
		if (mobj != NULL) {
			HSD_MObjSetFlags(mobj, mask);
		}
		dobj = HSD_DObjGetNext(dobj);
	}
}
// JObjSetMObjFlags
/**
 * @note Address: 0x801A3768
 * @note Size: 0x80
 */
extern "C" void JObjClearMObjFlags(HSD_JObj* jobj, void* flags, s32)
{
	u32 mask       = (u32)flags;
	HSD_DObj* dobj = HSD_JObjGetDObj(jobj);
	while (dobj != NULL) {
		HSD_MObj* mobj = HSD_DObjGetMObj(dobj);
		if (mobj != NULL) {
			HSD_MObjClearFlags(mobj, mask);
		}
		dobj = HSD_DObjGetNext(dobj);
	}
}
// JObjClearMObjFlags
/**
 * @note Address: 0x801A36E8
 * @note Size: 0x80
 */
extern "C" void JObjSetPObjFlags(HSD_JObj* jobj, void* flags, s32)
{
	u32 mask       = (u32)flags;
	HSD_DObj* dobj = HSD_JObjGetDObj(jobj);
	while (dobj != NULL) {
		HSD_PObj* pobj = HSD_DObjGetPObj(dobj);
		if (pobj != NULL) {
			HSD_PObjSetFlags(pobj, mask);
		}
		dobj = HSD_DObjGetNext(dobj);
	}
}
// JObjSetPObjFlags
/**
 * @note Address: 0x801A3668
 * @note Size: 0x80
 */
extern "C" void JObjClearPObjFlags(HSD_JObj* jobj, void* flags, s32)
{
	u32 mask       = (u32)flags;
	HSD_DObj* dobj = HSD_JObjGetDObj(jobj);
	while (dobj != NULL) {
		HSD_PObj* pobj = HSD_DObjGetPObj(dobj);
		if (pobj != NULL) {
			HSD_PObjClearFlags(pobj, mask);
		}
		dobj = HSD_DObjGetNext(dobj);
	}
}
// JObjClearPObjFlags
/**
 * @note Address: 0x801A346C
 * @note Size: 0x1FC
 */
u8 CObjVec::Step(u32 frames)
{
	u8 moving = IsMoving();

	if (x < mTarget.x) {
		if ((x += mSpeed.x * frames) > mTarget.x) {
			x = mTarget.x;
		}
	} else if (x > mTarget.x) {
		if ((x -= mSpeed.x * frames) < mTarget.x) {
			x = mTarget.x;
		}
	}
	if (y < mTarget.y) {
		if ((y += mSpeed.y * frames) > mTarget.y) {
			y = mTarget.y;
		}
	} else if (y > mTarget.y) {
		if ((y -= mSpeed.y * frames) < mTarget.y) {
			y = mTarget.y;
		}
	}
	if (z < mTarget.z) {
		if ((z += mSpeed.z * frames) > mTarget.z) {
			z = mTarget.z;
		}
	} else if (z > mTarget.z) {
		if ((z -= mSpeed.z * frames) < mTarget.z) {
			z = mTarget.z;
		}
	}
	return moving;
}
// CObjVec::Step(unsigned long)
/**
 * @note Address: 0x801A3414
 * @note Size: 0x58
 */
void XCObjInfoInit()
{
	hsdInitClassInfo(HSD_CLASS_INFO(&xCObj), HSD_CLASS_INFO(&hsdMObj), "x_cobj_class", "x_cobj", sizeof(HSD_MObjInfo), sizeof(XCObj));
	HSD_MOBJ_INFO(&xCObj)->make_texp = XCObjMakeTExp;
}
// XCObjInfoInit()

/**
 * @note Address: 0x801A326C
 * @note Size: 0x1A8
 */
// the parent's tev times the second color, then the first color added on top
HSD_TExp* XCObjMakeTExp(HSD_MObj* mobj, HSD_TObj* tobj_top, HSD_TExp** list)
{
	HSD_TExp* tev1;
	HSD_TExp* tev0;
	HSD_TExp* exp;
	HSD_TExp* cnst0;
	u32* colors;
	HSD_TExp* cnst1;
	exp    = hsdMObj.make_texp(mobj, tobj_top, list);
	colors = ((XCObj*)mobj)->mColors;
	if (colors == NULL) {
		return exp;
	}
	cnst1 = HSD_TExpCnst(&colors[1], HSD_TE_RGB, HSD_TE_U8, list);
	tev0  = HSD_TExpTev(list);
	HSD_TExpColorOp(tev0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE);
	HSD_TExpColorIn(tev0, HSD_TE_0, NULL, HSD_TE_RGB, exp, HSD_TE_RGB, cnst1, HSD_TE_0, NULL);
	HSD_TExpAlphaOp(tev0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE);
	HSD_TExpAlphaIn(tev0, HSD_TE_A, exp, HSD_TE_0, NULL, HSD_TE_0, NULL, HSD_TE_0, NULL);

	cnst0 = HSD_TExpCnst(colors, HSD_TE_RGB, HSD_TE_U8, list);
	tev1  = HSD_TExpTev(list);
	HSD_TExpColorOp(tev1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE);
	HSD_TExpColorIn(tev1, HSD_TE_RGB, tev0, HSD_TE_0, NULL, HSD_TE_0, NULL, HSD_TE_RGB, cnst0);
	HSD_TExpAlphaOp(tev1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE);
	HSD_TExpAlphaIn(tev1, HSD_TE_A, tev0, HSD_TE_0, NULL, HSD_TE_0, NULL, HSD_TE_0, NULL);
	return tev1;
}

/**
 * @note Address: 0x801A31E0
 * @note Size: 0x8C
 */
// HSD_JObjWalkTree callback, points every x_cobj material under the jobj at the
// colors
void XCObjSetColors(HSD_JObj* jobj, void* context, s32)
{
	u32* colors    = static_cast<u32*>(context);
	HSD_DObj* dobj = HSD_JObjGetDObj(jobj);
	while (dobj != NULL) {
		XCObj* mobj = (XCObj*)(dobj != NULL ? dobj->mobj : NULL);
		if (mobj != NULL) {
			mobj->mColors = colors;
			mobj->mAlpha  = mobj->parent.mat->alpha;
			HSD_MObjCompileTev(&mobj->parent);
		}
		HSD_DObj* next = dobj != NULL ? dobj->next : NULL;
		dobj           = next;
	}
}

/**
 * @note Address: 0x801A31D8
 * @note Size: 0x8
 */
f32 CXObj::GetMObjAlpha(HSD_MObj* mobj)
{
	return ((XCObj*)mobj)->mAlpha;
}

/**
 * @note Address: 0x801A30F4
 * @note Size: 0xE4
 */
void CXObj::Init(u32 id)
{
	mArchive.Clear();
	mId                      = id;
	mLoadState               = 0;
	mMoveTranslate.x         = 0.0f;
	mMoveTranslate.y         = 0.0f;
	mMoveTranslate.z         = 0.0f;
	mMoveTranslate.mTarget.x = 0.0f;
	mMoveTranslate.mTarget.y = 0.0f;
	mMoveTranslate.mTarget.z = 0.0f;
	mMoveTranslate.mSpeed.x  = 0.0f;
	mMoveTranslate.mSpeed.y  = 0.0f;
	mMoveTranslate.mSpeed.z  = 0.0f;
	mMoveRotate.x            = 0.0f;
	mMoveRotate.y            = 0.0f;
	mMoveRotate.z            = 0.0f;
	mMoveRotate.mTarget.x    = 0.0f;
	mMoveRotate.mTarget.y    = 0.0f;
	mMoveRotate.mTarget.z    = 0.0f;
	mMoveRotate.mSpeed.x     = 0.0f;
	mMoveRotate.mSpeed.y     = 0.0f;
	mMoveRotate.mSpeed.z     = 0.0f;
	mMoveScale.x             = 1.0f;
	mMoveScale.y             = 1.0f;
	mMoveScale.z             = 1.0f;
	mMoveScale.mTarget.x     = 1.0f;
	mMoveScale.mTarget.y     = 1.0f;
	mMoveScale.mTarget.z     = 1.0f;
	mMoveScale.mSpeed.x      = 0.0f;
	mMoveScale.mSpeed.y      = 0.0f;
	mMoveScale.mSpeed.z      = 0.0f;
	mAnimId                  = -1;
	mLastAnimRate            = 0.0f;
	mAnimRate                = 0.0f;
	mColors[0]               = 0;          // nothing added
	mColors[1]               = 0xFFFFFF00; // and times white, so the materials start untinted
}

/**
 * @note Address: 0x801A306C
 * @note Size: 0x88
 */
BOOL CXObj::ReadAsync(CObjArchive* archive, const char* path, u32 model, u32 flags, u32 aramAddr, u32 aramSize)
{
	if (mArchive.ReadAsync(archive, path, flags, aramAddr, aramSize)) {
		mLoadState++;
		mModel     = model;
		mLoadFlags = flags;
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x801A2F38
 * @note Size: 0x134
 */
// Retail calls both Attach and the base ReadCheck out of line.
void CXObj::Attach(HSD_JObj* jobj)
{
	CJObj::Attach(jobj);
	SyncMoveFromJoint(this);
}
// CXObj::Attach(_HSD_JObj*)
/**
 * @note Address: 0x801A2D68
 * @note Size: 0x1D0
 */
BOOL CXObj::ReadCheck()
{
	if (mLoadState == 1 && mArchive.ReadCheck()) {
		mArchive.SetupScene(mModel, 0);
		// load the materials as x_cobj, so XCObjSetColors can tint them
		HSD_MObjInfo* mobjClass = HSD_MObjGetDefaultClass();
		HSD_MObjSetDefaultClass(&xCObj);
		Attach(mArchive.LoadJObj());
		HSD_MObjSetDefaultClass(mobjClass);
		if (mJObj != NULL) {
			HSD_JObjWalkTree(mJObj, XCObjSetColors, mColors);
		}
		HSD_JObjGetScale(mJObj, &mMoveScale);
		ClearQueue();
		mLoadState = 0;
	}
	return mLoadState == 0;
}
// CXObj::ReadCheck()
/**
 * @note Address: 0x801A2D04
 * @note Size: 0x64
 */
void CXObj::Release()
{
	ReleaseJObj(this);
	mArchive.Release();
	mLoadState = 0;
}

/**
 * @note Address: 0x801A2918
 * @note Size: 0x3EC
 */
BOOL CXObj::StepRotate(u32 frames)
{
	u8 moving = mMoveRotate.IsMoving();
	mMoveRotate.Step(frames);
	if (mRotate != mMoveRotate) {
		mRotate.Set(mMoveRotate.x, mMoveRotate.y, mMoveRotate.z);
		UpdateRotation();
	}
	return moving;
}

/**
 * @note Address: 0x801A25DC
 * @note Size: 0x33C
 */
BOOL CXObj::StepTranslate(u32 frames)
{
	u8 moving = mMoveTranslate.IsMoving();
	mMoveTranslate.Step(frames);
	if (mTranslate != mMoveTranslate) {
		mTranslate.Set(mMoveTranslate.x, mMoveTranslate.y, mMoveTranslate.z);
		HSD_JObjSetTranslate(mJObj, &mTranslate);
	}
	return moving;
}

/**
 * @note Address: 0x801A22A4
 * @note Size: 0x338
 */
BOOL CXObj::StepScale(u32 frames)
{
	u8 moving = mMoveScale.IsMoving();
	mMoveScale.Step(frames);
	if (mScale != mMoveScale) {
		mScale.Set(mMoveScale.x, mMoveScale.y, mMoveScale.z);
		HSD_JObjSetScale(mJObj, &mScale);
	}
	return moving;
}

/**
 * @note Address: 0x801A211C
 * @note Size: 0x188
 */
// dir is the point to move to, not a direction; speed is the distance per frame
void CXObj::SetMoveDir(const Vec* dir, f32 speed)
{
	mMoveTranslate.mTarget.x = dir->x;
	mMoveTranslate.mTarget.y = dir->y;
	mMoveTranslate.mTarget.z = dir->z;
	CVec d(mMoveTranslate - *dir);
	if (PSVECMag(&d) > 0.0f) {
		d.x = ABS(d.x);
		d.y = ABS(d.y);
		d.z = ABS(d.z);
		// each axis gets its share of the distance, so all three arrive on the same frame
		PSVECNormalize(&d, &d);
		mMoveTranslate.mSpeed.Set(d * speed);
		if (mMoveTranslate.mSpeed.x > 0.0f && mMoveTranslate.mSpeed.x < 0.001f) {
			mMoveTranslate.mSpeed.x = 0.001f;
		}
		if (mMoveTranslate.mSpeed.z > 0.0f && mMoveTranslate.mSpeed.z < 0.001f) {
			mMoveTranslate.mSpeed.z = 0.001f;
		}
	}
}
// CXObj::SetMoveDir(const Vec*, float)
/**
 * @note Address: 0x801A2030
 * @note Size: 0xEC
 */
void CXObj::SetRotateY(f32 target, f32 speed)
{
	mMoveRotate.mTarget.y = target;
	mMoveRotate.mSpeed.y  = speed;
	if (speed == 0.0f) {
		mMoveRotate.y = target;
		return;
	}
	f32 cur               = WrapAngle(mMoveRotate.y);
	f32 tgt               = WrapAngle(mMoveRotate.mTarget.y);
	mMoveRotate.y         = cur;
	mMoveRotate.mTarget.y = tgt;
	if (tgt == cur) {
		return;
	}
	f32 diff = tgt - cur;
	if (diff < 0.0f) {
		diff = -diff;
	}
	if (diff >= TWO_PI) {
		diff -= TWO_PI;
	}
	// go the short way round
	if (diff > PI) {
		if (tgt < cur) {
			mMoveRotate.mTarget.y = TWO_PI + tgt;
		} else {
			mMoveRotate.mTarget.y = tgt - TWO_PI;
		}
	}
}

/**
 * @note Address: 0x801A1E10
 * @note Size: 0x220
 */
void CXObj::SetAnim(s32 id, BOOL force, void* jointAnim, void* matAnim, void* shapeAnim, f32 frame, f32 rate, f32 blend, f32 blendRate)
{
	if (force || id != mAnimId) {
		mAnimId     = id;
		void* joint = jointAnim != NULL ? jointAnim : mArchive.GetAnimJointRoot(id);
		void* mat   = matAnim != NULL ? matAnim : mArchive.GetMatAnimJointRoot(id);
		void* shape = shapeAnim != NULL ? shapeAnim : mArchive.GetShapeAnimJointRoot(id);
		if (joint != NULL || mat != NULL || shape != NULL) {
			HSD_JObjRemoveAnimAll(mJObj);
			HSD_JObjAddAnimAll(mJObj, (HSD_AnimJoint*)joint, (HSD_MatAnimJoint*)mat, (HSD_ShapeAnimJoint*)shape);
			HSD_JObjReqAnimAll(mJObj, frame);
		}
		// a new anim always takes the rate too
		force = TRUE;
	}
	if (rate != mAnimRate || force) {
		mAnimRate     = rate;
		mLastAnimRate = 0.01f + rate;
	}
}

/**
 * @note Address: 0x801A1DE4
 * @note Size: 0x2C
 */
void CXObj::ClearAnim()
{
	mAnimId = -1;
	HSD_JObjRemoveAnimAll(mJObj);
}

/**
 * @note Address: 0x801A1DD0
 * @note Size: 0x14
 */
void CXObj::SetAnimRate(f32 rate, u32 mask)
{
	mAnimRate = rate;
	// off by a bit so the next UpdateAnim always sets it
	mLastAnimRate = 0.01f + rate;
}

/**
 * @note Address: 0x801A16FC
 * @note Size: 0x6D4
 */
void CXObj::UpdateAnim(u32 speed)
{
	if (mLastAnimRate != mAnimRate * speed) {
		mLastAnimRate = mAnimRate * speed;
		CJObj::SetAnimRate(mLastAnimRate, JOBJ_MASK | MOBJ_MASK | POBJ_MASK | TOBJ_MASK);
	}
	if (mQueued != 0) {
		for (int i = 0; i < 10; i++) {
			if (mQueue[i].mJObj == NULL) {
				break;
			}
			// rates go on before the anim step: 0x200 the joint's own anim, 0x400 every anim in its subtree
			if (mQueue[i].mType == 0x200) {
				if (mQueue[i].mJObj->aobj != NULL) {
					HSD_AObjSetRate(mQueue[i].mJObj->aobj, mQueue[i].mValue * speed);
				}
			} else if (mQueue[i].mType == 0x400) {
				HSD_ForeachAnim(mQueue[i].mJObj, JOBJ_TYPE, (HSD_TypeMask)(JOBJ_MASK | MOBJ_MASK | POBJ_MASK | TOBJ_MASK), HSD_AObjSetRate,
				                AOBJ_ARG_AF, mQueue[i].mValue * speed);
			}
		}
	}
	HSD_JObjAnimAll(mJObj);
	if (mQueued != 0) {
		for (int i = 0; i < 10; i++) {
			if (mQueue[i].mJObj == NULL) {
				break;
			}
			// values go on after it, over what the anim set; the rate entries are only cleared
			HSD_JObj* jobj = mQueue[i].mJObj;
			if (mQueue[i].mType != 0) {
				switch (mQueue[i].mType) {
				case 1:
					HSD_JObjSetTranslateX(jobj, mQueue[i].mValue);
					break;
				case 2:
					HSD_JObjSetTranslateY(jobj, mQueue[i].mValue);
					break;
				case 4:
					HSD_JObjSetTranslateZ(jobj, mQueue[i].mValue);
					break;
				case 8:
					HSD_JObjSetScaleX(jobj, mQueue[i].mValue);
					break;
				case 0x10:
					HSD_JObjSetScaleY(jobj, mQueue[i].mValue);
					break;
				case 0x20:
					HSD_JObjSetScaleZ(jobj, mQueue[i].mValue);
					break;
				case 0x40:
					HSD_JObjSetRotationX(jobj, mQueue[i].mValue);
					break;
				case 0x80:
					HSD_JObjSetRotationY(jobj, mQueue[i].mValue);
					break;
				case 0x100:
					HSD_JObjSetRotationZ(jobj, mQueue[i].mValue);
					break;
				}
				mQueue[i].mJObj = NULL;
				mQueue[i].mType = 0;
			}
		}
		mQueued = 0;
	}
}
// CXObj::UpdateAnim(unsigned long)
/**
 * @note Address: 0x801A16B0
 * @note Size: 0x4C
 */
void CXObj::Push(HSD_JObj* jobj, u32 type, f32 value)
{
	for (int i = 0; i < 10; i++) {
		if (mQueue[i].mJObj == NULL) {
			mQueue[i].mJObj  = jobj;
			mQueue[i].mType  = type;
			mQueue[i].mValue = value;
			break;
		}
	}
	mQueued = 1;
}

/**
 * @note Address: 0x801A163C
 * @note Size: 0x74
 */
BOOL CXObj::Load(CObjArchive* archive, const char* path, u32 model, u32 flags, u32 aramAddr, u32 aramSize)
{
	switch (mLoadState) {
	case 0:
		ReadAsync(archive, path, model, flags, aramAddr, aramSize);
		break;
	case 1:
		ReadCheck();
		break;
	}
	return mLoadState == 0;
}

/**
 * @note Address: 0x801A1498
 * @note Size: 0x1A4
 */
BOOL CXObjIntp::ReadCheck()
{
	// 0x10000: load a second copy of the model to blend anims from
	if (mLoadState == 1 && CXObj::ReadCheck() && (mLoadFlags & 0x10000)) {
		mIntp.Attach(mArchive.LoadJObj());
#if defined(VERSION_JP)
		RemoveJointTreeDisplayObjects(mIntp.mJObj);
#endif
		ConvertJointTreeRotationToQuaternion(mIntp.mJObj);
		mBlend = 1.0f;
	}
	return mLoadState == 0;
}
// CXObjIntp::ReadCheck()

/**
 * @note Address: 0x801A13F0
 * @note Size: 0xA8
 */
void CXObjIntp::Release()
{
	if (mIntp.mJObj != NULL) {
		ReleaseJObj(&mIntp);
		// drop the archive reference mIntp's LoadJObj took, CJArchive::Release drops the model's
		if (mArchive.mArchive != NULL && mArchive.mArchive->mRefCount > 0) {
			mArchive.mArchive->mRefCount--;
		}
	}
	CXObj::Release();
}

/**
 * @note Address: 0x801A138C
 * @note Size: 0x64
 */
BOOL CXObjIntp::StepRotate(u32 frames)
{
	BOOL moving = CXObj::StepRotate(frames);
	if (mIntp.mJObj != NULL) {
		mIntp.SetRotation(mMoveRotate, 0);
	}
	return moving;
}

/**
 * @note Address: 0x801A1328
 * @note Size: 0x64
 */
BOOL CXObjIntp::StepTranslate(u32 frames)
{
	BOOL moving = CXObj::StepTranslate(frames);
	if (mIntp.mJObj != NULL) {
		mIntp.SetTranslate(mMoveTranslate, 0);
	}
	return moving;
}

/**
 * @note Address: 0x801A12C4
 * @note Size: 0x64
 */
BOOL CXObjIntp::StepScale(u32 frames)
{
	BOOL moving = CXObj::StepScale(frames);
	if (mIntp.mJObj != NULL) {
		mIntp.SetScale(mMoveScale, 0);
	}
	return moving;
}

/**
 * @note Address: 0x801A1020
 * @note Size: 0x2A4
 */
void CXObjIntp::SetAnim(s32 id, BOOL force, void* jointAnim, void* matAnim, void* shapeAnim, f32 frame, f32 rate, f32 blend, f32 blendRate)
{
	if ((force || id != mAnimId) && mIntp.mJObj != NULL) {
		if (blend < 1.0f) {
			// keep the pose the model has now in mIntp, UpdateAnim blends from it
			CopyJointPoseTree(mJObj, mIntp.mJObj);
			ConvertJointTreeRotationToQuaternion(mIntp.mJObj);
		} else if (mBlend < 1.0f) {
			ApplyTransform();
		}
		mBlend     = blend;
		mBlendRate = blendRate;
	}
	CXObj::SetAnim(id, force, jointAnim, matAnim, shapeAnim, frame, rate, blend, blendRate);
}
// CXObjIntp::SetAnim(long, int, void*, void*, void*, float, float, float, float)

/**
 * @note Address: 0x801A0EE8
 * @note Size: 0x138
 */
void CXObjIntp::UpdateAnim(u32 speed)
{
	if (mIntp.mJObj != NULL) {
		if (mBlend < 1.0f) {
			ApplyTransform();
		}
		CXObj::UpdateAnim(speed);
		if (mBlend < 1.0f) {
			// mBlendRate is the blend's length in frames
			f32 blend = mBlend + (1.0f / mBlendRate) * speed;
			mBlend    = blend;
			if (blend >= 1.0f || GetAObj(mJObj)->flags & AOBJ_NO_ANIM) {
				mBlend = 1.0f;
			} else {
				BlendJointTreePoses(mIntp.mJObj->child, mJObj->child, mJObj->child, mBlend);
			}
		}
	} else {
		CXObj::UpdateAnim(speed);
	}
}

/**
 * @note Address: 0x801A0BCC
 * @note Size: 0x31C
 */
// NONMATCHING: retail expands additional HSD assert/matrix-dirty paths around the inline setters. Pose restore,
// translation, scale and rotation calls are present; no hand-expanded setters are introduced.
void CXObjIntp::ApplyTransform()
{
	RestoreJointTreePose(mArchive.mJoint, mJObj);
	mTranslate.Set(mMoveTranslate.x, mMoveTranslate.y, mMoveTranslate.z);
	HSD_JObjSetTranslate(mJObj, &mTranslate);
	mScale.Set(mMoveScale.x, mMoveScale.y, mMoveScale.z);
	HSD_JObjSetScale(mJObj, &mScale);
	mRotate.Set(mMoveRotate.x, mMoveRotate.y, mMoveRotate.z);
	UpdateRotation();
}
// CXObjIntp::ApplyTransform()

/**
 * @note Address: 0x801A0A58
 * @note Size: 0x174
 */
void CJointRot::Init(CXObj* obj, s32 depth, s32* path, char axis)
{
	mMode   = 0;
	mSpeed  = 0.0f;
	mTarget = 0.0f;
	mAngle  = 0.0f;
	if (obj->mJObj != NULL) {
		mObj = obj;
		if (mObj != NULL) {
			mJoint = mObj->FindJoint(depth, path);
		}
	}
	// stored as CXObj's queue type for a rotation about that axis, see UpdateAnim
	switch (axis) {
	case 'x':
	case 'X':
		mAxis = 0x40;
		break;
	case 'y':
	case 'Y':
		mAxis = 0x80;
		break;
	case 'z':
	case 'Z':
		mAxis = 0x100;
		break;
	default:
#if defined(VERSION_JP)
		HALT(1226);
#else
		HALT(1253);
#endif
	}
	mLookAt = NULL;
}

/**
 * @note Address: 0x801A0774
 * @note Size: 0x2E4
 */
bool CJointRot::Update(u32 frames)
{
	if (mMode == 0) {
		return false;
	}
	if (mLookAt != NULL) {
		CXObj* obj = mObj;
		mTarget    = (f32)atan2(mLookAt->mMoveTranslate.x - obj->mMoveTranslate.x, mLookAt->mMoveTranslate.z - obj->mMoveTranslate.z)
		        - obj->mMoveRotate.y;
	}
	mAngle  = WrapAngle(mAngle);
	mTarget = WrapAngle(mTarget);

	f32 diff = Abs(mTarget - mAngle);
	f32 dist = diff;
	if (diff > 0.0f) {
		if (diff >= TWO_PI) {
			dist = diff - TWO_PI;
		}
		// go the short way round
		if (dist > PI) {
			if (mTarget < mAngle) {
				mTarget += TWO_PI;
			} else {
				mTarget -= TWO_PI;
			}
		}
	}

	if (dist >= 0.001f) {
		f32 left = mTarget - mAngle;
		if (left < 0.0f) {
			left = -left;
		}
		mSpeed = 0.1f * left;
		if (mSpeed < 0.01f) {
			mSpeed = 0.01f;
		}
		if (mAngle < mTarget) {
			if ((mAngle += mSpeed * frames) > mTarget) {
				mAngle = mTarget;
			}
		} else if (mAngle > mTarget) {
			if ((mAngle -= mSpeed * frames) < mTarget) {
				mAngle = mTarget;
			}
		}
	} else {
		// there: mode 2 stops, other positive modes turn negative and keep pushing the angle
		if (mMode > 0) {
			if (mMode == 2) {
				mMode = 0;
			} else {
				mMode = -mMode;
			}
		}
		mSpeed = 0.0f;
		mAngle = mTarget;
	}
	mObj->Push(mJoint, mAxis, mAngle);
	return mMode != 0;
}
