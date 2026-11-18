// The event script runner, .text 0x8009453C-0x800A18B8: its .rodata names it "evt.cpp" (wii_units.md Nr. 38),
// the GameCube's evt.cpp. The functions come in the reverse of the GameCube's order.
// NONMATCHING: the script control, the queue, the effect slots and the small commands have source; the
// expression evaluator and the big commands (CALL, CAMERA, WAIT, SCRN, ...) do not yet.
#include "nw4r/ef/ef_resource.h"
#include "game/wii/ceffect.h"
#include "game/wii/cgame.h"
#include "game/evt.h"
#include "game/evtqueue.h"
#include "game/coordinfo.h"
#include "game/inventory.h"
#include "game/savedata.h"
#include "game/globals.h"
#include "game/ut.h"
#include "game/cthread.h"
#include "Dolphin/os.h"
#include <string.h>

extern "C" {
extern s32 gRefreshRate;
extern f32 gScreenWidthScale;
void StartGameMovie(CGame* game, const char* path, BOOL loop, u32 flags, f32 volume);
void StopGameMovie(CGame* game);
void ChangeGameStage(CGame* game, s32 stage, BOOL showName);
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

// ids from 10000 up are script names, 0 and up slots in gGame.mXObjs, -1 the script's own object, anything lower a pointer
static inline XObj* CEvt_GetObj(CEvt* evt, s16 id)
{
	if (id >= 10000) {
		XObj* obj = CEvt_FindObj(id);
		// redundant, but it's in the asm
		if (obj == NULL) {
			return NULL;
		}
		return obj;
	}
	if (id >= 0) {
		return gGame.mXObjs.mObjects[id];
	}
	if (id == -1) {
		return evt->mData.mContext;
	}
	return (XObj*)id;
}

static inline s16 CEvt_FindObjIndex(s16 id)
{
	for (s32 i = 0; i < gEventObjectIds.mCount; i++) {
		if (gEventObjectIds.mEntry[i].mId == id) {
			return gEventObjectIds.mEntry[i].mIndex;
		}
	}
	return -1;
}

static inline void CEvt_ForgetObjId(s16 id)
{
	for (s32 i = 0; i < gEventObjectIds.mCount; i++) {
		if (gEventObjectIds.mEntry[i].mId == id) {
			for (s32 j = i; j < gEventObjectIds.mCount; j++) {
				gEventObjectIds.mEntry[j].mId    = gEventObjectIds.mEntry[j + 1].mId;
				gEventObjectIds.mEntry[j].mIndex = gEventObjectIds.mEntry[j + 1].mIndex;
			}
			gEventObjectIds.mCount--;
		}
	}
}

// the warning print is compiled out, only its length check is left
static inline void CEvt_Warn(const char* msg)
{
	strlen(msg);
}

// ends the running script; the stage's own effects end with the main script
static inline void CEvt_End(CEvt* evt)
{
	evt->mData.mScript = NULL;
	// a window MSG left up for STAY is idle again
	if (evt->mMsg != NULL && evt->mMsgState != 0) {
		evt->mMsgState   = 0;
		evt->mMsg->mIdle = 1;
	}
	Inventory* inventory = &gGame.GetPlayer()->mParams.mInventory;
	inventory->mCompact  = TRUE;
	CompactInventory(inventory);
	if (evt->mOwner == NULL) {
		if (gGame.mEffects.mResources[1].mNumEmitter != 0) {
			for (s32 i = 0; i < 0x80; i++) {
				EffectInstance* effect = (EffectInstance*)gEventPtcl.mEmitters[i];
				if (!effect->IsFinished() && effect->group == 3) {
					RetireEffect(&effect->handle, TRUE);
				}
			}
			gGame.mEffects.Unload(1);
		}
	} else {
		evt->mOwner->_9A4 = 0;
	}
	evt->mData.mScript = NULL;
}

/**
 * @note Address: 0x800A0B0C
 * @note Size: 0x19C
 */
s32 CEvt::_cmd_WARP(u8, s32)
{
	s32 stage    = CEvt_Evaluate(this);
	s32 showName = CEvt_Evaluate(this);
	// 10001 and 10002 are taken as is, 100 and up counts from the current stage
	if (stage != 10001 && stage != 10002 && stage >= 100) {
		stage += GetStageNum() - 99;
	}
	ChangeGameStage(&gGame, stage, showName);
	CEvt_End(this);
	return 1;
}

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
 * @note Address: 0x8009BC78
 * @note Size: 0x2F0
 */
// NONMATCHING: the string sits elsewhere in the file's pool; the pool is only right once the whole file is
s32 CEvt::_cmd_KILL(u8, s32)
{
	s16 id    = CEvt_Evaluate(this);
	XObj* obj = CEvt_GetObj(this, id);
	if (obj != NULL && (obj->mStatus.mFlags & 1)) {
		XObj* held = gGame.GetPlayer()->mLift.mTarget;
		if (held != NULL && obj == held && (obj->mStatus.mFlags & 0x0000400000000000)) {
			// chibi is carrying it, can't kill that
			CEvt_Warn(FormatToStaticBuffer("kill(%d) ... obj(%d) lifted!", id, id));
		} else {
			obj->Release();
			CEvt_ForgetObjId(id);
		}
	} else {
		// not spawned yet, mark its stage entry so it never gets read
		s16 index;
		if (id >= 10000) {
			index = CEvt_FindObjIndex(id);
		} else {
			index = id;
		}
		CEvt_ForgetObjId(id);
		UnkStageObj* entry = GetStagePlacementByIndex(index);
		entry->mFlags |= 0x80000000ULL;
	}
	return 0;
}

/**
 * @note Address: 0x8009BA88
 * @note Size: 0x1F0
 */
// NONMATCHING: the string sits elsewhere in the file's pool; the pool is only right once the whole file is
s32 CEvt::_cmd_DISP(u8, s32)
{
	s16 id    = CEvt_Evaluate(this);
	XObj* obj = CEvt_GetObj(this, id);
	u8 on     = CEvt_Evaluate(this);
	if (obj != NULL && (obj->mStatus.mFlags & 1)) {
		XObj* held = gGame.GetPlayer()->mLift.mTarget;
		if (held != NULL && obj == held && (obj->mStatus.mFlags & 0x0000400000000000)) {
			// chibi is carrying it, leave it be
			CEvt_Warn(FormatToStaticBuffer("disp(%d, %d) ... obj(%d) lifted!", id, on, id));
		} else if (on) {
			obj->mStatus.mFlags |= 0x10000;
		} else {
			obj->mStatus.mFlags &= ~0x10000;
		}
	} else {
		CEvt_Warn(FormatToStaticBuffer("disp(%d, %d) ... obj(%d) killed!", id, on, id));
	}
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

static BOOL EvalCmp(CEvt* evt);

/**
 * @note Address: 0x800978E4
 * @note Size: 0x1EC
 */
s32 CEvt::_cmd_ABORT(u8, s32)
{
	if (mOwner == NULL) {
		CEvtQueueEx* queue  = &gEventScriptQueue;
		s32 idx             = queue->mRead;
		u8* script          = queue->mScript[idx];
		XObj* owner         = queue->mContext[idx];
		s16 useValue        = queue->mUseValue[idx];
		s16 hitValue        = queue->mHitValue[idx];
		queue->mScript[idx] = 0;
		queue->mRead        = (queue->mRead + 1) % 0x80;
		if (script != 0) {
			mData.mScript  = script;
			mData.mContext = owner;
			if (owner != NULL) {
				owner->OnMoveRequest();
			}
			mUseValue = useValue;
			mHitValue = hitValue;
			return 0;
		}
	}
	CEvt_End(this);
	return 1;
}

/**
 * @note Address: 0x800976C8
 * @note Size: 0x21C
 */
s32 CEvt::_cmd_RETURN(u8, s32)
{
	if (mCalls.mCount != 0) {
		mData.mScript = (u8*)mCalls.mBuf[--mCalls.mCount];
		return 0;
	}
	// nothing to return to, end the script like ABORT
	if (mOwner == NULL) {
		CEvtQueueEx* queue  = &gEventScriptQueue;
		s32 idx             = queue->mRead;
		u8* script          = queue->mScript[idx];
		XObj* owner         = queue->mContext[idx];
		s16 useValue        = queue->mUseValue[idx];
		s16 hitValue        = queue->mHitValue[idx];
		queue->mScript[idx] = 0;
		queue->mRead        = (queue->mRead + 1) % 0x80;
		if (script != 0) {
			mData.mScript  = script;
			mData.mContext = owner;
			if (owner != NULL) {
				owner->OnMoveRequest();
			}
			mUseValue = useValue;
			mHitValue = hitValue;
			return 0;
		}
	}
	CEvt_End(this);
	return 1;
}

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
// NONMATCHING: the string sits elsewhere in the file's pool; the pool is only right once the whole file is
static BOOL EvalCmp(CEvt* evt)
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

// parks a script in the first free slot of the ring, from the read slot on
static inline void EvtQueue_Push(u8* script, XObj* owner, s16 useValue, s16 hitValue)
{
	s32 cursor = gEventScriptQueue.mRead;
	for (s32 i = 0; i < 128; cursor++, i++) {
		s32 index = cursor % 128;
		if (gEventScriptQueue.mScript[index] == NULL) {
			gEventScriptQueue.mScript[index]   = script;
			gEventScriptQueue.mContext[index]  = owner;
			gEventScriptQueue.mUseValue[index] = useValue;
			gEventScriptQueue.mHitValue[index] = hitValue;
			break;
		}
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
extern "C" void StartOrQueueEventScript(CEvt* evt, u8* script, XObj* owner, s16 useValue, s16 hitValue)
{
	if (script == NULL)
		return;
	if (evt->mData.mScript != NULL && evt->mOwner == NULL) {
		EvtQueue_Push(script, owner, useValue, hitValue);
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

/**
 * @note Address: 0x800A11A4
 * @note Size: 0x154
 */
// starts emitter id in the first free slot of effects, NULL when none is free or the id is unknown
extern "C" EffectInstance* fn_800A11A4(EffectInstance* effects, u32 id, s32 group)
{
	EffectInstance* effect = effects;
	for (s32 i = 0; i < 0x80; i++, effect++) {
		if (effect->IsFinished()) {
			s32 kind;
			// an id from 10000 up counts from the stage's own emitters, behind the common ones
			if (id >= 10000) {
				id   = id + gGame.mEffects.mResources[0].mNumEmitter - 10000;
				kind = 3;
			} else {
				kind = 2;
			}
			if (group >= 0) {
				kind = group;
			}
			u32 count = nw4r::ef::Resource::GetInstance() != NULL ? nw4r::ef::Resource::GetInstance()->NumEmitter(NULL) : 0;
			if (id >= count) {
				// the warning that printed the count is compiled out
				if (nw4r::ef::Resource::GetInstance() != NULL) {
					nw4r::ef::Resource::GetInstance()->NumEmitter(NULL);
				}
				return NULL;
			}
			if (CreateEffectInstance(effect, id, kind == 1 ? kind : 0)) {
				effects[i].id    = id;
				effects[i].group = kind;
				effects[i]._70   = 0;
				return &effects[i];
			}
		}
	}
	return NULL;
}

/**
 * @note Address: 0x800A12F8
 * @note Size: 0xD8
 */
// retires every running effect of emitter id
extern "C" void fn_800A12F8(EffectInstance* effects, u32 id, BOOL retireParticles)
{
	if (id >= 10000) {
		id = id + gGame.mEffects.mResources[0].mNumEmitter - 10000;
	}
	EffectInstance* effect = effects;
	for (s32 i = 0; i < 0x80; i++, effect++) {
		if (!effect->IsFinished() && id == effect->id) {
			RetireEffect(&effect->handle, retireParticles);
		}
	}
}

/**
 * @note Address: 0x800A13D0
 * @note Size: 0xD8
 */
// NONMATCHING: retail loads the three words of the interest before it stores them (88.7 %)
// steps the running effects' matrices under camera
extern "C" void fn_800A13D0(EffectInstance* effects, CCamera* camera)
{
	CVec interest          = camera->mView.mInterest;
	EffectInstance* effect = effects;
	for (s32 i = 0; i < 0x80; i++, effect++) {
		if (!effect->IsFinished()) {
			if (effect->mtx != NULL) {
				UpdateEffectMtx(effect);
			}
			if (effect->group != 3) {
				// the distance check that culled far effects is compiled out
				PSVECDistance(&interest, &effect->pos);
			}
		}
	}
}
