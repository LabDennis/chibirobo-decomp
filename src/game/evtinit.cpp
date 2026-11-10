#include "game/evtqueue.h"
#include "game/xobj.h"

// allocates a stack's buffer
extern "C" void AllocateWordBuffer(CStack* stack, s32 capacity);

static inline void ClearQueue()
{
	gEventScriptQueue.mRead = 0;
	for (u32 i = 0; i < 128; ++i)
		gEventScriptQueue.mScript[i] = NULL;
}

static inline void ClearTimers()
{
	for (u32 i = 0; i < 10; ++i) {
		gEventTimers.mFrames[i] = 0;
		gEventTimers.mValue[i]  = 0;
	}
}

/**
 * @note Address: 0x8007F194
 * @note Size: 0x3DC
 */
extern "C" void InitializeEventScriptContext(CEvt* evt, u8* context, void* owner)
{
	evt->_00 = (u32)owner;
	evt->_54 = context;
	if (evt->_00 == 0) {
		for (u32 i = 0; i < 128; ++i)
			gEventFlagWords[i] = 0;
		for (u32 i = 0; i < 2048; ++i)
			gEventVariables[i] = 0;
		ClearQueue();
		ClearTimers();
		InitializeObject__4XObjFUl(&gEventScriptObject, 0);
	}
	AllocateWordBuffer(&evt->mCalls, 32);
	AllocateWordBuffer(&evt->mFrames, 64);
	AllocateWordBuffer(&evt->mValues, 128);
	evt->mCameraMemento = NULL;
	evt->mData.mScript  = NULL;
	evt->_18            = 0;
	evt->_1C            = 0;
	evt->_20            = 0;
	evt->mCalls.mCount  = 0;
	evt->mFrames.mCount = 0;
	evt->mValues.mCount = 0;
	if (evt->_00 == 0) {
		ClearTimers();
		ClearQueue();
		gEventObjectIds.mCount = 0;
	}
	evt->_10       = 0;
	evt->_14       = 0;
	evt->_58       = 0;
	evt->mUseValue = -1;
	evt->mHitValue = -1;
}
