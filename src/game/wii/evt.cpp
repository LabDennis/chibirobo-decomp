// The event script runner, .text 0x8009453C-0x800A18B8: its .rodata names it "evt.cpp" (wii_units.md Nr. 38),
// the GameCube's evt.cpp. The functions come in the reverse of the GameCube's order.
// NONMATCHING: the script control, the queue, the effect slots and the small commands have source; the
// expression evaluator and the big commands (CALL, CAMERA, WAIT, SCRN, ...) do not yet.
#include "nw4r/ef/ef_resource.h"
#include "game/wii/ceffect.h"
#include "game/wii/cgame.h"
#include "game/wii/csound.h"
#include "game/evt.h"
#include "game/evtqueue.h"
#include "game/coordinfo.h"
#include "game/inventory.h"
#include "game/savedata.h"
#include "game/globals.h"
#include "game/ut.h"
#include "game/gmath.h"
#include "game/cthread.h"
#include "game/cfile.h"
#include "Dolphin/os.h"
#include <string.h>

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
BOOL fn_800AF098(XObj* obj, const char* path); // reads an object's animation file
void fn_800AF12C(XObj* obj);                   // drops the animation file READ gave it
}

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
