#include "game/xobjs.h"
#include "game/xobjread.h"
#include "game/cgame.h"
#include "game/xchr.h"
#include "Dolphin/OS/OSMessage.h"
#include "Dolphin/OS/OSThread.h"
#include "game/xobjchr.h"
#include "game/xobjactor.h"
#include "game/xobjfree.h"
#include "game/xobjhbox.h"
#include "game/xobjflags.h"
#include "game/xobjspiderboss.h"
#include "game/xobjspider.h"
#include "game/xobjspidersmall.h"
#include "game/xobjarmy.h"
#include "game/xobjleticker.h"
#include "game/xobjsound.h"
#include "game/xobjmap.h"

// the three reader threads, built by the file's static initializer
XObjReadAll gAllObjectsReader;
XObjReadOne gSingleObjectReader;
XObjReadAnm gObjectAnimationReader;

extern "C" void ReadAll__5XOBJSFv(XOBJS* list);

inline BOOL XObjRead::ReadStart(void* obj, void* info)
{
	for (s32 i = 0; i < 8; i++) {
		if (mReqs[i].mObj == NULL) {
			mReqs[i].mObj  = obj;
			mReqs[i].mInfo = info;
			OSSendMessage(&mQueue.mQueue, &mReqs[i], OS_MESSAGE_BLOCK);
			return TRUE;
		}
	}
	OSPanic(__FILE__, 356, "%s%s", "no more space for XObjRead thread!\n", __FUNCTION__);
	return FALSE;
}

/**
 * @note Address: 0x80132040
 * @note Size: 0xB4
 */
BOOL XObjReadAnm::QueueRead(void* object, void* info)
{
	((XObj*)object)->mStatus.mFlags |= 0x0080000000000000ULL;
	return ReadStart(object, info);
}

/**
 * @note Address: 0x801320F4
 * @note Size: 0x4C
 */
// reader thread entry
extern "C" void* ReadObjectAnimationsThread(void* arg)
{
	OSMessage msg;
	while (true) {
		OSReceiveMessage(&gObjectAnimationReader.mQueue.mQueue, &msg, OS_MESSAGE_BLOCK);
		XObjRead* self = &gObjectAnimationReader;
		self->ProcessRequest((XObjReadReq*)msg);
	}
}

/**
 * @note Address: 0x80132140
 * @note Size: 0x4
 */
void XObjRead::ProcessRequest(XObjReadReq* req)
{
}

/**
 * @note Address: 0x80132144
 * @note Size: 0xB8
 */
extern "C" BOOL QueueObjectAnimationRead(XObj* obj, char* path)
{
	obj->mStatus.mFlags |= 0x0080000000000000ULL;
	return gObjectAnimationReader.ReadStart(obj, path);
}

/**
 * @note Address: 0x801321FC
 * @note Size: 0xA8
 */
// Processes a single-object load request.
void XObjReadOne::ProcessRequest(XObjReadReq* req)
{
	XObj* obj = (XObj*)req->mObj;
	CoordInfo info(req->mInfo);
	while (!obj->LoadFromPlacement(info)) {
		OSYieldThread();
	}
	req->mObj = NULL;
	obj->mStatus.mFlags &= ~0x0040000000000000ULL;
}

/**
 * @note Address: 0x801322A4
 * @note Size: 0xB4
 */
BOOL XObjReadOne::QueueRead(void* object, void* info)
{
	((XObj*)object)->mStatus.mFlags |= 0x0040000000000000ULL;
	return ReadStart(object, info);
}

/**
 * @note Address: 0x80132358
 * @note Size: 0x4C
 */
extern "C" void* ReadSingleObjectThread(void* arg)
{
	OSMessage msg;
	while (true) {
		OSReceiveMessage(&gSingleObjectReader.mQueue.mQueue, &msg, OS_MESSAGE_BLOCK);
		XObjRead* self = &gSingleObjectReader;
		self->ProcessRequest((XObjReadReq*)msg);
	}
}

/**
 * @note Address: 0x801323A4
 * @note Size: 0x38
 */
void XObjReadAll::ProcessRequest(XObjReadReq* req)
{
	ReadAll__5XOBJSFv((XOBJS*)req->mInfo);
	req->mObj = NULL;
}

/**
 * @note Address: 0x801323DC
 * @note Size: 0x4C
 */
extern "C" void* ReadObjectListThread(void* arg)
{
	OSMessage msg;
	while (true) {
		OSReceiveMessage(&gAllObjectsReader.mQueue.mQueue, &msg, OS_MESSAGE_BLOCK);
		XObjRead* self = &gAllObjectsReader;
		self->ProcessRequest((XObjReadReq*)msg);
	}
}

/**
 * @note Address: 0x80132428
 * @note Size: 0x40
 */
void XObjRead::RunRequestLoop()
{
	OSMessage msg;
	while (true) {
		OSReceiveMessage(&mQueue.mQueue, &msg, OS_MESSAGE_BLOCK);
		ProcessRequest((XObjReadReq*)msg);
	}
}

/**
 * @note Address: 0x80132468
 * @note Size: 0x9C
 */
// NONMATCHING: regalloc — the inlined ReadStart loop lands its slot counter
// in a different arg slot, costing one extra mr.
BOOL XObjRead::QueueRead(void* object, void* info)
{
	return ReadStart(object, info);
}

// XObjRead::QueueRead
/*
    stwu     r1, -0x10(r1)
    mflr     r0
    mr       r8, r3
    li       r6, 0x0
    stw      r0, 0x14(r1)
    li       r0, 0x8
    mtctr    r0
.L_80132484:
    lwz      r0, 0x350(r3)
    cmplwi   r0, 0x0
    bne      .L_801324BC
    slwi     r7, r6, 3
    addi     r3, r8, 0x328
    add      r6, r8, r7
    stw      r4, 0x350(r6)
    addi     r4, r7, 0x350
    add      r4, r8, r4
    stw      r5, 0x354(r6)
    li       r5, 0x1
    bl       OSSendMessage
    li       r3, 0x1
    b        .L_801324F4
.L_801324BC:
    addi     r3, r3, 0x8
    addi     r6, r6, 0x1
    bdnz     .L_80132484
    lis      r3, sPathXobjsCpp@ha
    lis      r6, sObjectReaderFullMessage@ha
    lis      r5, sObjectReadStartName@ha
    li       r4, 0x164
    addi     r7, r5, sObjectReadStartName@l
    addi     r3, r3, sPathXobjsCpp@l
    addi     r6, r6, sObjectReaderFullMessage@l
    li       r5, lbl_8065E828@sda21
    crclr    cr1eq
    bl       OSPanic
    li       r3, 0x0
.L_801324F4:
    lwz      r0, 0x14(r1)
    mtlr     r0
    addi     r1, r1, 0x10
    blr
*/

/**
 * @note Address: 0x80132504
 * @note Size: 0xC8
 */
// XObjRead::vf0C
BOOL XObjRead::Create(OSThreadStartFunction func, void* param, u32 stackSize, OSPriority priority, u16 attr)
{
	mQueue.Initialize(8);
	for (s32 i = 0; i < 8; i++) {
		mReqs[i].mObj = NULL;
	}
	mStack     = (u8*)OSAllocFromHeap(__OSCurrHeap, stackSize);
	mStackSize = stackSize;
	if (OSCreateThread(&mThread, func, param, mStack + mStackSize, mStackSize, priority, attr)) {
		mStackSize = stackSize;
	}
	OSResumeThread(&mThread);
	return 0;
}

/**
 * @note Address: 0x801325CC
 * @note Size: 0x54
 */
void CMsgQueue::Initialize(s32 count)
{
	mMsgs = (OSMessage*)OSAllocFromHeap(__OSCurrHeap, count * sizeof(OSMessage));
	OSInitMessageQueue(&mQueue, mMsgs, count);
}

/**
 * @note Address: 0x80132620
 * @note Size: 0x3C
 */
// first free archive in a pool of 0x400
extern "C" CObjArchive* FindFreeObjectArchive(CObjArchive* archives)
{
	for (s32 i = 0; i < 0x400; i++) {
		if (archives[i].mFile.mBuffer == NULL) {
			return &archives[i];
		}
	}
	return NULL;
}

/**
 * @note Address: 0x8013265C
 * @note Size: 0x3C
 */
extern "C" s32 FindStageObjectIndex(XOBJS* list, CXObj* obj)
{
	for (s32 i = 0; i < list->mObjectCount; i++) {
		if (list->mObjects[i] == obj) {
			return i;
		}
	}
	return -1;
}

/**
 * @note Address: 0x80132698
 * @note Size: 0x230
 */
extern "C" XObj* SpawnObjectFromPlacement(XOBJS* list, CoordInfo* info, s32 unused, BOOL async)
{
	XObj* obj;
	AttrInfo attr(info);
	s16 pool = attr._00 ? attr._00->_14 : 0;

	s32 idx = list->mPools[pool].Find(1, 0);
	if (idx >= 0) {
		obj = list->mPools[pool].Get(idx);
		if (idx >= list->mPools[pool].mUsed) {
			list->mObjects[list->mObjectCount]             = obj;
			list->mObjects[list->mObjectCount]->mPoolIndex = list->mPools[pool].mUsed;
			list->mPools[pool].mUsed++;
			list->mObjectCount++;
		}
		if (async) {
			obj->mStatus.mFlags |= 0x0040000000000000ULL;
			gSingleObjectReader.ReadStart(obj, info->_00);
		} else {
			while (!obj->LoadFromPlacement(*info)) { }
		}
	} else {
		obj = NULL;
	}
	return obj;
}

// SpawnObjectFromPlacement
/*
    stwu     r1, -0x30(r1)
    mflr     r0
    lis      r5, __vt__8AttrInfo@ha
    stw      r0, 0x34(r1)
    addi     r0, r5, __vt__8AttrInfo@l
    stw      r31, 0x2c(r1)
    stw      r30, 0x28(r1)
    mr       r30, r6
    stw      r29, 0x24(r1)
    mr       r29, r4
    stw      r28, 0x20(r1)
    mr       r28, r3
    mr       r3, r29
    stw      r0, 0x10(r1)
    bl       GetObjData
    lwz      r0, 0x0(r29)
    cmplwi   r3, 0x0
    stw      r3, 0x8(r1)
    stw      r0, 0xc(r1)
    beq      .L_801326F0
    lha      r0, 0x14(r3)
    b        .L_801326F4
.L_801326F0:
    li       r0, 0x0
.L_801326F4:
    extsh    r0, r0
    li       r9, 0x0
    mulli    r11, r0, 0x14
    li       r7, 0x1
    mr       r6, r9
    mr       r5, r9
    add      r10, r28, r11
    mr       r4, r9
    lwz      r0, 0x8(r10)
    mtctr    r0
    cmpwi    r0, 0x0
    ble      .L_80132764
.L_80132724:
    lwz      r0, 0x10(r10)
    lwz      r8, 0x0(r10)
    mullw    r3, r9, r0
    addi     r0, r3, 0x228
    add      r3, r8, r0
    lwz      r0, 0x0(r3)
    lwz      r3, 0x4(r3)
    and      r0, r0, r6
    and      r3, r3, r7
    xor      r3, r3, r5
    xor      r0, r0, r4
    or.      r0, r3, r0
    bne      .L_8013275C
    b        .L_80132768
.L_8013275C:
    addi     r9, r9, 0x1
    bdnz     .L_80132724
.L_80132764:
    li       r9, -0x1
.L_80132768:
    cmpwi    r9, 0x0
    blt      .L_80132894
    add      r5, r28, r11
    lwz      r4, 0x0(r10)
    lwz      r3, 0x10(r5)
    lwz      r0, 0x4(r5)
    mullw    r3, r9, r3
    cmpw     r9, r0
    add      r31, r4, r3
    blt      .L_801327D0
    lwz      r0, 0x11cc(r28)
    slwi     r0, r0, 2
    add      r3, r28, r0
    stw      r31, 0x1cc(r3)
    lwz      r0, 0x11cc(r28)
    lwz      r4, 0x4(r5)
    slwi     r0, r0, 2
    add      r3, r28, r0
    lwz      r3, 0x1cc(r3)
    stw      r4, 0x258(r3)
    lwz      r3, 0x4(r5)
    addi     r0, r3, 0x1
    stw      r0, 0x4(r5)
    lwz      r3, 0x11cc(r28)
    addi     r0, r3, 0x1
    stw      r0, 0x11cc(r28)
.L_801327D0:
    cmpwi    r30, 0x0
    beq      .L_80132870
    lwz      r4, 0x228(r31)
    lis      r3, gSingleObjectReader@ha
    lis      r0, 0x40
    lwz      r7, 0x0(r29)
    addi     r6, r3, gSingleObjectReader@l
    or       r3, r4, r0
    li       r0, 0x8
    stw      r3, 0x228(r31)
    li       r4, 0x0
    mtctr    r0
.L_80132800:
    lwz      r0, 0x350(r6)
    cmplwi   r0, 0x0
    bne      .L_80132838
    lis      r3, gSingleObjectReader@ha
    slwi     r0, r4, 3
    addi     r3, r3, gSingleObjectReader@l
    li       r5, 0x1
    add      r6, r3, r0
    stw      r31, 0x350(r6)
    addi     r3, r3, 0x328
    addi     r4, r6, 0x350
    stw      r7, 0x354(r6)
    bl       OSSendMessage
    b        .L_80132898
.L_80132838:
    addi     r6, r6, 0x8
    addi     r4, r4, 0x1
    bdnz     .L_80132800
    lis      r3, sPathXobjsCpp@ha
    lis      r6, sObjectReaderFullMessage@ha
    lis      r5, sObjectReadStartName@ha
    li       r4, 0x164
    addi     r7, r5, sObjectReadStartName@l
    addi     r3, r3, sPathXobjsCpp@l
    addi     r6, r6, sObjectReaderFullMessage@l
    li       r5, lbl_8065E828@sda21
    crclr    cr1eq
    bl       OSPanic
    b        .L_80132898
.L_80132870:
    mr       r3, r31
    mr       r4, r29
    lwz      r12, 0x34(r31)
    lwz      r12, 0x9c(r12)
    mtctr    r12
    bctrl
    cmpwi    r3, 0x0
    beq      .L_80132870
    b        .L_80132898
.L_80132894:
    li       r31, 0x0
.L_80132898:
    lis      r4, __vt__8AttrInfo@ha
    mr       r3, r31
    addi     r0, r4, __vt__8AttrInfo@l
    stw      r0, 0x10(r1)
    lwz      r31, 0x2c(r1)
    lwz      r30, 0x28(r1)
    lwz      r29, 0x24(r1)
    lwz      r28, 0x20(r1)
    lwz      r0, 0x34(r1)
    mtlr     r0
    addi     r1, r1, 0x30
    blr
*/

/**
 * @note Address: 0x801328C8
 * @note Size: 0x88
 */
extern "C" void ReleaseStageObjectList(XOBJS* list)
{
	for (s32 i = 0; i < list->mObjectCount; i++) {
		list->mObjects[i]->Release();
		list->mObjects[i] = NULL;
	}
	list->mObjectCount     = 0;
	list->mLoadObjectIndex = 0;
}

/**
 * @note Address: 0x80132950
 * @note Size: 0x248
 */
// grabs an object from its pool for every stage entry, then reads them in order
inline void XOBJS::ReadAll()
{
	for (s32 i = 0; i < (s32)ARRAY_SIZE(mPools); i++) {
		mPools[i].mUsed = 0;
	}

	s32 i;
	for (i = 0; GetStagePlacementByIndex(i)->_00 >= 0; i++) {
		CoordInfo info(GetStagePlacementByIndex(i));
		if (mObjects[i] == NULL) {
			XObjPool& pool = mPools[GetObjData(&info)->_14];
			if (pool.mUsed >= pool.mCount) {
				OSPanic(__FILE__, 161, "%s%s", FormatToStaticBuffer("%s:: cnt=%d, max=%d\n", pool.mName, pool.mUsed, pool.mCount),
				        __FUNCTION__);
			}
			mObjects[i]             = pool.Get(pool.mUsed);
			mObjects[i]->mPoolIndex = pool.mUsed;
			pool.mUsed++;
		}
	}
	mObjectCount     = i;
	mLoadObjectIndex = 0;

	for (s32 i = 0; i < mObjectCount; i++) {
		UnkStageObj* obj = GetStagePlacementByIndex(i);
		CoordInfo info(obj);
		if ((obj->_34 & 0x80000000ULL) == 0) {
			while (!mObjects[mLoadObjectIndex]->LoadFromPlacement(info)) { }
		}
		mLoadObjectIndex++;
		if (gGame.mReset != 0) {
			break;
		}
	}
	mLoadState = 2;
}

// out of line XOBJS::ReadAll
extern "C" void ReadAll__5XOBJSFv(XOBJS* list)
{
	list->ReadAll();
}

// ReadAll__5XOBJSFv
/*
    stwu     r1, -0x30(r1)
    mflr     r0
    li       r4, 0x0
    li       r5, 0x15
    stw      r0, 0x34(r1)
    subfic   r0, r5, 0x17
    stmw     r27, 0x1c(r1)
    mr       r27, r3
    stw      r4, 0x4(r3)
    mulli    r3, r5, 0x14
    stw      r4, 0x18(r27)
    add      r3, r27, r3
    stw      r4, 0x2c(r27)
    stw      r4, 0x40(r27)
    stw      r4, 0x54(r27)
    stw      r4, 0x68(r27)
    stw      r4, 0x7c(r27)
    stw      r4, 0x90(r27)
    stw      r4, 0xa4(r27)
    stw      r4, 0xb8(r27)
    stw      r4, 0xcc(r27)
    stw      r4, 0xe0(r27)
    stw      r4, 0xf4(r27)
    stw      r4, 0x108(r27)
    stw      r4, 0x11c(r27)
    stw      r4, 0x130(r27)
    stw      r4, 0x144(r27)
    stw      r4, 0x158(r27)
    stw      r4, 0x16c(r27)
    stw      r4, 0x180(r27)
    stw      r4, 0x194(r27)
    mtctr    r0
    cmpwi    r5, 0x17
    bge      .L_801329E4
.L_801329D8:
    stw      r4, 0x4(r3)
    addi     r3, r3, 0x14
    bdnz     .L_801329D8
.L_801329E4:
    lis      r3, __vt__9CoordInfo@ha
    mr       r31, r27
    addi     r30, r3, __vt__9CoordInfo@l
    li       r28, 0x0
    b        .L_80132AA8
.L_801329F8:
    mr       r3, r28
    bl       GetStagePlacementByIndex
    stw      r30, 0x14(r1)
    stw      r3, 0x10(r1)
    lwz      r0, 0x1cc(r31)
    cmplwi   r0, 0x0
    bne      .L_80132A9C
    addi     r3, r1, 0x10
    bl       GetObjData
    lha      r0, 0x14(r3)
    mulli    r0, r0, 0x14
    add      r29, r27, r0
    lwz      r5, 0x4(r29)
    lwz      r6, 0x8(r29)
    cmpw     r5, r6
    blt      .L_80132A6C
    lis      r3, sObjectPoolCapacityFormat@ha
    lwz      r4, 0xc(r29)
    addi     r3, r3, sObjectPoolCapacityFormat@l
    crclr    cr1eq
    bl       FormatToStaticBuffer
    lis      r4, sPathXobjsCpp@ha
    mr       r6, r3
    addi     r3, r4, sPathXobjsCpp@l
    li       r5, lbl_8065E828@sda21
    li       r4, 0xa1
    li       r7, lbl_8065A9A8@sda21
    crclr    cr1eq
    bl       OSPanic
.L_80132A6C:
    lwz      r3, 0x4(r29)
    lwz      r0, 0x10(r29)
    lwz      r4, 0x0(r29)
    mullw    r0, r3, r0
    add      r0, r4, r0
    stw      r0, 0x1cc(r31)
    lwz      r0, 0x4(r29)
    lwz      r3, 0x1cc(r31)
    stw      r0, 0x258(r3)
    lwz      r3, 0x4(r29)
    addi     r0, r3, 0x1
    stw      r0, 0x4(r29)
.L_80132A9C:
    stw      r30, 0x14(r1)
    addi     r31, r31, 0x4
    addi     r28, r28, 0x1
.L_80132AA8:
    mr       r3, r28
    bl       GetStagePlacementByIndex
    lwz      r0, 0x0(r3)
    cmpwi    r0, 0x0
    bge      .L_801329F8
    stw      r28, 0x11cc(r27)
    li       r0, 0x0
    lis      r4, gGame@ha
    lis      r3, __vt__9CoordInfo@ha
    stw      r0, 0x11d0(r27)
    addi     r30, r4, gGame@l
    addi     r31, r3, __vt__9CoordInfo@l
    li       r28, 0x0
    lis      r29, 0x8000
    b        .L_80132B70
.L_80132AE4:
    mr       r3, r28
    bl       GetStagePlacementByIndex
    stw      r31, 0xc(r1)
    li       r4, 0x0
    and      r0, r4, r4
    stw      r3, 0x8(r1)
    xor      r0, r0, r4
    lwz      r3, 0x34(r3)
    and      r3, r3, r29
    xor      r3, r3, r4
    or.      r0, r3, r0
    bne      .L_80132B40
.L_80132B14:
    lwz      r0, 0x11d0(r27)
    addi     r4, r1, 0x8
    slwi     r3, r0, 2
    addi     r0, r3, 0x1cc
    lwzx     r3, r27, r0
    lwz      r12, 0x34(r3)
    lwz      r12, 0x9c(r12)
    mtctr    r12
    bctrl
    cmpwi    r3, 0x0
    beq      .L_80132B14
.L_80132B40:
    lwz      r3, 0x11d0(r27)
    addi     r0, r3, 0x1
    stw      r0, 0x11d0(r27)
    lwz      r0, 0x3330(r30)
    cmpwi    r0, 0x0
    beq      .L_80132B68
    lis      r3, __vt__9CoordInfo@ha
    addi     r0, r3, __vt__9CoordInfo@l
    stw      r0, 0xc(r1)
    b        .L_80132B7C
.L_80132B68:
    stw      r31, 0xc(r1)
    addi     r28, r28, 0x1
.L_80132B70:
    lwz      r0, 0x11cc(r27)
    cmpw     r28, r0
    blt      .L_80132AE4
.L_80132B7C:
    li       r0, 0x2
    stw      r0, 0x11d4(r27)
    lmw      r27, 0x1c(r1)
    lwz      r0, 0x34(r1)
    mtlr     r0
    addi     r1, r1, 0x30
    blr
*/

/**
 * @note Address: 0x80132B98
 * @note Size: 0x170
 */

extern "C" BOOL LoadRequestedHeldItem(XObj* player);
extern "C" BOOL RestorePlayerHeldItem(XObj* player);

// steps through the stage read and the player setup calls, true when done
extern "C" u8 UpdateStageObjectLoading(XOBJS* list)
{
	if (list->mLoadState == 0) {
		gAllObjectsReader.ReadStart((void*)1, list);
		list->mLoadState = 1;
	}
	if (list->mLoadState == 2) {
		list->mLoadState = 3;
	}
	if (list->mLoadState == 3 && LoadRequestedHeldItem(gGame.GetPlayer())) {
		list->mLoadState = 4;
	}
	if (list->mLoadState == 4 && RestorePlayerHeldItem(gGame.GetPlayer())) {
		list->mLoadState = 5;
	}
	if (list->mLoadState == 5 && LoadPlayerAttachmentAnimation(gGame.GetPlayer())) {
		list->mLoadState = 0;
	}
	return list->mLoadState == 0;
}

// the objects XOBJS hands out and their constructors

/**
 * @note Address: 0x80132D08
 * @note Size: 0x94
 */
// vf94 on every object of the 23 pools
extern "C" void ResetPooledObjects(XObjPool* pools)
{
	XObjPool* pool = pools;
	for (s32 index = 0; index < 23; ++index, ++pool) {
		for (s32 i = 0; i < pool->mCount; ++i) {
			pools[index].Get(i)->vf94();
		}
	}
}

// one pool: its objects, its entry in the list and an id for each object. retail builds the pools
// of one object as arrays of one as well, and gives every pool its own loop counter
#define XOBJ_POOL(slot, type, count, name)                       \
	{                                                            \
		static type objs[count];                                 \
		list->mPools[slot].Set(objs, count, name, sizeof(type)); \
		for (s32 i = 0; i < count; i++) {                        \
			objs[i].InitializeObject(id);                        \
			id++;                                                \
		}                                                        \
	}

/**
 * @note Address: 0x80132D9C
 * @note Size: 0xDFC
 */
// builds the 23 pools, numbers every object and starts the three reader threads
// NONMATCHING: retail keeps the frame of the empty loop near the end (li r3, 21 ... bdnz), so what
// left 21 in its counter was not a constant; and two pairs of saved registers are swapped
extern "C" void InitializeObjectPools(XOBJS* list)
{
	s32 id = 0;

	XOBJ_POOL(0, XObjCamera, 1, "CAMERA");
	XOBJ_POOL(1, XObjLight, 1, "LIGHT");
	XOBJ_POOL(3, XObjMap, 1, "MAP");
	XOBJ_POOL(2, XObjChr, 1, "CHR");
	XOBJ_POOL(4, XObj, 2, "ACTOR2");
	XOBJ_POOL(5, XObjActor3, 48, "ACTOR3");
	XOBJ_POOL(6, XObjSound, 8, "SOUND");
	XOBJ_POOL(7, XObjCoin, 96, "COIN");
	XOBJ_POOL(8, XObjItem, 64, "ITEM");
	XOBJ_POOL(9, XObjLeticker, 12, "LETICKER");
	XOBJ_POOL(10, XObjActorToy, 15, "ACTOR_TOY");
	XOBJ_POOL(11, XObjActorJenny, 1, "ACTOR_JENNY");
	XOBJ_POOL(12, XObjActorMama, 1, "ACTOR_MAMA");
	XOBJ_POOL(13, XObjActorPapa, 1, "ACTOR_PAPA");
	XOBJ_POOL(14, XObjActorTao, 1, "ACTOR_TAO");
	XOBJ_POOL(15, XObjActorDeca, 2, "ACTOR_DECA");
	XOBJ_POOL(16, XObjArmy, 21, "ARMY");
	XOBJ_POOL(17, XObjSpider, 16, "SPIDER");
	XOBJ_POOL(18, XObjSpiderSmall, 16, "SPIDER_SMALL");
	XOBJ_POOL(19, XObjSpiderBoss, 1, "SPIDER_BOSS");
	XOBJ_POOL(20, XObjDust, 64, "DUST");
	XOBJ_POOL(21, XObjHBox, 16, "HBOX");
	XOBJ_POOL(22, XObjFree, 288, "FREE");

	// nothing is left of this loop over the last two pools but its counter
	for (s32 i = 21; i < 23; i++) { }
	list->mLoadState = 0;

	gAllObjectsReader.Create(ReadObjectListThread, NULL, 0x2000, 17, OS_THREAD_ATTR_DETACH);
	gSingleObjectReader.Create(ReadSingleObjectThread, NULL, 0x2000, 16, OS_THREAD_ATTR_DETACH);
	gObjectAnimationReader.Create(ReadObjectAnimationsThread, NULL, 0x2000, 16, OS_THREAD_ATTR_DETACH);
}

/**
 * @note Address: 0x80133BD0
 * @note Size: 0x3C
 */
XObjFree::XObjFree()
{
}

/**
 * @note Address: 0x80133C44
 * @note Size: 0x44
 */
XObjHBox::XObjHBox()
{
}

/**
 * @note Address: 0x80133C88
 * @note Size: 0x30
 */
XObjHBoxPart::XObjHBoxPart()
{
}

/**
 * @note Address: 0x80133CF0
 * @note Size: 0x48
 */
XObjDust::XObjDust()
{
	mFlags = &gDustEventFlags;
}

/**
 * @note Address: 0x80133D7C
 * @note Size: 0x5C
 */
XObjSpiderBoss::XObjSpiderBoss()
{
}

/**
 * @note Address: 0x80133E10
 * @note Size: 0x74
 */
XObjSpiderSmall::XObjSpiderSmall()
{
}

/**
 * @note Address: 0x80133EBC
 * @note Size: 0x84
 */
XObjSpider::XObjSpider()
{
}

/**
 * @note Address: 0x80133F78
 * @note Size: 0x88
 */
XObjArmy::XObjArmy()
{
}

// 0x80134000, XObjArmyShot's ctor, the compiler emits it here

/**
 * @note Address: 0x8013404C
 * @note Size: 0x58
 */
XObjActorDeca::XObjActorDeca()
{
}

/**
 * @note Address: 0x80134100
 * @note Size: 0x50
 */
XObjActorTao::XObjActorTao()
{
}

/**
 * @note Address: 0x80134150
 * @note Size: 0x50
 */
XObjActorPapa::XObjActorPapa()
{
}

/**
 * @note Address: 0x801341A0
 * @note Size: 0x50
 */
XObjActorMama::XObjActorMama()
{
}

/**
 * @note Address: 0x801341F0
 * @note Size: 0x58
 */
XObjActorJenny::XObjActorJenny()
{
}

/**
 * @note Address: 0x80134280
 * @note Size: 0x80
 */
XObjActorToy::XObjActorToy()
{
}

/**
 * @note Address: 0x801340A4
 * @note Size: 0x5C
 */
XObjActor::XObjActor()
{
}

/**
 * @note Address: 0x80134338
 * @note Size: 0x48
 */
XObjLeticker::XObjLeticker()
{
}

/**
 * @note Address: 0x801343B8
 * @note Size: 0x48
 */
XObjItem::XObjItem()
{
	mFlags = &gItemEventFlags;
}

/**
 * @note Address: 0x80134438
 * @note Size: 0x48
 */
XObjCoin::XObjCoin()
{
	mFlags = &gCoinEventFlags;
}

/**
 * @note Address: 0x80133D38
 * @note Size: 0x44
 */
XObjFlags::XObjFlags()
{
	mFlags = NULL;
}

/**
 * @note Address: 0x801344B8
 * @note Size: 0x48
 */
XObjSound::XObjSound()
{
}

/**
 * @note Address: 0x80134538
 * @note Size: 0xF4
 */
XObjActor3::XObjActor3()
{
}

// 0x8013462C, CPtclDust3's ctor, the compiler emits it here

/**
 * @note Address: 0x80134694
 * @note Size: 0x354
 */
XObjChr::XObjChr()
{
}

#if !defined(VERSION_JP)
/**
 * @note Address: 0x801349E8
 * @note Size: 0x34
 */
OXMenuMark::OXMenuMark()
{
}
#endif

/**
 * @note Address: 0x80134A1C
 * @note Size: 0x3C
 */
XObjOX::XObjOX()
{
}

/**
 * @note Address: 0x80134A58
 * @note Size: 0x3C
 */
XObjAttachment::XObjAttachment()
{
}

/**
 * @note Address: 0x80134A94
 * @note Size: 0x3C
 */
XObjRadar::XObjRadar()
{
}

/**
 * @note Address: 0x80134AD0
 * @note Size: 0x3C
 */
XObjPopper::XObjPopper()
{
}

/**
 * @note Address: 0x80134B0C
 * @note Size: 0x3C
 */
XObjCopter::XObjCopter()
{
}

/**
 * @note Address: 0x80134B48
 * @note Size: 0x70
 */
XObjPlug::XObjPlug()
{
}

/**
 * @note Address: 0x80134BB8
 * @note Size: 0x3C
 */
XObjWireEff::XObjWireEff()
{
}

/**
 * @note Address: 0x80134BF4
 * @note Size: 0x2C
 */
XObjPlugCordNode::XObjPlugCordNode()
{
}

/**
 * @note Address: 0x80134C64
 * @note Size: 0x64
 */
XActPopperShot::XActPopperShot()
{
}

/**
 * @note Address: 0x80134CF8
 * @note Size: 0x7C
 */
// 0x80134CC8, CPtclDust2's ctor, the compiler emits it here

XObjMap::XObjMap()
{
}

/**
 * @note Address: 0x80134D8C
 * @note Size: 0x5C
 */
// 0x80134D74, CTrigon's ctor, the compiler emits it here

XObjLight::XObjLight()
{
}

/**
 * @note Address: 0x80134DE8
 * @note Size: 0x64
 */
XObjCamera::XObjCamera()
{
}

/**
 * @note Address: 0x80134E94
 * @note Size: 0x84
 */
XObjReadOne::~XObjReadOne()
{
}

/**
 * @note Address: 0x80134F18
 * @note Size: 0x84
 */
XObjReadAll::~XObjReadAll()
{
}
