// Retail xobj_ut.cpp (.text 0x800BD6B4-0x800C659C): object pools, member/subclass
// ctors and dtors, stage spawn, reader threads, and XAction base.
#include "Version.h"
#if defined(VERSION_R24J01)
#include "game/wii/xobjflags.h" // first: its AnimSfx needs the real nw4r::math, ahead of xobj.h's shim
#include "game/wii/xobj.h"
#include "game/wii/xobjfree.h"
#include "game/wii/xobjleticker.h"

// builds the 23 pools from heap arrays and starts the two reader threads
extern "C" void InitializeObjectPools(XOBJS* list)
{
#define XOBJ_POOL(slot, type, count, name)   \
	{                                        \
		type* objs     = new type[count];    \
		XObjPool& pool = list->mPools[slot]; \
		pool.mObjects  = (u8*)objs;          \
		pool.mUsed     = 0;                  \
		pool.mCount    = count;              \
		pool.mName     = name;               \
		pool.mStride   = sizeof(type);       \
		for (s32 i = 0; i < count; i++) {    \
			pool.Get(i)->InitializeObject(); \
		}                                    \
	}

	XOBJ_POOL(0, XObjScnCam, 1, "SCNCAM");
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
	XOBJ_POOL(22, XObjFree, 400, "FREE");
#undef XOBJ_POOL

	list->mLoadState = 0;

	gAllObjectsReader.Create(ReadObjectListThread, NULL, 0x2000, 17, 1);
	gSingleObjectReader.Create(ReadSingleObjectThread, NULL, 0x2000, 16, 1);
}

// the light object's own state is only its CLight array; everything before it
// is the inlined XObj construction
XObjLight::XObjLight()
{
}

// the member ctors install the three vtables (TVec, then the two CVecs)
CObjVec::CObjVec()
{
}

// the NW_Anm and the CFile initialize themselves
CJArchive::CJArchive()
{
}

XObjLight::~XObjLight()
{
}

XObjMap::XObjMap()
{
}

// 0x800BEB4C-0x800C0EF0: XObjMapListA/B, ~XObjMap, XObjChr ctor and member ctors
#include "xobjmember.cpp"

// 0x800C0EF0-0x800C13B0: pad and player-menu ctors/dtors
#include "game/wii/cpad.h"
#include "game/xpad.h"
#include "game/wii/xchrmenu.h"
#include "game/ringmenu.h"
#include "game/octamenu.h"

CPadOne::~CPadOne()
{
}

XPad::~XPad()
{
}

XObjChrMenu::XObjChrMenu()
{
}

OctaMenu::~OctaMenu()
{
}

AtcMenu::~AtcMenu()
{
}

DrsMenu::~DrsMenu()
{
}

XObjChrMenu::~XObjChrMenu()
{
}

OXMenu::OXMenu()
{
	mChr = NULL;
}

OXMenuMark::OXMenuMark()
{
}

OXMenuMark::~OXMenuMark()
{
}

OXMenu::~OXMenu()
{
}

// 0x800C13B0-0x800C5424: pool-subclass ctors and dtors
#include "xobjctor.cpp"

// 0x800C5424-0x800C6584: pool walks, stage spawn, reader threads, and XAction base
#include "game/wii/xobjs.h"
#include "game/cthread.h"
#include "revolution/mem.h"
#include "Dolphin/os.h"

// the expanded heap mm::InitMem1 creates over the MEM1 arena; the draw lists'
// ref tables come out of it
extern MEMiHeapHead* gWiiMem1Heap;

// first object whose status flags match value under mask, -1 without one
inline s32 XObjPool::Find(u64 mask, u64 value) const
{
	for (s32 i = 0; i < mCount; i++) {
		if ((Get(i)->mStatus.mFlags & mask) == value) {
			return i;
		}
	}
	return -1;
}

// ResetForNewGame on every object of the 23 pools
extern "C" void ResetPooledObjects(XObjPool* pools)
{
	XObjPool* pool = pools;
	for (s32 index = 0; index < 23; ++index, ++pool) {
		for (s32 i = 0; i < pool->mCount; ++i) {
			pool->Get(i)->ResetForNewGame();
		}
	}
}

// queues the stage read while idle, then steps through the list's states; true when done
extern "C" s32 UpdateStageObjectLoading(XOBJS* list)
{
	if (list->mLoadState == 0) {
		gAllObjectsReader.QueueRead((void*)1, list);
		list->mLoadState = 1;
	}
	if (list->mLoadState == 2) {
		list->mLoadState = 3;
	}
	if (list->mLoadState == 3) {
		list->mLoadState = 0;
	}
	return list->mLoadState == 0;
}

// grabs an object from its pool for every stage entry, then reads them in order
void XOBJS::ReadAll()
{
	for (s32 i = 0; i < (s32)ARRAY_SIZE(mPools); i++) {
		mPools[i].mUsed = 0;
	}

	gGame.ClearStatusFlags(0x2000000000ULL);
	gGame.ClearStatusFlags(0x100000000000ULL);

	s32 i;
	for (i = 0; GetStagePlacementByIndex(i)->mObjectId >= 0; i++) { // the placement list ends at id -1
		CoordInfo info(GetStagePlacementByIndex(i));
		if (mObjects[i] == NULL) {
			s32 category = GetObjData(&info)->mCategory;
			if (gGame.GetStatusFlags(0x2000000000ULL) == 0 && (info.mPlacement->mFlags & 0x2000)) {
				gGame.SetStatusFlags(0x2000000000ULL);
			}
			if (gGame.GetStatusFlags(0x100000000000ULL) == 0 && (info.mPlacement->mFlags & 0x4)) {
				gGame.SetStatusFlags(0x100000000000ULL);
			}
			if (mPools[category].mUsed >= mPools[category].mCount) {
				OSPanic("xobj_ut.cpp", 0xd1, "XOBJ_CLASS_%s:: cnt=%d, max=%d\n", mPools[category].mName, mPools[category].mUsed + 1,
				        mPools[category].mCount);
			}
			mObjects[i]              = mPools[category].Get(mPools[category].mUsed);
			mObjects[i]->mStageIndex = i;
			mObjects[i]->mPoolIndex  = mPools[category].mUsed;
			mPools[category].mUsed++;
		}
	}
	mObjectCount     = i;
	mLoadObjectIndex = 0;

	for (s32 i = 0; i < mObjectCount; i++) {
		StagePlacement* obj = GetStagePlacementByIndex(i);
		CoordInfo info(obj);
		if ((obj->mFlags & 0x80000000) == 0) { // 0x80000000 skips the placement load
			while (!mObjects[mLoadObjectIndex]->LoadFromPlacement(info)) { }
		}
		mLoadObjectIndex++;
		if (gGame.mResetRequested) {
			break;
		}
	}
	mLoadState = 2;
}

// releases every listed object; the first object of pool 3 (the map) goes last
extern "C" void ReleaseStageObjectList(XOBJS* list)
{
	for (s32 i = 0; i < list->mObjectCount; i++) {
		if (list->mObjects[i] != NULL && list->mObjects[i] != (XObj*)list->mPools[3].mObjects) {
			list->mObjects[i]->Release();
			list->mObjects[i] = NULL;
		}
	}
	for (s32 i = 0; i < list->mObjectCount; i++) {
		if (list->mObjects[i] != NULL && list->mObjects[i] == (XObj*)list->mPools[3].mObjects) {
			list->mObjects[i]->Release();
			list->mObjects[i] = NULL;
		}
	}
	list->mObjectCount     = 0;
	list->mLoadObjectIndex = 0;
}

// puts one read on the queue; QueueRead of the base and XObjReadOne and
// SpawnObjectFromPlacement all inline this body, so they share its panic line
inline BOOL XObjRead::ReadStart(void* obj, void* info)
{
	for (s32 i = 0; i < 8; i++) {
		if (mReqs[i].mObj == NULL) {
			mReqs[i].mObj  = obj;
			mReqs[i].mInfo = info;
			OSSendMessage(&mQueue, &mReqs[i], OS_MESSAGE_BLOCK);
			return TRUE;
		}
	}
	OSPanic("xobj_ut.cpp", 0x1d0, "no more space for XObjRead thread!\n");
	return FALSE;
}

// hands a free pool object to a stage placement; direct takes the next used slot, the
// other way finds an unused one first. async queues the read on the single-object thread
extern "C" XObj* SpawnObjectFromPlacement(XOBJS* list, CoordInfo* info, s32 direct, BOOL async)
{
	XObj* obj;
	AttrInfo attr(info);
	s16 pool = attr.mObjData ? attr.mObjData->mCategory : 0; // the category picks the pool

	if (direct) {
		obj = list->mPools[pool].Get(list->mPools[pool].mUsed);
		if (obj != NULL) {
			list->mObjects[list->mObjectCount]              = obj;
			list->mObjects[list->mObjectCount]->mStageIndex = list->mObjectCount;
			list->mObjects[list->mObjectCount]->mPoolIndex  = list->mPools[pool].mUsed;
			list->mPools[pool].mUsed++;
			list->mObjectCount++;
			if (async) {
				obj->mStatus.mFlags |= 0x0040000000000000ULL; // a placement read is queued
				if (!gSingleObjectReader.ReadStart(obj, info->mPlacement)) {
					obj = NULL;
				}
			} else {
				while (!obj->LoadFromPlacement(*info)) { }
				gGame.SetStatusFlags(0x1000);
			}
		}
	} else {
		s32 idx = list->mPools[pool].Find(0x4040000000000001ULL, 0);
		if (idx >= 0) {
			obj = list->mPools[pool].Get(idx);
			if (idx >= list->mPools[pool].mUsed) {
				// a slot past the used range joins the stage's object list
				list->mObjects[list->mObjectCount]              = obj;
				list->mObjects[list->mObjectCount]->mStageIndex = list->mObjectCount;
				list->mObjects[list->mObjectCount]->mPoolIndex  = list->mPools[pool].mUsed;
				list->mPools[pool].mUsed++;
				list->mObjectCount++;
			}
			if (async) {
				obj->mStatus.mFlags |= 0x0040000000000000ULL; // a placement read is queued
				if (!gSingleObjectReader.ReadStart(obj, info->mPlacement)) {
					obj = NULL;
				}
			} else {
				while (!obj->LoadFromPlacement(*info)) { }
				gGame.SetStatusFlags(0x1000);
			}
		} else {
			obj = NULL;
		}
	}
	return obj;
}

// the object's slot in the stage's object table, -1 when it is not listed
extern "C" s32 FindStageObjectIndex(XOBJS* list, XObj* obj)
{
	for (s32 i = 0; i < list->mObjectCount; i++) {
		if (list->mObjects[i] == obj) {
			return i;
		}
	}
	return -1;
}

// allocates the list's ref table off the MEM1 heap: 1024 slots, every one free
// at boot; the GameCube kept the table inside the list itself
void XObjDrawList::Initialize()
{
	mRefs = (WiiObjectRef*)MEMAllocFromExpHeapEx(gWiiMem1Heap, 0x2000, 4);
	for (s32 i = 0; i < 1024; i++) {
		mRefs[i].mObj = NULL;
	}
	mCount = 0;
}

// starts the queue, clears the request slots, makes the stack and runs the thread
BOOL XObjRead::Create(void* (*func)(void*), void* param, u32 stackSize, s32 priority, u16 attr)
{
	mQueue.Initialize(8);
	for (s32 i = 0; i < 8; i++) {
		mReqs[i].mObj = NULL;
	}
	mStack     = (u8*)MEMAllocFromExpHeapEx(gWiiGameHeap, stackSize, 4);
	mStackSize = stackSize;
	if (OSCreateThread(this, func, param, mStack + stackSize, stackSize, priority, attr)) {
		mStackSize = stackSize;
	}
	OSResumeThread(this);
	return FALSE;
}

// allocates the message buffer and inits the queue
void CMsgQueue::Initialize(s32 count)
{
	mMsgs = (OSMessage*)MEMAllocFromExpHeapEx(gWiiGameHeap, count * 4, 4);
	OSInitMessageQueue(this, mMsgs, count);
}

// the request slots, then the queue
BOOL XObjRead::QueueRead(void* object, void* info)
{
	return ReadStart(object, info);
}

// waits for a request and lets the object handle it
void XObjRead::RunRequestLoop()
{
	void* msg;
	for (;;) {
		OSReceiveMessage(&mQueue, &msg, OS_MESSAGE_BLOCK);
		ProcessRequest((XObjReadReq*)msg);
	}
}

void XObjRead::ProcessRequest(XObjReadReq* req)
{
}

// the all-objects thread body
extern "C" void* ReadObjectListThread(void* arg)
{
	gAllObjectsReader.RunRequestLoop();
	return NULL;
}

// the queued list fills the stage
void XObjReadAll::ProcessRequest(XObjReadReq* req)
{
	((XOBJS*)req->mInfo)->ReadAll();
	req->mObj = NULL;
}

// the single-object thread body
extern "C" void* ReadSingleObjectThread(void* arg)
{
	gSingleObjectReader.RunRequestLoop();
	return NULL;
}

// a placement read joins the queue
BOOL XObjReadOne::QueueRead(void* object, void* info)
{
	((XObj*)object)->mStatus.mFlags |= 0x0040000000000000ULL;
	return ReadStart(object, info);
}

// loads the queued object on this thread until the placement says it is done
void XObjReadOne::ProcessRequest(XObjReadReq* req)
{
	XObj* obj = (XObj*)req->mObj;
	CoordInfo info(static_cast<StagePlacement*>(req->mInfo));
	while (!obj->LoadFromPlacement(info)) {
		OSYieldThread();
	}
	req->mObj = NULL;
	obj->mStatus.mFlags &= ~0x0040000000000000ULL;
}

// true while a request waits in either reader
extern "C" BOOL HasPendingObjectReads()
{
	return gAllObjectsReader.HasPendingReads() || gSingleObjectReader.HasPendingReads();
}

// true while one of the eight slots holds a request
BOOL XObjRead::HasPendingReads()
{
	for (s32 i = 0; i < 8; i++) {
		if (mReqs[i].mObj != NULL) {
			return TRUE;
		}
	}
	return FALSE;
}

// the placement's flag index
s32 XObjFlags::GetFlagIndex()
{
	return mPlacementParam1;
}

// clears every flag the object's placements ever set
void XObjFlags::ResetForNewGame()
{
	u32* flags = mFlags->mFlags;
	for (u32 i = 0; i < ARRAY_SIZE(mFlags->mFlags); ++i) {
		flags[i] = 0;
	}
}

XObjReadAll gAllObjectsReader;
XObjReadOne gSingleObjectReader;

void XAction::Reset()
{
	mState         = 0;
	mMoveDirection = 0;
	mTarget        = NULL;
	mTargetState   = 0;
}

void XAction::Init(WiiPlayerState* owner)
{
	mOwner = owner;
	Reset();
}

XObjReadAll::~XObjReadAll()
{
}

XObjReadOne::~XObjReadOne()
{
}

#endif
