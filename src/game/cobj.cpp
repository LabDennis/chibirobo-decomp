#include "game/cobj.h"

#include "game/cjobjset.h"
#include "game/gmath.h"
#include "game/hsd_dobj.h"
extern "C" {
#include <sysdolphin/class.h>
#include <sysdolphin/dobj.h>
#include <sysdolphin/pobj.h>
#include <sysdolphin/texp.h>
#include <dolphin/mtx/vec.h>
#include <dolphin/os.h>
}
#include <math.h>
#include "game/xobj.h"

extern "C" {
void OSReport(const char* msg, ...);
u32 ARGetSize(void);
s32 HSD_DevComRequest(s32 file, u32 src, void* dest, u32 size, s32 type, s32 pri, void (*callback)(s32, s32*), s32* args);
void HSD_MObjSetDefaultClass(HSD_MObjInfo* info);
void HSD_JObjWalkTree(HSD_JObj* jobj, HSD_JObjWalkTreeCallback cb, f32** cb_args);
void HSD_PObjClearFlags(HSD_PObj* pobj, u32 flags);
void HSD_PObjSetFlags(HSD_PObj* pobj, u32 flags);
}

// report, then panic with the function name. the line goes in by hand like the asserts
#define HALT(line)              \
	OSReport("\n!!!HALT!!!\n"); \
	OSPanic(__FILE__, line, "%s%s", "", __FUNCTION__)

// the same with a formatted message
#define HALTF(line, ...)        \
	OSReport("\n!!!HALT!!!\n"); \
	OSPanic(__FILE__, line, "%s%s", FormatToStaticBuffer(__VA_ARGS__), __FUNCTION__)

// a material with two extra colors added on top of its tev result
struct XCObj {
	HSD_MObj parent; // _00
	u32* mColors;    // _20
	f32 mAlpha;      // _24, the material's own alpha
};

void XCObjInfoInit();
HSD_TExp* XCObjMakeTExp(HSD_MObj* mobj, HSD_TObj* tobj_top, HSD_TExp** list);
void XCObjSetColors(HSD_JObj* jobj, u32* colors, s32);
HSD_MObjInfo xCObj = { { { XCObjInfoInit } } };

void ObjectArchiveReadDone(s32, s32* state);

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
void ObjectArchiveReadDone(s32, s32* state)
{
	(*state)++;
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
	if (mArchive->mFile.mBuffer != NULL) {
		return TRUE;
	}

	s32 result;
	if (flags & 4) {
		result = aramAddr != 0 ? mArchive->mFile.ReadAsync(path, aramAddr, size, TRUE) : mArchive->mFile.ReadAsync(path, 0, 0, FALSE);
	} else {
		result = mArchive->mFile.ReadAsync(path, 0, 0, FALSE);
	}
	if (result >= 0) {
		mState++;
	} else {
		mResult = result;
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
/*
    stwu     r1, -0x20(r1)
    mflr     r0
    stw      r0, 0x24(r1)
    li       r0, 0x0
    stw      r31, 0x1c(r1)
    mr       r31, r5
    stw      r30, 0x18(r1)
    mr       r30, r3
    stw      r29, 0x14(r1)
    stw      r4, 0x0(r3)
    stw      r6, 0x8(r3)
    stw      r0, 0xc(r3)
    stw      r7, 0x10(r3)
    stw      r8, 0x14(r3)
    lwz      r0, 0x4(r3)
    cmpwi    r0, 0x0
    beq      .L_801A41A8
    li       r3, 0x0
    b        .L_801A42EC
.L_801A41A8:
    lwz      r3, 0x0(r30)
    lwz      r0, 0x110(r3)
    cmplwi   r0, 0x0
    beq      .L_801A41C0
    li       r3, 0x1
    b        .L_801A42EC
.L_801A41C0:
    rlwinm.  r0, r6, 0, 29, 29
    beq      .L_801A4224
    cmplwi   r7, 0x0
    beq      .L_801A41F8
    addi     r3, r3, 0x44
    mr       r4, r31
    lwz      r12, 0xe8(r3)
    mr       r5, r7
    mr       r6, r8
    li       r7, 0x1
    lwz      r12, 0x14(r12)
    mtctr    r12
    bctrl
    b        .L_801A421C
.L_801A41F8:
    addi     r3, r3, 0x44
    mr       r4, r31
    lwz      r12, 0xe8(r3)
    li       r5, 0x0
    li       r6, 0x0
    li       r7, 0x0
    lwz      r12, 0x14(r12)
    mtctr    r12
    bctrl
.L_801A421C:
    mr       r29, r3
    b        .L_801A424C
.L_801A4224:
    addi     r3, r3, 0x44
    mr       r4, r31
    lwz      r12, 0xe8(r3)
    li       r5, 0x0
    li       r6, 0x0
    li       r7, 0x0
    lwz      r12, 0x14(r12)
    mtctr    r12
    bctrl
    mr       r29, r3
.L_801A424C:
    cmpwi    r29, 0x0
    blt      .L_801A4264
    lwz      r3, 0x4(r30)
    addi     r0, r3, 0x1
    stw      r0, 0x4(r30)
    b        .L_801A42E8
.L_801A4264:
    stw      r29, 0xc(r30)
    lwz      r0, 0x8(r30)
    rlwinm.  r0, r0, 0, 23, 23
    beq      .L_801A42A0
    lwz      r3, 0x0(r30)
    lwz      r0, 0x130(r3)
    cmpwi    r0, 0x0
    bgt      .L_801A4298
    addi     r3, r3, 0x44
    lwz      r12, 0xe8(r3)
    lwz      r12, 0x20(r12)
    mtctr    r12
    bctrl
.L_801A4298:
    li       r3, 0x0
    b        .L_801A42EC
.L_801A42A0:
    lis      r3, sObjectHaltMessage@ha
    addi     r3, r3, sObjectHaltMessage@l
    crclr    cr1eq
    bl       OSReport
    mr       r4, r31
    mr       r5, r29
    li       r3, lbl_8065F750@sda21
    crclr    cr1eq
    bl       FormatToStaticBuffer
    lis      r7, sPathCobjCpp@ha
    lis      r5, sObjectReadAsyncName@ha
    mr       r6, r3
    li       r4, 0x11d
    addi     r3, r7, sPathCobjCpp@l
    addi     r7, r5, sObjectReadAsyncName@l
    li       r5, lbl_8065F738@sda21
    crclr    cr1eq
    bl       OSPanic
.L_801A42E8:
    li       r3, 0x1
.L_801A42EC:
    lwz      r0, 0x24(r1)
    lwz      r31, 0x1c(r1)
    lwz      r30, 0x18(r1)
    lwz      r29, 0x14(r1)
    mtlr     r0
    addi     r1, r1, 0x20
    blr
*/

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
			break;
		}
	}
	case 2:
		if (mFlags & 2) {
			u32 size = ((mArchive->mFile.mSize + 31) & ~31);
			if (mAramAddr + size < ARGetSize()) {
				HSD_DevComRequest(0, (u32)mArchive->mFile.mBuffer, (void*)mAramAddr, ((mArchive->mFile.mSize + 31) & ~31), 0xB, 1,
				                  ObjectArchiveReadDone, &mState);
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
/*
    stwu     r1, -0x20(r1)
    mflr     r0
    stw      r0, 0x24(r1)
    stw      r31, 0x1c(r1)
    stw      r30, 0x18(r1)
    mr       r30, r3
    lis      r3, sObjectRodataBase@ha
    stw      r29, 0x14(r1)
    addi     r31, r3, sObjectRodataBase@l
    stw      r28, 0x10(r1)
    lwz      r0, 0x4(r30)
    cmpwi    r0, 0x2
    beq      .L_801A4030
    bge      .L_801A3F5C
    cmpwi    r0, 0x1
    bge      .L_801A3F68
    b        .L_801A4130
.L_801A3F5C:
    cmpwi    r0, 0x4
    bge      .L_801A4130
    b        .L_801A40BC
.L_801A3F68:
    lwz      r28, 0x0(r30)
    addi     r3, r28, 0x44
    lwz      r12, 0x12c(r28)
    lwz      r12, 0x18(r12)
    mtctr    r12
    bctrl
    mr.      r29, r3
    bne      .L_801A3F90
    addi     r3, r28, 0x44
    bl       CloseHandle__5CFileFv
.L_801A3F90:
    cmpwi    r29, 0x0
    bne      .L_801A3FA8
    lwz      r3, 0x4(r30)
    addi     r0, r3, 0x1
    stw      r0, 0x4(r30)
    b        .L_801A4030
.L_801A3FA8:
    bge      .L_801A4130
    stw      r29, 0xc(r30)
    lwz      r0, 0x8(r30)
    rlwinm.  r0, r0, 0, 23, 23
    beq      .L_801A3FEC
    lwz      r3, 0x0(r30)
    lwz      r0, 0x130(r3)
    cmpwi    r0, 0x0
    bgt      .L_801A3FE0
    addi     r3, r3, 0x44
    lwz      r12, 0xe8(r3)
    lwz      r12, 0x20(r12)
    mtctr    r12
    bctrl
.L_801A3FE0:
    li       r0, 0x0
    stw      r0, 0x4(r30)
    b        .L_801A4130
.L_801A3FEC:
    addi     r3, r31, 0x64
    crclr    cr1eq
    bl       OSReport
    lwz      r4, 0x0(r30)
    mr       r5, r29
    li       r3, lbl_8065F750@sda21
    addi     r4, r4, 0x80
    crclr    cr1eq
    bl       FormatToStaticBuffer
    lis      r4, sObjectReadCheckName@ha
    mr       r6, r3
    addi     r7, r4, sObjectReadCheckName@l
    addi     r3, r31, 0x74
    li       r4, 0x133
    li       r5, lbl_8065F738@sda21
    crclr    cr1eq
    bl       OSPanic
.L_801A4030:
    lwz      r0, 0x8(r30)
    rlwinm.  r0, r0, 0, 30, 30
    beq      .L_801A40B0
    lwz      r3, 0x0(r30)
    lwz      r3, 0x10c(r3)
    addi     r0, r3, 0x1f
    clrrwi   r28, r0, 5
    bl       ARGetSize
    lwz      r5, 0x10(r30)
    add      r0, r5, r28
    cmplw    r0, r3
    bge      .L_801A40A0
    lwz      r4, 0x0(r30)
    lis      r3, ObjectArchiveReadDone__FlPl@ha
    addi     r9, r3, ObjectArchiveReadDone__FlPl@l
    addi     r10, r30, 0x4
    lwz      r3, 0x10c(r4)
    li       r7, 0xb
    lwz      r4, 0x110(r4)
    li       r8, 0x1
    addi     r0, r3, 0x1f
    li       r3, 0x0
    clrrwi   r6, r0, 5
    bl       HSD_DevComRequest
.L_801A4090:
    lwz      r0, 0x4(r30)
    cmpwi    r0, 0x2
    beq      .L_801A4090
    b        .L_801A40BC
.L_801A40A0:
    lwz      r3, 0x4(r30)
    addi     r0, r3, 0x1
    stw      r0, 0x4(r30)
    b        .L_801A40BC
.L_801A40B0:
    lwz      r3, 0x4(r30)
    addi     r0, r3, 0x1
    stw      r0, 0x4(r30)
.L_801A40BC:
    lwz      r28, 0x0(r30)
    lwz      r4, 0x110(r28)
    mr       r3, r28
    lwz      r5, 0x10c(r28)
    bl       HSD_ArchiveParse
    cmpwi    r3, 0x0
    bge      .L_801A4128
    addi     r3, r31, 0x64
    crclr    cr1eq
    bl       OSReport
    addi     r3, r31, 0x90
    addi     r5, r28, 0x80
    li       r4, sObjectArchiveParseName@sda21
    crclr    cr1eq
    bl       FormatToStaticBuffer
    mr       r6, r3
    addi     r3, r31, 0x74
    li       r4, 0x33
    li       r5, lbl_8065F738@sda21
    li       r7, sObjectArchiveParseName@sda21
    crclr    cr1eq
    bl       OSPanic
    addi     r3, r28, 0x44
    lwz      r12, 0x12c(r28)
    lwz      r12, 0x20(r12)
    mtctr    r12
    bctrl
.L_801A4128:
    li       r0, 0x0
    stw      r0, 0x4(r30)
.L_801A4130:
    lwz      r0, 0x4(r30)
    lwz      r31, 0x1c(r1)
    cntlzw   r0, r0
    lwz      r30, 0x18(r1)
    extrwi   r3, r0, 8, 19
    lwz      r0, 0x24(r1)
    lwz      r29, 0x14(r1)
    lwz      r28, 0x10(r1)
    mtlr     r0
    addi     r1, r1, 0x20
    blr
*/

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
/*
    stwu     r1, -0x20(r1)
    mflr     r0
    stw      r0, 0x24(r1)
    stmw     r26, 0x8(r1)
    mr       r27, r5
    mr       r28, r4
    mr       r31, r3
    mr       r4, r27
    lwz      r29, 0x0(r3)
    mr       r3, r29
    bl       HSD_ArchiveGetExtern
    mr       r4, r3
    mr       r3, r29
    bl       HSD_ArchiveGetPublicAddress
    cmplwi   r3, 0x0
    beq      .L_801A3AD8
    lwz      r3, 0x0(r3)
    slwi     r0, r28, 2
    lwzx     r3, r3, r0
    lwz      r0, 0x0(r3)
    b        .L_801A3ADC
.L_801A3AD8:
    li       r0, 0x0
.L_801A3ADC:
    stw      r0, 0x18(r31)
    mr       r4, r27
    lwz      r29, 0x0(r31)
    mr       r3, r29
    bl       HSD_ArchiveGetExtern
    mr       r4, r3
    mr       r3, r29
    bl       HSD_ArchiveGetPublicAddress
    cmplwi   r3, 0x0
    beq      .L_801A3B18
    lwz      r3, 0x0(r3)
    slwi     r0, r28, 2
    lwzx     r3, r3, r0
    lwz      r0, 0x4(r3)
    b        .L_801A3B1C
.L_801A3B18:
    li       r0, 0x0
.L_801A3B1C:
    stw      r0, 0x1c(r31)
    mr       r4, r27
    lwz      r29, 0x0(r31)
    mr       r3, r29
    bl       HSD_ArchiveGetExtern
    mr       r4, r3
    mr       r3, r29
    bl       HSD_ArchiveGetPublicAddress
    cmplwi   r3, 0x0
    beq      .L_801A3B58
    lwz      r3, 0x0(r3)
    slwi     r0, r28, 2
    lwzx     r3, r3, r0
    lwz      r0, 0x8(r3)
    b        .L_801A3B5C
.L_801A3B58:
    li       r0, 0x0
.L_801A3B5C:
    stw      r0, 0x20(r31)
    mr       r4, r27
    lwz      r29, 0x0(r31)
    mr       r3, r29
    bl       HSD_ArchiveGetExtern
    mr       r4, r3
    mr       r3, r29
    bl       HSD_ArchiveGetPublicAddress
    cmplwi   r3, 0x0
    beq      .L_801A3B98
    lwz      r3, 0x0(r3)
    slwi     r0, r28, 2
    lwzx     r3, r3, r0
    lwz      r0, 0xc(r3)
    b        .L_801A3B9C
.L_801A3B98:
    li       r0, 0x0
.L_801A3B9C:
    stw      r0, 0x24(r31)
    lwz      r0, 0x1c(r31)
    cmplwi   r0, 0x0
    beq      .L_801A3BD8
    li       r0, 0x0
    stw      r0, 0x50(r31)
.L_801A3BB4:
    lwz      r3, 0x50(r31)
    lwz      r4, 0x1c(r31)
    slwi     r0, r3, 2
    lwzx     r0, r4, r0
    cmplwi   r0, 0x0
    beq      .L_801A3C50
    addi     r0, r3, 0x1
    stw      r0, 0x50(r31)
    b        .L_801A3BB4
.L_801A3BD8:
    lwz      r0, 0x20(r31)
    cmplwi   r0, 0x0
    beq      .L_801A3C10
    li       r0, 0x0
    stw      r0, 0x50(r31)
.L_801A3BEC:
    lwz      r3, 0x50(r31)
    lwz      r4, 0x20(r31)
    slwi     r0, r3, 2
    lwzx     r0, r4, r0
    cmplwi   r0, 0x0
    beq      .L_801A3C50
    addi     r0, r3, 0x1
    stw      r0, 0x50(r31)
    b        .L_801A3BEC
.L_801A3C10:
    lwz      r0, 0x24(r31)
    cmplwi   r0, 0x0
    beq      .L_801A3C48
    li       r0, 0x0
    stw      r0, 0x50(r31)
.L_801A3C24:
    lwz      r3, 0x50(r31)
    lwz      r4, 0x24(r31)
    slwi     r0, r3, 2
    lwzx     r0, r4, r0
    cmplwi   r0, 0x0
    beq      .L_801A3C50
    addi     r0, r3, 0x1
    stw      r0, 0x50(r31)
    b        .L_801A3C24
.L_801A3C48:
    li       r0, 0x0
    stw      r0, 0x50(r31)
.L_801A3C50:
    lwz      r28, 0x0(r31)
    mr       r4, r27
    mr       r3, r28
    bl       HSD_ArchiveGetExtern
    mr       r4, r3
    mr       r3, r28
    bl       HSD_ArchiveGetPublicAddress
    cmplwi   r3, 0x0
    beq      .L_801A3C7C
    lwz      r0, 0x4(r3)
    b        .L_801A3C80
.L_801A3C7C:
    li       r0, 0x0
.L_801A3C80:
    stw      r0, 0x28(r31)
    lwz      r3, 0x28(r31)
    cmplwi   r3, 0x0
    beq      .L_801A3CCC
    lwz      r0, 0x4(r3)
    cmplwi   r0, 0x0
    beq      .L_801A3CCC
    li       r0, 0x0
    stw      r0, 0x54(r31)
.L_801A3CA4:
    lwz      r4, 0x28(r31)
    lwz      r3, 0x54(r31)
    lwz      r4, 0x4(r4)
    slwi     r0, r3, 2
    lwzx     r0, r4, r0
    cmplwi   r0, 0x0
    beq      .L_801A3CD4
    addi     r0, r3, 0x1
    stw      r0, 0x54(r31)
    b        .L_801A3CA4
.L_801A3CCC:
    li       r0, 0x0
    stw      r0, 0x54(r31)
.L_801A3CD4:
    mr       r29, r31
    li       r28, 0x0
    li       r30, 0x0
.L_801A3CE0:
    lwz      r26, 0x0(r31)
    mr       r4, r27
    mr       r3, r26
    bl       HSD_ArchiveGetExtern
    mr       r4, r3
    mr       r3, r26
    bl       HSD_ArchiveGetPublicAddress
    cmplwi   r3, 0x0
    beq      .L_801A3D10
    lwz      r3, 0x8(r3)
    lwzx     r0, r3, r30
    b        .L_801A3D14
.L_801A3D10:
    li       r0, 0x0
.L_801A3D14:
    stw      r0, 0x2c(r29)
    lwz      r3, 0x2c(r29)
    cmplwi   r3, 0x0
    beq      .L_801A3D7C
    lwz      r0, 0x4(r3)
    cmplwi   r0, 0x0
    beq      .L_801A3D60
    li       r0, 0x0
    stw      r0, 0x58(r29)
.L_801A3D38:
    lwz      r4, 0x2c(r29)
    lwz      r3, 0x58(r29)
    lwz      r4, 0x4(r4)
    slwi     r0, r3, 2
    lwzx     r0, r4, r0
    cmplwi   r0, 0x0
    beq      .L_801A3D68
    addi     r0, r3, 0x1
    stw      r0, 0x58(r29)
    b        .L_801A3D38
.L_801A3D60:
    li       r0, 0x0
    stw      r0, 0x58(r29)
.L_801A3D68:
    addi     r28, r28, 0x1
    addi     r29, r29, 0x4
    cmpwi    r28, 0x8
    addi     r30, r30, 0x4
    blt      .L_801A3CE0
.L_801A3D7C:
    lwz      r26, 0x0(r31)
    mr       r4, r27
    mr       r3, r26
    bl       HSD_ArchiveGetExtern
    mr       r4, r3
    mr       r3, r26
    bl       HSD_ArchiveGetPublicAddress
    cmplwi   r3, 0x0
    beq      .L_801A3DA8
    lwz      r0, 0xc(r3)
    b        .L_801A3DAC
.L_801A3DA8:
    li       r0, 0x0
.L_801A3DAC:
    stw      r0, 0x4c(r31)
    lwz      r3, 0x4c(r31)
    cmplwi   r3, 0x0
    beq      .L_801A3DF8
    lwz      r0, 0x4(r3)
    cmplwi   r0, 0x0
    beq      .L_801A3DF8
    li       r0, 0x0
    stw      r0, 0x78(r31)
.L_801A3DD0:
    lwz      r4, 0x28(r31)
    lwz      r3, 0x78(r31)
    lwz      r4, 0x4(r4)
    slwi     r0, r3, 2
    lwzx     r0, r4, r0
    cmplwi   r0, 0x0
    beq      .L_801A3E00
    addi     r0, r3, 0x1
    stw      r0, 0x78(r31)
    b        .L_801A3DD0
.L_801A3DF8:
    li       r0, 0x0
    stw      r0, 0x78(r31)
.L_801A3E00:
    lmw      r26, 0x8(r1)
    lwz      r0, 0x24(r1)
    mtlr     r0
    addi     r1, r1, 0x20
    blr
*/

/**
 * @note Address: 0x801A39D0
 * @note Size: 0xB4
 */
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
extern "C" void JObjSetMObjFlags(HSD_JObj* jobj, f32** flags, s32)
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
/*
    stwu     r1, -0x10(r1)
    mflr     r0
    stw      r0, 0x14(r1)
    stw      r31, 0xc(r1)
    mr       r31, r4
    stw      r30, 0x8(r1)
    bl       HSD_JObjGetDObj
    mr       r30, r3
    b        .L_801A3848
.L_801A380C:
    cmplwi   r30, 0x0
    beq      .L_801A381C
    lwz      r3, 0x8(r30)
    b        .L_801A3820
.L_801A381C:
    li       r3, 0x0
.L_801A3820:
    cmplwi   r3, 0x0
    beq      .L_801A3830
    mr       r4, r31
    bl       HSD_MObjSetFlags
.L_801A3830:
    cmplwi   r30, 0x0
    beq      .L_801A3840
    lwz      r0, 0x4(r30)
    b        .L_801A3844
.L_801A3840:
    li       r0, 0x0
.L_801A3844:
    mr       r30, r0
.L_801A3848:
    cmplwi   r30, 0x0
    bne      .L_801A380C
    lwz      r0, 0x14(r1)
    lwz      r31, 0xc(r1)
    lwz      r30, 0x8(r1)
    mtlr     r0
    addi     r1, r1, 0x10
    blr
*/

/**
 * @note Address: 0x801A3768
 * @note Size: 0x80
 */
extern "C" void JObjClearMObjFlags(HSD_JObj* jobj, f32** flags, s32)
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
/*
    stwu     r1, -0x10(r1)
    mflr     r0
    stw      r0, 0x14(r1)
    stw      r31, 0xc(r1)
    mr       r31, r4
    stw      r30, 0x8(r1)
    bl       HSD_JObjGetDObj
    mr       r30, r3
    b        .L_801A37C8
.L_801A378C:
    cmplwi   r30, 0x0
    beq      .L_801A379C
    lwz      r3, 0x8(r30)
    b        .L_801A37A0
.L_801A379C:
    li       r3, 0x0
.L_801A37A0:
    cmplwi   r3, 0x0
    beq      .L_801A37B0
    mr       r4, r31
    bl       HSD_MObjClearFlags
.L_801A37B0:
    cmplwi   r30, 0x0
    beq      .L_801A37C0
    lwz      r0, 0x4(r30)
    b        .L_801A37C4
.L_801A37C0:
    li       r0, 0x0
.L_801A37C4:
    mr       r30, r0
.L_801A37C8:
    cmplwi   r30, 0x0
    bne      .L_801A378C
    lwz      r0, 0x14(r1)
    lwz      r31, 0xc(r1)
    lwz      r30, 0x8(r1)
    mtlr     r0
    addi     r1, r1, 0x10
    blr
*/

/**
 * @note Address: 0x801A36E8
 * @note Size: 0x80
 */
extern "C" void JObjSetPObjFlags(HSD_JObj* jobj, f32** flags, s32)
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
/*
    stwu     r1, -0x10(r1)
    mflr     r0
    stw      r0, 0x14(r1)
    stw      r31, 0xc(r1)
    mr       r31, r4
    stw      r30, 0x8(r1)
    bl       HSD_JObjGetDObj
    mr       r30, r3
    b        .L_801A3748
.L_801A370C:
    cmplwi   r30, 0x0
    beq      .L_801A371C
    lwz      r3, 0xc(r30)
    b        .L_801A3720
.L_801A371C:
    li       r3, 0x0
.L_801A3720:
    cmplwi   r3, 0x0
    beq      .L_801A3730
    mr       r4, r31
    bl       HSD_PObjSetFlags
.L_801A3730:
    cmplwi   r30, 0x0
    beq      .L_801A3740
    lwz      r0, 0x4(r30)
    b        .L_801A3744
.L_801A3740:
    li       r0, 0x0
.L_801A3744:
    mr       r30, r0
.L_801A3748:
    cmplwi   r30, 0x0
    bne      .L_801A370C
    lwz      r0, 0x14(r1)
    lwz      r31, 0xc(r1)
    lwz      r30, 0x8(r1)
    mtlr     r0
    addi     r1, r1, 0x10
    blr
*/

/**
 * @note Address: 0x801A3668
 * @note Size: 0x80
 */
extern "C" void JObjClearPObjFlags(HSD_JObj* jobj, f32** flags, s32)
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
/*
    stwu     r1, -0x10(r1)
    mflr     r0
    stw      r0, 0x14(r1)
    stw      r31, 0xc(r1)
    mr       r31, r4
    stw      r30, 0x8(r1)
    bl       HSD_JObjGetDObj
    mr       r30, r3
    b        .L_801A36C8
.L_801A368C:
    cmplwi   r30, 0x0
    beq      .L_801A369C
    lwz      r3, 0xc(r30)
    b        .L_801A36A0
.L_801A369C:
    li       r3, 0x0
.L_801A36A0:
    cmplwi   r3, 0x0
    beq      .L_801A36B0
    mr       r4, r31
    bl       HSD_PObjClearFlags
.L_801A36B0:
    cmplwi   r30, 0x0
    beq      .L_801A36C0
    lwz      r0, 0x4(r30)
    b        .L_801A36C4
.L_801A36C0:
    li       r0, 0x0
.L_801A36C4:
    mr       r30, r0
.L_801A36C8:
    cmplwi   r30, 0x0
    bne      .L_801A368C
    lwz      r0, 0x14(r1)
    lwz      r31, 0xc(r1)
    lwz      r30, 0x8(r1)
    mtlr     r0
    addi     r1, r1, 0x10
    blr
*/

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
/*
    stwu     r1, -0x10(r1)
    li       r0, 0x0
    lfs      f3, 0x0(r3)
    lfs      f2, 0x10(r3)
    fcmpu    cr0, f3, f2
    beq      .L_801A3494
    lfs      f1, lbl_8065F71C@sda21(r0)
    lfs      f0, 0x20(r3)
    fcmpu    cr0, f1, f0
    bne      .L_801A34D4
.L_801A3494:
    lfs      f1, 0x4(r3)
    lfs      f0, 0x14(r3)
    fcmpu    cr0, f1, f0
    beq      .L_801A34B4
    lfs      f1, lbl_8065F71C@sda21(r0)
    lfs      f0, 0x24(r3)
    fcmpu    cr0, f1, f0
    bne      .L_801A34D4
.L_801A34B4:
    lfs      f1, 0x8(r3)
    lfs      f0, 0x18(r3)
    fcmpu    cr0, f1, f0
    beq      .L_801A34D8
    lfs      f1, lbl_8065F71C@sda21(r0)
    lfs      f0, 0x28(r3)
    fcmpu    cr0, f1, f0
    beq      .L_801A34D8
.L_801A34D4:
    li       r0, 0x1
.L_801A34D8:
    fcmpo    cr0, f3, f2
    clrlwi   r5, r0, 24
    bge      .L_801A3520
    lis      r0, 0x4330
    stw      r4, 0xc(r1)
    lfd      f2, lbl_8065F730@sda21(r0)
    stw      r0, 0x8(r1)
    lfs      f3, 0x20(r3)
    lfd      f1, 0x8(r1)
    lfs      f0, 0x0(r3)
    fsubs    f1, f1, f2
    fmadds   f0, f3, f1, f0
    stfs     f0, 0x0(r3)
    lfs      f1, 0x10(r3)
    fcmpo    cr0, f0, f1
    ble      .L_801A355C
    stfs     f1, 0x0(r3)
    b        .L_801A355C
.L_801A3520:
    ble      .L_801A355C
    lis      r0, 0x4330
    stw      r4, 0xc(r1)
    lfd      f2, lbl_8065F730@sda21(r0)
    stw      r0, 0x8(r1)
    lfs      f3, 0x20(r3)
    lfd      f1, 0x8(r1)
    lfs      f0, 0x0(r3)
    fsubs    f1, f1, f2
    fnmsubs  f0, f3, f1, f0
    stfs     f0, 0x0(r3)
    lfs      f1, 0x10(r3)
    fcmpo    cr0, f0, f1
    bge      .L_801A355C
    stfs     f1, 0x0(r3)
.L_801A355C:
    lfs      f3, 0x4(r3)
    lfs      f0, 0x14(r3)
    fcmpo    cr0, f3, f0
    bge      .L_801A35A4
    lis      r0, 0x4330
    stw      r4, 0xc(r1)
    lfd      f1, lbl_8065F730@sda21(r0)
    stw      r0, 0x8(r1)
    lfs      f2, 0x24(r3)
    lfd      f0, 0x8(r1)
    fsubs    f0, f0, f1
    fmadds   f0, f2, f0, f3
    stfs     f0, 0x4(r3)
    lfs      f1, 0x14(r3)
    fcmpo    cr0, f0, f1
    ble      .L_801A35DC
    stfs     f1, 0x4(r3)
    b        .L_801A35DC
.L_801A35A4:
    ble      .L_801A35DC
    lis      r0, 0x4330
    stw      r4, 0xc(r1)
    lfd      f1, lbl_8065F730@sda21(r0)
    stw      r0, 0x8(r1)
    lfs      f2, 0x24(r3)
    lfd      f0, 0x8(r1)
    fsubs    f0, f0, f1
    fnmsubs  f0, f2, f0, f3
    stfs     f0, 0x4(r3)
    lfs      f1, 0x14(r3)
    fcmpo    cr0, f0, f1
    bge      .L_801A35DC
    stfs     f1, 0x4(r3)
.L_801A35DC:
    lfs      f3, 0x8(r3)
    lfs      f0, 0x18(r3)
    fcmpo    cr0, f3, f0
    bge      .L_801A3624
    lis      r0, 0x4330
    stw      r4, 0xc(r1)
    lfd      f1, lbl_8065F730@sda21(r0)
    stw      r0, 0x8(r1)
    lfs      f2, 0x28(r3)
    lfd      f0, 0x8(r1)
    fsubs    f0, f0, f1
    fmadds   f0, f2, f0, f3
    stfs     f0, 0x8(r3)
    lfs      f1, 0x18(r3)
    fcmpo    cr0, f0, f1
    ble      .L_801A365C
    stfs     f1, 0x8(r3)
    b        .L_801A365C
.L_801A3624:
    ble      .L_801A365C
    lis      r0, 0x4330
    stw      r4, 0xc(r1)
    lfd      f1, lbl_8065F730@sda21(r0)
    stw      r0, 0x8(r1)
    lfs      f2, 0x28(r3)
    lfd      f0, 0x8(r1)
    fsubs    f0, f0, f1
    fnmsubs  f0, f2, f0, f3
    stfs     f0, 0x8(r3)
    lfs      f1, 0x18(r3)
    fcmpo    cr0, f0, f1
    bge      .L_801A365C
    stfs     f1, 0x8(r3)
.L_801A365C:
    mr       r3, r5
    addi     r1, r1, 0x10
    blr
*/

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
// the parent's tev, then the second color added and the first added on top
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
void XCObjSetColors(HSD_JObj* jobj, u32* colors, s32)
{
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
	mColors[0]               = 0;
	mColors[1]               = 0xFFFFFF00;
}

/**
 * @note Address: 0x801A306C
 * @note Size: 0x88
 */
BOOL CXObj::ReadAsync(CObjArchive* archive, const char* path, u32 a, u32 flags, u32 b, u32 c)
{
	if (mArchive.ReadAsync(archive, path, flags, b, c)) {
		mLoadState++;
		mModel     = a;
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
/*
    stwu     r1, -0x20(r1)
    mflr     r0
    stw      r0, 0x24(r1)
    stw      r31, 0x1c(r1)
    stw      r30, 0x18(r1)
    stw      r29, 0x14(r1)
    mr       r29, r3
    stw      r4, 0x0(r3)
    lwz      r31, 0x0(r3)
    cmplwi   r31, 0x0
    bne      .L_801A2F74
    li       r3, lbl_8065F6F8@sda21
    li       r4, 0x3e4
    li       r5, lbl_8065F700@sda21
    bl       __assert
.L_801A2F74:
    addic.   r30, r29, 0x4
    bne      .L_801A2F90
    lis      r4, sObjectTranslationCondition@ha
    li       r3, lbl_8065F6F8@sda21
    addi     r5, r4, sObjectTranslationCondition@l
    li       r4, 0x3e5
    bl       __assert
.L_801A2F90:
    lfs      f0, 0x38(r31)
    stfs     f0, 0x0(r30)
    lfs      f0, 0x3c(r31)
    stfs     f0, 0x4(r30)
    lfs      f0, 0x40(r31)
    stfs     f0, 0x8(r30)
    lwz      r31, 0x0(r29)
    cmplwi   r31, 0x0
    bne      .L_801A2FC4
    li       r3, lbl_8065F6F8@sda21
    li       r4, 0x2dd
    li       r5, lbl_8065F700@sda21
    bl       __assert
.L_801A2FC4:
    addic.   r30, r29, 0x14
    bne      .L_801A2FDC
    li       r3, lbl_8065F6F8@sda21
    li       r4, 0x2de
    li       r5, lbl_8065F710@sda21
    bl       __assert
.L_801A2FDC:
    lfs      f0, 0x1c(r31)
    stfs     f0, 0x0(r30)
    lfs      f0, 0x20(r31)
    stfs     f0, 0x4(r30)
    lfs      f0, 0x24(r31)
    stfs     f0, 0x8(r30)
    lfs      f0, 0x28(r31)
    stfs     f0, 0xc(r30)
    lwz      r31, 0x0(r29)
    cmplwi   r31, 0x0
    bne      .L_801A3018
    li       r3, lbl_8065F6F8@sda21
    li       r4, 0x351
    li       r5, lbl_8065F700@sda21
    bl       __assert
.L_801A3018:
    addic.   r30, r29, 0x24
    bne      .L_801A3030
    li       r3, lbl_8065F6F8@sda21
    li       r4, 0x352
    li       r5, lbl_8065F708@sda21
    bl       __assert
.L_801A3030:
    lfs      f0, 0x2c(r31)
    mr       r3, r29
    stfs     f0, 0x0(r30)
    lfs      f0, 0x30(r31)
    stfs     f0, 0x4(r30)
    lfs      f0, 0x34(r31)
    stfs     f0, 0x8(r30)
    bl       SyncMoveFromJoint
    lwz      r0, 0x24(r1)
    lwz      r31, 0x1c(r1)
    lwz      r30, 0x18(r1)
    lwz      r29, 0x14(r1)
    mtlr     r0
    addi     r1, r1, 0x20
    blr
*/

/**
 * @note Address: 0x801A2D68
 * @note Size: 0x1D0
 */
BOOL CXObj::ReadCheck()
{
	if (mLoadState == 1 && mArchive.ReadCheck()) {
		mArchive.SetupScene(mModel, 0);
		HSD_MObjInfo* mobjClass = HSD_MObjGetDefaultClass();
		HSD_MObjSetDefaultClass(&xCObj);
		Attach(mArchive.LoadJObj());
		HSD_MObjSetDefaultClass(mobjClass);
		if (mJObj != NULL) {
			HSD_JObjWalkTree(mJObj, (HSD_JObjWalkTreeCallback)XCObjSetColors, (f32**)mColors);
		}
		HSD_JObjGetScale(mJObj, &mMoveScale);
		ClearQueue();
		mLoadState = 0;
	}
	return mLoadState == 0;
}
// CXObj::ReadCheck()
/*
    stwu     r1, -0x20(r1)
    mflr     r0
    stw      r0, 0x24(r1)
    stw      r31, 0x1c(r1)
    mr       r31, r3
    stw      r30, 0x18(r1)
    stw      r29, 0x14(r1)
    lwz      r0, 0x3c(r3)
    cmpwi    r0, 0x1
    bne      .L_801A2F10
    addi     r3, r31, 0x48
    lwz      r12, 0xc4(r31)
    lwz      r12, 0x14(r12)
    mtctr    r12
    bctrl
    cmpwi    r3, 0x0
    beq      .L_801A2F10
    lwz      r4, 0x40(r31)
    addi     r3, r31, 0x48
    li       r5, 0x0
    bl       SetupScene__9CJArchiveFll
    bl       HSD_MObjGetDefaultClass
    lis      r4, xCObj@ha
    mr       r29, r3
    addi     r0, r4, xCObj@l
    mr       r3, r0
    bl       HSD_MObjSetDefaultClass
    lwz      r30, 0x48(r31)
    cmplwi   r30, 0x0
    beq      .L_801A2E0C
    lwz      r3, 0x60(r31)
    cmplwi   r3, 0x0
    beq      .L_801A2E00
    bl       HSD_JObjLoadJoint
    lwz      r4, 0x130(r30)
    addi     r0, r4, 0x1
    stw      r0, 0x130(r30)
    b        .L_801A2E04
.L_801A2E00:
    li       r3, 0x0
.L_801A2E04:
    mr       r4, r3
    b        .L_801A2E10
.L_801A2E0C:
    li       r4, 0x0
.L_801A2E10:
    mr       r3, r31
    bl       Attach__5CXObjFP9_HSD_JObj
    mr       r3, r29
    bl       HSD_MObjSetDefaultClass
    lwz      r3, 0x0(r31)
    cmplwi   r3, 0x0
    beq      .L_801A2E3C
    lis      r4, XCObjSetColors__FP9_HSD_JObjPUll@ha
    addi     r5, r31, 0x164
    addi     r4, r4, XCObjSetColors__FP9_HSD_JObjPUll@l
    bl       HSD_JObjWalkTree
.L_801A2E3C:
    lwz      r30, 0x0(r31)
    cmplwi   r30, 0x0
    bne      .L_801A2E58
    li       r3, lbl_8065F6F8@sda21
    li       r4, 0x351
    li       r5, lbl_8065F700@sda21
    bl       __assert
.L_801A2E58:
    addic.   r0, r31, 0x128
    bne      .L_801A2E70
    li       r3, lbl_8065F6F8@sda21
    li       r4, 0x352
    li       r5, lbl_8065F708@sda21
    bl       __assert
.L_801A2E70:
    lfs      f1, 0x2c(r30)
    li       r0, 0x0
    lfs      f0, lbl_8065F71C@sda21(r0)
    stfs     f1, 0x128(r31)
    lfs      f1, 0x30(r30)
    stfs     f1, 0x12c(r31)
    lfs      f1, 0x34(r30)
    stfs     f1, 0x130(r31)
    stw      r0, 0x16c(r31)
    stw      r0, 0x170(r31)
    stw      r0, 0x174(r31)
    stfs     f0, 0x178(r31)
    stw      r0, 0x17c(r31)
    stw      r0, 0x180(r31)
    stfs     f0, 0x184(r31)
    stw      r0, 0x188(r31)
    stw      r0, 0x18c(r31)
    stfs     f0, 0x190(r31)
    stw      r0, 0x194(r31)
    stw      r0, 0x198(r31)
    stfs     f0, 0x19c(r31)
    stw      r0, 0x1a0(r31)
    stw      r0, 0x1a4(r31)
    stfs     f0, 0x1a8(r31)
    stw      r0, 0x1ac(r31)
    stw      r0, 0x1b0(r31)
    stfs     f0, 0x1b4(r31)
    stw      r0, 0x1b8(r31)
    stw      r0, 0x1bc(r31)
    stfs     f0, 0x1c0(r31)
    stw      r0, 0x1c4(r31)
    stw      r0, 0x1c8(r31)
    stfs     f0, 0x1cc(r31)
    stw      r0, 0x1d0(r31)
    stw      r0, 0x1d4(r31)
    stfs     f0, 0x1d8(r31)
    stw      r0, 0x1dc(r31)
    stw      r0, 0x1e0(r31)
    stfs     f0, 0x1e4(r31)
    stw      r0, 0x3c(r31)
.L_801A2F10:
    lwz      r0, 0x3c(r31)
    lwz      r31, 0x1c(r1)
    cntlzw   r0, r0
    lwz      r30, 0x18(r1)
    extrwi   r3, r0, 8, 19
    lwz      r0, 0x24(r1)
    lwz      r29, 0x14(r1)
    mtlr     r0
    addi     r1, r1, 0x20
    blr
*/

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
/*
    stwu     r1, -0x60(r1)
    mflr     r0
    stw      r0, 0x64(r1)
    stfd     f31, 0x50(r1)
    psq_st   f31, 0x58(r1), 0, qr0
    stw      r31, 0x4c(r1)
    lfs      f0, 0x0(r4)
    mr       r31, r3
    lfs      f2, 0x4(r4)
    fmr      f31, f1
    stfs     f0, 0xd8(r3)
    addi     r3, r1, 0x18
    lfs      f0, 0x8(r4)
    addi     r5, r1, 0x28
    stfs     f2, 0xdc(r31)
    lwz      r7, 0x0(r4)
    stfs     f0, 0xe0(r31)
    lwz      r6, 0x4(r4)
    lwz      r0, 0x8(r4)
    addi     r4, r31, 0xc8
    stw      r7, 0x28(r1)
    stw      r6, 0x2c(r1)
    stw      r0, 0x30(r1)
    bl       __mi__4CVecCF3Vec
    lwz      r6, 0x18(r1)
    lis      r3, __vt__4CVec@ha
    addi     r0, r3, __vt__4CVec@l
    lwz      r5, 0x1c(r1)
    lwz      r4, 0x20(r1)
    addi     r3, r1, 0x34
    stw      r6, 0x34(r1)
    stw      r5, 0x38(r1)
    stw      r4, 0x3c(r1)
    stw      r0, 0x40(r1)
    stw      r0, 0x24(r1)
    bl       PSVECMag
    lfs      f0, lbl_8065F71C@sda21(r0)
    fcmpo    cr0, f1, f0
    ble      .L_801A227C
    lfs      f1, 0x34(r1)
    fcmpo    cr0, f1, f0
    bge      .L_801A21C8
    fneg     f1, f1
.L_801A21C8:
    lfs      f2, 0x38(r1)
    lfs      f0, lbl_8065F71C@sda21(r0)
    stfs     f1, 0x34(r1)
    fcmpo    cr0, f2, f0
    bge      .L_801A21E0
    fneg     f2, f2
.L_801A21E0:
    lfs      f1, 0x3c(r1)
    lfs      f0, lbl_8065F71C@sda21(r0)
    stfs     f2, 0x38(r1)
    fcmpo    cr0, f1, f0
    bge      .L_801A21F8
    fneg     f1, f1
.L_801A21F8:
    addi     r3, r1, 0x34
    stfs     f1, 0x3c(r1)
    mr       r4, r3
    bl       PSVECNormalize
    fmr      f1, f31
    addi     r3, r1, 0x8
    addi     r4, r1, 0x34
    bl       __ml__4CVecCFf
    lfs      f3, 0x10(r1)
    lis      r3, __vt__4CVec@ha
    lfs      f2, 0xc(r1)
    addi     r0, r3, __vt__4CVec@l
    lfs      f1, 0x8(r1)
    lfs      f0, lbl_8065F71C@sda21(r0)
    stfs     f1, 0xe8(r31)
    stfs     f2, 0xec(r31)
    stfs     f3, 0xf0(r31)
    stw      r0, 0x14(r1)
    lfs      f1, 0xe8(r31)
    fcmpo    cr0, f1, f0
    ble      .L_801A225C
    lfs      f0, lbl_8065F724@sda21(r0)
    fcmpo    cr0, f1, f0
    bge      .L_801A225C
    stfs     f0, 0xe8(r31)
.L_801A225C:
    lfs      f1, 0xf0(r31)
    lfs      f0, lbl_8065F71C@sda21(r0)
    fcmpo    cr0, f1, f0
    ble      .L_801A227C
    lfs      f0, lbl_8065F724@sda21(r0)
    fcmpo    cr0, f1, f0
    bge      .L_801A227C
    stfs     f0, 0xf0(r31)
.L_801A227C:
    lis      r3, __vt__4CVec@ha
    addi     r0, r3, __vt__4CVec@l
    stw      r0, 0x40(r1)
    psq_l    f31, 0x58(r1), 0, qr0
    lwz      r0, 0x64(r1)
    lfd      f31, 0x50(r1)
    lwz      r31, 0x4c(r1)
    mtlr     r0
    addi     r1, r1, 0x60
    blr
*/

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
/*
    stwu     r1, -0x30(r1)
    mflr     r0
    stw      r0, 0x34(r1)
    stfd     f31, 0x20(r1)
    psq_st   f31, 0x28(r1), 0, qr0
    stw      r31, 0x1c(r1)
    stw      r30, 0x18(r1)
    stw      r29, 0x14(r1)
    stw      r28, 0x10(r1)
    mr       r31, r4
    lis      r0, 0x4330
    stw      r31, 0xc(r1)
    mr       r28, r3
    lfd      f1, lbl_8065F730@sda21(r0)
    stw      r0, 0x8(r1)
    lfs      f2, 0x15c(r3)
    lfd      f0, 0x8(r1)
    lfs      f3, 0x160(r3)
    fsubs    f0, f0, f1
    fmuls    f0, f2, f0
    fcmpu    cr0, f3, f0
    beq      .L_801A179C
    stfs     f0, 0x160(r28)
    lfs      f31, 0x160(r28)
    lwz      r3, 0x0(r28)
    bl       HSD_JObjGetFlags
    rlwinm.  r0, r3, 0, 19, 19
    beq      .L_801A1778
    lwz      r3, 0x0(r28)
    lwz      r3, 0x10(r3)
    b        .L_801A177C
.L_801A1778:
    lwz      r3, 0x0(r28)
.L_801A177C:
    fmr      f1, f31
    lis      r4, HSD_AObjSetRate@ha
    addi     r6, r4, HSD_AObjSetRate@l
    li       r5, 0x5a0
    li       r4, 0x6
    li       r7, 0x1
    crset    cr1eq
    bl       HSD_ForeachAnim
.L_801A179C:
    lwz      r0, 0x16c(r28)
    cmpwi    r0, 0x0
    beq      .L_801A1850
    li       r30, 0x0
    mr       r29, r28
.L_801A17B0:
    lwz      r3, 0x170(r29)
    cmplwi   r3, 0x0
    beq      .L_801A1850
    lwz      r0, 0x174(r29)
    cmplwi   r0, 0x200
    bne      .L_801A17FC
    lwz      r3, 0x7c(r3)
    cmplwi   r3, 0x0
    beq      .L_801A1840
    lis      r0, 0x4330
    stw      r31, 0xc(r1)
    lfd      f1, lbl_8065F730@sda21(r0)
    stw      r0, 0x8(r1)
    lfs      f2, 0x178(r29)
    lfd      f0, 0x8(r1)
    fsubs    f0, f0, f1
    fmuls    f1, f2, f0
    bl       HSD_AObjSetRate
    b        .L_801A1840
.L_801A17FC:
    cmplwi   r0, 0x400
    bne      .L_801A1840
    lis      r0, 0x4330
    lis      r4, HSD_AObjSetRate@ha
    stw      r31, 0xc(r1)
    addi     r6, r4, HSD_AObjSetRate@l
    lfd      f1, lbl_8065F730@sda21(r0)
    li       r4, 0x6
    stw      r0, 0x8(r1)
    li       r5, 0x5a0
    lfs      f2, 0x178(r29)
    li       r7, 0x1
    lfd      f0, 0x8(r1)
    fsubs    f0, f0, f1
    fmuls    f1, f2, f0
    crset    cr1eq
    bl       HSD_ForeachAnim
.L_801A1840:
    addi     r30, r30, 0x1
    addi     r29, r29, 0xc
    cmpwi    r30, 0xa
    blt      .L_801A17B0
.L_801A1850:
    lwz      r3, 0x0(r28)
    bl       HSD_JObjAnimAll
    lwz      r0, 0x16c(r28)
    cmpwi    r0, 0x0
    beq      .L_801A1DA8
    li       r29, 0x0
    mr       r31, r28
.L_801A186C:
    lwz      r30, 0x170(r31)
    cmplwi   r30, 0x0
    beq      .L_801A1DA0
    lwz      r0, 0x174(r31)
    cmplwi   r0, 0x0
    beq      .L_801A1D90
    cmpwi    r0, 0x10
    beq      .L_801A1AD0
    bge      .L_801A18C0
    cmpwi    r0, 0x4
    beq      .L_801A19E0
    bge      .L_801A18B4
    cmpwi    r0, 0x2
    beq      .L_801A1968
    bge      .L_801A1D84
    cmpwi    r0, 0x1
    bge      .L_801A18F0
    b        .L_801A1D84
.L_801A18B4:
    cmpwi    r0, 0x8
    beq      .L_801A1A58
    b        .L_801A1D84
.L_801A18C0:
    cmpwi    r0, 0x80
    beq      .L_801A1C58
    bge      .L_801A18E4
    cmpwi    r0, 0x40
    beq      .L_801A1BC0
    bge      .L_801A1D84
    cmpwi    r0, 0x20
    beq      .L_801A1B48
    b        .L_801A1D84
.L_801A18E4:
    cmpwi    r0, 0x100
    beq      .L_801A1CF0
    b        .L_801A1D84
.L_801A18F0:
    cmplwi   r30, 0x0
    lfs      f31, 0x178(r31)
    bne      .L_801A190C
    li       r3, lbl_8065F6F8@sda21
    li       r4, 0x3b8
    li       r5, lbl_8065F700@sda21
    bl       __assert
.L_801A190C:
    stfs     f31, 0x38(r30)
    lwz      r0, 0x14(r30)
    rlwinm.  r0, r0, 0, 6, 6
    bne      .L_801A1D84
    cmplwi   r30, 0x0
    beq      .L_801A1D84
    bne      .L_801A1938
    li       r3, lbl_8065F6F8@sda21
    li       r4, 0x25d
    li       r5, lbl_8065F700@sda21
    bl       __assert
.L_801A1938:
    lwz      r4, 0x14(r30)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_801A1954
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_801A1954
    li       r3, 0x1
.L_801A1954:
    clrlwi.  r0, r3, 24
    bne      .L_801A1D84
    mr       r3, r30
    bl       HSD_JObjSetMtxDirtySub
    b        .L_801A1D84
.L_801A1968:
    cmplwi   r30, 0x0
    lfs      f31, 0x178(r31)
    bne      .L_801A1984
    li       r3, lbl_8065F6F8@sda21
    li       r4, 0x3c6
    li       r5, lbl_8065F700@sda21
    bl       __assert
.L_801A1984:
    stfs     f31, 0x3c(r30)
    lwz      r0, 0x14(r30)
    rlwinm.  r0, r0, 0, 6, 6
    bne      .L_801A1D84
    cmplwi   r30, 0x0
    beq      .L_801A1D84
    bne      .L_801A19B0
    li       r3, lbl_8065F6F8@sda21
    li       r4, 0x25d
    li       r5, lbl_8065F700@sda21
    bl       __assert
.L_801A19B0:
    lwz      r4, 0x14(r30)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_801A19CC
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_801A19CC
    li       r3, 0x1
.L_801A19CC:
    clrlwi.  r0, r3, 24
    bne      .L_801A1D84
    mr       r3, r30
    bl       HSD_JObjSetMtxDirtySub
    b        .L_801A1D84
.L_801A19E0:
    cmplwi   r30, 0x0
    lfs      f31, 0x178(r31)
    bne      .L_801A19FC
    li       r3, lbl_8065F6F8@sda21
    li       r4, 0x3d4
    li       r5, lbl_8065F700@sda21
    bl       __assert
.L_801A19FC:
    stfs     f31, 0x40(r30)
    lwz      r0, 0x14(r30)
    rlwinm.  r0, r0, 0, 6, 6
    bne      .L_801A1D84
    cmplwi   r30, 0x0
    beq      .L_801A1D84
    bne      .L_801A1A28
    li       r3, lbl_8065F6F8@sda21
    li       r4, 0x25d
    li       r5, lbl_8065F700@sda21
    bl       __assert
.L_801A1A28:
    lwz      r4, 0x14(r30)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_801A1A44
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_801A1A44
    li       r3, 0x1
.L_801A1A44:
    clrlwi.  r0, r3, 24
    bne      .L_801A1D84
    mr       r3, r30
    bl       HSD_JObjSetMtxDirtySub
    b        .L_801A1D84
.L_801A1A58:
    cmplwi   r30, 0x0
    lfs      f31, 0x178(r31)
    bne      .L_801A1A74
    li       r3, lbl_8065F6F8@sda21
    li       r4, 0x325
    li       r5, lbl_8065F700@sda21
    bl       __assert
.L_801A1A74:
    stfs     f31, 0x2c(r30)
    lwz      r0, 0x14(r30)
    rlwinm.  r0, r0, 0, 6, 6
    bne      .L_801A1D84
    cmplwi   r30, 0x0
    beq      .L_801A1D84
    bne      .L_801A1AA0
    li       r3, lbl_8065F6F8@sda21
    li       r4, 0x25d
    li       r5, lbl_8065F700@sda21
    bl       __assert
.L_801A1AA0:
    lwz      r4, 0x14(r30)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_801A1ABC
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_801A1ABC
    li       r3, 0x1
.L_801A1ABC:
    clrlwi.  r0, r3, 24
    bne      .L_801A1D84
    mr       r3, r30
    bl       HSD_JObjSetMtxDirtySub
    b        .L_801A1D84
.L_801A1AD0:
    cmplwi   r30, 0x0
    lfs      f31, 0x178(r31)
    bne      .L_801A1AEC
    li       r3, lbl_8065F6F8@sda21
    li       r4, 0x333
    li       r5, lbl_8065F700@sda21
    bl       __assert
.L_801A1AEC:
    stfs     f31, 0x30(r30)
    lwz      r0, 0x14(r30)
    rlwinm.  r0, r0, 0, 6, 6
    bne      .L_801A1D84
    cmplwi   r30, 0x0
    beq      .L_801A1D84
    bne      .L_801A1B18
    li       r3, lbl_8065F6F8@sda21
    li       r4, 0x25d
    li       r5, lbl_8065F700@sda21
    bl       __assert
.L_801A1B18:
    lwz      r4, 0x14(r30)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_801A1B34
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_801A1B34
    li       r3, 0x1
.L_801A1B34:
    clrlwi.  r0, r3, 24
    bne      .L_801A1D84
    mr       r3, r30
    bl       HSD_JObjSetMtxDirtySub
    b        .L_801A1D84
.L_801A1B48:
    cmplwi   r30, 0x0
    lfs      f31, 0x178(r31)
    bne      .L_801A1B64
    li       r3, lbl_8065F6F8@sda21
    li       r4, 0x341
    li       r5, lbl_8065F700@sda21
    bl       __assert
.L_801A1B64:
    stfs     f31, 0x34(r30)
    lwz      r0, 0x14(r30)
    rlwinm.  r0, r0, 0, 6, 6
    bne      .L_801A1D84
    cmplwi   r30, 0x0
    beq      .L_801A1D84
    bne      .L_801A1B90
    li       r3, lbl_8065F6F8@sda21
    li       r4, 0x25d
    li       r5, lbl_8065F700@sda21
    bl       __assert
.L_801A1B90:
    lwz      r4, 0x14(r30)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_801A1BAC
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_801A1BAC
    li       r3, 0x1
.L_801A1BAC:
    clrlwi.  r0, r3, 24
    bne      .L_801A1D84
    mr       r3, r30
    bl       HSD_JObjSetMtxDirtySub
    b        .L_801A1D84
.L_801A1BC0:
    cmplwi   r30, 0x0
    lfs      f31, 0x178(r31)
    bne      .L_801A1BDC
    li       r3, lbl_8065F6F8@sda21
    li       r4, 0x2a4
    li       r5, lbl_8065F700@sda21
    bl       __assert
.L_801A1BDC:
    lwz      r0, 0x14(r30)
    rlwinm.  r0, r0, 0, 14, 14
    beq      .L_801A1BFC
    lis      r4, sObjectNoQuaternionCondition@ha
    li       r3, lbl_8065F6F8@sda21
    addi     r5, r4, sObjectNoQuaternionCondition@l
    li       r4, 0x2a5
    bl       __assert
.L_801A1BFC:
    stfs     f31, 0x1c(r30)
    lwz      r0, 0x14(r30)
    rlwinm.  r0, r0, 0, 6, 6
    bne      .L_801A1D84
    cmplwi   r30, 0x0
    beq      .L_801A1D84
    bne      .L_801A1C28
    li       r3, lbl_8065F6F8@sda21
    li       r4, 0x25d
    li       r5, lbl_8065F700@sda21
    bl       __assert
.L_801A1C28:
    lwz      r4, 0x14(r30)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_801A1C44
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_801A1C44
    li       r3, 0x1
.L_801A1C44:
    clrlwi.  r0, r3, 24
    bne      .L_801A1D84
    mr       r3, r30
    bl       HSD_JObjSetMtxDirtySub
    b        .L_801A1D84
.L_801A1C58:
    cmplwi   r30, 0x0
    lfs      f31, 0x178(r31)
    bne      .L_801A1C74
    li       r3, lbl_8065F6F8@sda21
    li       r4, 0x2b8
    li       r5, lbl_8065F700@sda21
    bl       __assert
.L_801A1C74:
    lwz      r0, 0x14(r30)
    rlwinm.  r0, r0, 0, 14, 14
    beq      .L_801A1C94
    lis      r4, sObjectNoQuaternionCondition@ha
    li       r3, lbl_8065F6F8@sda21
    addi     r5, r4, sObjectNoQuaternionCondition@l
    li       r4, 0x2b9
    bl       __assert
.L_801A1C94:
    stfs     f31, 0x20(r30)
    lwz      r0, 0x14(r30)
    rlwinm.  r0, r0, 0, 6, 6
    bne      .L_801A1D84
    cmplwi   r30, 0x0
    beq      .L_801A1D84
    bne      .L_801A1CC0
    li       r3, lbl_8065F6F8@sda21
    li       r4, 0x25d
    li       r5, lbl_8065F700@sda21
    bl       __assert
.L_801A1CC0:
    lwz      r4, 0x14(r30)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_801A1CDC
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_801A1CDC
    li       r3, 0x1
.L_801A1CDC:
    clrlwi.  r0, r3, 24
    bne      .L_801A1D84
    mr       r3, r30
    bl       HSD_JObjSetMtxDirtySub
    b        .L_801A1D84
.L_801A1CF0:
    cmplwi   r30, 0x0
    lfs      f31, 0x178(r31)
    bne      .L_801A1D0C
    li       r3, lbl_8065F6F8@sda21
    li       r4, 0x2cc
    li       r5, lbl_8065F700@sda21
    bl       __assert
.L_801A1D0C:
    lwz      r0, 0x14(r30)
    rlwinm.  r0, r0, 0, 14, 14
    beq      .L_801A1D2C
    lis      r4, sObjectNoQuaternionCondition@ha
    li       r3, lbl_8065F6F8@sda21
    addi     r5, r4, sObjectNoQuaternionCondition@l
    li       r4, 0x2cd
    bl       __assert
.L_801A1D2C:
    stfs     f31, 0x24(r30)
    lwz      r0, 0x14(r30)
    rlwinm.  r0, r0, 0, 6, 6
    bne      .L_801A1D84
    cmplwi   r30, 0x0
    beq      .L_801A1D84
    bne      .L_801A1D58
    li       r3, lbl_8065F6F8@sda21
    li       r4, 0x25d
    li       r5, lbl_8065F700@sda21
    bl       __assert
.L_801A1D58:
    lwz      r4, 0x14(r30)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_801A1D74
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_801A1D74
    li       r3, 0x1
.L_801A1D74:
    clrlwi.  r0, r3, 24
    bne      .L_801A1D84
    mr       r3, r30
    bl       HSD_JObjSetMtxDirtySub
.L_801A1D84:
    li       r0, 0x0
    stw      r0, 0x170(r31)
    stw      r0, 0x174(r31)
.L_801A1D90:
    addi     r29, r29, 0x1
    addi     r31, r31, 0xc
    cmpwi    r29, 0xa
    blt      .L_801A186C
.L_801A1DA0:
    li       r0, 0x0
    stw      r0, 0x16c(r28)
.L_801A1DA8:
    psq_l    f31, 0x28(r1), 0, qr0
    lwz      r0, 0x34(r1)
    lfd      f31, 0x20(r1)
    lwz      r31, 0x1c(r1)
    lwz      r30, 0x18(r1)
    lwz      r29, 0x14(r1)
    lwz      r28, 0x10(r1)
    mtlr     r0
    addi     r1, r1, 0x30
    blr
*/

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
BOOL CXObj::Load(CObjArchive* archive, const char* path, u32 a, u32 flags, u32 b, u32 c)
{
	switch (mLoadState) {
	case 0:
		ReadAsync(archive, path, a, flags, b, c);
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
			f32 t  = mBlend + (1.0f / mBlendRate) * speed;
			mBlend = t;
			if (t >= 1.0f || GetAObj(mJObj)->flags & AOBJ_NO_ANIM) {
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
