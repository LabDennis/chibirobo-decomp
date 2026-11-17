// The event script runner, .text 0x8009453C-0x800A18B8: its .rodata names it "evt.cpp" (wii_units.md Nr. 38),
// the GameCube's evt.cpp. The functions come in the reverse of the GameCube's order.
// NONMATCHING: only the small commands and the script control have source so far.
#include "game/wii/ceffect.h"
#include "game/wii/cgame.h"
#include "game/evt.h"
#include "game/evtqueue.h"
#include "game/globals.h"
#include "game/ut.h"
#include "game/cthread.h"
#include "Dolphin/os.h"

extern "C" {
extern s32 gRefreshRate;
extern f32 gScreenWidthScale;
void StartGameMovie(CGame* game, const char* path, BOOL loop, u32 flags, f32 volume);
void StopGameMovie(CGame* game);
}

CEvtFlags gEventFlags;
CEvtVars gEventVars;
CEvtQueueEx gEventScriptQueue;
CEvtTimer gEventTimers;
// the scripts' own effects, the second set next to CGame's
CEvtPtcl gEventPtcl;
EvtObjIdTable gEventObjectIds = { 0 };
// the event system's own object
XObj gEventScriptObject;

extern "C" s32 CEvt_Evaluate(CEvt* evt);
extern "C" void CEvt_EvalSet(CEvt* evt);

/**
 * @note Address: 0x8009F7E0
 * @note Size: 0x9C
 */
s32 CEvt::_cmd_TIMER(u8, s32)
{
	// first arg is in seconds
	s32 frames = CEvt_Evaluate(this) * gRefreshRate;
	s32 script = CEvt_Evaluate(this);
	// a count of 0 marks a free slot that never fires
	if (frames == 0) {
		frames = 1;
	}
	for (s32 i = 0; i < 10; i++) {
		if (gEventTimers.mFrames[i] == 0) {
			gEventTimers.mFrames[i] = frames;
			gEventTimers.mValue[i]  = script;
			break;
		}
	}
	return 0;
}

/**
 * @note Address: 0x8009D5A4
 * @note Size: 0x20
 */
s32 CEvt::_cmd_PRINTF(u8, s32)
{
	// the print itself is compiled out, only the string is skipped
	u8* str;
	u8 c;
	do {
		str           = mData.mScript;
		c             = *str;
		mData.mScript = str + 1;
	} while (c != 0);
	return 0;
}

/**
 * @note Address: 0x8009CE98
 * @note Size: 0x160
 */
s32 CEvt::_cmd_MOVIE(u8, s32)
{
	const char* path = (const char*)CEvt_Evaluate(this);
	if (path != NULL) {
		// the script's rect is in 4:3 screen units, the width scales to the tv's aspect
		f32 x     = gScreenWidthScale * CEvt_Evaluate(this);
		f32 y     = CEvt_Evaluate(this);
		f32 w     = CEvt_Evaluate(this);
		f32 h     = CEvt_Evaluate(this);
		s32 flags = CEvt_Evaluate(this);
		// x and y are the center, the rect wants the corner
		x -= w / 2;
		y -= h / 2;
		StartGameMovie(&gGame, path, FALSE, flags, 1.0f);
		gGame.mMovieRect[0] = x;
		gGame.mMovieRect[1] = y;
		gGame.mMovieRect[2] = w;
		gGame.mMovieRect[3] = h;
	} else {
		StopGameMovie(&gGame);
	}
	return 0;
}

s32 CEvt::_cmd_SETSP(u8, s32)
{
	s32 value                      = CEvt_Evaluate(this);
	mValues.mBuf[mValues.mCount++] = value;
	return 0;
}

s32 CEvt::_cmd_RUN(u8, s32)
{
	mCalls.mBuf[mCalls.mCount++] = (s32)(mData.mScript + 4);
	mData.mScript                = ResolveEventScriptOffset(mData.mScript);
	return 0;
}

s32 CEvt::_cmd_LIB(u8, s32)
{
	// like RUN but the target is a 2 byte index into the library table
	mCalls.mBuf[mCalls.mCount++] = (s32)(mData.mScript + 2);
	u8* p                        = mData.mScript;
	s16 idx                      = p[0] + (p[1] << 8);
	mData.mScript                = p + 2;
	mData.mScript                = GetGlobalScriptByIndex(idx);
	return 0;
}

s32 CEvt::_cmd_GOTO(u8, s32)
{
	mData.mScript = ResolveEventScriptOffset(mData.mScript);
	return 0;
}

s32 CEvt::_cmd_POPBP(u8, s32)
{
	mValues.mCount = mFrames.mBuf[--mFrames.mCount];
	return 0;
}

s32 CEvt::_cmd_PUSHBP(u8, s32)
{
	mFrames.mBuf[mFrames.mCount++] = mValues.mCount;
	return 0;
}

static bool EvalCmp(CEvt* evt);

/**
 * @note Address: 0x800974E0
 * @note Size: 0x54
 */
s32 CEvt::_cmd_IF(u8, s32)
{
	// a true test skips the 4 byte jump offset, a false one takes it
	if (EvalCmp(this)) {
		mData.mScript += 4;
	} else {
		mData.mScript = ResolveEventScriptOffset(mData.mScript);
	}
	return 0;
}

// any other op leaves result alone
static inline void Compare(BOOL& result, s32 opcode, s32 lhs, s32 rhs)
{
	u8 op = opcode;
	if (op == EVT_EXPR_EQUAL) {
		result = lhs == rhs;
	}
	if (op == EVT_EXPR_NOT_EQUAL) {
		result = lhs != rhs;
	}
	if (op == EVT_EXPR_LESS) {
		result = lhs < rhs;
	}
	if (op == EVT_EXPR_LESS_EQUAL) {
		result = lhs <= rhs;
	}
	if (op == EVT_EXPR_GREATER) {
		result = lhs > rhs;
	}
	if (op == EVT_EXPR_GREATER_EQUAL) {
		result = lhs >= rhs;
	}
}

/**
 * @note Address: 0x80097324
 * @note Size: 0x198
 */
static bool EvalCmp(CEvt* evt)
{
	BOOL result = false;
	u8 op       = *evt->mData.mScript;

	switch (op) {
	// anything that isn't a comparison is true when nonzero
	case EVT_EXPR_NOT:
	case EVT_EXPR_ADD:
	case EVT_EXPR_SUB:
	case EVT_EXPR_MUL:
	case EVT_EXPR_DIV:
	case EVT_EXPR_MOD:
	case EVT_EXPR_AND:
	case EVT_EXPR_OR:
	case EVT_EXPR_XOR:
	case EVT_EXPR_IMM16:
	case EVT_EXPR_IMM32:
	case EVT_EXPR_ADDRESS_OF:
	case EVT_EXPR_STACK:
	case EVT_EXPR_PARENT_STACK:
	case EVT_EXPR_FLAG:
	case EVT_EXPR_VAR:
	case EVT_EXPR_RESULT1:
	case EVT_EXPR_RESULT2:
	case EVT_EXPR_PAD:
	case EVT_EXPR_BATTERY:
	case EVT_EXPR_MONEY:
	case EVT_EXPR_ITEM:
	case EVT_EXPR_ATC:
	case EVT_EXPR_RANK:
	case EVT_EXPR_EXP:
	case EVT_EXPR_LEVEL:
	case EVT_EXPR_HOLD:
	case EVT_EXPR_MAP:
	case EVT_EXPR_ACTOR_NAME:
	case EVT_EXPR_ITEM_NAME:
	case EVT_EXPR_TIME:
	case EVT_EXPR_CURRENT_SUIT:
	case EVT_EXPR_SCRAP:
	case EVT_EXPR_CURRENT_ATC:
	case EVT_EXPR_USE:
	case EVT_EXPR_HIT:
	case EVT_EXPR_STICKER_NAME:
	case EVT_EXPR_OBJ:
	case EVT_EXPR_RANDOM:
	case EVT_EXPR_SIN:
	case EVT_EXPR_COS:
	case EVT_EXPR_ARRAY_ELEMENT:
		result = CEvt_Evaluate(evt) != 0;
		break;
	case EVT_EXPR_EQUAL:
	case EVT_EXPR_NOT_EQUAL:
	case EVT_EXPR_LESS:
	case EVT_EXPR_LESS_EQUAL:
	case EVT_EXPR_GREATER:
	case EVT_EXPR_GREATER_EQUAL: {
		evt->mData.mScript++;
		// the operand read second is the left side of the test
		s32 rhs = CEvt_Evaluate(evt);
		s32 lhs = CEvt_Evaluate(evt);
		Compare(result, op, lhs, rhs);
		break;
	}
	default:
		OSPanic(__FILE__, 1388, FormatToStaticBuffer("Unknown EvalCmp op - %d\n", op));
		break;
	}

	return result;
}

s32 CEvt::_cmd_SET(u8, s32)
{
	CEvt_EvalSet(this);
	return 0;
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
	for (u32 i = 0; i < 128; ++i) {
		gEventScriptQueue.mScript[i] = NULL;
	}
}

static inline void ClearTimers()
{
	for (u32 i = 0; i < 10; ++i) {
		gEventTimers.mFrames[i] = 0;
		gEventTimers.mValue[i]  = 0;
	}
}

/**
 * @note Address: 0x80094E90
 * @note Size: 0x4
 */
extern "C" void CEvt_Draw(CEvt* evt)
{
}

/**
 * @note Address: 0x80094C20
 * @note Size: 0x270
 */
// NONMATCHING: the timer loop's counters sit in other registers (99.3 %)
extern "C" void CEvt_Update(CEvt* evt, s32 step)
{
	if (evt->mData.mScript != NULL) {
		// the Wii's table is indexed by the opcode
		s32 result = 0;
		while (result == 0) {
			s32 op = *evt->mData.mScript;
			evt->mData.mScript++;
			u8 command = op;
			result     = (evt->*evtCmdTable[command].handler)(command, step);
		}
	}
	if (evt->mOwner == NULL && !gGame.GetStatusFlags(0xC00)) {
		if (evt->mData.mScript == NULL) {
			for (s32 i = 0; i < 10; i++) {
				if (gEventTimers.mFrames[i] > 0) {
					if ((gEventTimers.mFrames[i] -= step) <= 0) {
						u8* script = (u8*)gEventTimers.mValue[i];
						s32 j      = 0;
						s32 cursor = gEventScriptQueue.mRead;
						for (; j < 128; cursor++, j++) {
							s32 index = cursor % 128;
							if (gEventScriptQueue.mScript[index] == NULL) {
								gEventScriptQueue.mScript[index] = script;
								break;
							}
						}
						gEventTimers.mFrames[i] = 0;
						gEventTimers.mValue[i]  = 0;
					}
				}
			}
		}
		if (evt->mData.mScript == NULL) {
			CEvtQueueEx* queue    = &gEventScriptQueue;
			s32 index             = queue->mRead;
			u8* script            = queue->mScript[index];
			XObj* owner           = queue->mContext[index];
			s16 useValue          = queue->mUseValue[index];
			s16 hitValue          = queue->mHitValue[index];
			queue->mScript[index] = NULL;
			queue->mRead          = (queue->mRead + 1) % 128;
			if (script != NULL) {
				StartOrQueueEventScript(evt, script, owner, useValue, hitValue);
			}
		}
	}
}

/**
 * @note Address: 0x80094B0C
 * @note Size: 0x114
 */
// NONMATCHING: retail keeps the slot index and the slot's address in each other's registers (99.5 %)
extern "C" void StartOrQueueEventScript(CEvt* evt, u8* script, XObj* owner, s16 useValue, s16 hitValue)
{
	if (script == NULL)
		return;
	if (evt->mData.mScript != NULL && evt->mOwner == NULL) {
		s32 cursor = gEventScriptQueue.mRead;
		for (s32 i = 0; i < 128; ++i) {
			s32 index = cursor % 128;
			if (gEventScriptQueue.mScript[index] == NULL) {
				gEventScriptQueue.mScript[index]   = script;
				gEventScriptQueue.mContext[index]  = owner;
				gEventScriptQueue.mUseValue[index] = useValue;
				gEventScriptQueue.mHitValue[index] = hitValue;
				return;
			}
			++cursor;
		}
		return;
	}
	ResetEventScriptState(evt, FALSE, FALSE, FALSE, FALSE);
	evt->mData.mScript = script;
	if (evt->mOwner != NULL) {
		evt->mData.mContext = evt->mOwner;
	} else {
		evt->mData.mContext = owner;
		// the object hears that a script takes it over
		if (owner != NULL)
			owner->OnMoveRequest();
	}
	evt->mUseValue = useValue;
	evt->mHitValue = hitValue;
}

/**
 * @note Address: 0x80094B04
 * @note Size: 0x8
 */
extern "C" BOOL LoadEventResources(CEvt* evt)
{
	return TRUE;
}

/**
 * @note Address: 0x80094904
 * @note Size: 0x200
 */
extern "C" void ResetEventScriptState(CEvt* evt, BOOL clearTimers, BOOL clearGlobal, BOOL clearEffects, BOOL clearQueue)
{
	ClearScript(evt);
	if (evt->mOwner == NULL) {
		if (clearTimers) {
			ClearTimers();
		}
		if (clearQueue) {
			ClearQueue();
		}
		if (clearEffects) {
			for (s32 i = 0; i < 0x80; i++) {
				EffectInstance* effect = (EffectInstance*)gEventPtcl.mEmitters[i];
				if (!effect->IsFinished()) {
					RetireEffect(&effect->handle, TRUE);
				}
			}
		}
		if (clearGlobal)
			gEventObjectIds.mCount = 0;
	}
	evt->mSelectResult  = 0;
	evt->mResult2       = 0;
	evt->mMsgState      = 0;
	evt->mUseValue      = -1;
	evt->mHitValue      = -1;
	evt->mCameraMemento = NULL;
}

// a stack's buffer from the game heap
static inline void AllocateWordBuffer(CStack* stack, s32 capacity)
{
	stack->mSize = capacity;
	stack->mBuf  = (s32*)MEMAllocFromExpHeapEx(gWiiGameHeap, capacity * 4, 4);
}

/**
 * @note Address: 0x80094614
 * @note Size: 0x2F0
 */
extern "C" void InitializeEventScriptContext(CEvt* evt, CMsg* message, XObj* owner)
{
	evt->mOwner = owner;
	evt->mMsg   = message;
	if (evt->mOwner == NULL) {
		for (u32 i = 0; i < 128; ++i)
			gEventFlags.mFlags[i] = 0;
		for (u32 i = 0; i < 2048; ++i)
			gEventVars.mVars[i] = 0;
		ClearQueue();
		ClearTimers();
		gEventScriptObject.InitializeObject();
	}
	AllocateWordBuffer(&evt->mCalls, 32);
	evt->mCalls.mCount = 0;
	AllocateWordBuffer(&evt->mFrames, 64);
	evt->mFrames.mCount = 0;
	AllocateWordBuffer(&evt->mValues, 128);
	evt->mValues.mCount = 0;
	evt->_64            = 0;
	ResetEventScriptState(evt, TRUE, TRUE, TRUE, TRUE);
}

// the object registered under a script name from 10000 up, NULL unless it's in use
static inline XObj* CEvt_FindObj(s16 id)
{
	for (s32 i = 0; i < gEventObjectIds.mCount; i++) {
		if (gEventObjectIds.mEntry[i].mId == id) {
			XObj* obj = gGame.mXObjs.mObjects[gEventObjectIds.mEntry[i].mIndex];
			if (obj != NULL && (obj->mStatus.mFlags & 1)) {
				return obj;
			}
			return NULL;
		}
	}
	return NULL;
}

/**
 * @note Address: 0x8009453C
 * @note Size: 0xD8
 */
extern "C" XObj* ResolveEventObjectId(const CEvtData* data, s16 id)
{
	if (id >= 10000) {
		XObj* object = CEvt_FindObj(id);
		if (object != NULL)
			return object;
		return NULL;
	}
	if (id >= 0)
		return gGame.mXObjs.mObjects[id];
	if (id == -1)
		return data->mContext;
	return (XObj*)(s32)id;
}

/**
 * @note Address: 0x800A10E4
 * @note Size: 0xC0
 */
extern "C" void RegisterEventObjectId(CEvt*, s16 id, s16 index)
{
	if (CEvt_FindObj(id) == NULL) {
		gEventObjectIds.mEntry[gEventObjectIds.mCount].mId    = id;
		gEventObjectIds.mEntry[gEventObjectIds.mCount].mIndex = index;
		++gEventObjectIds.mCount;
	}
}
