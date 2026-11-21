// The event script runner, .text 0x8009453C-0x800A18B8: its .rodata names it "evt.cpp" (wii_units.md Nr. 38),
// the GameCube's evt.cpp. The functions come in the reverse of the GameCube's order.
// NONMATCHING: the script control, the queue, the effect slots and the small commands have source; the
// expression evaluator and the big commands (CALL, CAMERA, WAIT, SCRN, ...) do not yet.
#include "nw4r/ef/ef_resource.h"
#include "game/wii/xobjflags.h" // ahead of cgame.h: its AnimSfx needs the real nw4r::math, before xobj.h's shim
#include "game/wii/ceffect.h"
#include "game/wii/cgame.h"
#include "game/wii/csound.h"
#include "game/wii/cpad.h"
#include "game/wii/cpadact.h"
#include "game/wii/xobjs.h"
#include "game/evt.h"
#include "game/evtqueue.h"
#include "game/coordinfo.h"
#include "game/stagedata.h"
#include "game/inventory.h"
#include "game/attachmentvalues.h"
#include "game/savedata.h"
#include "game/globals.h"
#include "game/ut.h"
#include "game/gmath.h"
#include "game/cthread.h"
#include "game/cfile.h"
#include "Dolphin/os.h"
#include <string.h>
#include <stdlib.h>

extern "C" {
extern s32 gRefreshRate;
extern f32 gScreenWidthScale;
extern s32 gScreenSafeBottom;
extern s32 gPointerScreenWidth;
void StartGameMovie(CGame* game, const char* path, BOOL loop, u32 flags, f32 volume);
void StopGameMovie(CGame* game);
void ChangeGameStage(CGame* game, s32 stage, BOOL showName);
void fn_80084B90(void* menu, s32 index); // opens one of the menus
void SelectPlayerEquipmentRing(WiiPlayerState* chr, s32 selection);
void ClosePlayerEquipmentMenu(WiiPlayerState* chr, s32 result);
void ResetPlayerHeadOffset(WiiPlayerState* chr);
void ResetPlayerPlug(WiiPlayerState* chr);
void UpdateHeldObject(XAction* action, u32 frames);
void CancelPlayerPushAction(XAction* action);
void UpdateObjectFloorHeights(XObj* object);
void UpdateObjectFloorXZExtent(XObj* object);
void ClearActiveObjectFloors(XObj* object);
BOOL fn_800AF098(XObj* obj, const char* path); // reads an object's animation file
void fn_800AF12C(XObj* obj);                   // drops the animation file READ gave it
void fn_800C807C(XObj* obj);                   // a category 3 object's follow-up to a new anim
void AdjustPlayerMoney(WiiPlayerParams* params, s32 amount);
void AddPlayerScrap(WiiPlayerParams* params, s32 amount);
void AdjustPlayerExperience(WiiPlayerParams* params, s32 amount);
void AdjustPlayerLevel(WiiPlayerParams* params, s32 amount);
void AddPlayerRank(WiiPlayerParams* params, s32 amount);
s32 GetAttachmentCountById(AttachmentValues* values, s32 id); // below 0 when there is none
void AddMainMenuExtraRow(void* menu, s32 arg1, s32 arg2);
}

void OpenPlayerYesNoMenu(WiiPlayerState* chr, s32 crossMask, s32 selectedIndex);      // 0x800D2B50
void HandleOXMenuInput(WiiOXMenu* menu, CPad* pad);                                   // 0x800EF58C
BOOL LoadPlayerAttachmentAnimation(WiiPlayerState* player);                           // xobj_chr.cpp

#include "game/fbtexeff.h"

CEvtFlags gEventFlags;
CEvtVars gEventVars;
CEvtQueueEx gEventScriptQueue;
CEvtTimer gEventTimers;
// the scripts' own effects, the second set next to CGame's
CEvtPtcl gEventPtcl;
EvtObjIdTable gEventObjectIds = { 0 };
// the message window's color after WIN @RESET
GXColor sEvtWindowDefaultColor = { 0x00, 0x00, 0x00, 0x80 };
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

// the header only declares it: the body needs XObjFlags, which not every unit can afford to include
inline CEvtFlags& XOBJS::GetCollectionFlags(CollectionPool pool) const
{
	return *static_cast<XObjFlags*>(mPools[pool].First())->mFlags;
}

static inline s32 EvtRead16(const u8* p)
{
	return p[0] + (p[1] << 8);
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

// heading from one point to another, in radians
static inline f32 CEvt_Angle(const Vec& from, const Vec& to)
{
	return atan2(to.x - from.x, to.z - from.z);
}

// stops the running script, DETACH's whole job; the stage's own effects end with the main script
static inline void CEvt_Detach(CEvt* evt)
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
}

// WARP, ABORT and RETURN all finish through this
static inline void CEvt_End(CEvt* evt)
{
	CEvt_Detach(evt);
	evt->mData.mScript = NULL;
}

/**
 * @note Address: 0x800A0CC8
 * @note Size: 0x43C
 */
// NONMATCHING: @LETTERBOX's float registers and @RESET's message pointer register differ
s32 CEvt::_cmd_WIN(u8, s32)
{
	// an object's script waits while the main script runs
	if (mOwner != NULL && mOwner->_9A4 != 0 && gGame.mEvt.mData.mScript != NULL) {
		mData.mScript--;
		return 1;
	}
	switch ((u16)CEvt_Evaluate(this)) {
	case EVT_ATOM_POS: {
		f32 x = CEvt_Evaluate(this);
		f32 y = CEvt_Evaluate(this);
		if (mMsg != NULL) {
			mMsg->SetWindowPosition(x * gScreenWidthScale, y);
		}
		break;
	}
	case EVT_ATOM_LETTERBOX: {
		// centered, just above the bottom of the safe area
		f32 x = gPointerScreenWidth / 2;
		f32 y = 2.0f + ((gScreenSafeBottom - 357.0f) * 0.5f + 357.0f - 11.0f);
		if (mMsg != NULL) {
			mMsg->SetWindowPosition(x, y);
		}
		break;
	}
	case EVT_ATOM_OBJ: {
		s16 id    = CEvt_Evaluate(this);
		XObj* obj = CEvt_GetObj(this, id);
		f32 x     = CEvt_Evaluate(this) / 100.0f;
		f32 y     = CEvt_Evaluate(this) / 100.0f;
		f32 z     = CEvt_Evaluate(this) / 100.0f;
		if (obj != NULL && (obj->mStatus.mFlags & 1)) {
			if (mMsg != NULL) {
				mMsg->SetWindowAnchor((WiiTitleObject*)obj, x, y, z);
			}
		} else {
			CEvt_Warn(FormatToStaticBuffer("win(@OBJ, %d, ...) obj(%d) killed!", id, id));
		}
		break;
	}
	case EVT_ATOM_COLOR: {
		u8 r      = CEvt_Evaluate(this);
		u8 g      = CEvt_Evaluate(this);
		u8 b      = CEvt_Evaluate(this);
		u8 a      = CEvt_Evaluate(this);
		CMsg* msg = mMsg;
		if (msg != NULL) {
			msg->mFont.mColor.r = r;
			msg->mFont.mColor.g = g;
			msg->mFont.mColor.b = b;
			msg->mFont.mColor.a = a;
		}
		break;
	}
	case EVT_ATOM_RESET:
		if (mMsg != NULL) {
			CMsg_ResetPos(mMsg);
			GXColor color      = sEvtWindowDefaultColor;
			mMsg->mFont.mColor = color;
		}
		break;
	}
	return 0;
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
 * @note Address: 0x80097B10
 * @note Size: 0x360
 */
s32 CEvt::_cmd_ANIM(u8 command, s32)
{
	s16 id    = CEvt_Evaluate(this);
	XObj* obj = CEvt_GetObj(this, id);
	for (s32 count = 0;; count++) {
		s32 anim = CEvt_Evaluate(this);
		if (anim == -1) {
			break;
		}
		if (obj != NULL && (obj->mStatus.mFlags & 1)) {
			if (obj->mObject.m_pScnMdl == NULL) {
				CEvt_Warn(FormatToStaticBuffer("anim(%d, %d) ... obj(%d) is collision!", id, anim, id));
			} else if (count == 0) { // the first anim plays now, the ones after it are queued
				obj->ClearAnimQueue();
				obj->SetAnim(anim, FALSE, 0.0f, 1.0f, command == EVT_CMD_ANIM1 ? 1 : 0, 15.0f);
				obj->UpdateAnim(1.0f);
				if (obj->GetPool() == 3) {
					fn_800C807C(obj);
				}
			} else {
				obj->PushAnim(anim, 0, 100);
			}
		} else {
			CEvt_Warn(FormatToStaticBuffer("anim(%d, %d) ... obj(%d) killed!", id, anim, id));
		}
	}
	return 0;
}

/**
 * @note Address: 0x80097E70
 * @note Size: 0x378
 */
s32 CEvt::_cmd_ANIM2(u8, s32)
{
	s16 id    = CEvt_Evaluate(this);
	XObj* obj = CEvt_GetObj(this, id);
	for (s32 count = 0;; count++) {
		s32 anim = CEvt_Evaluate(this);
		if (anim == -1) {
			break;
		}
		u16 frame = CEvt_Evaluate(this);
		s16 rate  = CEvt_Evaluate(this);
		if (obj != NULL && (obj->mStatus.mFlags & 1)) {
			if (obj->mObject.m_pScnMdl == NULL) {
				CEvt_Warn(FormatToStaticBuffer("anim2(%d, %d,%d,%d) ... obj(%d) is collision!", id, anim, frame, rate, id));
			} else if (count == 0) {
				obj->ClearAnimQueue();
				obj->SetAnim(anim, FALSE, frame, rate / 100.0f, 0.0f, 15.0f);
				obj->UpdateAnim(1.0f);
			} else {
				obj->PushAnim(anim, frame, rate);
			}
		} else {
			CEvt_Warn(FormatToStaticBuffer("anim2(%d, %d,%d,%d) ... obj(%d) killed!", id, anim, frame, rate, id));
		}
	}
	return 0;
}

/**
 * @note Address: 0x800981D0
 * @note Size: 0x2D4
 */
s32 CEvt::_cmd_ATTACH(u8 command, s32)
{
	s16 id    = CEvt_Evaluate(this);
	XObj* obj = CEvt_GetObj(this, id);
	if (command == EVT_CMD_ATTACH) {
		u8* script = (u8*)CEvt_Evaluate(this);
		if (obj != NULL && (obj->mStatus.mFlags & 1)) {
			StartOrQueueEventScript(&obj->mEvt, script, mOwner, -1, -1);
		} else {
			CEvt_Warn(FormatToStaticBuffer("attach(%d, ...): obj(%d) killed!", id, id));
		}
	} else if (obj != NULL && (obj->mStatus.mFlags & 1)) {
		CEvt_Detach(&obj->mEvt);
	} else {
		CEvt_Warn(FormatToStaticBuffer("detach(%d, ...): obj(%d) killed!", id, id));
	}
	return 0;
}

/**
 * @note Address: 0x8009AE98
 * @note Size: 0x3CC
 */
s32 CEvt::_cmd_COLOR(u8, s32)
{
	s16 id    = CEvt_Evaluate(this);
	XObj* obj = CEvt_GetObj(this, id);
	u16 mode  = CEvt_Evaluate(this);
	u8 r      = CEvt_Evaluate(this);
	u8 g      = CEvt_Evaluate(this);
	u8 b      = CEvt_Evaluate(this);
	f32 time  = CEvt_Evaluate(this) / 100.0f;
	if (obj == (XObj*)-2) { // id -2: every object in use that has a model
		for (s32 i = 0; i < gGame.mXObjs.mObjectCount; i++) {
			XObj* current = gGame.mXObjs.mObjects[i];
			if ((current->mStatus.mFlags & 1) && current->mObject.m_pScnMdl != NULL) {
				if (mode == EVT_ATOM_MODULATE) {
					current->SetColor(0, r, g, b, time);
				} else if (mode == EVT_ATOM_BLEND) {
					current->SetColor(1, r, g, b, time);
				}
			}
		}
	} else if (obj != NULL && (obj->mStatus.mFlags & 1)) {
		if (obj->mObject.m_pScnMdl == NULL) {
			if (mode == EVT_ATOM_MODULATE) {
				CEvt_Warn(FormatToStaticBuffer("color(%d, @MODULATE, %d,%d,%d, %d) ... obj(%d) is collision!", id, r, g, b,
				                               (s32)(100.0f * time), id));
			} else {
				CEvt_Warn(FormatToStaticBuffer("color(%d, @BLEND, %d,%d,%d, %d) ... obj(%d) is collision!", id, r, g, b,
				                               (s32)(100.0f * time), id));
			}
		}
		if (mode == EVT_ATOM_MODULATE) {
			obj->SetColor(0, r, g, b, time);
		} else if (mode == EVT_ATOM_BLEND) {
			obj->SetColor(1, r, g, b, time);
		}
	} else {
		if (mode == EVT_ATOM_MODULATE) {
			CEvt_Warn(
			    FormatToStaticBuffer("color(%d, @MODULATE, %d,%d,%d, %d) ... obj(%d) killed!", id, r, g, b, (s32)(100.0f * time), id));
		} else {
			CEvt_Warn(FormatToStaticBuffer("color(%d, @BLEND, %d,%d,%d, %d) ... obj(%d) killed!", id, r, g, b, (s32)(100.0f * time), id));
		}
	}
	return 0;
}

// null when the pool is full
static inline XObj* CEvt_Born(s32 index)
{
	CoordInfo info(GetSpawnData(index));
	return SpawnObjectFromPlacement(&gGame.mXObjs, &info, 1, FALSE);
}

/**
 * @note Address: 0x80098534
 * @note Size: 0x430
 */
s32 CEvt::_cmd_BORN(u8, s32)
{
	s16 id    = CEvt_Evaluate(this);
	s16 type  = CEvt_Evaluate(this);
	f32 x     = CEvt_Evaluate(this) / 100.0f;
	f32 y     = CEvt_Evaluate(this) / 100.0f;
	f32 z     = CEvt_Evaluate(this) / 100.0f;
	s16 rot   = CEvt_Evaluate(this);
	f32 sx    = CEvt_Evaluate(this) / 100.0f;
	f32 sy    = CEvt_Evaluate(this) / 100.0f;
	f32 sz    = CEvt_Evaluate(this) / 100.0f;
	s32 arg   = CEvt_Evaluate(this);
	XObj* obj = CEvt_FindObj(id);
	if (obj == NULL) {
		obj = CEvt_Born(type + 0x16);
		if (obj != NULL) {
			gGame.SetStatusFlags(0x1000); // BORN raises it after a spawn
		}
	}
	if (obj != NULL && (obj->mStatus.mFlags & 1)) {
		obj->SetPos(x, y, z);
		obj->SetRotationY(DEG2RAD * rot);
		obj->SetScale(sx, sy, sz);
		UpdateObjectFloorHeights(obj);
		UpdateObjectFloorXZExtent(obj);
		obj->mEventScript = (const void*)arg;
		s16 index         = FindStageObjectIndex(&gGame.mXObjs, obj);
		if (CEvt_FindObj(id) == NULL) {
			gEventObjectIds.mEntry[gEventObjectIds.mCount].mId    = id;
			gEventObjectIds.mEntry[gEventObjectIds.mCount].mIndex = index;
			gEventObjectIds.mCount++;
		}
	} else {
		OSPanic(__FILE__, 1778, FormatToStaticBuffer("No more space for born(%d,...)-command!\n", id));
	}
	return 0;
}

/**
 * @note Address: 0x800988AC
 * @note Size: 0x1978
 */
// the GameCube's _cmd_CALL: runs one syscall on the target object. The Wii adds case -46 and
// reads the game status through CBase's virtuals instead of the flags member
s32 CEvt::_cmd_CALL(u8, s32)
{
	s32 yield  = 0;
	u8* start  = mData.mScript - 1;
	u8* begin  = mData.mScript;
	u16 length = EvtRead16(begin);
	mData.mScript += 2;
	u8* end      = begin + length;
	s16 id       = CEvt_Evaluate(this);
	XObj* target = CEvt_GetObj(this, id);
	s32 count    = 0;
	s32 args[24];
	for (s32 i = 0;; i++) {
		if (mData.mScript >= end)
			break;
		if (i < 24) {
			args[i] = CEvt_Evaluate(this);
			count++;
		}
	}

	switch ((s32)target) {
	case -10: { // keeps the object inside these limits around where it stands now
		XObj* obj = CEvt_GetObj(this, (s16)args[0]);
		f32 negZ = args[4], posZ = args[3], negX = args[2], posX = args[1];
		obj->mAnmQueue.mAnchorX   = obj->mMoveTranslate.x;
		obj->mAnmQueue.mAnchorZ   = obj->mMoveTranslate.z;
		obj->mAnmQueue.mLimitPosX = posX;
		obj->mAnmQueue.mLimitNegX = negX;
		obj->mAnmQueue.mLimitPosZ = posZ;
		obj->mAnmQueue.mLimitNegZ = negZ;
		break;
	}
	case -11: {
		s16 id    = args[0];
		s32 flags = args[1];
		if (flags == -1) {
			XObj* obj                   = CEvt_GetObj(this, id);
			obj->mAnmQueue.mMarkerOverride  = 0;
			obj->mAnmQueue.mMarkerOverride |= 0x80000000;
		} else {
			XObj* obj = CEvt_GetObj(this, id);
			obj->mAnmQueue.mMarkerOverride &= ~0x80000000;
			obj->mAnmQueue.mMarkerOverride |= flags;
		}
		break;
	}
	case -12:
	case -13:
	case -14:
	case -15:
		if (mYesNoActive != 0) {
			// OpenPlayerYesNoMenu sets 0x800, it clears once the marks have faded out after the answer
			if (gGame.GetStatusFlags(0x800)) {
				CPad pad(0);
				HandleOXMenuInput(&gGame.GetPlayer()->mOXMenu, &pad);
				mData.mScript = start;
				yield         = 1;
			} else {
				mYesNoActive  = 0;
				mSelectResult = gGame.GetPlayer()->mOXMenu.mConfirmedIndex;
			}
		} else {
			if (gGame.GetPlayer()->mAttachmentRequest < 0) {
				s32 crossMask = 2;
				if ((s32)target == -12)
					crossMask = 2;
				else if ((s32)target == -13)
					crossMask = 1;
				else if ((s32)target == -14)
					crossMask = 0;
				else if ((s32)target == -15)
					crossMask = 3;
				OpenPlayerYesNoMenu(gGame.GetPlayer(), crossMask, args[0]);
				mYesNoActive = 1;
			}
			mData.mScript = start;
			yield         = 1;
		}
		break;
	case -16:
		// the subwindow: 231 sets its rect, @CAM aims its camera and shows it, @RESET and 230 (the two
		// atoms that end the scripted camera in _cmd_CAMERA) hide it. The Wii scales the rect's x
		// by the screen's width scale, its centre is in pixels of the widescreen frame
		if (args[0] == 231) {
			gGame.mSubwin.mRectX      = args[1] * gScreenWidthScale;
			gGame.mSubwin.mRectY      = args[2];
			gGame.mSubwin.mRectWidth  = args[3];
			gGame.mSubwin.mRectHeight = args[4];
		} else if (args[0] == EVT_ATOM_CAM) {
			Vec pos;
			pos.x = args[1] / 100.0f;
			pos.y   = args[2] / 100.0f;
			pos.z   = args[3] / 100.0f;
			ConfigureSubwindowCamera(&gGame.mSubwin, pos, DEG2RAD * args[4], DEG2RAD * args[5], args[6], args[7]);
			gGame.mSubwin.mEnabled      = 1;
			gGame.mSubwin.mFlags.mFlags = 0;
		} else if (args[0] == EVT_ATOM_RESET || args[0] == 230) {
			gGame.mSubwin.mEnabled = 0;
		}
		break;
	// -17 to -19 set, clear and test an item's collection flag, -20 to -22 a coin's, -23 to -25 a dust pile's;
	// id -2 sets or clears all of them
	case -17: {
		s16 id = args[0];
		if (id > 0) {
			u32* bits = gGame.mXObjs.GetCollectionFlags(XOBJS::ITEMS).mFlags;
			bits[(u32)id >> 5] |= 1U << (id & 31);
		}
		if (id == -2) {
			u32* bits = gGame.mXObjs.GetCollectionFlags(XOBJS::ITEMS).mFlags;
			for (int i = 0; i < 128; i++)
				bits[i] = -1;
		}
		break;
	}
	case -18: {
		s16 id = args[0];
		if (id > 0) {
			u32* bits = gGame.mXObjs.GetCollectionFlags(XOBJS::ITEMS).mFlags;
			bits[(u32)id >> 5] &= ~(1U << (id & 31));
		}
		if (id == -2) {
			u32* bits = gGame.mXObjs.GetCollectionFlags(XOBJS::ITEMS).mFlags;
			for (int i = 0; i < 128; i++)
				bits[i] = 0;
		}
		break;
	}
	case -19: {
		s16 id        = args[0];
		mSelectResult = (gGame.mXObjs.GetCollectionFlags(XOBJS::ITEMS).mFlags[(u32)id >> 5] & (1U << (id & 31))) != 0;
		break;
	}
	case -20: {
		s16 id = args[0];
		if (id > 0) {
			u32* bits = gGame.mXObjs.GetCollectionFlags(XOBJS::COINS).mFlags;
			bits[(u32)id >> 5] |= 1U << (id & 31);
		}
		if (id == -2) {
			u32* bits = gGame.mXObjs.GetCollectionFlags(XOBJS::COINS).mFlags;
			for (int i = 0; i < 128; i++)
				bits[i] = -1;
		}
		break;
	}
	case -21: {
		s16 id = args[0];
		if (id > 0) {
			u32* bits = gGame.mXObjs.GetCollectionFlags(XOBJS::COINS).mFlags;
			bits[(u32)id >> 5] &= ~(1U << (id & 31));
		}
		if (id == -2) {
			u32* bits = gGame.mXObjs.GetCollectionFlags(XOBJS::COINS).mFlags;
			for (int i = 0; i < 128; i++)
				bits[i] = 0;
		}
		break;
	}
	case -22: {
		s16 id        = args[0];
		mSelectResult = (gGame.mXObjs.GetCollectionFlags(XOBJS::COINS).mFlags[(u32)id >> 5] & (1U << (id & 31))) != 0;
		break;
	}
	case -23: {
		s16 id = args[0];
		if (id > 0) {
			u32* bits = gGame.mXObjs.GetCollectionFlags(XOBJS::DUST).mFlags;
			bits[(u32)id >> 5] |= 1U << (id & 31);
		}
		if (id == -2) {
			u32* bits = gGame.mXObjs.GetCollectionFlags(XOBJS::DUST).mFlags;
			for (int i = 0; i < 128; i++)
				bits[i] = -1;
		}
		break;
	}
	case -24: {
		s16 id = args[0];
		if (id > 0) {
			u32* bits = gGame.mXObjs.GetCollectionFlags(XOBJS::DUST).mFlags;
			bits[(u32)id >> 5] &= ~(1U << (id & 31));
		}
		if (id == -2) {
			u32* bits = gGame.mXObjs.GetCollectionFlags(XOBJS::DUST).mFlags;
			for (int i = 0; i < 128; i++)
				bits[i] = 0;
		}
		break;
	}
	case -25: {
		s16 id        = args[0];
		mSelectResult = (gGame.mXObjs.GetCollectionFlags(XOBJS::DUST).mFlags[(u32)id >> 5] & (1U << (id & 31))) != 0;
		break;
	}
	case SYSCALL_SET_OBJSTAT: {
		s16 id    = args[0];
		s32 bit   = args[1];
		u64 mask  = (u64)1 << bit;
		XObj* obj = CEvt_GetObj(this, id);
		if (obj != NULL) {
			// the Wii learns the floor reflection here: the first time an object asks for it,
			// its floor data is refreshed so the reflection has something to stand on
			if (mask == 0x200000000000000ULL && !(obj->mStatus.mFlags & mask)) {
				obj->mStatus.mFlags |= mask;
				if ((obj->mStatus.mFlags & 1) == 1) {
					UpdateObjectFloorHeights(obj);
					UpdateObjectFloorXZExtent(obj);
					ClearActiveObjectFloors(obj);
				}
			} else {
				obj->mStatus.mFlags |= mask;
			}
			if (mask & 0x20000) {
				u32 flags[5];
				flags[0]     = obj->mStatus.mFlags & 0x800000;
				flags[1]     = obj->mStatus.mFlags & 0x200000;
				flags[2]     = obj->mStatus.mFlags & 0x40;
				flags[3]     = obj->mStatus.mFlags & 0x20000;
				flags[4]     = obj->mStatus.mFlags & 0x400000;
				obj->ApplyMaterialPresets(flags);
			}
			if (mask & ((u64)0x4000 << 32)) {
				WiiPlayerState* player = gGame.GetPlayer();
				if ((s16)obj->GetPool() == 8 && obj == player->mLift.mTarget) {
					s16 item = obj->GetObjFlags();
					switch (item) {
					case 16:
						player->mAttachment = 4;
						break;
					case 21:
						player->mAttachment = 5;
						break;
					case 22:
						player->mAttachment = 6;
						break;
					case 56:
						player->mAttachment = 7;
						break;
					}
					switch (item) {
					case 16:
					case 21:
					case 22:
					case 56:
						if (!LoadPlayerAttachmentAnimation(player)) {
							mData.mScript = start;
							yield         = 1;
						}
						break;
					}
				}
			}
		} else
			CEvt_Warn(FormatToStaticBuffer("call(SYSCALL_SET_OBJSTAT, %d, 1<<%d): obj(%d) killed!", id, bit, id));
		break;
	}
	case SYSCALL_CLR_OBJSTAT: {
		s16 id    = args[0];
		s32 bit   = args[1];
		u64 mask  = (u64)1 << bit;
		XObj* obj = CEvt_GetObj(this, id);
		if (obj != NULL) {
			obj->mStatus.mFlags &= ~mask;
			if (mask & 0x20000)
				obj->SetDestinationAlpha(0, 0);
			if (mask & ((u64)0x4000 << 32)) {
				WiiPlayerState* player = gGame.GetPlayer();
				if ((s16)obj->GetPool() == 8 && obj == player->mLift.mTarget) {
					s16 item = obj->GetObjFlags();
					switch (item) {
					case 16:
					case 21:
					case 22:
					case 56:
						player->mAttachment = 0;
						// the GameCube released the CJAnim here; the Wii's archive releases
						// its model and closes its file
						if (player->mAttachmentAnim.mFile.IsLoaded()) {
							player->mAttachmentAnim.mAnm.Release(TRUE);
							player->mAttachmentAnim.mFile.Close();
						}
						break;
					}
				}
			}
		} else
			CEvt_Warn(FormatToStaticBuffer("call(SYSCALL_CLR_OBJSTAT, %d, 1<<%d): obj(%d) killed!", id, bit, id));
		break;
	}
	case SYSCALL_CHK_OBJSTAT: {
		s16 id    = args[0];
		s32 bit   = args[1];
		u64 mask  = (u64)1 << bit;
		XObj* obj = CEvt_GetObj(this, id);
		if (obj != NULL)
			mSelectResult = (obj->mStatus.mFlags & mask) ? 1 : 0;
		else
			CEvt_Warn(FormatToStaticBuffer("call(SYSCALL_CHK_OBJSTAT, %d, 1<<%d): obj(%d) killed!", id, bit, id));
		break;
	}
	case -29:
		SetItemFlags((s16)args[0], (u16)args[1]);
		break;
	case -30:
		ClearItemFlags((s16)args[0], (u16)args[1]);
		break;
	case -31:
		mSelectResult = (u16)GetItemFlagsMasked((s16)args[0], (u16)args[1]);
		break;
	case -41:
		gGame.SetStatusFlags((u64)1 << args[0]);
		break;
	case -42:
		gGame.ClearStatusFlags((u64)1 << args[0]);
		break;
	case -43:
		mSelectResult = gGame.GetStatusFlags((u64)1 << args[0]) ? 1 : 0;
		break;
	case -32:
		gGame.SetStatusFlags((u64)0x8000 << 32);
		break;
	case -33:
		gGame.ClearStatusFlags((u64)0x8000 << 32);
		break;
	case -37:
		gGame.SetStatusFlags((u64)0x10000 << 32);
		break;
	case -38:
		gGame.ClearStatusFlags((u64)0x10000 << 32);
		break;
	case -34:
		gGame.SetStatusFlags((u64)0x20 << 32);
		break;
	case -35:
		gGame.ClearStatusFlags((u64)0x20 << 32);
		break;
	case -36:
		// the GameCube walked the objects for one with the reflection bit here; the Wii keeps
		// its answer on the pointer, an object asks for the floor reflection through -26
		if (gGame.mPointer.mFloorReflect) {
			gGame.SetStatusFlags((u64)0x20 << 32);
		} else {
			gGame.ClearStatusFlags((u64)0x20 << 32);
		}
		break;
	// -44 turns back on and -45 turns off every camera area record whose mCameraAction is the id
	case -44: {
		s16 id                 = args[0];
		s32 areaCount          = gCameraAreaState.mAreaExCount;
		CamAreaRecord* records = gCameraAreaState.mAreaExRecords;
		for (s32 i = 0; i < areaCount; i++) {
			CamAreaRecord* record = CamAreaInfo(&records[i]).GetRecord();
			if (id == record->mCameraAction)
				record->mFlags &= ~0x80;
		}
		break;
	}
	case -45: {
		s16 id                 = args[0];
		s32 areaCount          = gCameraAreaState.mAreaExCount;
		CamAreaRecord* records = gCameraAreaState.mAreaExRecords;
		for (s32 i = 0; i < areaCount; i++) {
			CamAreaRecord* record = CamAreaInfo(&records[i]).GetRecord();
			if (id == record->mCameraAction)
				record->mFlags |= 0x80;
		}
		break;
	}
	case -39: {
		s16 result    = GetItemScrapValue(args[0]);
		mSelectResult = result;
		break;
	}
	case -40:
		mSelectResult = (s16)GetItemHappyPointValue(args[0]);
		break;
	case -46:
		// Wii only: wakes the script's owner, CEvt's end clears the word again
		if (mOwner != NULL)
			mOwner->_9A4 = 1;
		break;
	case SYSCALL_STGFUNC:
		if (count > 0)
			mSelectResult = CallStageModule(args[0] + 7, count - 1, &args[1]);
		else
			OSPanic(__FILE__, 2244, "SYSCALL_STGFUNC, noargs\n");
		break;
	case -200:
		AddMainMenuExtraRow(gGame.mMenu.mMenus[0], args[0], args[1]);
		break;
	default:
		if (target != NULL && (target->mStatus.mFlags & 1)) {
			s32 result = target->HandleEventCommand(count, args);
			if ((s16)target->GetPool() == 2) {
				if (result == -1 && args[0] != 101)
					yield = 1;
				else
					mSelectResult = result;
			} else
				mSelectResult = result;
			for (s32 i = 0; i < count; i++) { }
		}
		break;
	}
	return yield;
}

/**
 * @note Address: 0x8009CABC
 * @note Size: 0x3DC
 */
s32 CEvt::_cmd_MOVETO(u8, s32)
{
	s16 id    = CEvt_Evaluate(this);
	XObj* obj = CEvt_GetObj(this, id);
	f32 x     = CEvt_Evaluate(this) / 100.0f;
	f32 y     = CEvt_Evaluate(this) / 100.0f;
	f32 z     = CEvt_Evaluate(this) / 100.0f;
	f32 time  = CEvt_Evaluate(this) / 100.0f;
	s32 anim  = CEvt_Evaluate(this);
	s16 dir   = CEvt_Evaluate(this);
	// only a shown object moves (DISP sets 0x10000), a hidden one is skipped without a warning
	if (obj != NULL && (obj->mStatus.mFlags & 1) && (obj->mStatus.mFlags & 0x10000)) {
		if (obj->mObject.m_pScnMdl == NULL) {
			CEvt_Warn(FormatToStaticBuffer("moveto(%d, %d,%d,%d, %d, %d, %d) ... obj(%d) is collision!", id, (s32)(x * 100.0f),
			                               (s32)(y * 100.0f), (s32)(z * 100.0f), (s32)(time * 100.0f), anim, dir, id));
		}
		XObj* player = gGame.GetPlayer();
		if (obj == player) {
			CancelPlayerPushAction(&((WiiPlayerState*)player)->mPush);
		}
		CVec target(x, y, z);
		f32 distance = PSVECDistance(obj->GetPos(), &target);
		if (distance < 0.1f) {
			// already there
			obj->SetPos(x, y, z);
		} else {
			// time 0 gets there in one frame, a positive time is seconds, a negative one the speed per frame
			if (time == 0.0f) {
				CVec to(x, y, z);
				obj->RequestMoveDir(&to, distance);
			} else if (time > 0.0f) {
				CVec to(x, y, z);
				obj->RequestMoveDir(&to, distance / (time * gRefreshRate));
			} else {
				CVec to(x, y, z);
				obj->RequestMoveDir(&to, -time);
			}
			if (dir != -1) {
				f32 yaw;
				// -2 faces where it's going
				if (dir == -2) {
					yaw = CEvt_Angle(*obj->GetPos(), CVec(x, y, z));
				} else {
					yaw = DEG2RAD * dir;
				}
				obj->SetRotationY(yaw);
			}
			if (anim != -1) {
				obj->SetAnim(anim, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
				obj->UpdateAnim(1.0f);
			}
		}
	} else if (obj == NULL || !(obj->mStatus.mFlags & 1)) {
		CEvt_Warn(FormatToStaticBuffer("moveto(%d, ...) obj(%d) killed!", id, id));
	}
	return 0;
}

/**
 * @note Address: 0x8009B26C
 * @note Size: 0x81C
 */
// NONMATCHING: retail keeps the turn's fabs unrounded and the angle in another register
s32 CEvt::_cmd_DIR(u8 command, s32)
{
	s16 id    = CEvt_Evaluate(this);
	XObj* obj = CEvt_GetObj(this, id);
	if (obj != NULL && (obj->mStatus.mFlags & 1)) {
		XObj* player = gGame.GetPlayer();
		if (obj == player)
			CancelPlayerPushAction(&((WiiPlayerState*)player)->mPush);
	}
	if (command == EVT_CMD_DIR) {
		s16 direction = CEvt_Evaluate(this);
		if (obj != NULL && (obj->mStatus.mFlags & 1)) {
			obj->SetRotationY(DEG2RAD * direction);
			gGame.mMotionControlCamera.UpdateTarget(obj);
			XObj* player = gGame.GetPlayer();
			if (obj == player) {
				player->UpdateAnim(0.0f);
				ResetPlayerHeadOffset((WiiPlayerState*)player);
				XAction* carry = &((WiiPlayerState*)player)->mLift;
				if (carry->mTarget != NULL) {
					UpdateHeldObject(carry, 1);
					if (carry->mTarget != NULL) {
						carry->mTarget->UpdateObject(1);
					}
				}
			}
		}
	} else {
		u16 mode = CEvt_Evaluate(this);
		s16 direction;
		switch (mode) {
		case EVT_ATOM_DIR:
			direction = CEvt_Evaluate(this);
			break;
		case EVT_ATOM_POS: {
			// the point is level with the object
			Vec to;
			to.x = CEvt_Evaluate(this) / 100.0f;
			to.z = CEvt_Evaluate(this) / 100.0f;
			if (obj != NULL && (obj->mStatus.mFlags & 1)) {
				to.y      = obj->GetPos()->y;
				direction = RAD2DEG * CEvt_Angle(*obj->GetPos(), to);
			} else {
				CEvt_Warn(FormatToStaticBuffer("mdir(%d, @POS, ...): obj(%d) killed!", id, id));
			}
			break;
		}
		case EVT_ATOM_OBJ: {
			s16 targetId = CEvt_Evaluate(this);
			XObj* target = CEvt_GetObj(this, targetId);
			if (obj != NULL && (obj->mStatus.mFlags & 1)) {
				if (target != NULL && (target->mStatus.mFlags & 1)) {
					direction = RAD2DEG * CEvt_Angle(*obj->GetPos(), *target->GetPos());
				} else {
					CEvt_Warn(FormatToStaticBuffer("mdir(%d, @OBJ, %d, ...): obj(%d) killed!", id, targetId, targetId));
					direction = RAD2DEG * obj->mMoveRotate.y;
				}
			} else {
				CEvt_Warn(FormatToStaticBuffer("mdir(%d, @OBJ, ...): obj(%d) killed!", id, id));
			}
			break;
		}
		case EVT_ATOM_CAM:
			if (obj != NULL && (obj->mStatus.mFlags & 1)) {
				CCamera* camera = &gGame.mMotionControlCamera;
				direction       = RAD2DEG * CEvt_Angle(*obj->GetPos(), camera->mView.mEye);
			} else {
				CEvt_Warn(FormatToStaticBuffer("mdir(%d, @CAM, ...): obj(%d) killed!", id, id));
			}
			break;
		default:
			OSPanic(__FILE__, 2787, FormatToStaticBuffer("mdir(obj, %d,...) - unresolved!\n", mode));
			break;
		}
		f32 time = CEvt_Evaluate(this) / 100.0f;
		s32 anim = CEvt_Evaluate(this);
		if (obj != NULL && (obj->mStatus.mFlags & 1) && (obj->mStatus.mFlags & 0x10000)) {
			if (obj->mObject.m_pScnMdl == NULL) {
				CEvt_Warn(FormatToStaticBuffer("mdir(%d, %d, %d, %d, %d) ... obj(%d) is collision!", id, mode, direction,
				                               (s32)(100.0f * time), anim, id));
			}
			f32 difference = obj->mMoveRotate.y - DEG2RAD * direction;
			while (difference < 0.0f)
				difference += TWO_PI;
			while (difference >= TWO_PI)
				difference -= TWO_PI;
			// already within a tenth of a degree of it: set it instead of turning
			if (difference < 0.0017453292f || difference > 6.2814398f) {
				obj->RequestRotationY(DEG2RAD * direction);
			} else {
				obj->RequestRotateY(DEG2RAD * direction, time);
				f32 angle = obj->mMoveTarget1.y - obj->mMoveRotate.y;
				if (angle > 3.1415927f)
					angle = TWO_PI - angle;
				obj->mMoveSpeed1.x = 0.0f;
				f32 speed          = fabs(angle);
				obj->mMoveSpeed1.y = speed / (time * gRefreshRate);
				obj->mMoveSpeed1.z = 0.0f;
				if (anim != -1) {
					obj->SetAnim(anim, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
					obj->UpdateAnim(1.0f);
				}
			}
		}
	}
	return 0;
}

// retires every running effect of emitter id, fn_800A12F8 written out again
static inline void RetireEffects(EffectInstance* effects, u32 id, BOOL retireParticles)
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

extern "C" EffectInstance* fn_800A11A4(EffectInstance* effects, u32 id, s32 group);

/**
 * @note Address: 0x8009D6C0
 * @note Size: 0x81C
 */
// NONMATCHING: retail's frame has @OBJ's rotation below the sum's temporary, every other slot 4 bytes lower
s32 CEvt::_cmd_PTCL(u8, s32)
{
	u32 code = CEvt_Evaluate(this);
	s16 type = CEvt_Evaluate(this);
	Vec translate, scale;
	if (type == EVT_ATOM_POS) {
		translate.x            = CEvt_Evaluate(this) / 100.0f;
		translate.y            = CEvt_Evaluate(this) / 100.0f;
		translate.z            = CEvt_Evaluate(this) / 100.0f;
		scale.x                = CEvt_Evaluate(this) / 100.0f;
		scale.y                = CEvt_Evaluate(this) / 100.0f;
		scale.z                = CEvt_Evaluate(this) / 100.0f;
		f32 angle              = CEvt_Evaluate(this);
		Vec rotation           = { 0.0f, 0.0f, 0.0f };
		rotation.y             = DEG2RAD * angle;
		EffectInstance* effect = fn_800A11A4((EffectInstance*)&gEventPtcl, code, -1);
		if (effect != NULL) {
			effect->pos   = translate;
			effect->rot   = rotation;
			effect->scale = scale;
			UpdateEffectMtx(effect);
		}
	} else if (type == EVT_ATOM_OBJ) {
		s16 id      = CEvt_Evaluate(this);
		XObj* obj   = CEvt_GetObj(this, id);
		translate.x = CEvt_Evaluate(this) / 100.0f;
		translate.y = CEvt_Evaluate(this) / 100.0f;
		translate.z = CEvt_Evaluate(this) / 100.0f;
		scale.x     = CEvt_Evaluate(this) / 100.0f;
		scale.y     = CEvt_Evaluate(this) / 100.0f;
		scale.z     = CEvt_Evaluate(this) / 100.0f;
		f32 angle   = CEvt_Evaluate(this);
		if (obj != NULL && (obj->mStatus.mFlags & 1)) {
			translate              = *obj->GetPos() + translate;
			Vec rotation           = { 0.0f, 0.0f, 0.0f };
			rotation.y             = DEG2RAD * angle;
			EffectInstance* effect = fn_800A11A4((EffectInstance*)&gEventPtcl, code, -1);
			if (effect != NULL) {
				effect->pos   = translate;
				effect->rot   = rotation;
				effect->scale = scale;
				UpdateEffectMtx(effect);
			}
		} else {
			CEvt_Warn(FormatToStaticBuffer("ptcl(..., @OBJ, %d, ...) obj(%d) killed!", id, id));
		}
	} else if (type == EVT_ATOM_LEAD) {
		s16 id    = CEvt_Evaluate(this);
		XObj* obj = CEvt_GetObj(this, id);
		s32 depth = CEvt_Evaluate(this);
		if (depth > 16)
			depth = 16;
		s32 path[16];
		for (s32 i = 0; i < depth; i++) {
			path[i] = CEvt_Evaluate(this);
		}
		if (obj != NULL && (obj->mStatus.mFlags & 1)) {
			if (obj->mObject.m_pScnMdl == NULL) {
				CEvt_Warn(FormatToStaticBuffer("ptcl(%d, @LEAD, %d, ...) ... obj(%d) is collision!\n", code, id, id));
			}
			// the effect follows the joint the path leads to, or the model's first one
			nw4r::g3d::ResNode node = obj->vf284(depth, path);
			if (node.IsValid()) {
				MtxP mtx               = obj->GetWorldMtx(node);
				EffectInstance* effect = fn_800A11A4((EffectInstance*)&gEventPtcl, code, -1);
				if (effect != NULL) {
					effect->mtx = mtx;
					UpdateEffectMtx(effect);
				}
			} else {
				MtxP mtx               = obj->GetWorldMtx((u32)0);
				EffectInstance* effect = fn_800A11A4((EffectInstance*)&gEventPtcl, code, -1);
				if (effect != NULL) {
					effect->mtx = mtx;
					UpdateEffectMtx(effect);
				}
			}
		} else {
			CEvt_Warn(FormatToStaticBuffer("ptcl(..., @LEAD, %d, ...) obj(%d) killed!", id, id));
		}
	} else if (type == EVT_ATOM_KILL) {
		RetireEffects((EffectInstance*)&gEventPtcl, code, FALSE);
	} else {
		OSPanic(__FILE__, 3528, FormatToStaticBuffer("ptcl(...) - invalid type - %d!\n", type));
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

/**
 * @note Address: 0x8009D014
 * @note Size: 0x220
 */
s32 CEvt::_cmd_MSG(u8 cmd, s32)
{
	CMsg* owner = mMsg;
	if (owner != NULL) {
		if (owner->mStay > 0) {
			// 1: the last message was left up by STAY; mStay 2 has END fade it out
			if (mMsgState == 1) {
				owner->mStay = 2;
				mMsgState++;
				mData.mScript--;
				return 1;
			}
			// 2: wait for that window to finish; a message with no text only closes it
			if (mMsgState == 2) {
				if (owner->mData.mCursor != NULL) {
					mData.mScript--;
					return 1;
				}
				mMsgState   = 0;
				mMsg->mIdle = 1;
				if (mData.mScript[4] == 0) {
					mData.mScript = ResolveEventScriptOffset(mData.mScript);
					return 0;
				}
			}
		}
		owner = mMsg;
		if (owner->mIdle != 0) {
			if (mOwner != NULL && mOwner->_9A4 != 0 && gGame.mEvt.mData.mScript != NULL) {
				// an object's script waits while the main script runs
				mData.mScript--;
			} else {
				CMsg_Open(owner, cmd, mData.mScript + 4, (MsgFormatArgs*)&mValues.mBuf);
				mMsg->mIdle = 0;
				// back onto the opcode and yield: MSG runs again each frame until the message is done
				mData.mScript--;
				if (cmd == EVT_CMD_SELECT) {
					PlayWiiSoundEffect(&gGame.mSound, 0xF3, 1.0f, 0.0f);
				}
			}
			return 1;
		}
		// STAY: the script goes on with the window left up
		if (owner->mStay > 0) {
			mMsgState     = 1;
			mData.mScript = ResolveEventScriptOffset(mData.mScript);
			return 0;
		}
		if (owner->mData.mCursor != NULL) {
			mData.mScript--;
			return 1;
		}
		if (cmd == EVT_CMD_SELECT) {
			mSelectResult = owner->mSelectResult;
		}
		mMsg->mIdle   = 1;
		mData.mScript = ResolveEventScriptOffset(mData.mScript);
		return 0;
	}
	mData.mScript = ResolveEventScriptOffset(mData.mScript);
	return 0;
}

/**
 * @note Address: 0x8009C3EC
 * @note Size: 0x1D8
 */
s32 CEvt::_cmd_MENU(u8, s32)
{
	s32 ret = 0;
	// rewind to here until the menu closes
	u8* cmd = mData.mScript - 1;
	s32 id  = CEvt_Evaluate(this);
	if (mMenuActive != 0) {
		if (gGame.mMenu.mActive != 0) {
			mData.mScript = cmd;
			ret           = 1;
		} else {
			mMenuActive = 0;
		}
	} else if (id == 1000) {
		// equipment rings: ring 0 (attachments) or 1 (costumes) gets the cursor, opening them if needed; below 0 closes them
		s32 ring = CEvt_Evaluate(this);
		if (ring >= 0) {
			SelectPlayerEquipmentRing(gGame.GetPlayer(), ring);
		} else {
			WiiPlayerState* player = gGame.GetPlayer();
			bool ringsOpen         = false;
			if (player->mMenu.mAtc.mOpen != 0 || player->mMenu.mDrs.mOpen != 0) {
				ringsOpen = true;
			}
			if (ringsOpen) {
				ClosePlayerEquipmentMenu(player, -1);
			}
		}
	} else if (id == 1001) {
		s32 atcItem = CEvt_Evaluate(this);
		s32 drsItem = CEvt_Evaluate(this);
		if (atcItem >= 0) {
			gGame.GetPlayer()->mMenu.PointAtcItem(atcItem);
			gGame.GetPlayer()->mMenu.PointDrsItem(atcItem);
		}
		if (drsItem >= 0) {
			gGame.GetPlayer()->mMenu.PointDrsItem(drsItem);
			gGame.GetPlayer()->mMenu.PointAtcItem(drsItem);
		}
	} else {
		fn_80084B90(&gGame.mMenu, id);
		mMenuActive   = 1;
		ret           = 1;
		mData.mScript = cmd;
	}
	return ret;
}

/**
 * @note Address: 0x8009D234
 * @note Size: 0x38C
 */
s32 CEvt::_cmd_POS(u8, s32)
{
	s16 id    = CEvt_Evaluate(this);
	XObj* obj = CEvt_GetObj(this, id);
	f32 x     = CEvt_Evaluate(this) / 100.0f;
	f32 y     = CEvt_Evaluate(this) / 100.0f;
	f32 z     = CEvt_Evaluate(this) / 100.0f;
	if (obj != NULL && (obj->mStatus.mFlags & 1)) {
		obj->SetPos(x, y, z);
		// a placement in 4:3 screen units, x widens to the tv's aspect
		if (obj->mStatus.mFlags & 0x800) {
			obj->SetTranslateX(gScreenWidthScale * obj->GetPos()->x);
		}
		// mLastPos is overwritten below, so flag the floor heights as stale for UpdateObject
		if (obj->mMoveTranslate.y != obj->mLastPos.y) {
			obj->mStatus.mFlags |= 0x0010000000000000;
		}
		obj->mLastPos.x = obj->mMoveTranslate.x;
		obj->mLastPos.y = obj->mMoveTranslate.y;
		obj->mLastPos.z = obj->mMoveTranslate.z;
		if (obj == gGame.GetPlayer()) {
			// restart his animation where it is, blending from the new place
			obj->SetAnim(obj->GetAnimId(), TRUE, obj->GetAnimFrame(), 1.0f, 1.0f, 15.0f);
			obj->UpdateAnim(0.0f);
		}
		gGame.mMotionControlCamera.UpdateTarget(obj);
		XAction* lift;
		XObj* player = gGame.GetPlayer();
		if (obj == player) {
			ResetPlayerHeadOffset((WiiPlayerState*)player);
			ResetPlayerPlug((WiiPlayerState*)player);
			lift = &((WiiPlayerState*)player)->mLift;
			if (lift->mTarget != NULL) {
				UpdateHeldObject(lift, 1);
				if (lift->mTarget != NULL) {
					lift->mTarget->UpdateObject(1);
				}
			}
			CancelPlayerPushAction(&((WiiPlayerState*)player)->mPush);
		}
	} else {
		CEvt_Warn(FormatToStaticBuffer("pos(%d, ...) obj(%d) killed!", id, id));
	}
	return 0;
}

/**
 * @note Address: 0x8009E07C
 * @note Size: 0x410
 */
// NONMATCHING: retail keeps the three fabs results in their own registers and rounds them only for the target
s32 CEvt::_cmd_SCALE(u8 cmd, s32)
{
	s16 id    = CEvt_Evaluate(this);
	XObj* obj = CEvt_GetObj(this, id);
	f32 x     = CEvt_Evaluate(this) / 100.0f;
	f32 y     = CEvt_Evaluate(this) / 100.0f;
	f32 z     = CEvt_Evaluate(this) / 100.0f;
	if (cmd == EVT_CMD_SCALE) {
		if (obj != NULL && (obj->mStatus.mFlags & 1)) {
			obj->SetScale(x, y, z);
			UpdateObjectFloorHeights(obj);
			UpdateObjectFloorXZExtent(obj);
		} else {
			CEvt_Warn(FormatToStaticBuffer("scale(%d, ...) obj(%d) killed!", id, id));
		}
	} else {
		f32 time = CEvt_Evaluate(this) / 100.0f;
		if (obj != NULL && (obj->mStatus.mFlags & 1)) {
			if (obj->mObject.m_pScnMdl == NULL) {
				CEvt_Warn(FormatToStaticBuffer("mscale(%d, %d,%d,%d) ... obj(%d) is collision!", id, (s32)(100.0f * x), (s32)(100.0f * y),
				                               (s32)(100.0f * z), id));
			}
			if (time == 0.0f) {
				obj->SetScale(x, y, z);
				UpdateObjectFloorHeights(obj);
				UpdateObjectFloorXZExtent(obj);
			} else {
				CVec step = (CVec(x, y, z) - *obj->GetScale()) / (gRefreshRate * time);
				obj->mMoveTarget2.Set(x, y, z);
				step.x = fabs(step.x);
				step.y = fabs(step.y);
				step.z = fabs(step.z);
				obj->mMoveSpeed2.Set(step);
			}
		} else {
			CEvt_Warn(FormatToStaticBuffer("mscale(%d, ...) obj(%d) killed!", id, id));
		}
	}
	return 0;
}

/**
 * @note Address: 0x8009C6C4
 * @note Size: 0x3F8
 */
s32 CEvt::_cmd_MOVE(u8, s32)
{
	s16 id       = CEvt_Evaluate(this);
	XObj* obj    = CEvt_GetObj(this, id);
	f32 distance = CEvt_Evaluate(this) / 100.0f;
	f32 time     = CEvt_Evaluate(this) / 100.0f;
	s32 anim     = CEvt_Evaluate(this);
	s16 dir      = CEvt_Evaluate(this);
	// only a shown object moves (DISP sets 0x10000), a hidden one is skipped without a warning
	if (obj != NULL && (obj->mStatus.mFlags & 1) && (obj->mStatus.mFlags & 0x10000)) {
		if (obj->mObject.m_pScnMdl == NULL) {
			// retail's format says %d but passes a float
			CEvt_Warn(FormatToStaticBuffer("move(%d, %d, %d, %d, %d) ... obj(%d) is collision!", id, distance, (s32)(time * 100.0f), anim,
			                               dir, id));
		}
		XObj* player = gGame.GetPlayer();
		if (obj == player) {
			CancelPlayerPushAction(&((WiiPlayerState*)player)->mPush);
		}
		Vec target = *obj->GetPos();
		// -1 and -2 keep the way it faces
		f32 yaw = (dir == -1 || dir == -2) ? obj->mMoveRotate.y : DEG2RAD * dir;
		target.x += distance * (f32)sin(yaw);
		target.z += distance * (f32)cos(yaw);
		// time 0 gets there in one frame, a positive time is seconds, a negative one the speed per frame
		if (time == 0.0f) {
			obj->RequestMoveDir(&target, distance);
		} else if (time > 0.0f) {
			obj->RequestMoveDir(&target, distance / (time * gRefreshRate));
		} else {
			obj->RequestMoveDir(&target, -time);
		}
		if (anim != -1) {
			obj->SetAnim(anim, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			obj->UpdateAnim(1.0f);
		}
	} else if (obj == NULL || !(obj->mStatus.mFlags & 1)) {
		CEvt_Warn(FormatToStaticBuffer("move(%d, ...) obj(%d) killed!", id, id));
	}
	return 0;
}

/**
 * @note Address: 0x8009DDF4
 * @note Size: 0x2D0
 */
s32 CEvt::_cmd_READ(u8, s32)
{
	u8* cmd  = mData.mScript - 1;
	s32 ret  = 0;
	u16 type = CEvt_Evaluate(this);
	switch (type) {
	case EVT_ATOM_ANIM: {
		s16 id           = CEvt_Evaluate(this);
		XObj* obj        = CEvt_GetObj(this, id);
		const char* path = (const char*)CEvt_Evaluate(this);
		if (obj != NULL && (obj->mStatus.mFlags & 1)) {
			// no path drops the file, a path that isn't on the disc is ignored
			if (path != NULL) {
				CFile file;
				if (file.ConvertPathToEntrynum(path) >= 0) {
					fn_800AF098(obj, path);
				}
			} else {
				fn_800AF12C(obj);
			}
		}
		break;
	}
	case EVT_ATOM_SFX: {
		s16 id           = CEvt_Evaluate(this);
		XObj* obj        = CEvt_GetObj(this, id); // looked up and not used
		const char* name = (const char*)CEvt_Evaluate(this);
		if (!mSoundBankLoading) {
			LoadWiiSoundBankByName(&gGame.mSound, name);
			mSoundBankLoading = TRUE;
		}
		// back onto the opcode until the bank is in
		if (gGame.mSound.mLoadPending) {
			mData.mScript = cmd;
			ret           = 1;
		} else {
			mSoundBankLoading = FALSE;
		}
		break;
	}
	default:
		OSPanic(__FILE__, 3629, FormatToStaticBuffer("read(...) - invalid type - %d!\n", type));
		break;
	}
	return ret;
}

// sound -2 is every sound the player has going
static inline void StopAllSounds(XSound* sound, f32 fade)
{
	s32 frames = gRefreshRate * fade;
	for (s32 i = 0; i < 0x40; i++) {
		if (sound->mArchivePlayer.mTracks.mHandles[i].IsAttachedSound()) {
			sound->mArchivePlayer.mTracks.mHandles[i].Stop(frames);
		}
	}
}

static inline void PauseAllSounds(XSound* sound, f32 fade)
{
	s32 frames = gRefreshRate * fade;
	for (s32 i = 0; i < 0x40; i++) {
		if (sound->mArchivePlayer.mTracks.mHandles[i].IsAttachedSound()) {
			sound->mArchivePlayer.mTracks.mHandles[i].Pause(frames);
		}
	}
}

static inline void ResumeAllSounds(XSound* sound, f32 fade)
{
	s32 frames = gRefreshRate * fade;
	for (s32 i = 0; i < 0x40; i++) {
		if (sound->mArchivePlayer.mTracks.mHandles[i].IsAttachedSound()) {
			sound->mArchivePlayer.mTracks.mHandles[i].Resume(frames);
		}
	}
}

/**
 * @note Address: 0x8009F3B8
 * @note Size: 0x428
 */
// NONMATCHING: retail tests the id against 0xF7 unsigned (cmplwi)
s32 CEvt::_cmd_SFX(u8, s32)
{
	s32 sfx  = CEvt_Evaluate(this);
	s16 type = CEvt_Evaluate(this);

	switch (type) {
	case EVT_SFX_STOP:
		if (sfx == -2) {
			StopAllSounds(&gGame.mSound, 0.0f);
		} else {
			StopWiiSfx(&gGame.mSound, sfx, 0.0f);
		}
		break;
	case EVT_SFX_PLAY: {
		s32 state;
		if (sfx == 0xF7) {
			state                              = 3;
			gGame.mSound.mArchivePlayer.mReady = state;
		} else {
			state = -1;
		}
		PlayWiiSoundEffect(&gGame.mSound, sfx, 1.0f, 0.0f);
		if (state != -1) {
			gGame.mSound.mArchivePlayer.mReady = 1;
		}
		break;
	}
	case EVT_ATOM_CUE:
		// the GameCube's streamed cues have no Wii counterpart
		break;
	case EVT_SFX_FADE_OUT:
		FadeOutGameSfx(&gGame.mSound, sfx, CEvt_Evaluate(this) / 100.0f);
		break;
	case EVT_SFX_FADE_IN:
		FadeInGameSfx(&gGame.mSound, sfx, CEvt_Evaluate(this) / 100.0f);
		break;
	case EVT_SFX_FADE: {
		f32 fade   = CEvt_Evaluate(this) / 100.0f;
		f32 volume = CEvt_Evaluate(this) / 255.0f;
		if (!IsWiiSfxPlaying(&gGame.mSound, sfx)) {
			PlayWiiSoundEffect(&gGame.mSound, sfx, volume, fade);
		} else {
			SetWiiSfxVolume(&gGame.mSound, sfx, volume, fade);
		}
		break;
	}
	case EVT_SFX_PAUSE:
		if (sfx == -2) {
			PauseAllSounds(&gGame.mSound, 0.0f);
		} else {
			PauseWiiSfx(&gGame.mSound, sfx, 0.0f);
		}
		break;
	case EVT_SFX_RESUME:
		if (sfx == -2) {
			ResumeAllSounds(&gGame.mSound, 0.0f);
		} else {
			ResumeWiiSfx(&gGame.mSound, sfx, 0.0f);
		}
		break;
	default:
		OSPanic(__FILE__, 4116, FormatToStaticBuffer("sfx(%d,%d) - unresolved!\n", sfx, type));
		break;
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

/**
 * @note Address: 0x80096A38
 * @note Size: 0x8EC
 */
extern "C" void CEvt_EvalSet(CEvt* evt)
{
	s32 code = *evt->mData.mScript;
	u8 op    = code;
	s32 value;
	switch (op) {
	case EVT_EXPR_EQUAL:
	case EVT_EXPR_NOT_EQUAL:
	case EVT_EXPR_LESS:
	case EVT_EXPR_LESS_EQUAL:
	case EVT_EXPR_GREATER:
	case EVT_EXPR_GREATER_EQUAL:
		value = EvalCmp(evt) != FALSE;
		break;
	case EVT_EXPR_ADD_ASSIGN:
	case EVT_EXPR_SUB_ASSIGN:
	case EVT_EXPR_MUL_ASSIGN:
	case EVT_EXPR_DIV_ASSIGN:
	case EVT_EXPR_MOD_ASSIGN:
	case EVT_EXPR_AND_ASSIGN:
	case EVT_EXPR_OR_ASSIGN:
	case EVT_EXPR_XOR_ASSIGN: {
		evt->mData.mScript++;
		s32 rhs         = CEvt_Evaluate(evt);
		u8* destination = evt->mData.mScript;
		value           = CEvt_Evaluate(evt);
		if (op == EVT_EXPR_ADD_ASSIGN)
			value += rhs;
		else if (op == EVT_EXPR_SUB_ASSIGN)
			value -= rhs;
		else if (op == EVT_EXPR_MUL_ASSIGN)
			value *= rhs;
		else if (op == EVT_EXPR_DIV_ASSIGN)
			value /= rhs;
		else if (op == EVT_EXPR_MOD_ASSIGN)
			value %= rhs;
		else if (op == EVT_EXPR_AND_ASSIGN)
			value &= rhs;
		else if (op == EVT_EXPR_OR_ASSIGN)
			value |= rhs;
		else if (op == EVT_EXPR_XOR_ASSIGN)
			value ^= rhs;
		evt->mData.mScript = destination;
		break;
	}
	default:
		value = CEvt_Evaluate(evt);
		break;
	}
	{
		s32 op = *evt->mData.mScript;
		evt->mData.mScript++;
		s32 index;
		switch (op) {
		case EVT_EXPR_STACK:
			index = *evt->mData.mScript;
			evt->mData.mScript++;
			evt->mValues.mBuf[index + evt->mFrames.mBuf[evt->mFrames.mCount - 1]] = value;
			break;
		case EVT_EXPR_FLAG:
			index = CEvt_Evaluate(evt);
			if (index >= 0 && index < 4096) {
				if (value != 0) {
					gEventFlags.mFlags[(u32)index >> 5] |= 1U << (index & 31);
				} else {
					gEventFlags.mFlags[(u32)index >> 5] &= ~(1U << (index & 31));
				}
			} else if (index == -1 || index == -2 || index == -3 || index == -4 || index == -5) {
				// Evaluate's read-only flags, setting one does nothing
			} else {
				OSPanic(__FILE__, 1153, FormatToStaticBuffer("Invalid EvalSet flag[%d] - 0~%d\n", index, 4096));
			}
			break;
		case EVT_EXPR_VAR:
			index = CEvt_Evaluate(evt);
			if (index >= 0 && index < 2048) {
				gEventVars.mVars[index] = value;
			} else {
				OSPanic(__FILE__, 1162, FormatToStaticBuffer("Invalid EvalSet var[%d] - 0~%d\n", index, 2048));
			}
			break;
		case EVT_EXPR_RESULT1:
			evt->mSelectResult = value;
			break;
		case EVT_EXPR_RESULT2:
			evt->mResult2 = value;
			break;
		case EVT_EXPR_PAD:
			index = CEvt_Evaluate(evt);
			// pad[7] is the rumble, it only runs with the rumble option (0x800000000) on: 1 starts it, 0 stops it
			if (index == 7) {
				if (gGame.GetStatusFlags(0x800000000ULL)) {
					if (value == 1) {
						gWiiPads[0].mRumble[0].Start(evt->mOwner != NULL ? 4 : 8);
					} else if (value == 0) {
						gWiiPads[0].mRumble[0].Stop();
					} else {
						gWiiPads[0].mRumble[0].Start(evt->mOwner != NULL ? 4 : 8);
					}
				}
			} else {
				OSPanic(__FILE__, 1202, FormatToStaticBuffer("Invalid EvalSet pad[%d] - %d\n", index, 7));
			}
			break;
		case EVT_EXPR_BATTERY:
			index = CEvt_Evaluate(evt);
			if (index == 0) {
				gGame.GetPlayer()->mParams.SetBattery(value / 100.0f);
			} else if (index == 1) {
				WiiPlayerParams* stats = &gGame.GetPlayer()->mParams;
				stats->SetMaxBattery(value / 100.0f);
				stats->SetBattery(stats->mBattery);
			} else {
				OSPanic(__FILE__, 1215, FormatToStaticBuffer("Invalid EvalSet battery[%d] - 0~1\n", index));
			}
			break;
		case EVT_EXPR_MONEY:
			AdjustPlayerMoney(&gGame.GetPlayer()->mParams, value - gGame.GetPlayer()->mParams.mMoney);
			break;
		case EVT_EXPR_SCRAP:
			AddPlayerScrap(&gGame.GetPlayer()->mParams, value - gGame.GetPlayer()->mParams.mScrap);
			break;
		case EVT_EXPR_ITEM:
			index = CEvt_Evaluate(evt);
			if (index >= 0 && index < 159) {
				SetOrAddInventoryItemCount(&gGame.GetPlayer()->mParams.mInventory, index, value, 0);
			} else {
				OSPanic(__FILE__, 1234, FormatToStaticBuffer("Invalid EvalSet item[%d] - 0~%d\n", index, 159));
			}
			break;
		case EVT_EXPR_ATC:
			index = CEvt_Evaluate(evt);
			if (index >= 0 && index < 9) {
				SetAttachmentCountById((AttachmentValues*)gGame.GetPlayer()->mAttachments, index, value);
			} else {
				OSPanic(__FILE__, 1243, FormatToStaticBuffer("Invalid EvalSet atc[%d] - 0~%d\n", index, 9));
			}
			break;
		case EVT_EXPR_CURRENT_SUIT: {
			WiiPlayerState* player = gGame.GetPlayer();
			if (player->mSuits.mChangeStep == FALSE) {
				player->mSuits.mRequested  = value;
				player->mSuits.mChangeStep = TRUE;
			}
			break;
		}
		case EVT_EXPR_EXP:
			AdjustPlayerExperience(&gGame.GetPlayer()->mParams, value - gGame.GetPlayer()->mParams.mExperience);
			break;
		case EVT_EXPR_LEVEL:
			AdjustPlayerLevel(&gGame.GetPlayer()->mParams, value - gGame.GetPlayer()->mParams.mLevel);
			break;
		case EVT_EXPR_RANK:
			AddPlayerRank(&gGame.GetPlayer()->mParams, value - gGame.GetPlayer()->mParams.mRank);
			break;
		case EVT_EXPR_CURRENT_ATC: {
			WiiPlayerState* player = gGame.GetPlayer();
			if (value != player->mAttachment) {
				player->mAttachmentRequest = value;
				player->mAttachmentStep    = 0;
			}
			break;
		}
		case EVT_EXPR_TIME:
			index = CEvt_Evaluate(evt);
			if (index == 0) {
				gGame.mTimeState.mMode    = value;
				gGame.mTimeState.mTime    = 0.0f;
				gGame.mTimeState.mRunning = 0;
			} else if (index == 1) {
				gGame.mTimeState.mRunning = 0;
				gGame.mTimeState.mTime    = value / 100.0f;
			} else if (index == 2) {
				gGame.mTimeState.mDuration = value / 100.0f;
			} else {
				OSPanic(__FILE__, 1292, FormatToStaticBuffer("Invalid EvalSet time[%d] - 0~2\n", index));
			}
			break;
		case EVT_EXPR_HOLD:
		case EVT_EXPR_MAP:
		case EVT_EXPR_ACTOR_NAME:
		case EVT_EXPR_ITEM_NAME:
		case EVT_EXPR_USE:
		case EVT_EXPR_HIT:
		case EVT_EXPR_STICKER_NAME:
		case EVT_EXPR_OBJ:
		case EVT_EXPR_RANDOM:
		case EVT_EXPR_SIN:
		case EVT_EXPR_COS:
			break;
		default:
			OSPanic(__FILE__, 1303, FormatToStaticBuffer("Unknown EvalSet op - %d\n", op));
			break;
		}
	}
}

/**
 * @note Address: 0x80094ED8
 * @note Size: 0x1B60
 */
// NONMATCHING: retail's pad[5] runs through an FSqrt with nw4r's domain warning still in, and a few float
// registers differ (pad[6], the percentages)
extern "C" s32 CEvt_Evaluate(CEvt* evt)
{
	s32 op = *evt->mData.mScript;
	evt->mData.mScript++;
	s32 result;
	switch (op) {
	case EVT_EXPR_EQUAL:
	case EVT_EXPR_NOT_EQUAL:
	case EVT_EXPR_LESS:
	case EVT_EXPR_LESS_EQUAL:
	case EVT_EXPR_GREATER:
	case EVT_EXPR_GREATER_EQUAL:
	case EVT_EXPR_ADD:
	case EVT_EXPR_SUB:
	case EVT_EXPR_MUL:
	case EVT_EXPR_DIV:
	case EVT_EXPR_MOD:
	case EVT_EXPR_AND:
	case EVT_EXPR_OR:
	case EVT_EXPR_XOR: {
		u8 binary = op;
		s32 rhs   = CEvt_Evaluate(evt);
		s32 lhs   = CEvt_Evaluate(evt);
		if (binary == EVT_EXPR_EQUAL)
			result = lhs == rhs;
		if (binary == EVT_EXPR_NOT_EQUAL)
			result = lhs != rhs;
		if (binary == EVT_EXPR_LESS)
			result = lhs < rhs;
		if (binary == EVT_EXPR_LESS_EQUAL)
			result = lhs <= rhs;
		if (binary == EVT_EXPR_GREATER)
			result = lhs > rhs;
		if (binary == EVT_EXPR_GREATER_EQUAL)
			result = lhs >= rhs;
		if (binary == EVT_EXPR_ADD)
			result = lhs + rhs;
		if (binary == EVT_EXPR_SUB)
			result = lhs - rhs;
		if (binary == EVT_EXPR_MUL)
			result = lhs * rhs;
		if (binary == EVT_EXPR_DIV)
			result = lhs / rhs;
		if (binary == EVT_EXPR_MOD)
			result = lhs % rhs;
		if (binary == EVT_EXPR_AND)
			result = lhs & rhs;
		if (binary == EVT_EXPR_OR)
			result = lhs | rhs;
		if (binary == EVT_EXPR_XOR)
			result = lhs ^ rhs;
		break;
	}
	case EVT_EXPR_NOT:
		result = !CEvt_Evaluate(evt);
		break;
	case EVT_EXPR_IMM16:
		result = (s16)EvtRead16(evt->mData.mScript);
		evt->mData.mScript += 2;
		break;
	case EVT_EXPR_IMM32: {
		s32 value = ReadResourceOffset(evt->mData.mScript);
		evt->mData.mScript += 4;
		result = value;
		break;
	}
	case EVT_EXPR_ADDRESS_OF: {
		s32 offset = ReadResourceOffset(evt->mData.mScript);
		evt->mData.mScript += 4;
		s32 library = IsGlobalEventScript(evt->mData.mScript);
		if (library != 0)
			result = (s32)(GetGlobalScriptTable(library) + offset);
		else
			result = (s32)(GetStageDataBuffer() + offset);
		break;
	}
	case EVT_EXPR_STACK: {
		s32 index = *evt->mData.mScript;
		evt->mData.mScript++;
		result = evt->mValues.mBuf[index + evt->mFrames.mBuf[evt->mFrames.mCount - 1]];
		break;
	}
	case EVT_EXPR_PARENT_STACK: {
		s32 index = *evt->mData.mScript;
		evt->mData.mScript++;
		s32 frame = evt->mFrames.mCount - 2;
		result    = evt->mValues.mBuf[index + evt->mFrames.mBuf[frame]];
		break;
	}
	case EVT_EXPR_FLAG:
		result = CEvt_Evaluate(evt);
		if (result >= 0 && result < 4096) {
			result = (gEventFlags.mFlags[(u32)result >> 5] & (1U << (result & 31))) != 0;
		} else if (result == -1)
			result = 0;
		else if (result == -2)
			result = gGame.GetStatusFlags(0x8000) ? 1 : 0;
		else if (result == -3)
			result = gGame.GetStatusFlags(0x8000000000000ULL) ? 1 : 0;
		else if (result == -4)
			result = gGame.mMenuControls & 1;
		else if (result == -5)
			result = gGame.mMenuControls & 2;
		else
			OSPanic(__FILE__, 721, FormatToStaticBuffer("Invalid Evaluate flg[%d] - 0~%d\n", result, 4096));
		break;
	case EVT_EXPR_VAR:
		result = CEvt_Evaluate(evt);
		if (result >= 0 && result < 2048)
			result = gEventVars.mVars[result];
		else
			OSPanic(__FILE__, 729, FormatToStaticBuffer("Invalid Evaluate var[%d] - 0~%d\n", result, 2048));
		break;
	case EVT_EXPR_RESULT1:
		result = evt->mSelectResult;
		break;
	case EVT_EXPR_RESULT2:
		result = evt->mResult2;
		break;
	case EVT_EXPR_PAD: {
		result       = CEvt_Evaluate(evt);
		CPadOne* pad = &gWiiPads[0];
		switch (result) {
		case 0:
			result = gWiiPointer.GetEventButtons();
			break;
		case 1:
			result = gWiiPointer.GetEventTrigger();
			break;
		case 2:
			result = 100.0f * pad->mStick.x;
			break;
		case 3:
			result = 100.0f * pad->mStick.y;
			break;
		case 4:
			result = RAD2DEG * WrapAngle(-(f32)atan2(pad->mStick.x, pad->mStick.y));
			break;
		case 5: {
			f32 x  = pad->mStick.x;
			f32 y  = pad->mStick.y;
			result = 100.0f * nw4r::math::FSqrt(x * x + y * y);
			break;
		}
		case 6: {
			f32 stickAngle = (f32)atan2(pad->mStick.x, pad->mStick.y);
			f32 cameraYaw  = gGame.mMotionControlCamera.mView.mAngle.y;
			f32 playerYaw  = gGame.GetPlayer()->mMoveRotate.y;
			result         = RAD2DEG * WrapAngle(-stickAngle + cameraYaw - playerYaw);
			break;
		}
		case 7:
			if (gGame.GetStatusFlags(0x800000000ULL))
				result = pad->mRumble[0].remaining;
			else
				result = 0;
			break;
		case 8:
			// a swing of the remote since the last frame
			if (pad->GetSwing(NULL, NULL))
				result = 1;
			else
				result = 0;
			break;
		default:
			OSPanic(__FILE__, 781, FormatToStaticBuffer("Invalid Evaluate pad[%d] - 0~7\n", result));
			break;
		}
		break;
	}
	case EVT_EXPR_BATTERY:
		result = CEvt_Evaluate(evt);
		if (result == 0)
			result = 100.0f * gGame.GetPlayer()->mParams.mBattery;
		else if (result == 1)
			result = 100.0f * gGame.GetPlayer()->mParams.mMaxBattery;
		else
			OSPanic(__FILE__, 794, FormatToStaticBuffer("Invalid Evaluate battery[%d] - 0~1\n", result));
		break;
	case EVT_EXPR_MONEY:
		result = gGame.GetPlayer()->mParams.mMoney;
		break;
	case EVT_EXPR_SCRAP:
		result = gGame.GetPlayer()->mParams.mScrap;
		break;
	case EVT_EXPR_ITEM:
		result = CEvt_Evaluate(evt);
		if (result >= 0 && result < 159) {
			s32 index = FindOwnedInventoryItemIndex(gGame.GetPlayer()->mParams.mInventory.mItems, result);
			result    = index >= 0 ? gGame.GetPlayer()->mParams.mInventory.mItems[index].mCount : 0;
		} else
			OSPanic(__FILE__, 813, FormatToStaticBuffer("Invalid Evaluate item[%d] - 0~%d\n", result, 159));
		break;
	case EVT_EXPR_ATC:
		result = CEvt_Evaluate(evt);
		if (result >= 0 && result < 9) {
			result = GetAttachmentCountById((AttachmentValues*)gGame.GetPlayer()->mAttachments, result);
			if (result < 0)
				result = 0;
		} else
			OSPanic(__FILE__, 822, FormatToStaticBuffer("Invalid Evaluate atc[%d] - 0~%d\n", result, 9));
		break;
	case EVT_EXPR_CURRENT_SUIT:
		result = gGame.GetPlayer()->mSuits.mCurrent;
		break;
	case EVT_EXPR_EXP:
		result = gGame.GetPlayer()->mParams.mExperience;
		break;
	case EVT_EXPR_LEVEL:
		result = gGame.GetPlayer()->mParams.mLevel;
		break;
	case EVT_EXPR_RANK:
		result = gGame.GetPlayer()->mParams.mRank;
		break;
	case EVT_EXPR_HOLD: {
		XObj* held = gGame.GetPlayer()->mLift.mTarget;
		if (held != NULL) {
			if (held->GetPool() == 24)
				result = 1000;
			else if (held->GetPool() == 8)
				result = held->GetObjFlags();
			else
				result = -1;
		} else
			result = -1;
		break;
	}
	case EVT_EXPR_USE:
		result = evt->mUseValue;
		break;
	case EVT_EXPR_HIT:
		result = evt->mHitValue;
		break;
	case EVT_EXPR_CURRENT_ATC:
		result = gGame.GetPlayer()->GetAttachment();
		break;
	case EVT_EXPR_MAP:
		result = CEvt_Evaluate(evt);
		if (result >= 0 && result <= 1) {
			// 0 the stage, 1 the one before it: the two fields are read as an array
			s32 stage = (&gGame.mStage)[result];
			result    = stage;
			if (stage > GetStageNum())
				result = stage - GetStageNum() + 99;
		} else
			OSPanic(__FILE__, 871, FormatToStaticBuffer("Invalid Evaluate map[%d] - 0~1\n", result));
		break;
	case EVT_EXPR_ACTOR_NAME:
		result = CEvt_Evaluate(evt);
		if (result >= 20000)
			result -= 20000;
		result = (s32)GetActorName(result);
		break;
	case EVT_EXPR_ITEM_NAME:
		result = (s32)GetItemName(CEvt_Evaluate(evt));
		break;
	case EVT_EXPR_STICKER_NAME:
		result = (s32)GetStatusIconName(CEvt_Evaluate(evt));
		break;
	case EVT_EXPR_TIME:
		result = CEvt_Evaluate(evt);
		if (result == 0)
			result = gGame.mTimeState.mMode;
		else if (result == 1)
			result = 100.0f * gGame.mTimeState.mTime;
		else if (result == 2)
			result = 100.0f * gGame.mTimeState.mDuration;
		else
			OSPanic(__FILE__, 898, FormatToStaticBuffer("Invalid Evaluate time[%d] - 0~2\n", result));
		break;
	case EVT_EXPR_OBJ:
		result = CEvt_Evaluate(evt);
		switch (result) {
		case EVT_ATOM_ANIM:
		case EVT_ATOM_DIR:
		case EVT_ATOM_POSX:
		case EVT_ATOM_POSY:
		case EVT_ATOM_POSZ:
		case 235:
		case 247:
		case 248: {
			XObj* obj = CEvt_GetObj(evt, CEvt_Evaluate(evt));
			if (obj != NULL && (obj->mStatus.mFlags & 1)) {
				switch (result) {
				case EVT_ATOM_ANIM:
					result = obj->GetAnimId();
					break;
				case EVT_ATOM_DIR:
					result = RAD2DEG * WrapAngle(obj->mMoveRotate.y);
					break;
				case EVT_ATOM_POSX:
					result = 100.0f * obj->GetPos()->x;
					break;
				case EVT_ATOM_POSY:
					result = 100.0f * obj->GetPos()->y;
					break;
				case EVT_ATOM_POSZ:
					result = 100.0f * obj->GetPos()->z;
					break;
				case 235:
					result = (obj->mStatus.mFlags & 0x10000) ? 1 : 0;
					break;
				case 247: {
					// in 4:3 screen units
					Vec pos = { 0.0f, 0.0f, 0.0f };
					f32 x;
					obj->ProjectOffsetToScreen(&pos, &x, NULL, NULL, NULL);
					result = 100.0f * (x / gScreenWidthScale);
					break;
				}
				case 248: {
					Vec pos = { 0.0f, 0.0f, 0.0f };
					f32 y;
					obj->ProjectOffsetToScreen(&pos, NULL, &y, NULL, NULL);
					result = 100.0f * y;
					break;
				}
				}
			} else
				result = -1;
			break;
		}
		case EVT_ATOM_BONEX:
		case EVT_ATOM_BONEY:
		case EVT_ATOM_BONEZ:
		case 249:
		case 250: {
			u8* data  = (u8*)CEvt_Evaluate(evt);
			s32 id    = (s16)EvtRead16(data);
			XObj* obj = CEvt_GetObj(evt, id);
			if (obj != NULL && (obj->mStatus.mFlags & 1)) {
				s32 depth = (s16)EvtRead16(data + 2);
				data += 4;
				s32 path[20];
				for (s32 i = 0; i < depth; i++) {
					path[i] = (s16)EvtRead16(data);
					data += 2;
				}
				Vec pos;
				switch (result) {
				case EVT_ATOM_BONEX:
					result = 100.0f * obj->vf288(depth, path).x;
					break;
				case EVT_ATOM_BONEY:
					result = 100.0f * obj->vf288(depth, path).y;
					break;
				case EVT_ATOM_BONEZ:
					result = 100.0f * obj->vf288(depth, path).z;
					break;
				case 249: {
					pos = obj->vf288(depth, path) - *obj->GetPos();
					f32 x;
					obj->ProjectOffsetToScreen(&pos, &x, NULL, NULL, NULL);
					result = 100.0f * (x / gScreenWidthScale);
					break;
				}
				case 250: {
					pos = obj->vf288(depth, path) - *obj->GetPos();
					f32 y;
					obj->ProjectOffsetToScreen(&pos, NULL, &y, NULL, NULL);
					result = 100.0f * y;
					break;
				}
				}
			} else
				result = -1;
			break;
		}
		case EVT_ATOM_DIRTO: {
			u8* data   = (u8*)CEvt_Evaluate(evt);
			XObj* from = CEvt_GetObj(evt, (s16)EvtRead16(data));
			XObj* to   = CEvt_GetObj(evt, (s16)EvtRead16(data + 2));
			if (from != NULL && (from->mStatus.mFlags & 1) && to != NULL && (to->mStatus.mFlags & 1)) {
				f32 angle = CEvt_Angle(*from->GetPos(), *to->GetPos());
				result    = RAD2DEG * WrapAngle(angle - from->mMoveRotate.y);
			} else
				result = -1;
			break;
		}
		case EVT_ATOM_DISTANCE: {
			u8* data   = (u8*)CEvt_Evaluate(evt);
			XObj* from = CEvt_GetObj(evt, (s16)EvtRead16(data));
			XObj* to   = CEvt_GetObj(evt, (s16)EvtRead16(data + 2));
			if (from != NULL && (from->mStatus.mFlags & 1) && to != NULL && (to->mStatus.mFlags & 1)) {
				result = 100.0f * PSVECDistance(from->GetPos(), to->GetPos());
			} else
				result = -1;
			break;
		}
		default:
			OSPanic(__FILE__, 1021, FormatToStaticBuffer("Invalid Evaluate obj(...,%d)\n", result));
			break;
		}
		break;
	case EVT_EXPR_RANDOM: {
		s32 max = CEvt_Evaluate(evt);
		result  = rand() % (max + 1);
		break;
	}
	case EVT_EXPR_SIN:
		result = 100.0f * (f32)sin(DEG2RAD * (CEvt_Evaluate(evt) / 100.0f));
		break;
	case EVT_EXPR_COS:
		result = 100.0f * (f32)cos(DEG2RAD * (CEvt_Evaluate(evt) / 100.0f));
		break;
	case EVT_EXPR_ARRAY_ELEMENT: {
		// the element size in bytes, negative for signed elements, then the index and the array
		s8 size   = CEvt_Evaluate(evt);
		s32 index = CEvt_Evaluate(evt);
		u8* p     = (u8*)CEvt_Evaluate(evt) + index * abs(size);
		switch (size) {
		case 1:
			result = *p;
			break;
		case -1:
			result = *(s8*)p;
			break;
		case 2:
			result = (u16)EvtRead16(p);
			break;
		case -2:
			result = (s16)EvtRead16(p);
			break;
		case 4:
		case -4:
			result = ReadResourceOffset(p);
			break;
		default:
			result = 0;
			break;
		}
		break;
	}
	default:
		OSPanic(__FILE__, 1067, FormatToStaticBuffer("Unknown Evaluate op - %d\n", op));
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
	evt->mSelectResult     = 0;
	evt->mResult2          = 0;
	evt->mMsgState         = 0;
	evt->mUseValue         = -1;
	evt->mHitValue         = -1;
	evt->mSoundBankLoading = FALSE;
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
	evt->mCameraMemento = NULL;
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
