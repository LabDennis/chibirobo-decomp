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

/**
 * @note Address: 0x80131BE8
 * @note Size: 0x84
 */
XObjReadAnm::~XObjReadAnm()
{
}

inline BOOL XObjRead::ReadStart(void* obj, void* info)
{
	for (s32 i = 0; i < 8; i++) { // eight pending requests at most
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
 * @note Address: 0x80132428
 * @note Size: 0x40
 */
// the reader thread entries below inline it
inline void XObjRead::RunRequestLoop()
{
	OSMessage msg;
	while (true) {
		OSReceiveMessage(&mQueue.mQueue, &msg, OS_MESSAGE_BLOCK);
		ProcessRequest((XObjReadReq*)msg);
	}
}

/**
 * @note Address: 0x80132040
 * @note Size: 0xB4
 */
BOOL XObjReadAnm::QueueRead(void* object, void* info)
{
	static_cast<XObj*>(object)->mStatus.mFlags |= 0x0080000000000000ULL; // mark the anim read as queued
	return ReadStart(object, info);
}

/**
 * @note Address: 0x801320F4
 * @note Size: 0x4C
 */
// reader thread entry
extern "C" void* ReadObjectAnimationsThread(void* arg)
{
	gObjectAnimationReader.RunRequestLoop();
	return NULL;
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
	return gObjectAnimationReader.QueueRead(obj, path);
}

/**
 * @note Address: 0x801321FC
 * @note Size: 0xA8
 */
// Processes a single-object load request.
void XObjReadOne::ProcessRequest(XObjReadReq* req)
{
	XObj* obj = static_cast<XObj*>(req->mObj);
	CoordInfo info(static_cast<StagePlacement*>(req->mInfo));
	while (!obj->LoadFromPlacement(info)) {
		OSYieldThread();
	}
	req->mObj = NULL;
	obj->mStatus.mFlags &= ~0x0040000000000000ULL; // the placement load finished
}

/**
 * @note Address: 0x801322A4
 * @note Size: 0xB4
 */
BOOL XObjReadOne::QueueRead(void* object, void* info)
{
	static_cast<XObj*>(object)->mStatus.mFlags |= 0x0040000000000000ULL; // a placement read is queued
	return ReadStart(object, info);
}

/**
 * @note Address: 0x80132358
 * @note Size: 0x4C
 */
extern "C" void* ReadSingleObjectThread(void* arg)
{
	gSingleObjectReader.RunRequestLoop();
	return NULL;
}

/**
 * @note Address: 0x801323A4
 * @note Size: 0x38
 */
void XObjReadAll::ProcessRequest(XObjReadReq* req)
{
	static_cast<XOBJS*>(req->mInfo)->ReadAll();
	req->mObj = NULL;
}

/**
 * @note Address: 0x801323DC
 * @note Size: 0x4C
 */
extern "C" void* ReadObjectListThread(void* arg)
{
	gAllObjectsReader.RunRequestLoop();
	return NULL;
}

/**
 * @note Address: 0x80132468
 * @note Size: 0x9C
 */
BOOL XObjRead::QueueRead(void* object, void* info)
{
	return ReadStart(object, info);
}

/**
 * @note Address: 0x80132504
 * @note Size: 0xC8
 */
// NONMATCHING: retail conditionally repeats the mStackSize store after successful OSCreateThread; that redundant assignment stays removed.
BOOL XObjRead::Create(OSThreadStartFunction func, void* param, u32 stackSize, OSPriority priority, u16 attr)
{
	mQueue.Initialize(8);
	for (s32 i = 0; i < 8; i++) {
		mReqs[i].mObj = NULL;
	}
	mStack     = (u8*)OSAllocFromHeap(__OSCurrHeap, stackSize);
	mStackSize = stackSize;
	// OSCreateThread wants the stack's top, stacks grow down
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
	AttrInfo attr(info);
	s16 category   = attr.mObjData ? attr.mObjData->mCategory : 0;
	XObjPool& pool = list->mPools[category];

	s32 index = pool.Find(1, 0);
	if (index < 0)
		return NULL;

	XObj* obj = pool.Get(index);
	if (index >= pool.mUsed) {
		// A newly used slot joins the stage's object list.
		list->mObjects[list->mObjectCount] = obj;
		obj->mPoolIndex                    = pool.mUsed++;
		++list->mObjectCount;
	}
	if (async) {
		gSingleObjectReader.QueueRead(obj, info->mPlacement);
	} else {
		while (!obj->LoadFromPlacement(*info)) { }
	}
	return obj;
}

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
void XOBJS::ReadAll()
{
	for (s32 i = 0; i < (s32)ARRAY_SIZE(mPools); i++) {
		mPools[i].mUsed = 0;
	}

	s32 i;
	for (i = 0; GetStagePlacementByIndex(i)->mObjectId >= 0; i++) { // the placement list ends at id -1
		CoordInfo info(GetStagePlacementByIndex(i));
		if (mObjects[i] == NULL) {
			XObjPool& pool = mPools[GetObjData(&info)->mCategory];
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
		StagePlacement* obj = GetStagePlacementByIndex(i);
		CoordInfo info(obj);
		if ((obj->mFlags & 0x80000000ULL) == 0) { // 0x80000000 (bit 31) skips the placement load
			while (!mObjects[mLoadObjectIndex]->LoadFromPlacement(info)) { }
		}
		mLoadObjectIndex++;
		if (gGame.mReset != 0) {
			break;
		}
	}
	mLoadState = 2;
}

extern "C" BOOL LoadRequestedHeldItem(XObj* player);
extern "C" BOOL RestorePlayerHeldItem(XObj* player);

/**
 * @note Address: 0x80132B98
 * @note Size: 0x170
 */

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
// ResetForNewGame on every object of the 23 pools
extern "C" void ResetPooledObjects(XObjPool* pools)
{
	for (s32 index = 0; index < 23; ++index) {
		XObjPool& pool = pools[index];
		for (s32 i = 0; i < pool.mCount; ++i) {
			pool.Get(i)->ResetForNewGame();
		}
	}
}

// Each pool owns a static array. Initialize the pools in order and give every object a consecutive ID.
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
