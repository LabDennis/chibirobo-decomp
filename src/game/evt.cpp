#include "game/hsd_jobj.h"
extern "C" {
#include <sysdolphin/mtx_transform.h>
#include <sysdolphin/aobj.h>
#include <sysdolphin/axdriver.h>
#include <sysdolphin/rumble.h>
}
#include "game/cjobj.h"
#include "game/clight.h"
#include "game/ccameraaction.h"
#include "game/ccameraactioneye.h"
#include "game/cmotioncontrolcamera.h"
#include "game/cparticle.h"
#include "game/ctpl.h"
#include "game/xobjs.h"
#include <stdlib.h>
#include "game/ut.h"
#include <stdio.h>
#include "game/evtqueue.h"
#include "game/msg.h"
#include "game/gmath.h"
#include "game/globals.h"
#include "types.h"
#include "Version.h"
#include "game/csound.h"
#include "game/globals.h"
#include "game/stagedata.h"
#include "game/xchr.h"
#include "game/xaction.h"
#include "game/cscrn.h"
#include "game/inventory.h"
#include "game/video.h"
#include "game/cgame.h"
#include "game/infoscrn.h"
#include "game/cmovie.h"
#include "game/stg.h"
#include "game/xobjflags.h"

extern "C" {
GXColor sEvtWindowColorTemplate;
GXColor sEvtWindowDefaultColor = { 0x00, 0x00, 0x00, 0x80 };
}

extern "C" void CEvt_EvalSet(CEvt* evt);
extern "C" s32 CEvt_Evaluate(CEvt* evt);
extern "C" void OSPanic(const char* file, int line, const char* msg, ...);

// the event globals, in the order the file's static initializer builds them
CEvtFlags gEventFlags;
CEvtVars gEventVars;
CEvtQueueEx gEventScriptQueue;
CEvtTimer gEventTimers;

// script names for objects
EvtObjIdTable gEventObjectIds = { 0 };

// the stream's beat callback latches the beat here for WAIT
EvtBeatLatch gEventBeatLatch;

// the handler of each command. Update looks an opcode up here; nothing reads the second byte
EvtCmdEntry evtCmdTable[] = {
	{ EVT_CMD_ABORT, 3, "ABORT", &CEvt::_cmd_ABORT },
	{ EVT_CMD_RETURN, 11, "RETURN", &CEvt::_cmd_RETURN },
	{ EVT_CMD_GOTO, 3, "GOTO", &CEvt::_cmd_GOTO },
	{ EVT_CMD_SET, 3, "SET", &CEvt::_cmd_SET },
	{ EVT_CMD_IF, 3, "IF", &CEvt::_cmd_IF },
	{ EVT_CMD_ELIF, 3, "ELIF", &CEvt::_cmd_IF },
	{ EVT_CMD_ENDIF, 3, "ENDIF", &CEvt::_cmd_GOTO },
	{ EVT_CMD_CASE, 3, "CASE", &CEvt::_cmd_IF },
	{ EVT_CMD_EXPR, 3, "EXPR", &CEvt::_cmd_IF },
	{ EVT_CMD_WHILE, 3, "WHILE", &CEvt::_cmd_IF },
	{ EVT_CMD_BREAK, 3, "BREAK", &CEvt::_cmd_GOTO },
	{ EVT_CMD_RUN, 7, "RUN", &CEvt::_cmd_RUN },
	{ EVT_CMD_LIB, 7, "LIB", &CEvt::_cmd_LIB },
	{ EVT_CMD_PUSHBP, 0, "PUSHBP", &CEvt::_cmd_PUSHBP },
	{ EVT_CMD_POPBP, 0, "POPBP", &CEvt::_cmd_POPBP },
	{ EVT_CMD_SETSP, 0, "SETSP", &CEvt::_cmd_SETSP },
	{ EVT_CMD_ANIM, 3, "ANIM", &CEvt::_cmd_ANIM },
	{ EVT_CMD_ANIM1, 3, "ANIM1", &CEvt::_cmd_ANIM },
	{ EVT_CMD_ANIM2, 3, "ANIM2", &CEvt::_cmd_ANIM2 },
	{ EVT_CMD_ATTACH, 1, "ATTACH", &CEvt::_cmd_ATTACH },
	{ EVT_CMD_BORN, 3, "BORN", &CEvt::_cmd_BORN },
	{ EVT_CMD_CALL, 3, "CALL", &CEvt::_cmd_CALL },
	{ EVT_CMD_CAMERA, 3, "CAMERA", &CEvt::_cmd_CAMERA },
	{ EVT_CMD_CHECK, 3, "CHECK", &CEvt::_cmd_WAIT },
	{ EVT_CMD_COLOR, 3, "COLOR", &CEvt::_cmd_COLOR },
	{ EVT_CMD_DETACH, 3, "DETACH", &CEvt::_cmd_ATTACH },
	{ EVT_CMD_DIR, 3, "DIR", &CEvt::_cmd_DIR },
	{ EVT_CMD_MDIR, 3, "MDIR", &CEvt::_cmd_DIR },
	{ EVT_CMD_DISP, 3, "DISP", &CEvt::_cmd_DISP },
	{ EVT_CMD_KILL, 3, "KILL", &CEvt::_cmd_KILL },
	{ EVT_CMD_LIGHT, 3, "LIGHT", &CEvt::_cmd_LIGHT },
	{ EVT_CMD_MENU, 3, "MENU", &CEvt::_cmd_MENU },
	{ EVT_CMD_MOVE, 3, "MOVE", &CEvt::_cmd_MOVE },
	{ EVT_CMD_MOVETO, 3, "MOVETO", &CEvt::_cmd_MOVETO },
	{ EVT_CMD_MOVIE, 3, "MOVIE", &CEvt::_cmd_MOVIE },
	{ EVT_CMD_MSG, 3, "MSG", &CEvt::_cmd_MSG },
	{ EVT_CMD_POS, 3, "POS", &CEvt::_cmd_POS },
	{ EVT_CMD_PRINTF, 1, "PRINTF", &CEvt::_cmd_PRINTF },
	{ EVT_CMD_PTCL, 3, "PTCL", &CEvt::_cmd_PTCL },
	{ EVT_CMD_READ, 3, "READ", &CEvt::_cmd_READ },
	{ EVT_CMD_SCALE, 3, "SCALE", &CEvt::_cmd_SCALE },
	{ EVT_CMD_MSCALE, 3, "MSCALE", &CEvt::_cmd_SCALE },
	{ EVT_CMD_SCRN, 3, "SCRN", &CEvt::_cmd_SCRN },
	{ EVT_CMD_SELECT, 3, "SELECT", &CEvt::_cmd_MSG },
	{ EVT_CMD_SFX, 3, "SFX", &CEvt::_cmd_SFX },
	{ EVT_CMD_TIMER, 3, "TIMER", &CEvt::_cmd_TIMER },
	{ EVT_CMD_WAIT, 3, "WAIT", &CEvt::_cmd_WAIT },
	{ EVT_CMD_WARP, 3, "WARP", &CEvt::_cmd_WARP },
	{ EVT_CMD_WIN, 3, "WIN", &CEvt::_cmd_WIN },
	{ 0 },
};

// the event system's own object, and the camera states scripts save and load
XObj gEventScriptObject;
CCameraMemento gEventCameraMementos[4];

// the game object

/**
 * @note Address: 0x8006FD8C
 * @note Size: 0x48C
 */
// CEvt::_cmd_WIN
#define EVT_PLAYER (*gGame.GetPlayer())

extern "C" s32 GetStageNum(void);

// WARP, ABORT and RETURN all finish through this
static inline void CEvt_End(CEvt* evt)
{
	evt->mData.mScript = 0;
	// a window MSG left up for STAY is idle again
	if (evt->mMsg != 0 && evt->mMsgState != 0) {
		evt->mMsgState   = 0;
		evt->mMsg->mIdle = 1;
	}
	Inventory* inventory = &EVT_PLAYER.mParams.mInventory;
	inventory->mCompact  = TRUE;
	CompactInventory(inventory);
	evt->mData.mScript = 0;
}

/**
 * @note Address: 0x80070304
 * @note Size: 0x1B9C
 */
// CEvt::_cmd_WAIT
#if defined(VERSION_PAL)
// the refresh rate, 50 or 60
#define FRAMES_PER_SEC gVideoFramesPerSecond
#else
#define FRAMES_PER_SEC 60
#endif

/**
 * @note Address: 0x80071EA0
 * @note Size: 0x98
 */
s32 CEvt::_cmd_TIMER(u8, s32)
{
	// first arg is in seconds
	s32 frames = CEvt_Evaluate(this) * FRAMES_PER_SEC;
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

extern "C" void LatchEventBeat(s32 id, s32 beat, void* context);

/**
 * @note Address: 0x80071F38
 * @note Size: 0x2DC
 */
s32 CEvt::_cmd_SFX(u8, s32)
{
	s32 sfx  = CEvt_Evaluate(this);
	s16 type = CEvt_Evaluate(this);

	switch (type) {
	case EVT_SFX_STOP:
		if (sfx == -2) {
			KeyOffGameSoundTrack(&gGame.mSound, -1);
		} else {
			StopGameSfx(&gGame.mSound, sfx);
		}
		break;
	case EVT_SFX_PLAY:
		PlayGameSound(&gGame.mSound, sfx, 0xFF);
		gEventBeatLatch.mPending = 0;
		AXDriverSetAllStreamParams(LatchEventBeat, &gEventBeatLatch);
		break;
	case EVT_ATOM_CUE:
		// here the value is a stream path, not an id
		HSD_AudioPStreamStartChParam((const char*)sfx, 0xFF, 0x1F);
		gEventBeatLatch.mPending = 0;
		AXDriverSetAllStreamParams(LatchEventBeat, &gEventBeatLatch);
		break;
	case EVT_SFX_FADE_OUT:
		FadeOutGameSfx(&gGame.mSound, sfx, CEvt_Evaluate(this) * 2);
		break;
	case EVT_SFX_FADE_IN:
		FadeInGameSfx(&gGame.mSound, sfx, CEvt_Evaluate(this) * 2);
		break;
	case EVT_SFX_FADE: {
		XSound* sound;
		u16 frames;
		u8 vol;
		frames = CEvt_Evaluate(this) * 2;
		vol    = CEvt_Evaluate(this);
		sound  = &gGame.mSound;
		if (!IsGameSoundPlaying(sound, sfx)) {
			PlayGameSound(sound, sfx, vol);
		} else {
			SetGameSfxVolume(sound, sfx, vol, frames);
		}
		break;
	}
	case EVT_SFX_PAUSE:
		if (sfx == -2) {
			PauseGameSoundTrack(&gGame.mSound, -1);
		} else if ((sfx & 0xFFFF0000) == 0xFFFF0000) {
			// ids from 0xFFFF0000 up are music tracks
			PauseGameMusic(&gGame.mSound, sfx, 0);
		} else {
			AXDriverPauseSfx(sfx);
		}
		break;
	case EVT_SFX_RESUME:
		if (sfx == -2) {
			ResumeGameSoundTrack(&gGame.mSound, -1);
		} else {
			ResumeGameSfx(&gGame.mSound, sfx);
		}
		break;
	default:
#if defined(VERSION_PAL)
		OSPanic(__FILE__, 3820, "%s%s", FormatToStaticBuffer("sfx(%d,%d) - unresolved!\n", sfx, type), __FUNCTION__);
#else
		OSPanic(__FILE__, 3809, "%s%s", FormatToStaticBuffer("sfx(%d,%d) - unresolved!\n", sfx, type), __FUNCTION__);
#endif
		break;
	}

	return 0;
}

/**
 * @note Address: 0x80072214
 * @note Size: 0x10
 */
extern "C" void LatchEventBeat(s32 id, s32 beat, void* context)
{
	EvtBeatLatch* latch = static_cast<EvtBeatLatch*>(context);

	latch->mPending = 1;
	latch->mBeat    = beat;
}

/**
 * @note Address: 0x80073768
 * @note Size: 0x4D0
 */
// CEvt::_cmd_READ
/**
 * @note Address: 0x80074614
 * @note Size: 0x38
 */
s32 CEvt::_cmd_PRINTF(u8, s32)
{
	// Printing is disabled. Consume the string operand.
	u8 c;
	u8* str;
	do {
		str           = mData.mScript;
		c             = *str;
		mData.mScript = str + 1;
	} while (c != 0);

	return 0;
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
 * @note Address: 0x8006FCAC
 * @note Size: 0xE0
 */
extern "C" void RegisterEventObjectId(CEvt*, s16 id, s16 index)
{
	if (CEvt_FindObj(id) == NULL) {
		gEventObjectIds.mEntry[gEventObjectIds.mCount].mId    = id;
		gEventObjectIds.mEntry[gEventObjectIds.mCount].mIndex = index;
		++gEventObjectIds.mCount;
	}
}

// ids from 10000 up are script names, 0 and up slots in gGame.mXObjs, -1 the script's own object, anything lower a pointer
static inline XObj* CEvt_GetObj(CEvt* evt, s16 id)
{
	if (id >= 10000) {
		return CEvt_FindObj(id);
	}
	if (id >= 0) {
		return gGame.mXObjs.mObjects[id];
	}
	if (id == -1) {
		return (XObj*)evt->mData.mContext;
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

/**
 * @note Address: 0x80073C38
 * @note Size: 0x9DC
 */
s32 CEvt::_cmd_PTCL(u8, s32)
{
	u32 code = CEvt_Evaluate(this);
	s16 type = CEvt_Evaluate(this);
	Vec translate, scale;
	if (type == EVT_ATOM_POS) {
		translate.x  = CEvt_Evaluate(this) / 100.0f;
		translate.y  = CEvt_Evaluate(this) / 100.0f;
		translate.z  = CEvt_Evaluate(this) / 100.0f;
		scale.x      = CEvt_Evaluate(this) / 100.0f;
		scale.y      = CEvt_Evaluate(this) / 100.0f;
		scale.z      = CEvt_Evaluate(this) / 100.0f;
		f32 angle    = CEvt_Evaluate(this);
		Vec rotation = { 0.0f, 0.0f, 0.0f };
		rotation.y   = DEG2RAD * angle;
		(&gGame.mParticle)->CreateSRT(code, 1, translate, scale, rotation, TRUE);
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
			translate    = obj->mMoveTranslate + translate;
			Vec rotation = { 0.0f, 0.0f, 0.0f };
			rotation.y   = DEG2RAD * angle;
			(&gGame.mParticle)->CreateSRT(code, 1, translate, scale, rotation, TRUE);
		} else {
			// Diagnostic formatting still writes the shared text buffer.
			FormatToStaticBuffer("ptcl(..., @OBJ, %d, ...) obj(%d) killed!", id, id);
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
			if (obj->mJObj == NULL) {
				FormatToStaticBuffer("ptcl(%d, @LEAD, %d, ...) ... obj(%d) is collision!\n", code, id, id);
			}
			(&gGame.mParticle)->Create(code, 0, GetJointByPath(obj->mJObj, path, depth), TRUE);
		} else {
			FormatToStaticBuffer("ptcl(..., @LEAD, %d, ...) obj(%d) killed!", id, id);
		}
	} else if (type == EVT_ATOM_KILL) {
		(&gGame.mParticle)->Kill((s32)code);
	} else {
#if defined(VERSION_PAL)
		const s32 line = 3315;
#else
		const s32 line = 3304;
#endif
		OSPanic("evt.cpp", line, "%s%s", FormatToStaticBuffer("ptcl(...) - invalid type - %d!\n", type), "_cmd_PTCL");
	}
	return 0;
}
/**
 * @note Address: 0x8007311C
 * @note Size: 0x64C
 */
s32 CEvt::_cmd_SCALE(u8 cmd, s32)
{
	s16 id    = CEvt_Evaluate(this);
	XObj* obj = CEvt_GetObj(this, id);
	f32 x     = CEvt_Evaluate(this) / 100.0f;
	f32 y     = CEvt_Evaluate(this) / 100.0f;
	f32 z     = CEvt_Evaluate(this) / 100.0f;
	if (cmd == EVT_CMD_SCALE) {
		if (obj != NULL && (obj->mStatus.mFlags & 1)) {
			obj->SetScale(x, y, z, 0);
			UpdateObjectFloorHeights(obj);
			UpdateObjectFloorXZExtent(obj);
		} else {
			FormatToStaticBuffer("scale(%d, ...) obj(%d) killed!", id, id);
		}
	} else {
		f32 time = CEvt_Evaluate(this) / 100.0f;
		if (obj != NULL && (obj->mStatus.mFlags & 1)) {
			if (obj->mJObj == NULL) {
				FormatToStaticBuffer("mscale(%d, %d,%d,%d) ... obj(%d) is collision!", id, (s32)(100.0f * x), (s32)(100.0f * y),
				                     (s32)(100.0f * z), id);
			}
			if (time == 0.0f) {
				obj->SetScale(x, y, z, 0);
				UpdateObjectFloorHeights(obj);
				UpdateObjectFloorXZExtent(obj);
			} else {
#if defined(VERSION_PAL)
				CVec step = (CVec(x, y, z) - obj->mMoveScale) / (gVideoFramesPerSecond * time);
#else
				CVec step = (CVec(x, y, z) - obj->mMoveScale) / (60.0f * time);
#endif
				obj->mMoveScale.mTarget.Set(x, y, z);
				step.x = Abs(step.x);
				step.y = Abs(step.y);
				step.z = Abs(step.z);
				obj->mMoveScale.mSpeed.Set(step);
			}
		} else {
			FormatToStaticBuffer("mscale(%d, ...) obj(%d) killed!", id, id);
		}
	}
	return 0;
}
static inline XObj* CEvt_FindObjUnchecked(s16 id)
{
	for (s32 i = 0; i < gEventObjectIds.mCount; i++) {
		if (gEventObjectIds.mEntry[i].mId == id) {
			XObj* obj = gGame.mXObjs.mObjects[gEventObjectIds.mEntry[i].mIndex];
			if (obj != NULL) {
				return obj;
			}
			return NULL;
		}
	}
	return NULL;
}

/**
 * @note Address: 0x80073768
 * @note Size: 0x4D0
 */
s32 CEvt::_cmd_READ(u8, s32)
{
	u16 type = CEvt_Evaluate(this);
	switch (type) {
	case EVT_ATOM_ANIM: {
		s16 id     = CEvt_Evaluate(this);
		XObj* obj  = CEvt_GetObj(this, id);
		char* path = (char*)CEvt_Evaluate(this);
		if (obj != NULL && (obj->mStatus.mFlags & 1)) {
			if (obj->mJObj == NULL) {
				FormatToStaticBuffer("read(@ANIM, %d, %s) ... obj(%d) is collision!", id, path, id);
			}
			// an anim from mAnim is playing and this read reloads mAnim: switch to the model's own anim with that index
			if (obj->mStatus.mFlags & 0x0008000000000000) {
				HSD_AObj* anim = GetAObj(obj->mJObj);
				if (anim != NULL && !(anim->flags & AOBJ_NO_ANIM)) {
					obj->SetAnim(obj->mAnimId % 1000, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				}
			}
			QueueObjectAnimationRead(obj, path);
		} else {
			FormatToStaticBuffer("read(@ANIM, %d, ...) obj(%d) killed!", id, id);
		}
		break;
	}
	case EVT_ATOM_SFX: {
		s16 id = CEvt_Evaluate(this);
		if (id >= 10000) {
			CEvt_FindObjUnchecked(id);
		}
		const char* name = (const char*)CEvt_Evaluate(this);
		RegisterGameSoundName(&gGame.mSound, name);
		break;
	}
	default:
#if defined(VERSION_PAL)
		const s32 line = 3379;
#else
		const s32 line = 3368;
#endif
		OSPanic("evt.cpp", line, "%s%s", FormatToStaticBuffer("read(...) - invalid type - %d!\n", type), "_cmd_READ");
		break;
	}
	return 0;
}

extern "C" void CMsg_ResetPos(CMsg* msg);

/**
 * @note Address: 0x8006FD8C
 * @note Size: 0x48C
 */
s32 CEvt::_cmd_WIN(u8, s32)
{
	switch ((u16)CEvt_Evaluate(this)) {
	case EVT_ATOM_POS: {
		f32 x = CEvt_Evaluate(this);
		f32 y = CEvt_Evaluate(this);
		if (mMsg != NULL) {
			mMsg->mWindowX        = x;
			mMsg->mWindowY        = y;
			mMsg->mPlacement      = NULL;
			mMsg->mFontPosition.x = 0.0f;
			mMsg->mFontPosition.y = 0.0f;
			mMsg->mFontPosition.z = 0.0f;
		}
		break;
	}
	case EVT_ATOM_LETTERBOX:
		if (mMsg != NULL) {
			mMsg->mWindowX        = 320.0f;
			mMsg->mWindowY        = 378.5f;
			mMsg->mPlacement      = NULL;
			mMsg->mFontPosition.x = 0.0f;
			mMsg->mFontPosition.y = 0.0f;
			mMsg->mFontPosition.z = 0.0f;
		}
		break;
	case EVT_ATOM_OBJ: {
		s16 id    = CEvt_Evaluate(this);
		XObj* obj = CEvt_GetObj(this, id);
		f32 x     = CEvt_Evaluate(this) / 100.0f;
		f32 y     = CEvt_Evaluate(this) / 100.0f;
		f32 z     = CEvt_Evaluate(this) / 100.0f;
		if (obj != NULL && (obj->mStatus.mFlags & 1)) {
			if (mMsg != NULL) {
				mMsg->mPlacement      = obj;
				mMsg->mFontPosition.x = x;
				mMsg->mFontPosition.y = y;
				mMsg->mFontPosition.z = z;
			}
		} else {
			FormatToStaticBuffer("win(@OBJ, %d, ...) obj(%d) killed!", id, id);
		}
		break;
	}
	case EVT_ATOM_COLOR: {
		u8 r = CEvt_Evaluate(this);
		u8 g = CEvt_Evaluate(this);
		u8 b = CEvt_Evaluate(this);
		u8 a = CEvt_Evaluate(this);
		if (mMsg != NULL) {
			GXColor color      = sEvtWindowColorTemplate;
			color.r            = r;
			color.g            = g;
			color.b            = b;
			color.a            = a;
			mMsg->mFont.mColor = color;
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
 * @note Address: 0x80070218
 * @note Size: 0xEC
 */
s32 CEvt::_cmd_WARP(u8, s32)
{
	s32 stage    = CEvt_Evaluate(this);
	s32 showName = CEvt_Evaluate(this);
	// 10000 and 20000 are taken as is, 100 and up counts from the current stage
	if (stage != 10000 && stage != 20000 && stage >= 100) {
		stage += GetStageNum() - 99;
	}
	ChangeGameStage(&gGame, stage, showName);
	CEvt_End(this);
	return 1;
}

/**
 * @note Address: 0x8007464C
 * @note Size: 0x390
 */
s32 CEvt::_cmd_POS(u8, s32)
{
	s16 id    = CEvt_Evaluate(this);
	XObj* obj = CEvt_GetObj(this, id);
	f32 x     = CEvt_Evaluate(this) / 100.0f;
	f32 y     = CEvt_Evaluate(this) / 100.0f;
	f32 z     = CEvt_Evaluate(this) / 100.0f;
	if (obj != NULL && (obj->mStatus.mFlags & 1)) {
		obj->SetPos(x, y, z, 0);
		// mLastPos is overwritten below, so flag the floor heights as stale for UpdateObject
		if (obj->mMoveTranslate.y != obj->mLastPos.y) {
			obj->mStatus.mFlags |= 0x0010000000000000;
		}
		obj->mLastPos.x = obj->mMoveTranslate.x;
		obj->mLastPos.y = obj->mMoveTranslate.y;
		obj->mLastPos.z = obj->mMoveTranslate.z;
		gGame.mMotionControlCamera.UpdateTarget(obj);
		XActLift* lift;
		XObjChr* player = gGame.GetPlayer();
		if (obj == player) {
			ResetPlayerHeadOffset(player);
			ResetPlayerPlug(player);
			lift = &player->mLift;
			if (lift->mTarget != NULL) {
				UpdateHeldObject(lift, 1);
				lift->mTarget->UpdateObject(1);
			}
			CancelPlayerPushAction(&player->mPush);
		}
	} else {
		FormatToStaticBuffer("pos(%d, ...) obj(%d) killed!", id, id);
	}
	return 0;
}

// CEvt::_cmd_POS
/**
 * @note Address: 0x800749DC
 * @note Size: 0x1E8
 */
s32 CEvt::_cmd_MSG(u8 cmd, s32)
{
	CMsg* owner = mMsg;
	if (owner != 0) {
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
			CMsg_Open(owner, cmd, mData.mScript + 4, (MsgFormatArgs*)&mValues.mBuf);
			mMsg->mIdle = 0;
			// back onto the opcode and yield: MSG runs again each frame until the message is done
			mData.mScript--;
			if (cmd == EVT_CMD_SELECT) {
				PlayGameSound(&gGame.mSound, 0x86, 0xFF);
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
 * @note Address: 0x80074BC4
 * @note Size: 0x130
 */
s32 CEvt::_cmd_MOVIE(u8, s32)
{
	const char* path = (const char*)CEvt_Evaluate(this);
	if (path != NULL) {
#if defined(VERSION_PAL)
		// at 50hz europe plays its own 25 fps cut of the movie
		char buf[0x80];
		if (gVideoFramesPerSecond == 50) {
			sprintf(buf, "%s%s", path, "25");
			path = buf;
		}
#endif
		s32 x     = CEvt_Evaluate(this);
		s32 y     = CEvt_Evaluate(this);
		s32 w     = CEvt_Evaluate(this);
		s32 h     = CEvt_Evaluate(this);
		s32 flags = CEvt_Evaluate(this);
		// x and y are the center, the rect wants the corner
		x -= w / 2;
		y -= h / 2;
		// bit 0 of flags plays the movie with alpha
		StartGameMovie(&gGame, path, FALSE, flags);
		gGame.mMovieRect[0] = x;
		gGame.mMovieRect[1] = y;
		gGame.mMovieRect[2] = w;
		gGame.mMovieRect[3] = h;
	} else {
		StopGameMovie(&gGame);
	}
	return 0;
}

// CEvt::_cmd_MOVIE
// the y angle that faces from toward to
static inline f32 CEvt_Angle(const Vec& from, const Vec& to)
{
	return atan2(to.x - from.x, to.z - from.z);
}

/**
 * @note Address: 0x80074CF4
 * @note Size: 0x760
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
		if (obj->mJObj == NULL) {
			FormatToStaticBuffer("moveto(%d, %d,%d,%d, %d, %d, %d) ... obj(%d) is collision!", id, (s32)(x * 100.0f), (s32)(y * 100.0f),
			                     (s32)(z * 100.0f), (s32)(time * 100.0f), anim, dir, id);
		}
		XObjChr* player = gGame.GetPlayer();
		if (obj == player) {
			CancelPlayerPushAction(&player->mPush);
		}
		f32 distance;
		{
			CVec target(x, y, z);
			distance = PSVECDistance(&obj->mMoveTranslate, &target);
		}
		if (distance < 0.1f) {
			// already there
			obj->SetPos(x, y, z, 0);
		} else {
			// time 0 gets there in one frame, a positive time is seconds, a negative one the speed per frame
			if (time == 0.0f) {
				obj->SetMoveDir(CVec(x, y, z), distance);
			} else if (time > 0.0f) {
				obj->SetMoveDir(CVec(x, y, z), distance / (time * FRAMES_PER_SEC));
			} else {
				obj->SetMoveDir(CVec(x, y, z), -time);
			}
			if (dir != -1) {
				f32 yaw;
				// -2 faces where it's going
				if (dir == -2) {
					yaw = CEvt_Angle(obj->mMoveTranslate, CVec(x, y, z));
				} else {
					yaw = DEG2RAD * dir;
				}
				obj->SetRotationY(yaw, 0);
			}
			if (anim != -1) {
				if (anim & 0xFFFF0000) {
					s32 jointId     = anim & 0xFFFF;
					s32 matId       = anim >> 16;
					void* jointAnim = obj->GetJointAnimation(jointId);
					void* matAnim   = obj->GetMaterialAnimation(matId);
					void* shapeAnim = obj->GetShapeAnimation(jointId);
					obj->SetAnim(0, TRUE, jointAnim, matAnim, shapeAnim, 0.0f, 1.0f, 0.0f, 10.0f);
					// XObj::SetAnim(0, ...) cleared the from-mAnim bit; set it again if either id is from mAnim (1000 up)
					if (jointId >= 1000 || matId >= 1000) {
						obj->mStatus.mFlags |= 0x0008000000000000;
					}
				} else {
					obj->SetAnim(anim, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				}
				obj->UpdateAnim(1);
			}
		}
	} else if (obj == NULL || !(obj->mStatus.mFlags & 1)) {
		FormatToStaticBuffer("moveto(%d, ...) obj(%d) killed!", id, id);
	}
	return 0;
}
/**
 * @note Address: 0x80075454
 * @note Size: 0x62C
 */
s32 CEvt::_cmd_MOVE(u8, s32)
{
	s16 id          = CEvt_Evaluate(this);
	XObj* obj       = CEvt_GetObj(this, id);
	s32 distanceArg = CEvt_Evaluate(this);
	f32 distance    = distanceArg / 100.0f;
	f32 time        = CEvt_Evaluate(this) / 100.0f;
	s32 anim        = CEvt_Evaluate(this);
	s16 dir         = CEvt_Evaluate(this);
	// only a shown object moves (DISP sets 0x10000), a hidden one is skipped without a warning
	if (obj != NULL && (obj->mStatus.mFlags & 1) && (obj->mStatus.mFlags & 0x10000)) {
		if (obj->mJObj == NULL) {
			FormatToStaticBuffer("move(%d, %d, %d, %d, %d) ... obj(%d) is collision!", id, distanceArg, (s32)(time * 100.0f), anim, dir,
			                     id);
		}
		XObjChr* player = gGame.GetPlayer();
		if (obj == player) {
			CancelPlayerPushAction(&player->mPush);
		}
		Vec target = obj->mMoveTranslate;
		// -1 and -2 keep the way it faces
		f32 yaw = (dir == -1 || dir == -2) ? obj->mMoveRotate.y : DEG2RAD * dir;
		target.x += distance * (f32)sin(yaw);
		target.z += distance * (f32)cos(yaw);
		// time 0 gets there in one frame, a positive time is seconds, a negative one the speed per frame
		if (time == 0.0f) {
			obj->SetMoveDir(target, distance);
		} else if (time > 0.0f) {
			obj->SetMoveDir(target, distance / (time * FRAMES_PER_SEC));
		} else {
			obj->SetMoveDir(target, -time);
		}
		if (anim != -1) {
			if (anim & 0xFFFF0000) {
				s32 jointId     = anim & 0xFFFF;
				s32 matId       = anim >> 16;
				void* jointAnim = obj->GetJointAnimation(jointId);
				void* matAnim   = obj->GetMaterialAnimation(matId);
				void* shapeAnim = obj->GetShapeAnimation(jointId);
				obj->SetAnim(0, TRUE, jointAnim, matAnim, shapeAnim, 0.0f, 1.0f, 0.0f, 10.0f);
				// XObj::SetAnim(0, ...) cleared the from-mAnim bit; set it again if either id is from mAnim (1000 up)
				if (jointId >= 1000 || matId >= 1000) {
					obj->mStatus.mFlags |= 0x0008000000000000;
				}
			} else {
				obj->SetAnim(anim, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			}
			obj->UpdateAnim(1);
		}
	} else if (obj == NULL || !(obj->mStatus.mFlags & 1)) {
		FormatToStaticBuffer("move(%d, ...) obj(%d) killed!", id, id);
	}
	return 0;
}

/**
 * @note Address: 0x80075A80
 * @note Size: 0x19C
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
			SelectPlayerEquipmentRing(&EVT_PLAYER, ring);
		} else {
			XObjChr* player = gGame.GetPlayer();
			bool ringsOpen  = false;
			if (player->mMenu.mAtc.mOpen != 0 || player->mMenu.mDrs.mOpen != 0) {
				ringsOpen = true;
			}
			if (ringsOpen) {
				ClosePlayerEquipmentMenu(player, -1);
			}
		}
	} else if (id == 1001) {
		s32 atcItem                                            = CEvt_Evaluate(this);
		s32 drsItem                                            = CEvt_Evaluate(this);
		gGame.GetPlayer()->mMenu.mAtc.mPendingFirstVisibleItem = atcItem;
		gGame.GetPlayer()->mMenu.mDrs.mPendingFirstVisibleItem = drsItem;
	} else {
		CMenu* menu = gGame.mMenu.mMenus[id];
#if defined(VERSION_JP)
		// no null check in japan
		menu->Open();
		gGame.mMenu.mActive = 1;
#else
		if (menu != 0) {
			menu->Open();
			gGame.mMenu.mActive = 1;
		}
#endif
		mMenuActive   = 1;
		ret           = 1;
		mData.mScript = cmd;
	}
	return ret;
}

// walked through the lobj next links

/**
 * @note Address: 0x80075C1C
 * @note Size: 0x290
 */
s32 CEvt::_cmd_LIGHT(u8, s32)
{
	s32 index     = CEvt_Evaluate(this);
	CLight* light = gGame.mLightList;
	for (s32 i = 0; i < index; i++) {
		light = (CLight*)light->next;
		if (light == NULL) {
			break;
		}
	}

	u16 type = CEvt_Evaluate(this);
	switch (type) {
	case EVT_ATOM_COLOR: {
		u8 r            = CEvt_Evaluate(this);
		u8 g            = CEvt_Evaluate(this);
		u8 b            = CEvt_Evaluate(this);
		light->mColor.r = r;
		light->mColor.g = g;
		light->mColor.b = b;
		light->mColor.a = 255;
		HSD_LObjSetColor(light, light->mColor);
		break;
	}
	case EVT_ATOM_POS: {
		// positions come in hundredths
		f32 x = CEvt_Evaluate(this) / 100.0f;
		f32 y = CEvt_Evaluate(this) / 100.0f;
		light->SetPosition(x, y, CEvt_Evaluate(this) / 100.0f);
		break;
	}
	case EVT_ATOM_INTEREST: {
		f32 x = CEvt_Evaluate(this) / 100.0f;
		f32 y = CEvt_Evaluate(this) / 100.0f;
		light->SetInterest(x, y, CEvt_Evaluate(this) / 100.0f);
		break;
	}
	default:
#if defined(VERSION_PAL)
		OSPanic(__FILE__, 2846, "%s%s", FormatToStaticBuffer("light(%d, %d,...) - unresolved!\n", index, type), __FUNCTION__);
#else
		OSPanic(__FILE__, 2842, "%s%s", FormatToStaticBuffer("light(%d, %d,...) - unresolved!\n", index, type), __FUNCTION__);
#endif
		break;
	}
	return 0;
}

/**
 * @note Address: 0x80075EAC
 * @note Size: 0x400
 */
s32 CEvt::_cmd_KILL(u8, s32)
{
	s16 id    = CEvt_Evaluate(this);
	XObj* obj = CEvt_GetObj(this, id);
	if (obj != NULL && (obj->mStatus.mFlags & 1)) {
		XObj* held = gGame.GetPlayer()->mLift.mTarget;
		if (held != NULL && obj == held && (obj->mStatus.mFlags & 0x0000400000000000)) {
			// chibi is carrying it, can't kill that
			FormatToStaticBuffer("kill(%d) ... obj(%d) lifted!", id, id);
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
		StagePlacement* entry = GetStagePlacementByIndex(index);
		CoordInfo info(entry);
		entry->mFlags |= 0x80000000ULL;
	}
	return 0;
}

/**
 * @note Address: 0x800762AC
 * @note Size: 0x358
 */
s32 CEvt::_cmd_DISP(u8, s32)
{
	s16 id    = CEvt_Evaluate(this);
	XObj* obj = CEvt_GetObj(this, id);
	u8 on     = CEvt_Evaluate(this);
	if (obj != NULL && (obj->mStatus.mFlags & 1)) {
		XObj* held = gGame.GetPlayer()->mLift.mTarget;
		if (held != NULL && obj == held && (obj->mStatus.mFlags & 0x0000400000000000)) {
			// chibi is carrying it, leave it be
			FormatToStaticBuffer("disp(%d, %d) ... obj(%d) lifted!", id, on, id);
		} else if (on) {
			obj->mStatus.mFlags |= 0x10000;
		} else {
			obj->mStatus.mFlags &= ~0x10000;
		}
	} else {
		FormatToStaticBuffer("disp(%d, %d) ... obj(%d) killed!", id, on, id);
	}
	return 0;
}

/**
 * @note Address: 0x80076604
 * @note Size: 0xC08
 */
s32 CEvt::_cmd_DIR(u8 command, s32)
{
	s16 id    = CEvt_Evaluate(this);
	XObj* obj = CEvt_GetObj(this, id);
	if (obj != NULL && (obj->mStatus.mFlags & 1)) {
		XObjChr* player = gGame.GetPlayer();
		if (obj == player)
			CancelPlayerPushAction(&player->mPush);
	}
	if (command == EVT_CMD_DIR) {
		s16 direction = CEvt_Evaluate(this);
		if (obj != NULL && (obj->mStatus.mFlags & 1)) {
			obj->SetRotationY(DEG2RAD * direction, 0);
			gGame.mMotionControlCamera.UpdateTarget(obj);
			XObjChr* player = gGame.GetPlayer();
			if (obj == player) {
				ResetPlayerHeadOffset(player);
				XActLift* carry = &player->mLift;
				if (carry->mTarget != NULL) {
					UpdateHeldObject(carry, 1);
					carry->mTarget->UpdateObject(1);
				}
			}
		}
	} else {
		u16 mode = CEvt_Evaluate(this);
		s32 direction;
		switch (mode) {
		case EVT_ATOM_DIR:
			direction = (s16)CEvt_Evaluate(this);
			break;
		case EVT_ATOM_POS: {
			f32 x = CEvt_Evaluate(this) / 100.0f;
			f32 z = CEvt_Evaluate(this) / 100.0f;
			if (obj != NULL && (obj->mStatus.mFlags & 1)) {
				direction = RAD2DEG * (f32)atan2(x - obj->mMoveTranslate.x, z - obj->mMoveTranslate.z);
			} else {
				FormatToStaticBuffer("mdir(%d, @POS, ...): obj(%d) killed!", id, id);
			}
			break;
		}
		case EVT_ATOM_OBJ: {
			s16 targetId = CEvt_Evaluate(this);
			XObj* target = CEvt_GetObj(this, targetId);
			if (obj != NULL && (obj->mStatus.mFlags & 1)) {
				if (target != NULL && (target->mStatus.mFlags & 1)) {
					direction
					    = RAD2DEG
					    * (f32)atan2(target->mMoveTranslate.x - obj->mMoveTranslate.x, target->mMoveTranslate.z - obj->mMoveTranslate.z);
				} else {
					FormatToStaticBuffer("mdir(%d, @OBJ, %d, ...): obj(%d) killed!", id, targetId, targetId);
					direction = RAD2DEG * obj->mMoveRotate.y;
				}
			} else {
				FormatToStaticBuffer("mdir(%d, @OBJ, ...): obj(%d) killed!", id, id);
			}
			break;
		}
		case EVT_ATOM_CAM:
			if (obj != NULL && (obj->mStatus.mFlags & 1)) {
				CCamera* camera = &gGame.mMotionControlCamera.mCamera;
				direction       = RAD2DEG * (f32)atan2(camera->mEye.x - obj->mMoveTranslate.x, camera->mEye.z - obj->mMoveTranslate.z);
			} else {
				FormatToStaticBuffer("mdir(%d, @CAM, ...): obj(%d) killed!", id, id);
			}
			break;
		default:
#if defined(VERSION_PAL)
			const s32 line = 2671;
#else
			const s32 line = 2667;
#endif
			OSPanic("evt.cpp", line, "%s%s", FormatToStaticBuffer("mdir(obj, %d,...) - unresolved!\n", mode), "_cmd_DIR");
			break;
		}
		f32 time = CEvt_Evaluate(this) / 100.0f;
		s32 anim = CEvt_Evaluate(this);
		if (obj != NULL && (obj->mStatus.mFlags & 1) && (obj->mStatus.mFlags & 0x10000)) {
			if (obj->mJObj == NULL) {
				FormatToStaticBuffer("mdir(%d, %d, %d, %d, %d) ... obj(%d) is collision!", id, mode, (s16)direction, (s32)(100.0f * time),
				                     anim, id);
			}
			f32 yaw        = DEG2RAD * (s16)direction;
			f32 difference = obj->mMoveRotate.y - yaw;
			while (difference < 0.0f)
				difference += TWO_PI;
			while (difference >= TWO_PI)
				difference -= TWO_PI;
			// already within a tenth of a degree of it: set it instead of turning
			if (difference < 0.0017453292f || difference > 6.2814398f) {
				obj->SetRotationY(yaw, 0);
			} else {
				obj->SetRotateY(yaw, time);
				f32 angle = obj->mMoveRotate.mTarget.y - obj->mMoveRotate.y;
				if (angle > PI)
					angle = TWO_PI - angle;
				obj->mMoveRotate.mSpeed.x = 0.0f;
				if (angle < 0.0f)
					angle = -angle;
				obj->mMoveRotate.mSpeed.y = angle / (FRAMES_PER_SEC * time);
				obj->mMoveRotate.mSpeed.z = 0.0f;
				if (anim != -1) {
					if (anim & 0xFFFF0000) {
						s32 jointId     = anim & 0xFFFF;
						s32 matId       = anim >> 16;
						void* jointAnim = obj->GetJointAnimation(jointId);
						void* matAnim   = obj->GetMaterialAnimation(matId);
						void* shapeAnim = obj->GetShapeAnimation(jointId);
						obj->SetAnim(0, TRUE, jointAnim, matAnim, shapeAnim, 0.0f, 1.0f, 0.0f, 10.0f);
						// SetAnim with id 0 cleared the anim archive bit; ids from 1000 up are archive anims
						if (jointId >= 1000 || matId >= 1000)
							obj->mStatus.mFlags |= 0x0008000000000000;
					} else {
						obj->SetAnim(anim, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
					}
					obj->UpdateAnim(1);
				}
			}
		}
	}
	return 0;
}
/**
 * @note Address: 0x8007720C
 * @note Size: 0x64C
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
			if ((current->mStatus.mFlags & 1) && current->mJObj != NULL) {
				if (mode == EVT_ATOM_MODULATE) {
					current->SetColor(0, r, g, b, time);
				} else if (mode == EVT_ATOM_BLEND) {
					current->SetColor(1, r, g, b, time);
				}
			}
		}
	} else if (obj != NULL && (obj->mStatus.mFlags & 1)) {
		if (obj->mJObj == NULL) {
			if (mode == EVT_ATOM_MODULATE) {
				FormatToStaticBuffer("color(%d, @MODULATE, %d,%d,%d, %d) ... obj(%d) is collision!", id, r, g, b, (s32)(100.0f * time), id);
			} else {
				FormatToStaticBuffer("color(%d, @BLEND, %d,%d,%d, %d) ... obj(%d) is collision!", id, r, g, b, (s32)(100.0f * time), id);
			}
		}
		if (mode == EVT_ATOM_MODULATE) {
			obj->SetColor(0, r, g, b, time);
		} else if (mode == EVT_ATOM_BLEND) {
			obj->SetColor(1, r, g, b, time);
		}
	} else {
		if (mode == EVT_ATOM_MODULATE) {
			FormatToStaticBuffer("color(%d, @MODULATE, %d,%d,%d, %d) ... obj(%d) killed!", id, r, g, b, (s32)(100.0f * time), id);
		} else {
			FormatToStaticBuffer("color(%d, @BLEND, %d,%d,%d, %d) ... obj(%d) killed!", id, r, g, b, (s32)(100.0f * time), id);
		}
	}
	return 0;
}
extern "C" void SetStageCameraAnimation(XObj* obj, s32 anim, f32 frame);

/**
 * @note Address: 0x80077858
 * @note Size: 0xEFC
 */
s32 CEvt::_cmd_CAMERA(u8, s32)
{
	CCameraActionEvt* action   = static_cast<CCameraActionEvt*>(gGame.mCameraActions.mTable[CAMACTION_EVT]);
	CCameraActionAreaExM* area = static_cast<CCameraActionAreaExM*>(gGame.mCameraActions.mTable[CAMACTION_AREA]);
	CCameraActionReset* reset  = static_cast<CCameraActionReset*>(gGame.mCameraActions.mTable[CAMACTION_RESET]);
	u16 command                = CEvt_Evaluate(this);
	switch (command) {
	case EVT_ATOM_AREA: {
		s32 areaIndex = CEvt_Evaluate(this);
		s32 entry     = CEvt_Evaluate(this);
		s32 mode      = CEvt_Evaluate(this);
		f32 time      = CEvt_Evaluate(this) / 100.0f;
		if (GetStageCameraInfoRecords() != NULL && area->IsCameraArea(areaIndex) != 0) {
			CMotionControlCamera* camera = &gGame.mMotionControlCamera;
			camera->ChangeAction(CAMACTION_AREA);
			SelectAreaCameraEntryWithControl((CCameraActionArea*)area, areaIndex, entry, mode, 0, time);
			// mode 3 jumps straight to the entry instead of easing, so apply it now
			if (mode == 3) {
				camera->ApplyAction();
			}
		}
		break;
	}
	case EVT_ATOM_POS: {
		f32 x    = CEvt_Evaluate(this) / 100.0f;
		f32 y    = CEvt_Evaluate(this) / 100.0f;
		f32 z    = CEvt_Evaluate(this) / 100.0f;
		s32 mode = CEvt_Evaluate(this);
		f32 time = CEvt_Evaluate(this) / 100.0f;
		gGame.mMotionControlCamera.ChangeAction(CAMACTION_EVT);
		action->mCameraFlags = 11;
		Vec eye              = { 0.0f, 0.0f, 0.0f };
		eye.x                = x;
		eye.y                = y;
		eye.z                = z;
		Vec target           = eye;
		action->SetEye_Interp(mode, target, time);
		break;
	}
	case 240:
	case 243:
	case 251:
	case 252: {
		s32 axis      = CEvt_Evaluate(this);
		f32 degrees   = CEvt_Evaluate(this);
		s32 mode      = CEvt_Evaluate(this);
		f32 time      = CEvt_Evaluate(this) / 100.0f;
		u32 type      = command == 240 ? 2 : 1;
		BOOL relative = (u16)(command - 251) <= 1 ? true : false;
		gGame.mMotionControlCamera.ChangeAction(CAMACTION_EVT);
		switch (axis) {
		case 2: {
			Vec angle  = { 0.0f, 0.0f, 0.0f };
			angle.y    = DEG2RAD * degrees;
			Vec target = angle;
			action->SetAngle_Interp(mode, target, 2, type, relative, time);
			break;
		}
		case 3: {
			Vec angle  = { 0.0f, 0.0f, 0.0f };
			angle.x    = DEG2RAD * degrees;
			Vec target = angle;
			action->SetAngle_Interp(mode, target, 1, type, relative, time);
			break;
		}
		case 5: {
			Vec angle  = { 0.0f, 0.0f, 0.0f };
			angle.z    = DEG2RAD * degrees;
			Vec target = angle;
			action->SetAngle_Interp(mode, target, 4, type, relative, time);
			break;
		}
		}
		break;
	}
	case EVT_ATOM_INTEREST: {
		f32 x    = CEvt_Evaluate(this) / 100.0f;
		f32 y    = CEvt_Evaluate(this) / 100.0f;
		f32 z    = CEvt_Evaluate(this) / 100.0f;
		s32 mode = CEvt_Evaluate(this);
		f32 time = CEvt_Evaluate(this) / 100.0f;
		gGame.mMotionControlCamera.ChangeAction(CAMACTION_EVT);
		action->mCameraFlags = 11;
		Vec interest         = { 0.0f, 0.0f, 0.0f };
		interest.x           = x;
		interest.y           = y;
		interest.z           = z;
		Vec target           = interest;
		action->SetInterest_Interp(mode, target, time);
		break;
	}
	case EVT_ATOM_OBJ: {
		s32 id    = CEvt_Evaluate(this);
		s32 mode  = CEvt_Evaluate(this);
		f32 time  = CEvt_Evaluate(this) / 100.0f;
		XObj* obj = CEvt_GetObj(this, (s16)id);
		if (obj != NULL) {
			Vec target = obj->GetCameraTarget();
			gGame.mMotionControlCamera.ChangeAction(CAMACTION_EVT);
			action->mCameraFlags = 11;
			Vec interest         = target;
			action->SetInterest_Interp(mode, interest, time);
		} else {
			FormatToStaticBuffer("cam(@OBJ, %d, ...): obj(%d) killed!", id, id);
		}
		break;
	}
	case EVT_ATOM_LEAD: {
		s32 id    = CEvt_Evaluate(this);
		XObj* obj = CEvt_GetObj(this, (s16)id);
		if (obj != NULL) {
			gGame.mMotionControlCamera.ChangeAction(CAMACTION_EVT);
			action->mCameraFlags = 11;
			action->SetInterest_Interp(3, obj, 0.0f);
		} else {
			FormatToStaticBuffer("cam(@LEAD, %d): obj(%d) killed!", id, id);
		}
		break;
	}
	case EVT_ATOM_DISTANCE:
		CEvt_Evaluate(this);
		CEvt_Evaluate(this);
		CEvt_Evaluate(this);
		break;
	case EVT_ATOM_FOV: {
		f32 fov  = CEvt_Evaluate(this);
		s32 mode = CEvt_Evaluate(this);
		f32 time = CEvt_Evaluate(this) / 100.0f;
		gGame.mMotionControlCamera.ChangeAction(CAMACTION_EVT);
		action->mCameraFlags = action->GetCameraFlags() | 8;
		action->SetFov_Interp(mode, fov, time);
		break;
	}
	case EVT_ATOM_ANIM: {
		s32 anim = CEvt_Evaluate(this);
		CEvt_Evaluate(this);
		CEvt_Evaluate(this);
		gGame.mMotionControlCamera.ChangeAction(CAMACTION_ANIM);
		// pool 0's object is the XObjCamera the anim camera plays
		XObj* obj = (XObj*)gGame.mXObjs.mPools[0].mObjects;
		if (obj != NULL && (obj->mStatus.mFlags & 1) != 0) {
			SetStageCameraAnimation(obj, anim, 0.0f);
		}
		break;
	}
	case EVT_ATOM_RESET: {
		s32 mode                     = CEvt_Evaluate(this);
		f32 time                     = CEvt_Evaluate(this) / 100.0f;
		CMotionControlCamera* camera = &gGame.mMotionControlCamera;
		if (camera->IsAction(CAMACTION_EVT)) {
			reset->mMode      = mode;
			reset->mTime      = time;
			reset->mImmediate = FALSE;
			camera->ChangeAction(CAMACTION_RESET);
		}
		break;
	}
	case 230: { // finish the active scripted camera
		CMotionControlCamera* camera = &gGame.mMotionControlCamera;
		if (camera->IsAction(CAMACTION_AREA)) {
			ResumeAreaCameraControl(static_cast<CCameraActionArea*>(gGame.mCameraActions.mTable[CAMACTION_AREA]));
		} else if (camera->IsAction(CAMACTION_EVT)) {
			reset->mMode      = 0;
			reset->mTime      = 0.0f;
			reset->mImmediate = TRUE;
			camera->ChangeAction(CAMACTION_RESET);
		}
		break;
	}
	case EVT_ATOM_SAVE: {
		CMotionControlCamera* camera = &gGame.mMotionControlCamera;
		camera->EndEye();
		s32 slot = CEvt_Evaluate(this);
		if (slot >= 0) {
			camera->CreateMemento(&gEventCameraMementos[slot]);
		} else {
			mCameraMemento = camera->CreateMemento(NULL);
		}
		break;
	}
	case EVT_ATOM_LOAD: {
		s32 slot = CEvt_Evaluate(this);
		if (slot >= 0) {
			gGame.mMotionControlCamera.SetMemento(&gEventCameraMementos[slot]);
		} else if (mCameraMemento != NULL) {
			gGame.mMotionControlCamera.SetMemento(mCameraMemento);
			mCameraMemento = NULL;
		}
		break;
	}
	case 238: { // area camera zoom level, passed negated; values above 0 do nothing
		s32 zoom = CEvt_Evaluate(this);
		if (zoom <= 0) {
			s32 sign = zoom >> 31;
			SetAreaCameraHeightIndex(&gGame.mMotionControlCamera, (zoom ^ sign) - sign);
		}
		break;
	}
	case 232: // first-person controls
		switch (CEvt_Evaluate(this)) {
		case -1:
			gGame.mStatus.mFlags &= ~0x80ULL;
			break;
		case -2:
			gGame.mStatus.mFlags |= 0x80ULL;
			break;
		case 0:
			gGame.mMotionControlCamera.EndEye();
			break;
		case 1:
			gGame.mMotionControlCamera.StartEye();
			break;
		case 2: {
			f32 degrees                                                                             = CEvt_Evaluate(this);
			static_cast<CCameraActionEye*>(gGame.mCameraActions.mTable[CAMACTION_EYE])->mOverride.y = DEG2RAD * degrees;
			break;
		}
		case 3: {
			f32 degrees                                                                             = CEvt_Evaluate(this);
			static_cast<CCameraActionEye*>(gGame.mCameraActions.mTable[CAMACTION_EYE])->mOverride.x = DEG2RAD * degrees;
			break;
		}
		case 4: {
			f32 fov                                                                                 = CEvt_Evaluate(this);
			static_cast<CCameraActionEye*>(gGame.mCameraActions.mTable[CAMACTION_EYE])->mOverride.z = fov;
			break;
		}
		}
		break;
	default:
#if defined(VERSION_PAL)
		OSPanic("evt.cpp", 2515, "%s%s", FormatToStaticBuffer("cam(%d,...) - unresolved!\n", command), "_cmd_CAMERA");
#else
		OSPanic("evt.cpp", 2511, "%s%s", FormatToStaticBuffer("cam(%d,...) - unresolved!\n", command), "_cmd_CAMERA");
#endif
		break;
	}
	return 0;
}

// CEvt::_cmd_CAMERA
/**
 * @note Address: 0x80078754
 * @note Size: 0x1D38
 */
// CEvt::_cmd_CALL
// null when the pool is full
static inline XObj* CEvt_Born(s32 index)
{
	CoordInfo info(GetSpawnData(index));
	return (XObj*)SpawnObjectFromPlacement(&gGame.mXObjs, &info, 1, FALSE);
}

/**
 * @note Address: 0x8007A48C
 * @note Size: 0x4C4
 */
s32 CEvt::_cmd_BORN(u8, s32)
{
	s16 id     = CEvt_Evaluate(this);
	s16 type   = CEvt_Evaluate(this);
	f32 x      = CEvt_Evaluate(this) / 100.0f;
	f32 y      = CEvt_Evaluate(this) / 100.0f;
	f32 z      = CEvt_Evaluate(this) / 100.0f;
	s16 rot    = CEvt_Evaluate(this);
	f32 sx     = CEvt_Evaluate(this) / 100.0f;
	f32 sy     = CEvt_Evaluate(this) / 100.0f;
	f32 sz     = CEvt_Evaluate(this) / 100.0f;
	u8* script = (u8*)CEvt_Evaluate(this);
	XObj* obj  = CEvt_FindObj(id);
	if (obj == NULL) {
		obj = CEvt_Born(type + 0x16);
		if (obj != NULL) {
			gGame.mStatus.mFlags |= 0x1000; // BORN raises it after a spawn
		}
	}
	if (obj != NULL && (obj->mStatus.mFlags & 1)) {
		obj->SetPos(x, y, z, 0);
		obj->SetRotationY(DEG2RAD * rot, 0);
		obj->SetScale(sx, sy, sz, 0);
		UpdateObjectFloorHeights(obj);
		UpdateObjectFloorXZExtent(obj);
		obj->mEventScript = script;
		s16 index         = FindStageObjectIndex(&gGame.mXObjs, (CXObj*)obj);
		if (CEvt_FindObj(id) == NULL) {
			gEventObjectIds.mEntry[gEventObjectIds.mCount].mId    = id;
			gEventObjectIds.mEntry[gEventObjectIds.mCount].mIndex = index;
			gEventObjectIds.mCount++;
		}
	} else {
		OSPanic(__FILE__, 1705, "%s%s", FormatToStaticBuffer("No more space for born(%d,...)-command!\n", id), __FUNCTION__);
	}
	return 0;
}

/**
 * @note Address: 0x8007A950
 * @note Size: 0x50C
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
			FormatToStaticBuffer("attach(%d, ...): obj(%d) killed!", id, id);
		}
	} else if (obj != NULL && (obj->mStatus.mFlags & 1)) {
		obj->mEvt.mData.mScript = NULL;
		if (obj->mEvt.mMsg != NULL && obj->mEvt.mMsgState != 0) {
			obj->mEvt.mMsgState   = 0;
			obj->mEvt.mMsg->mIdle = 1;
		}
		Inventory* inventory = &EVT_PLAYER.mParams.mInventory;
		inventory->mCompact  = TRUE;
		CompactInventory(inventory);
	} else {
		FormatToStaticBuffer("detach(%d, ...): obj(%d) killed!", id, id);
	}
	return 0;
}
/**
 * @note Address: 0x8007AE5C
 * @note Size: 0x630
 */
s32 CEvt::_cmd_ANIM2(u8 command, s32)
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
			if (obj->mJObj == NULL) {
				FormatToStaticBuffer("anim2(%d, %d,%d,%d) ... obj(%d) is collision!", id, anim, frame, rate, id);
			} else if (count == 0) {
				obj->mAnmQueue.Clear();
				if (anim & 0xFFFF0000) {
					s32 jointId     = anim & 0xFFFF;
					s32 matId       = anim >> 16;
					void* jointAnim = obj->GetJointAnimation(jointId);
					void* matAnim   = obj->GetMaterialAnimation(matId);
					void* shapeAnim = obj->GetShapeAnimation(jointId);
					obj->SetAnim(0, TRUE, jointAnim, matAnim, shapeAnim, frame, rate / 100.0f, command == EVT_CMD_ANIM1 ? 1 : 0, 10.0f);
					// SetAnim with id 0 cleared the anim archive bit; ids from 1000 up are archive anims
					if (jointId >= 1000 || matId >= 1000) {
						obj->mStatus.mFlags |= 0x0008000000000000;
					}
				} else {
					obj->SetAnim(anim, FALSE, NULL, NULL, NULL, frame, rate / 100.0f, 0.0f, 10.0f);
				}
				obj->UpdateAnim(1);
			} else {
				obj->PushAnim(anim, frame, rate);
			}
		} else {
			FormatToStaticBuffer("anim2(%d, %d,%d,%d) ... obj(%d) killed!", id, anim, frame, rate, id);
		}
	}
	return 0;
}
/**
 * @note Address: 0x8007B48C
 * @note Size: 0x5F0
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
			if (obj->mJObj == NULL) {
				FormatToStaticBuffer("anim(%d, %d) ... obj(%d) is collision!", id, anim, id);
			} else if (count == 0) { // the first anim plays now, the ones after it are queued
				obj->mAnmQueue.Clear();
				// a pair: the material anim id in the high half, the joint and shape anim id in the low half
				if (anim & 0xFFFF0000) {
					s32 jointId     = anim & 0xFFFF;
					s32 matId       = anim >> 16;
					void* jointAnim = obj->GetJointAnimation(jointId);
					void* matAnim   = obj->GetMaterialAnimation(matId);
					void* shapeAnim = obj->GetShapeAnimation(jointId);
					obj->SetAnim(0, TRUE, jointAnim, matAnim, shapeAnim, 0.0f, 1.0f, command == EVT_CMD_ANIM1 ? 1 : 0, 10.0f);
					// ids from 1000 up are from the second archive (mAnim), XObj::SetAnim marks those with this bit
					if (jointId >= 1000 || matId >= 1000) {
						obj->mStatus.mFlags |= 0x0008000000000000;
					}
				} else {
					obj->SetAnim(anim, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, command == EVT_CMD_ANIM1 ? 1 : 0, 10.0f);
				}
				obj->UpdateAnim(1);
			} else {
				for (s32 i = 0; i < 16; i++) {
					s32 index = (obj->mAnmQueue.mNext + i) % 16;
					if (obj->mAnmQueue.mEntry[index].mId == -1) {
						obj->mAnmQueue.mEntry[index].mId    = anim;
						obj->mAnmQueue.mEntry[index].mFrame = 0;
						obj->mAnmQueue.mEntry[index].mRate  = 100;
						break;
					}
				}
			}
		} else {
			FormatToStaticBuffer("anim(%d, %d) ... obj(%d) killed!", id, anim, id);
		}
	}
	return 0;
}
/**
 * @note Address: 0x8007BA7C
 * @note Size: 0x10C
 */
s32 CEvt::_cmd_ABORT(u8, s32)
{
	if (mOwner == 0) {
		CEvtQueueEx* queue  = &gEventScriptQueue;
		s32 idx             = queue->mRead;
		u8* script          = queue->mScript[idx];
		u32 context         = queue->mContext[idx];
		s16 useValue        = queue->mUseValue[idx];
		s16 hitValue        = queue->mHitValue[idx];
		queue->mScript[idx] = 0;
		queue->mRead        = (queue->mRead + 1) % 0x80;
		if (script != 0) {
			mData.mScript  = script;
			mData.mContext = context;
			mUseValue      = useValue;
			mHitValue      = hitValue;
			return 0;
		}
	}
	CEvt_End(this);
	return 1;
}

/**
 * @note Address: 0x8007BB88
 * @note Size: 0x138
 */
s32 CEvt::_cmd_RETURN(u8, s32)
{
	if (mCalls.mCount > 0) {
		mData.mScript = (u8*)mCalls.mBuf[--mCalls.mCount];
		return 0;
	}
	// nothing to return to, end the script like ABORT
	if (mOwner == 0) {
		CEvtQueueEx* queue  = &gEventScriptQueue;
		s32 idx             = queue->mRead;
		u8* script          = queue->mScript[idx];
		u32 context         = queue->mContext[idx];
		s16 useValue        = queue->mUseValue[idx];
		s16 hitValue        = queue->mHitValue[idx];
		queue->mScript[idx] = 0;
		queue->mRead        = (queue->mRead + 1) % 0x80;
		if (script != 0) {
			mData.mScript  = script;
			mData.mContext = context;
			mUseValue      = useValue;
			mHitValue      = hitValue;
			return 0;
		}
	}
	CEvt_End(this);
	return 1;
}

/**
 * @note Address: 0x8007BCC0
 * @note Size: 0x74
 */
s32 CEvt::_cmd_LIB(u8, s32)
{
	// like RUN but the target is a 2 byte index into the library table
	mCalls.mBuf[mCalls.mCount++] = (s32)(mData.mScript + 2);
	u8* p                        = mData.mScript;
	s16 idx                      = p[0] + (p[1] << 8);
	mData.mScript                = GetGlobalScriptByIndex(idx);
	return 0;
}

/**
 * @note Address: 0x8007BD34
 * @note Size: 0x58
 */
s32 CEvt::_cmd_RUN(u8, s32)
{
	mCalls.mBuf[mCalls.mCount++] = (s32)(mData.mScript + 4);
	mData.mScript                = ResolveEventScriptOffset(mData.mScript);
	return 0;
}

/**
 * @note Address: 0x8007BD8C
 * @note Size: 0x38
 */
s32 CEvt::_cmd_GOTO(u8, s32)
{
	mData.mScript = ResolveEventScriptOffset(mData.mScript);
	return 0;
}

/**
 * @note Address: 0x8007BDC4
 * @note Size: 0x48
 */
s32 CEvt::_cmd_SETSP(u8, s32)
{
	mValues.mBuf[mValues.mCount++] = CEvt_Evaluate(this);
	return 0;
}

/**
 * @note Address: 0x8007BE0C
 * @note Size: 0x24
 */
s32 CEvt::_cmd_POPBP(u8, s32)
{
	mValues.mCount = mFrames.mBuf[--mFrames.mCount];
	return 0;
}

/**
 * @note Address: 0x8007BE30
 * @note Size: 0x24
 */
s32 CEvt::_cmd_PUSHBP(u8, s32)
{
	mFrames.mBuf[mFrames.mCount++] = mValues.mCount;
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

// EvalSet inlines this too
static inline bool EvalCmp(CEvt* evt)
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
		OSPanic(__FILE__, 1330, "%s%s", FormatToStaticBuffer("Unknown EvalCmp op - %d\n", op), __FUNCTION__);
		break;
	}

	return result;
}

/**
 * @note Address: 0x8007BE54
 * @note Size: 0x1DC
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

// CEvt::_cmd_IF
/**
 * @note Address: 0x8007C030
 * @note Size: 0x24
 */
s32 CEvt::_cmd_SET(u8, s32)
{
	CEvt_EvalSet(this);
	return 0;
}

extern "C" void SetAttachmentCountById(void* attachments, s32 index, s32 value);
#define EVT_PAD (&gGame.mPad.mPad[0])

/**
 * @note Address: 0x8007C054
 * @note Size: 0xA58
 */
extern "C" void CEvt_EvalSet(CEvt* evt)
{
	u8 op = *evt->mData.mScript;
	s32 value;
	switch (op) {
	case EVT_EXPR_EQUAL:
	case EVT_EXPR_NOT_EQUAL:
	case EVT_EXPR_LESS:
	case EVT_EXPR_LESS_EQUAL:
	case EVT_EXPR_GREATER:
	case EVT_EXPR_GREATER_EQUAL:
		value = EvalCmp(evt);
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
			} else if (index != -1 && index != -2 && index != -3) { // Evaluate's read-only flags, setting one does nothing
				OSPanic("evt.cpp", 1107, "%s%s", FormatToStaticBuffer("Invalid EvalSet flag[%d] - 0~%d\n", index, 4096), "EvalSet");
			}
			break;
		case EVT_EXPR_VAR:
			index = CEvt_Evaluate(evt);
			if (index >= 0 && index < 2048) {
				gEventVars.mVars[index] = value;
			} else {
				OSPanic("evt.cpp", 1116, "%s%s", FormatToStaticBuffer("Invalid EvalSet var[%d] - 0~%d\n", index, 2048), "EvalSet");
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
			// pad[7] is the rumble, it only runs with the rumble option (0x800000000) on: 1 starts it, 0 stops it,
			// anything else is the address of a rumble list for HSD_PadRumbleAdd
			if (index == 7) {
				if (gGame.mStatus.mFlags & 0x800000000ULL) {
					if (value == 1) {
						if (EVT_PAD->mConnected)
							HSD_PadRumbleOn(EVT_PAD->mPort);
					} else if (value == 0) {
						if (EVT_PAD->mConnected)
							HSD_PadRumbleOffN(EVT_PAD->mPort);
						if (EVT_PAD->mConnected)
							HSD_PadRumbleRemove(EVT_PAD->mPort);
					} else {
						if (EVT_PAD->mConnected)
							HSD_PadRumbleAdd(EVT_PAD->mPort, 0, -2, 128, (void*)value);
					}
				}
			} else {
				OSPanic("evt.cpp", 1144, "%s%s", FormatToStaticBuffer("Invalid EvalSet pad[%d] - %d\n", index, 7), "EvalSet");
			}
			break;
		case EVT_EXPR_BATTERY:
			index = CEvt_Evaluate(evt);
			if (index == 0) {
				EVT_PLAYER.mParams.SetBattery(value / 100.0f);
			} else if (index == 1) {
				Params* stats      = &EVT_PLAYER.mParams;
				f32 amount         = value / 100.0f;
				stats->mMaxBattery = amount > 0.0f ? amount : 0.0f;
				if (stats->mMaxBattery > 999.9f)
					stats->mMaxBattery = 999.9f;
				stats->SetBattery(stats->mBattery);
			} else {
				OSPanic("evt.cpp", 1157, "%s%s", FormatToStaticBuffer("Invalid EvalSet battery[%d] - 0~1\n", index), "EvalSet");
			}
			break;
		case EVT_EXPR_MONEY:
			AdjustPlayerMoney(&EVT_PLAYER.mParams, value - EVT_PLAYER.mParams.mMoney);
			break;
		case EVT_EXPR_SCRAP:
			AddPlayerScrap(&EVT_PLAYER.mParams, value - EVT_PLAYER.mParams.mScrap);
			break;
		case EVT_EXPR_ITEM:
			index = CEvt_Evaluate(evt);
			if (index >= 0 && index < 159) {
				SetOrAddInventoryItemCount(&EVT_PLAYER.mParams.mInventory, index, value, 0);
			} else {
				OSPanic("evt.cpp", 1176, "%s%s", FormatToStaticBuffer("Invalid EvalSet item[%d] - 0~%d\n", index, 159), "EvalSet");
			}
			break;
		case EVT_EXPR_ATC:
			index = CEvt_Evaluate(evt);
			if (index >= 0 && index < 9) {
				SetAttachmentCountById(&EVT_PLAYER.mAttachments, index, value);
			} else {
				OSPanic("evt.cpp", 1185, "%s%s", FormatToStaticBuffer("Invalid EvalSet atc[%d] - 0~%d\n", index, 9), "EvalSet");
			}
			break;
		case EVT_EXPR_CURRENT_SUIT: {
			XObjChr& player = EVT_PLAYER;
			if (player.mSuits.mChangeStep == FALSE) {
				player.mSuits.mRequested  = value;
				player.mSuits.mChangeStep = TRUE;
			}
			break;
		}
		case EVT_EXPR_EXP:
			AdjustPlayerExperience(&EVT_PLAYER.mParams, value - EVT_PLAYER.mParams.mExperience);
			break;
		case EVT_EXPR_LEVEL:
			AdjustPlayerLevel(&EVT_PLAYER.mParams, value - EVT_PLAYER.mParams.mLevel);
			break;
		case EVT_EXPR_RANK:
			AddPlayerRank(&EVT_PLAYER.mParams, value - EVT_PLAYER.mParams.mRank);
			break;
		case EVT_EXPR_CURRENT_ATC: {
			XObjChr& player = EVT_PLAYER;
			if (value != player.mAttachment) {
				player.mAttachmentRequest = value;
				player.mAttachmentStep    = 0;
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
				OSPanic("evt.cpp", 1234, "%s%s", FormatToStaticBuffer("Invalid EvalSet time[%d] - 0~2\n", index), "EvalSet");
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
			OSPanic("evt.cpp", 1245, "%s%s", FormatToStaticBuffer("Unknown EvalSet op - %d\n", op), "EvalSet");
			break;
		}
	}
}
extern "C" {
u8 GetPadRumbleStatus(u8 port);
}

static inline s32 EvtRead16(const u8* p)
{
	return p[0] + (p[1] << 8);
}

/**
 * @note Address: 0x8007CAAC
 * @note Size: 0x1D1C
 */
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
		s32 rhs = CEvt_Evaluate(evt);
		s32 lhs = CEvt_Evaluate(evt);
		if ((u8)op == EVT_EXPR_EQUAL)
			result = lhs == rhs;
		if ((u8)op == EVT_EXPR_NOT_EQUAL)
			result = lhs != rhs;
		if ((u8)op == EVT_EXPR_LESS)
			result = lhs < rhs;
		if ((u8)op == EVT_EXPR_LESS_EQUAL)
			result = lhs <= rhs;
		if ((u8)op == EVT_EXPR_GREATER)
			result = lhs > rhs;
		if ((u8)op == EVT_EXPR_GREATER_EQUAL)
			result = lhs >= rhs;
		if ((u8)op == EVT_EXPR_ADD)
			result = lhs + rhs;
		if ((u8)op == EVT_EXPR_SUB)
			result = lhs - rhs;
		if ((u8)op == EVT_EXPR_MUL)
			result = lhs * rhs;
		if ((u8)op == EVT_EXPR_DIV)
			result = lhs / rhs;
		if ((u8)op == EVT_EXPR_MOD)
			result = lhs % rhs;
		if ((u8)op == EVT_EXPR_AND)
			result = lhs & rhs;
		if ((u8)op == EVT_EXPR_OR)
			result = lhs | rhs;
		if ((u8)op == EVT_EXPR_XOR)
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
	case EVT_EXPR_IMM32:
		result = ReadResourceOffset(evt->mData.mScript);
		evt->mData.mScript += 4;
		break;
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
		result = evt->mValues.mBuf[index + evt->mFrames.mBuf[evt->mFrames.mCount - 2]];
		break;
	}
	case EVT_EXPR_FLAG:
		result = CEvt_Evaluate(evt);
		if (result >= 0 && result < 4096) {
			result = (gEventFlags.mFlags[(u32)result >> 5] & (1U << (result & 31))) ? 1 : 0;
		} else if (result == -1)
			result = 0;
		else if (result == -2)
			result = (gGame.mStatus.mFlags & 0x2000) ? 1 : 0;
		else if (result == -3)
			result = (gGame.mStatus.mFlags & 0x8000000000000ULL) ? 1 : 0;
		else
			OSPanic("evt.cpp", 686, "%s%s", FormatToStaticBuffer("Invalid Evaluate flg[%d] - 0~%d\n", result, 4096), "Evaluate");
		break;
	case EVT_EXPR_VAR:
		result = CEvt_Evaluate(evt);
		if (result >= 0 && result < 2048)
			result = gEventVars.mVars[result];
		else
			OSPanic("evt.cpp", 694, "%s%s", FormatToStaticBuffer("Invalid Evaluate var[%d] - 0~%d\n", result, 2048), "Evaluate");
		break;
	case EVT_EXPR_RESULT1:
		result = evt->mSelectResult;
		break;
	case EVT_EXPR_RESULT2:
		result = evt->mResult2;
		break;
	case EVT_EXPR_PAD: {
		result = CEvt_Evaluate(evt);
		CPad pad(0);
		switch (result) {
		case 0:
			result = pad.mPad->mDirButton;
			break;
		case 1:
			result = pad.mPad->mDirTrigger;
			break;
		case 2:
			result = 100.0f * pad.mPad->mStickX;
			break;
		case 3:
			result = 100.0f * pad.mPad->mStickY;
			break;
		case 4:
			result = RAD2DEG * WrapAngle(-(f32)atan2(pad.mPad->mStickX, pad.mPad->mStickY));
			break;
		case 5: {
			f32 x  = pad.mPad->mStickX;
			f32 y  = pad.mPad->mStickY;
			result = 100.0f * sqrtf(x * x + y * y);
			break;
		}
		case 6: {
			f32 stickAngle = (f32)atan2(pad.mPad->mStickX, pad.mPad->mStickY);
			f32 cameraYaw  = gGame.mMotionControlCamera.mCamera.mAngle.y;
			f32 playerYaw  = gGame.GetPlayer()->mMoveRotate.y;
			result         = RAD2DEG * WrapAngle(-stickAngle + cameraYaw - playerYaw);
			break;
		}
		case 7:
			if (gGame.mStatus.mFlags & 0x800000000ULL) {
				u8 status = 0;
				if (pad.mPad->mConnected)
					status = GetPadRumbleStatus(pad.mPad->mPort);
				result = status == 2;
			} else
				result = 0;
			break;
		default:
			OSPanic("evt.cpp", 737, "%s%s", FormatToStaticBuffer("Invalid Evaluate pad[%d] - 0~7\n", result), "Evaluate");
			break;
		}
		break;
	}
	case EVT_EXPR_BATTERY:
		result = CEvt_Evaluate(evt);
		if (result == 0)
			result = 100.0f * EVT_PLAYER.mParams.mBattery;
		else if (result == 1)
			result = 100.0f * EVT_PLAYER.mParams.mMaxBattery;
		else
			OSPanic("evt.cpp", 750, "%s%s", FormatToStaticBuffer("Invalid Evaluate battery[%d] - 0~1\n", result), "Evaluate");
		break;
	case EVT_EXPR_MONEY:
		result = EVT_PLAYER.mParams.mMoney;
		break;
	case EVT_EXPR_SCRAP:
		result = EVT_PLAYER.mParams.mScrap;
		break;
	case EVT_EXPR_ITEM:
		result = CEvt_Evaluate(evt);
		if (result >= 0 && result < 159) {
			s32 index = FindOwnedInventoryItemIndex(EVT_PLAYER.mParams.mInventory.mItems, result);
			result    = index >= 0 ? EVT_PLAYER.mParams.mInventory.mItems[index].mCount : 0;
		} else
			OSPanic("evt.cpp", 769, "%s%s", FormatToStaticBuffer("Invalid Evaluate item[%d] - 0~%d\n", result, 159), "Evaluate");
		break;
	case EVT_EXPR_ATC:
		result = CEvt_Evaluate(evt);
		if (result >= 0 && result < 9) {
			result = GetAttachmentCountById(&EVT_PLAYER.mAttachments, result);
			if (result < 0)
				result = 0;
		} else
			OSPanic("evt.cpp", 778, "%s%s", FormatToStaticBuffer("Invalid Evaluate atc[%d] - 0~%d\n", result, 9), "Evaluate");
		break;
	case EVT_EXPR_CURRENT_SUIT:
		result = EVT_PLAYER.mSuits.mCurrent;
		break;
	case EVT_EXPR_EXP:
		result = EVT_PLAYER.mParams.mExperience;
		break;
	case EVT_EXPR_LEVEL:
		result = EVT_PLAYER.mParams.mLevel;
		break;
	case EVT_EXPR_RANK:
		result = EVT_PLAYER.mParams.mRank;
		break;
	case EVT_EXPR_HOLD: {
		XObj* held = gGame.GetPlayer()->mLift.mTarget;
		if (held != NULL) {
			if ((s16)held->GetPool() == 24)
				result = 1000;
			else if ((s16)held->GetPool() == 8)
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
		result = EVT_PLAYER.GetAttachment();
		break;
	case EVT_EXPR_MAP:
		result = CEvt_Evaluate(evt);
		if (result >= 0 && result <= 1) {
			s32 stage = result == 0 ? gGame.mStage : gGame.mPrevStage;
			result    = stage;
			if (stage > GetStageNum())
				result = stage - GetStageNum() + 99;
		} else
			OSPanic("evt.cpp", 827, "%s%s", FormatToStaticBuffer("Invalid Evaluate map[%d] - 0~1\n", result), "Evaluate");
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
			OSPanic("evt.cpp", 854, "%s%s", FormatToStaticBuffer("Invalid Evaluate time[%d] - 0~2\n", result), "Evaluate");
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
					result = obj->mAnimId;
					break;
				case EVT_ATOM_DIR:
					result = RAD2DEG * WrapAngle(obj->mMoveRotate.y);
					break;
				case EVT_ATOM_POSX:
					result = 100.0f * obj->mMoveTranslate.x;
					break;
				case EVT_ATOM_POSY:
					result = 100.0f * obj->mMoveTranslate.y;
					break;
				case EVT_ATOM_POSZ:
					result = 100.0f * obj->mMoveTranslate.z;
					break;
				case 235:
					result = (obj->mStatus.mFlags & 0x10000) ? 1 : 0;
					break;
				case 247: {
					Vec pos = { 0.0f, 0.0f, 0.0f };
					f32 x;
					obj->ProjectOffsetToScreen(&pos, &x, NULL, NULL, 0);
					result = 100.0f * x;
					break;
				}
				case 248: {
					Vec pos = { 0.0f, 0.0f, 0.0f };
					f32 y;
					obj->ProjectOffsetToScreen(&pos, NULL, &y, NULL, 0);
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
			XObj* obj = CEvt_GetObj(evt, (s16)EvtRead16(data));
			if (obj != NULL && (obj->mStatus.mFlags & 1)) {
				s32 depth = (s16)EvtRead16(data + 2);
				data += 4;
				s32 path[20];
				Vec pos;
				for (s32 i = 0; i < depth; i++, data += 2)
					path[i] = (s16)EvtRead16(data);
				switch (result) {
				case EVT_ATOM_BONEX:
					result = 100.0f * GetEventJointWorldPosition((CJObj*)obj, depth, path).x;
					break;
				case EVT_ATOM_BONEY:
					result = 100.0f * GetEventJointWorldPosition((CJObj*)obj, depth, path).y;
					break;
				case EVT_ATOM_BONEZ:
					result = 100.0f * GetEventJointWorldPosition((CJObj*)obj, depth, path).z;
					break;
				case 249: {
					pos = GetEventJointWorldPosition((CJObj*)obj, depth, path) - (Vec)obj->mMoveTranslate;
					f32 x;
					obj->ProjectOffsetToScreen(&pos, &x, NULL, NULL, 0);
					result = 100.0f * x;
					break;
				}
				case 250: {
					pos = GetEventJointWorldPosition((CJObj*)obj, depth, path) - (Vec)obj->mMoveTranslate;
					f32 y;
					obj->ProjectOffsetToScreen(&pos, NULL, &y, NULL, 0);
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
				f32 angle = (f32)atan2(to->mMoveTranslate.x - from->mMoveTranslate.x, to->mMoveTranslate.z - from->mMoveTranslate.z);
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
				result = 100.0f * PSVECDistance(&from->mMoveTranslate, &to->mMoveTranslate);
			} else
				result = -1;
			break;
		}
		default:
			OSPanic("evt.cpp", 977, "%s%s", FormatToStaticBuffer("Invalid Evaluate obj(...,%d)\n", result), "Evaluate");
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
		result     = (s8)CEvt_Evaluate(evt);
		s32 index  = CEvt_Evaluate(evt);
		s32 stride = result;
		if ((s8)result < 0)
			stride = -result;
		u8* p = (u8*)CEvt_Evaluate(evt) + index * stride;
		switch (result) {
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
		OSPanic("evt.cpp", 1023, "%s%s", FormatToStaticBuffer("Unknown Evaluate op - %d\n", op), "Evaluate");
		break;
	}
	return result;
}

// world position of the joint path leads to, path holds a sibling count per level
/**
 * @note Address: 0x8007E7C8
 * @note Size: 0x2C
 */
extern "C" CVec GetEventJointWorldPosition(CJObj* obj, s32 depth, s32* path)
{
	return CJObj_GetJointWorldPositionByPath(obj, depth, path);
}

extern "C" CVec GetEventJObjWorldPosition(CJObj* obj, HSD_JObj* jobj);

/**
 * @note Address: 0x8007E7F4
 * @note Size: 0xAC
 */
// NONMATCHING: only the initial joint load/null test is scheduled differently around the r31 save; the joint
// walk, GetEventJObjWorldPosition call and CVec return are otherwise instruction-identical.
extern "C" CVec CJObj_GetJointWorldPositionByPath(CJObj* obj, s32 depth, s32* path)
{
#if defined(VERSION_JP)
	return GetJointPosition(FindJointByPath(obj->mJObj, path, depth));
#else
	return GetEventJObjWorldPosition(obj, GetJointByPath(obj->mJObj, path, depth));
#endif
}

/**
 * @note Address: 0x8007E8A0
 * @note Size: 0xD8
 */
extern "C" CVec GetEventJObjWorldPosition(CJObj* obj, HSD_JObj* jobj)
{
	return GetJointPosition(jobj);
}

/**
 * @note Address: 0x8007E978
 * @note Size: 0x4
 */
extern "C" void CEvt_Draw(void)
{
}

extern "C" s32 InfoScrn_IsBatteryGaugeFull(void* hud);
extern "C" s32 HasCameraAnimation(XObj* obj);

static inline void EvtWaitFor(CEvt* evt, u8 command, u8* start, s32 ready, s32& yield)
{
	if (command == EVT_CMD_WAIT) {
		if (ready == 0) {
			evt->mData.mScript = start;
			yield              = 1;
		}
	} else
		evt->mSelectResult = ready;
}

// a wipe is done once its size stops
static inline bool EvtZero2(const CScrnWipe& wipe)
{
	return 0.0f == wipe.mSpeedWidth && 0.0f == wipe.mSpeedHeight;
}

static inline bool EvtZero3(const f32* p)
{
	return 0.0f == p[0] && 0.0f == p[1] && 0.0f == p[2];
}

// a screen color is done once its speed is all zero
static inline bool EvtZero4(const ScrnColorF& speed)
{
	return 0.0f == speed.r && 0.0f == speed.g && 0.0f == speed.b && 0.0f == speed.a;
}

extern "C" {
void ConfigureSubwindowCamera(FbSubwin* subwin, const Vec& interest, f32 angleY, f32 angleX, f32 distance, f32 fov);
void AddMainMenuExtraRow(MenuMain* menu, char* text, s32 id);
}

enum CollectionObjectPool {
	ItemPool = 8,
	CoinPool = 7,
	DustPool = 20,
};

static CEvtFlags& GetCollectionFlags(CollectionObjectPool pool)
{
	return *static_cast<XObjFlags*>(gGame.mXObjs.mPools[pool].First())->mFlags;
}

static void SetCollectionFlag(CollectionObjectPool pool, s32 id, bool enabled)
{
	if (id == -2) {
		GetCollectionFlags(pool).SetAll(enabled);
	} else if (id > 0) {
		GetCollectionFlags(pool).Set(id, enabled);
	}
}

/**
 * @note Address: 0x80078754
 * @note Size: 0x1D38
 */
s32 CEvt::_cmd_CALL(u8, s32)
{
	s32 yield  = 0;
	u8* start  = mData.mScript - 1;
	u8* begin  = mData.mScript;
	u16 length = EvtRead16(begin);
	mData.mScript += 2;
	if (length < 3) {
		return 0;
	}
	u8* end      = begin + length;
	XObj* target = CEvt_GetObj(this, (s16)CEvt_Evaluate(this));
	s32 count    = 0;
	s32 args[24];
	while (mData.mScript < end && count < ARRAY_SIZE(args)) {
		args[count++] = CEvt_Evaluate(this);
	}
	if (mData.mScript != end) {
		mData.mScript = end;
		return 0;
	}

	switch ((s32)target) {
	case -10: { // keeps the object inside these limits around where it stands now
		f32 negZ = args[4], posZ = args[3], negX = args[2], posX = args[1];
		XObj* obj       = CEvt_GetObj(this, (s16)args[0]);
		obj->mAnchorX   = obj->mMoveTranslate.x;
		obj->mAnchorZ   = obj->mMoveTranslate.z;
		obj->mLimitPosX = posX;
		obj->mLimitNegX = negX;
		obj->mLimitPosZ = posZ;
		obj->mLimitNegZ = negZ;
		break;
	}
	case -11: {
		s16 id    = args[0];
		s32 flags = args[1];
		if (flags == -1) {
			XObj* obj            = CEvt_GetObj(this, id);
			obj->mMarkerOverride = 0;
			obj->mMarkerOverride |= MarkerUseAvailable;
		} else {
			XObj* obj = CEvt_GetObj(this, id);
			obj->mMarkerOverride &= ~MarkerUseAvailable;
			obj->mMarkerOverride |= flags;
		}
		break;
	}
	case -12:
	case -13:
	case -14:
	case -15:
		if (mYesNoActive != 0) {
			// OpenPlayerYesNoMenu sets 0x800, it clears once the marks have faded out after the answer
			if (gGame.mStatus.mFlags & 0x800) {
				CPad pad(0);
				HandleOXMenuInput(&gGame.GetPlayer()->mOXMenu, &pad);
				mData.mScript = start;
				yield         = 1;
			} else {
				mYesNoActive  = 0;
				mSelectResult = EVT_PLAYER.mOXMenu.mConfirmedIndex;
			}
		} else {
			if (EVT_PLAYER.mAttachmentRequest < 0) {
				s32 crossMask = 2;
				if ((s32)target == -12)
					crossMask = 2;
				else if ((s32)target == -13)
					crossMask = 1;
				else if ((s32)target == -14)
					crossMask = 0;
				else if ((s32)target == -15)
					crossMask = 3;
				OpenPlayerYesNoMenu(&EVT_PLAYER, crossMask, args[0]);
				mYesNoActive = 1;
			}
			mData.mScript = start;
			yield         = 1;
		}
		break;
	case -16:
		// the subwindow: 231 sets its rect, @CAM aims its camera and shows it, @RESET and 230 (the two
		// atoms that end the scripted camera in _cmd_CAMERA) hide it
		if (args[0] == 231) {
			gGame.mSubwin.mRectX      = args[1];
			gGame.mSubwin.mRectY      = args[2];
			gGame.mSubwin.mRectWidth  = args[3];
			gGame.mSubwin.mRectHeight = args[4];
		} else if (args[0] == EVT_ATOM_CAM) {
			Vec pos = { 0.0f, 0.0f, 0.0f };
			pos.x   = args[1] / 100.0f;
			pos.y   = args[2] / 100.0f;
			pos.z   = args[3] / 100.0f;
			ConfigureSubwindowCamera(&gGame.mSubwin, pos, DEG2RAD * args[4], DEG2RAD * args[5], args[6], args[7]);
			gGame.mSubwin.mEnabled      = 1;
			gGame.mSubwin.mFlags.mFlags = 0;
		} else if (args[0] == EVT_ATOM_RESET || args[0] == 230) {
			gGame.mSubwin.mEnabled = 0;
		}
		break;
	// Each group sets, clears, or tests the flags for one type of collectible.
	case -17:
		if (count > 0) {
			SetCollectionFlag(ItemPool, args[0], true);
		}
		break;
	case -18:
		if (count > 0) {
			SetCollectionFlag(ItemPool, args[0], false);
		}
		break;
	case -19:
		mSelectResult = count > 0 && GetCollectionFlags(ItemPool).Test(args[0]);
		break;
	case -20:
		if (count > 0) {
			SetCollectionFlag(CoinPool, args[0], true);
		}
		break;
	case -21:
		if (count > 0) {
			SetCollectionFlag(CoinPool, args[0], false);
		}
		break;
	case -22:
		mSelectResult = count > 0 && GetCollectionFlags(CoinPool).Test(args[0]);
		break;
	case -23:
		if (count > 0) {
			SetCollectionFlag(DustPool, args[0], true);
		}
		break;
	case -24:
		if (count > 0) {
			SetCollectionFlag(DustPool, args[0], false);
		}
		break;
	case -25:
		mSelectResult = count > 0 && GetCollectionFlags(DustPool).Test(args[0]);
		break;
	case SYSCALL_SET_OBJSTAT: {
		s16 id    = args[0];
		s32 bit   = args[1];
		u64 mask  = (u64)1 << bit;
		XObj* obj = CEvt_GetObj(this, id);
		if (obj != NULL) {
			obj->mStatus.mFlags |= mask;
			if (mask & 0x20000) {
				u32 flags[5] = { 0, 0, 0, 0, 0 };
				flags[0]     = obj->mStatus.mFlags & 0x800000;
				flags[1]     = obj->mStatus.mFlags & 0x200000;
				flags[2]     = obj->mStatus.mFlags & 0x40;
				flags[3]     = obj->mStatus.mFlags & 0x20000;
				flags[4]     = obj->mStatus.mFlags & 0x400000;
				obj->ApplyMaterialPresets(flags);
			}
			if (mask & ((u64)0x4000 << 32)) {
				XObjChr* player = gGame.GetPlayer();
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
			FormatToStaticBuffer("call(SYSCALL_SET_OBJSTAT, %d, 1<<%d): obj(%d) killed!", id, bit, id);
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
				XObjChr* player = gGame.GetPlayer();
				if ((s16)obj->GetPool() == 8 && obj == player->mLift.mTarget) {
					s16 item = obj->GetObjFlags();
					switch (item) {
					case 16:
					case 21:
					case 22:
					case 56:
						player->mAttachment = 0;
						CJAnim* anim        = &player->mAttachmentAnim;
						anim->Release();
						break;
					}
				}
			}
		} else
			FormatToStaticBuffer("call(SYSCALL_CLR_OBJSTAT, %d, 1<<%d): obj(%d) killed!", id, bit, id);
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
			FormatToStaticBuffer("call(SYSCALL_CHK_OBJSTAT, %d, 1<<%d): obj(%d) killed!", id, bit, id);
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
		gGame.mStatus.mFlags |= (u64)1 << args[0];
		break;
	case -42:
		gGame.mStatus.mFlags &= ~((u64)1 << args[0]);
		break;
	case -43:
		mSelectResult = (gGame.mStatus.mFlags & ((u64)1 << args[0])) ? 1 : 0;
		break;
	case -32:
		gGame.mStatus.mFlags |= (u64)0x8000 << 32;
		break;
	case -33:
		gGame.mStatus.mFlags &= ~((u64)0x8000 << 32);
		break;
	case -37:
		gGame.mStatus.mFlags |= (u64)0x10000 << 32;
		break;
	case -38:
		gGame.mStatus.mFlags &= ~((u64)0x10000 << 32);
		break;
	case -34:
		gGame.mStatus.mFlags |= (u64)0x20 << 32;
		break;
	case -35:
		gGame.mStatus.mFlags &= ~((u64)0x20 << 32);
		break;
	case -36:
		gGame.mStatus.mFlags &= ~((u64)0x20 << 32);
		for (s32 i = 0; i < gGame.mXObjs.mObjectCount; i++) {
			XObj* obj = gGame.mXObjs.mObjects[i];
			if ((obj->mStatus.mFlags & 1) && (obj->mStatus.mFlags & 0x20000000)) {
				gGame.mStatus.mFlags |= (u64)0x20 << 32;
				break;
			}
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
		s16 result = GetItemScrapValue(args[0]);
#if defined(VERSION_PAL)
		if (result == 1)
			result = 2;
#endif
		mSelectResult = result;
		break;
	}
	case -40:
		mSelectResult = (s16)GetItemHappyPointValue(args[0]);
		break;
	case SYSCALL_STGFUNC:
		if (count > 0)
			mSelectResult = CallStageModule(args[0] + 7, count - 1, &args[1]);
#if defined(VERSION_PAL)
		else
			OSPanic("evt.cpp", 2161, "%s%s", "SYSCALL_STGFUNC, noargs\n", "_cmd_CALL");
#else
		else
			OSPanic("evt.cpp", 2157, "%s%s", "SYSCALL_STGFUNC, noargs\n", "_cmd_CALL");
#endif
		break;
	case -200:
		AddMainMenuExtraRow(static_cast<MenuMain*>(gGame.mMenu.mMenus[0]), reinterpret_cast<char*>(args[0]), args[1]);
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
		}
		break;
	}
	return yield;
}

/**
 * @note Address: 0x80070304
 * @note Size: 0x1B9C
 */
s32 CEvt::_cmd_WAIT(u8 command, s32 step)
{
	u8* start = mData.mScript - 1;
	s32 yield = 0;
	u16 mode  = CEvt_Evaluate(this);
	switch (mode) {
	case EVT_ATOM_TIME: {
		s32 duration = CEvt_Evaluate(this);
		if (command == EVT_CMD_WAIT) {
			if (mWaitTimer == 0) {
				// duration is in hundredths of a second, mWaitTimer counts hundredths of a frame
				mWaitTimer = (duration / 100) * FRAMES_PER_SEC * 100;
				mWaitTimer += 100.0f * ((duration % 100 * FRAMES_PER_SEC) / 100.0f);
				mData.mScript = start;
				yield         = 1;
			} else {
				s32 remain = mWaitTimer - step * 100;
				mWaitTimer = remain;
				if (remain <= 0)
					mWaitTimer = 0;
				else {
					mData.mScript = start;
					yield         = 1;
				}
			}
		} else
			mSelectResult = 1;
		break;
	}
	case EVT_ATOM_FADE:
		EvtWaitFor(this, command, start, EvtZero4(gGame.mFade.mSpeed) != 0, yield);
		break;
	case EVT_ATOM_MONO:
		EvtWaitFor(this, command, start, EvtZero4(gGame.mColor0.mColor.mSpeed) != 0, yield);
		break;
	case EVT_ATOM_WIPE:
		EvtWaitFor(this, command, start,
		           (EvtZero2(gGame.mWipeCircle) && EvtZero2(gGame.mWipeTex) && EvtZero2(gGame.mWipeCustom) && EvtZero2(gGame.mWipeTile0))
		               != 0,
		           yield);
		break;
	case EVT_ATOM_LETTERBOX:
		EvtWaitFor(this, command, start, EvtZero2(gGame.mWipeTile1) != 0, yield);
		break;
	case 203:
		if (command != EVT_CMD_WAIT)
			mSelectResult = 1;
		break;
	case EVT_ATOM_ANIM: {
		s16 id    = CEvt_Evaluate(this);
		XObj* obj = CEvt_GetObj(this, id);
		f32 frame = CEvt_Evaluate(this);
		if (obj != NULL && (obj->mStatus.mFlags & 1)) {
			if (obj->mJObj == NULL)
				FormatToStaticBuffer("wait(@ANIM, %d) ... obj(%d) is collision!", id, id);
			HSD_AObj* anim = GetAObj(obj->mJObj);
			if (anim != NULL) {
				if (frame >= 0.0f) {
					HSD_AObj* aobj = GetAObj(obj->mJObj);
					f32 current    = aobj != NULL ? HSD_AObjGetCurrentFrame(aobj) : 0.0f;
					EvtWaitFor(this, command, start, current >= frame, yield);
				} else
					EvtWaitFor(this, command, start, (anim->flags & (AOBJ_NO_ANIM | AOBJ_REWINDED)) != 0, yield);
			} else if (command == EVT_CMD_CHECK)
				mSelectResult = 1;
		} else {
			FormatToStaticBuffer("wait(@ANIM, %d, ...) obj(%d) killed!", id, id);
			if (command == EVT_CMD_CHECK)
				mSelectResult = 1;
		}
		break;
	}
	case EVT_ATOM_DIR: {
		s16 id    = CEvt_Evaluate(this);
		XObj* obj = CEvt_GetObj(this, id);
		if (obj != NULL && (obj->mStatus.mFlags & 1)) {
			if (obj->mJObj == NULL)
				FormatToStaticBuffer("wait(@DIR, %d) ... obj(%d) is collision!", id, id);
			EvtWaitFor(this, command, start, obj->mMoveRotate.y == obj->mMoveRotate.mTarget.y, yield);
		} else {
			FormatToStaticBuffer("wait(@DIR, %d, ...) obj(%d) killed!", id, id);
			if (command == EVT_CMD_CHECK)
				mSelectResult = 1;
		}
		break;
	}
	case EVT_ATOM_MOVE: {
		s16 id    = CEvt_Evaluate(this);
		XObj* obj = CEvt_GetObj(this, id);
		if (obj != NULL && (obj->mStatus.mFlags & 1)) {
			if (obj->mJObj == NULL)
				FormatToStaticBuffer("wait(@MOVE, %d) ... obj(%d) is collision!", id, id);
			const Vec& target = obj->mMoveTranslate.mTarget;
			EvtWaitFor(this, command, start, !(obj->mMoveTranslate != target), yield);
		} else {
			FormatToStaticBuffer("wait(@MOVE, %d, ...) obj(%d) killed!", id, id);
			if (command == EVT_CMD_CHECK)
				mSelectResult = 1;
		}
		break;
	}
	case EVT_ATOM_SCALE: {
		s16 id    = CEvt_Evaluate(this);
		XObj* obj = CEvt_GetObj(this, id);
		if (obj != NULL && (obj->mStatus.mFlags & 1)) {
			if (obj->mJObj == NULL)
				FormatToStaticBuffer("wait(@SCALE, %d) ... obj(%d) is collision!", id, id);
			EvtWaitFor(this, command, start, obj->mMoveScale.y == obj->mMoveScale.mTarget.y, yield);
		} else {
			FormatToStaticBuffer("wait(@SCALE, %d, ...) obj(%d) killed!", id, id);
			if (command == EVT_CMD_CHECK)
				mSelectResult = 1;
		}
		break;
	}
	case EVT_ATOM_SFX:
		EvtWaitFor(this, command, start, !IsGameSoundPlaying(&gGame.mSound, CEvt_Evaluate(this)), yield);
		break;
	case EVT_ATOM_COLOR: {
		s16 id    = CEvt_Evaluate(this);
		XObj* obj = CEvt_GetObj(this, id);
		if (obj != NULL && (obj->mStatus.mFlags & 1)) {
			if (obj->mJObj == NULL)
				FormatToStaticBuffer("wait(@COLOR, %d) ... obj(%d) is collision!", id, id);
			EvtWaitFor(this, command, start, EvtZero3(&obj->mColorStep[0].mSpeed.x) && EvtZero3(&obj->mColorStep[1].mSpeed.x), yield);
		} else {
			FormatToStaticBuffer("wait(@COLOR, %d, ...) obj(%d) killed!", id, id);
			if (command == EVT_CMD_CHECK)
				mSelectResult = 1;
		}
		break;
	}
	case EVT_ATOM_REAL: {
		s32 index = CEvt_Evaluate(this);
		if (index == 1)
			EvtWaitFor(this, command, start, InfoScrn_IsBatteryGaugeFull(&gGame.mInfoScrn), yield);
		else {
#if defined(VERSION_PAL)
			OSPanic("evt.cpp", 4157, "%s%s", FormatToStaticBuffer("wait(@REAL, %d) - unresolved!\n", index), "_cmd_WAIT");
#else
			OSPanic("evt.cpp", 4146, "%s%s", FormatToStaticBuffer("wait(@REAL, %d) - unresolved!\n", index), "_cmd_WAIT");
#endif
		}
		break;
	}
	case EVT_ATOM_CAM: {
		s32 ready                    = 1;
		CMotionControlCamera* camera = &gGame.mMotionControlCamera;
		if (camera->IsAction(CAMACTION_RESET))
			ready = 0;
		if (camera->IsAction(CAMACTION_AREA)) {
			CCameraActionArea* area = static_cast<CCameraActionArea*>(gGame.mCameraActions.mTable[CAMACTION_AREA]);
			ready                   = !(area->mEyeInterp != NULL || area->mInterestInterp != NULL || area->mFovInterp != NULL);
		}
		if (camera->IsAction(CAMACTION_EVT)) {
			CCameraActionEvt* action = static_cast<CCameraActionEvt*>(gGame.mCameraActions.mTable[CAMACTION_EVT]);
			ready                    = !(action->mEyeInterp != NULL || action->mInterestInterp != NULL || action->mDistanceInterp != NULL
                      || action->mFovInterp != NULL || action->mAngleInterp != NULL);
		}
		if (camera->IsAction(CAMACTION_ANIM)) {
			XObj* obj = (XObj*)gGame.mXObjs.mPools[0].mObjects;
			if (obj != NULL && (obj->mStatus.mFlags & 1))
				ready = !HasCameraAnimation(obj);
		}
		EvtWaitFor(this, command, start, ready, yield);
		break;
	}
	case EVT_ATOM_READ: {
		s16 id    = CEvt_Evaluate(this);
		XObj* obj = CEvt_GetObj(this, id);
		if (obj != NULL && (obj->mStatus.mFlags & 1)) {
			if (obj->mJObj == NULL)
				FormatToStaticBuffer("wait(@READ, %d) ... obj(%d) is collision!", id, id);
			EvtWaitFor(this, command, start, !(obj->mStatus.mFlags & 0x40000000000000ULL) && !(obj->mStatus.mFlags & 0x80000000000000ULL),
			           yield);
		} else {
			FormatToStaticBuffer("wait(@READ, %d, ...) obj(%d) killed!", id, id);
			if (command == EVT_CMD_CHECK)
				mSelectResult = 1;
		}
		break;
	}
	case EVT_ATOM_ZBLUR:
		EvtWaitFor(this, command, start, !(0.0f != gGame.mBlur.mSpeed), yield);
		break;
	case EVT_ATOM_SHAKE:
		EvtWaitFor(this, command, start, EvtZero3(&gGame.mQuake.mShake[2].x) != 0, yield);
		break;
	case EVT_ATOM_CUE: {
		s32 beat = gEventBeatLatch.mPending;
		if (beat != 0) {
			beat                     = gEventBeatLatch.mBeat;
			gEventBeatLatch.mPending = 0;
			beat += 1000; // so beat 0 is told apart from no beat
		}
		if (command == EVT_CMD_WAIT && beat == 0) {
			mData.mScript = start;
			yield         = 1;
		} else
			mSelectResult = beat;
		break;
	}
	case EVT_ATOM_MOVIE: {
		s32 frame = CEvt_Evaluate(this);
		s32 ready = gGame.mMovie.mMovie.mState == 0;
		if (!ready && frame >= 0)
			ready = gGame.mMovie.mMovie.mFrame >= frame;
		if (command == EVT_CMD_WAIT && ready == 0) {
			mData.mScript = start;
			yield         = 1;
		} else
			mSelectResult = ready;
		break;
	}
	default:
		if (command == EVT_CMD_WAIT) {
#if defined(VERSION_PAL)
			OSPanic("evt.cpp", 4300, "%s%s", FormatToStaticBuffer("wait(%d,...) - unresolved!\n", mode), "_cmd_WAIT");
#else
			OSPanic("evt.cpp", 4289, "%s%s", FormatToStaticBuffer("wait(%d,...) - unresolved!\n", mode), "_cmd_WAIT");
#endif
		} else {
#if defined(VERSION_PAL)
			OSPanic("evt.cpp", 4302, "%s%s", FormatToStaticBuffer("check(%d,...) - unresolved!\n", mode), "_cmd_WAIT");
#else
			OSPanic("evt.cpp", 4291, "%s%s", FormatToStaticBuffer("check(%d,...) - unresolved!\n", mode), "_cmd_WAIT");
#endif
		}
		break;
	}
	return yield;
}

extern "C" {
void InfoScrn_ApplyBatteryHudCommand(InfoScrn* scrn, u8 show, s32 mode);
void InfoScrn_ApplyTimerHudCommand(InfoScrn* scrn, u8 show, s32 mode);
s32 InfoScrn_GetTimerHundredths(InfoScrn* scrn);
void InfoScrn_SetTimerHundredths(InfoScrn* scrn, s32 hundredths);
void InfoScrn_ShowTimer(InfoScrn* scrn);
void InfoScrn_HideTimer(InfoScrn* scrn);
}

struct EvtHudCommand {
	u8 enabled; // _00
	s32 mode;   // _04
};

/**
 * @note Address: 0x80072224
 * @note Size: 0xEF8
 */
s32 CEvt::_cmd_SCRN(u8, s32)
{
	u16 mode = CEvt_Evaluate(this);
	switch (mode) {
	case EVT_ATOM_FADE: {
		u8 r0    = CEvt_Evaluate(this);
		u8 g0    = CEvt_Evaluate(this);
		u8 b0    = CEvt_Evaluate(this);
		u8 a0    = CEvt_Evaluate(this);
		u8 r1    = CEvt_Evaluate(this);
		u8 g1    = CEvt_Evaluate(this);
		u8 b1    = CEvt_Evaluate(this);
		u8 a1    = CEvt_Evaluate(this);
		f32 time = CEvt_Evaluate(this) / 100.0f;
		StartScreenFade(&gGame.mFade, r0, g0, b0, a0, r1, g1, b1, a1, time);
		break;
	}
	case EVT_ATOM_MONO: {
		u8 r0    = CEvt_Evaluate(this);
		u8 g0    = CEvt_Evaluate(this);
		u8 b0    = CEvt_Evaluate(this);
		u8 a0    = CEvt_Evaluate(this);
		u8 r1    = CEvt_Evaluate(this);
		u8 g1    = CEvt_Evaluate(this);
		u8 b1    = CEvt_Evaluate(this);
		u8 a1    = CEvt_Evaluate(this);
		f32 time = CEvt_Evaluate(this) / 100.0f;
		StartScreenFade(&gGame.mColor0.mColor, r0, g0, b0, a0, r1, g1, b1, a1, time);
		break;
	}
	case EVT_ATOM_WIPE: {
		s32 type = CEvt_Evaluate(this);
		s32 open = CEvt_Evaluate(this);
		s32 x    = CEvt_Evaluate(this);
		s32 y    = CEvt_Evaluate(this);
		s32 w0   = CEvt_Evaluate(this);
		s32 h0   = CEvt_Evaluate(this);
		u8 r0    = CEvt_Evaluate(this);
		u8 g0    = CEvt_Evaluate(this);
		u8 b0    = CEvt_Evaluate(this);
		u8 a0    = CEvt_Evaluate(this);
		s32 w1   = CEvt_Evaluate(this);
		s32 h1   = CEvt_Evaluate(this);
		u8 r1    = CEvt_Evaluate(this);
		u8 g1    = CEvt_Evaluate(this);
		u8 b1    = CEvt_Evaluate(this);
		u8 a1    = CEvt_Evaluate(this);
		f32 time = CEvt_Evaluate(this) / 100.0f;
		switch (type) {
		case 0:
			StartScreenWipeColorTransition(&gGame.mWipeCircle, r0, g0, b0, a0, r1, g1, b1, x, y, w0, h0, w1, h1, time, a1, !open);
			break;
		case 1:
			StartScreenWipeColorTransition(&gGame.mWipeTile0, r0, g0, b0, a0, r1, g1, b1, x, y, w0, h0, w1, h1, time, a1, !open);
			break;
		default:
			if (type >= 2000) {
				if (open) {
					StartScreenWipeColorTransition(&gGame.mWipeCustom, r0, g0, b0, a0, r1, g1, b1, x, y, 57.0f, 57.0f, 0.0f, 0.0f, time, a1,
					                               !open);
				} else {
					StartScreenWipeColorTransition(&gGame.mWipeCustom, r0, g0, b0, a0, r1, g1, b1, x, y, 0.0f, 0.0f, 57.0f, 57.0f, time, a1,
					                               !open);
				}
			} else if (type >= 1000) {
				TPLPalette* palette = (TPLPalette*)(&gGame.mTpl)->mBuffer;
				CTexObj* texture    = &(&gGame.mTpl)->mTex;
				gGame.mWipeTex.mTex = texture->Init(&palette->descriptorArray[type - 1000], 0);
				StartScreenWipeColorTransition(&gGame.mWipeTex, r0, g0, b0, a0, r1, g1, b1, x, y, w0, h0, w1, h1, time, a1, !open);
			} else {
#if defined(VERSION_PAL)
				const s32 line = 3542;
#else
				const s32 line = 3531;
#endif
				OSPanic("evt.cpp", line, "%s%s", FormatToStaticBuffer("scrn(@WIPE, %d,...) - unresolved!\n", type), "_cmd_SCRN");
			}
			break;
		}
		break;
	}
	case EVT_ATOM_LETTERBOX: {
		s32 open    = CEvt_Evaluate(this);
		s32 height0 = open ? 448 : 314;
		u8 r0       = CEvt_Evaluate(this);
		u8 g0       = CEvt_Evaluate(this);
		u8 b0       = CEvt_Evaluate(this);
		u8 a0       = CEvt_Evaluate(this);
		s32 height1 = open ? 314 : 448;
		u8 r1       = CEvt_Evaluate(this);
		u8 g1       = CEvt_Evaluate(this);
		u8 b1       = CEvt_Evaluate(this);
		u8 a1       = CEvt_Evaluate(this);
		f32 time    = CEvt_Evaluate(this) / 100.0f;
		StartScreenWipeColorTransition(&gGame.mWipeTile1, r0, g0, b0, a0, r1, g1, b1, 320.0f, 200.0f, 640.0f, height0, 640.0f, height1,
		                               time, a1, !open);
		break;
	}
	case EVT_ATOM_HUD:
		switch (CEvt_Evaluate(this)) {
		case EVT_HUD_BATTERY: {
			s32 index                 = CEvt_Evaluate(this);
			EvtHudCommand commands[8] = { { 0, 0 }, { 1, 0 }, { 0, 1 }, { 1, 1 }, { 0, 2 }, { 1, 2 }, { 0, 3 }, { 1, 3 } };
			InfoScrn_ApplyBatteryHudCommand(&gGame.mInfoScrn, commands[index].enabled, commands[index].mode);
			break;
		}
		case EVT_HUD_CLOCK: {
			s32 index                 = CEvt_Evaluate(this);
			EvtHudCommand commands[8] = { { 0, 0 }, { 1, 0 }, { 0, 1 }, { 1, 1 }, { 1, 2 }, { 1, 3 }, { 0, 4 }, { 1, 4 } };
			InfoScrn_ApplyClockHudCommand(&gGame.mInfoScrn, commands[index].enabled, commands[index].mode, 0);
			break;
		}
		case EVT_HUD_SCORE: {
			s32 index                 = CEvt_Evaluate(this);
			EvtHudCommand commands[4] = { { 0, 0 }, { 1, 0 }, { 0, 1 }, { 1, 1 } };
			InfoScrn_ApplyScoreHudCommand(&gGame.mInfoScrn, commands[index].enabled, commands[index].mode);
			break;
		}
		case 3:
			CEvt_Evaluate(this);
			CEvt_Evaluate(this);
			CEvt_Evaluate(this);
			CEvt_Evaluate(this);
			break;
		case EVT_HUD_TIMER: {
			s32 index = CEvt_Evaluate(this);
			if (index >= 0) {
				EvtHudCommand commands[4] = { { 0, 0 }, { 1, 0 }, { 0, 1 }, { 1, 1 } };
				InfoScrn_ApplyTimerHudCommand(&gGame.mInfoScrn, commands[index].enabled, commands[index].mode);
			} else {
				switch (index) {
				case -1:
					InfoScrn_ShowTimer(&gGame.mInfoScrn);
					break;
				case -2:
					InfoScrn_HideTimer(&gGame.mInfoScrn);
					break;
				case -3: {
					s32 hundredths = CEvt_Evaluate(this);
					InfoScrn_SetTimerHundredths(&gGame.mInfoScrn, hundredths);
					break;
				}
				case -4:
					mSelectResult = InfoScrn_GetTimerHundredths(&gGame.mInfoScrn);
					break;
				}
			}
			break;
		}
		}
		break;
	case EVT_ATOM_ZBLUR: {
		u8 a0       = CEvt_Evaluate(this);
		u8 a1       = CEvt_Evaluate(this);
		s32 offsetX = CEvt_Evaluate(this);
		s32 offsetY = CEvt_Evaluate(this);
		f32 time    = CEvt_Evaluate(this) / 100.0f;
		f32 start   = a0 / 255.0f;
		f32 end     = a1 / 255.0f;
		f32 step;
		if (time == 0.0f) {
			start = end;
			step  = 0.0f;
		} else {
			step = (end - start) / (FRAMES_PER_SEC * time);
		}
		if (start >= 0.0f)
			gGame.mBlur.mAlpha = start;
		if (end >= 0.0f)
			gGame.mBlur.mTarget = end;
		step                 = step < 0.0f ? -step : step;
		gGame.mBlur.mSpeed   = step;
		gGame.mBlur.mOffsetX = offsetX;
		gGame.mBlur.mOffsetY = offsetY;
		break;
	}
	case EVT_ATOM_SHAKE: {
		f32 x0   = CEvt_Evaluate(this) / 100.0f;
		f32 y0   = CEvt_Evaluate(this) / 100.0f;
		f32 z0   = CEvt_Evaluate(this) / 100.0f;
		f32 x1   = CEvt_Evaluate(this) / 100.0f;
		f32 y1   = CEvt_Evaluate(this) / 100.0f;
		f32 z1   = CEvt_Evaluate(this) / 100.0f;
		f32 time = CEvt_Evaluate(this) / 100.0f;
		StartScreenShake(&gGame.mQuake, x0, y0, z0, x1, y1, z1, time);
		break;
	}
	default:
#if defined(VERSION_PAL)
		const s32 line = 3737;
#else
		const s32 line = 3726;
#endif
		OSPanic("evt.cpp", line, "%s%s", FormatToStaticBuffer("scrn(%d,...) - unresolved!\n", mode), "_cmd_SCRN");
		break;
	}
	return 0;
}

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
