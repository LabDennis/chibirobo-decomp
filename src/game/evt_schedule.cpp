#include "game/hsd_jobj.h"
#include "game/cgame.h"
#include "game/evtqueue.h"
#include "game/ut.h"

extern "C" void OSPanic(const char* file, int line, const char* msg, ...);

static s32 FindFreeQueueSlot()
{
	const s32 capacity = ARRAY_SIZE(gEventScriptQueue.mScript);
	for (s32 i = 0; i < capacity; ++i) {
		s32 slot = (gEventScriptQueue.mRead + i) % capacity;
		if (gEventScriptQueue.mScript[slot] == NULL) {
			return slot;
		}
	}
	return -1;
}

static void TickEventTimers(s32 step)
{
	for (u32 i = 0; i < ARRAY_SIZE(gEventTimers.mFrames); ++i) {
		if (gEventTimers.mFrames[i] <= 0) {
			continue;
		}
		gEventTimers.mFrames[i] -= step;
		if (gEventTimers.mFrames[i] <= 0) {
			s32 slot = FindFreeQueueSlot();
			if (slot >= 0) {
				// A timer replaces only the script pointer in this queue entry.
				gEventScriptQueue.mScript[slot] = reinterpret_cast<u8*>(gEventTimers.mValue[i]);
			}
			gEventTimers.mFrames[i] = 0;
			gEventTimers.mValue[i]  = 0;
		}
	}
}

/**
 * @note Address: 0x8007E97C
 * @note Size: 0x528
 */
extern "C" void CEvt_Update(CEvt* evt, s32 step)
{
	while (evt->mData.mScript != NULL) {
		u8 command = *evt->mData.mScript++;
		// ABORT occupies the first entry and has its own entry point.
		const EvtCmdEntry* entry = evtCmdTable + 1;
		while (entry->command != 0 && entry->command != command) {
			++entry;
		}
		if (entry->command == 0) {
			OSPanic("evt.cpp", 558, "%s%s", FormatToStaticBuffer("Unknown event cmd - %d\n", command), "Update");
			return;
		}
		if ((evt->*entry->handler)(command, step) != 0) {
			break;
		}
	}

	if (evt->mOwner == 0 && !(gGame.mStatus.mFlags & 0xC00) && evt->mData.mScript == NULL) {
		TickEventTimers(step);
		CEvtQueueEx& queue   = gEventScriptQueue;
		s32 index            = queue.mRead;
		u8* script           = queue.mScript[index];
		queue.mScript[index] = NULL;
		queue.mRead          = (index + 1) % ARRAY_SIZE(queue.mScript);
		if (script != NULL) {
			StartOrQueueEventScript(evt, script, queue.mContext[index], queue.mUseValue[index], queue.mHitValue[index]);
		}
	}
}
static inline void ClearScript(CEvt* evt)
{
	evt->mData.mScript  = NULL;
	evt->mWaitTimer     = 0;
	evt->mYesNoActive   = 0;
	evt->mMenuActive    = 0;
	evt->mCalls.mCount  = 0;
	evt->mFrames.mCount = 0;
	evt->mValues.mCount = 0;
}

static inline void ClearQueue()
{
	gEventScriptQueue.mRead = 0;
	for (u32 i = 0; i < ARRAY_SIZE(gEventScriptQueue.mScript); ++i) {
		gEventScriptQueue.mScript[i] = NULL;
	}
}

static inline void ClearResults(CEvt* evt)
{
	evt->mSelectResult = 0;
	evt->mResult2      = 0;
	evt->mMsgState     = 0;
	evt->mUseValue     = -1;
	evt->mHitValue     = -1;
}

/**
 * @note Address: 0x8007EEA4
 * @note Size: 0x194
 */
extern "C" void StartOrQueueEventScript(CEvt* evt, u8* script, u32 context, s16 useValue, s16 hitValue)
{
	if (script == NULL)
		return;
	if (evt->mData.mScript != NULL && evt->mOwner == 0) {
		s32 slot = FindFreeQueueSlot();
		if (slot >= 0) {
			gEventScriptQueue.mScript[slot]   = script;
			gEventScriptQueue.mContext[slot]  = context;
			gEventScriptQueue.mUseValue[slot] = useValue;
			gEventScriptQueue.mHitValue[slot] = hitValue;
		}
		return;
	}
	ClearScript(evt);
	if (evt->mOwner == 0)
		ClearQueue();
	ClearResults(evt);
	evt->mData.mScript  = script;
	evt->mData.mContext = evt->mOwner != 0 ? evt->mOwner : context;
	evt->mUseValue      = useValue;
	evt->mHitValue      = hitValue;
}

/**
 * @note Address: 0x8007F038
 * @note Size: 0x8
 */
extern "C" BOOL LoadEventResources(CEvt*)
{
	return TRUE;
}

/**
 * @note Address: 0x8007F040
 * @note Size: 0x154
 */
extern "C" void ResetEventScriptState(CEvt* evt, BOOL clearTimers, BOOL clearGlobal)
{
	ClearScript(evt);
	if (evt->mOwner == 0) {
		if (clearTimers) {
			for (u32 i = 0; i < ARRAY_SIZE(gEventTimers.mFrames); ++i) {
				gEventTimers.mFrames[i] = 0;
				gEventTimers.mValue[i]  = 0;
			}
		}
		ClearQueue();
		if (clearGlobal)
			gEventObjectIds.mCount = 0;
	}
	ClearResults(evt);
}

static inline void ClearTimers()
{
	for (u32 i = 0; i < ARRAY_SIZE(gEventTimers.mFrames); ++i) {
		gEventTimers.mFrames[i] = 0;
		gEventTimers.mValue[i]  = 0;
	}
}

/**
 * @note Address: 0x8007F194
 * @note Size: 0x3DC
 */
extern "C" void InitializeEventScriptContext(CEvt* evt, CMsg* msg, XObj* owner)
{
	evt->mOwner = reinterpret_cast<u32>(owner);
	evt->mMsg   = msg;
	if (evt->mOwner == 0) {
		gEventFlags.SetAll(false);
		for (u32 i = 0; i < 2048; ++i)
			gEventVars.mVars[i] = 0;
		ClearQueue();
		ClearTimers();
		gEventScriptObject.XObj::InitializeObject(0);
	}
	AllocateWordBuffer(&evt->mCalls, 32);
	AllocateWordBuffer(&evt->mFrames, 64);
	AllocateWordBuffer(&evt->mValues, 128);
	evt->mCameraMemento = NULL;
	ClearScript(evt);
	if (evt->mOwner == 0) {
		ClearTimers();
		ClearQueue();
		gEventObjectIds.mCount = 0;
	}
	ClearResults(evt);
}

static inline XObj* NamedObject(s16 id)
{
	for (s32 i = 0; i < gEventObjectIds.mCount; ++i) {
		if (gEventObjectIds.mEntry[i].mId == id) {
			XObj* object = gGame.GetObject(gEventObjectIds.mEntry[i].mIndex);
			if (object != NULL && (object->mStatus.mFlags & 1ULL))
				return object;
			return NULL;
		}
	}
	return NULL;
}

/**
 * @note Address: 0x8007F570
 * @note Size: 0xF4
 */
extern "C" XObj* ResolveEventObjectId(const CEvtData* context, s16 id)
{
	if (id >= 10000) {
		return NamedObject(id);
	}
	if (id >= 0)
		return gGame.GetObject(id);
	if (id == -1)
		return reinterpret_cast<XObj*>(context->mContext);
	return reinterpret_cast<XObj*>(static_cast<s32>(id));
}
