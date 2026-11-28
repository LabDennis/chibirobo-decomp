#if defined(VERSION_R24J01)
#include "game/wii/cgame.h"

extern "C" void InitTrash(void);
extern "C" void UpdateTrash(u32 frames);
extern "C" void DrawTrashStack(void);
extern "C" void ExitTrash(void);
extern "C" void SetFafaPartVisibility(s32 mode);
extern "C" f32 GetSoundVolumeAtPosition(XSound* sound, Vec* pos, s32 kind);

extern "C" s32 Stage07_Init(void* arg)
{
	InitTrash();
	return 1;
}

// NONMATCHING: the object-table cursor and current-object registers are swapped; every test and call otherwise matches.
extern "C" s32 Stage07_Update(void* arg)
{
	XObj* actor;
	s32 i;
	if (IsWiiSfxPlaying(&gGame.mSound, 0x3CB)) {
		for (i = 0; i < gGame.mXObjs.mObjectCount; i++) {
			actor = gGame.mXObjs.mObjects[i];
			if ((actor->mStatus.mFlags & 1) && (actor->mStatus.mFlags & 0x10000) && actor->mCoord.mPlacement->mObjectId == 0xD0) {
				PSVECDistance(&gGame.mMotionControlCamera.mCamera.mView.mInterest, actor->GetPos());
				SetWiiSfxVolume(&gGame.mSound, 0x3CB, GetSoundVolumeAtPosition(&gGame.mSound, actor->GetPos(), 1), 0.0f);
			}
		}
	}
	UpdateTrash((u32)arg);
	return 1;
}

extern "C" s32 Stage07_Draw(void* arg)
{
	DrawTrashStack();
	return 1;
}

extern "C" s32 Stage07_Exit(void* arg)
{
	ExitTrash();
	return 1;
}

extern "C" s32 Stage07_CmdNop(s32 cmd, s32 count, s32* args)
{
	return 0;
}

extern "C" s32 CmdSetFafaPartVisibility(s32 cmd, s32 count, s32* args)
{
	SetFafaPartVisibility(args[0]);
	return 0;
}

// NONMATCHING: pool/stride/counter registers differ; all 400 checks and the return value match retail.
extern "C" s32 CmdSetObj37PartVisibility(s32 cmd, s32 count, s32* args)
{
	s32 size = gGame.mXObjs.mPools[10].mStride;
	u8* pool = gGame.mXObjs.mPools[10].mObjects;
	s32 i;
	for (i = 0; i < 400; i++) {
		XObj* entry = (XObj*)(pool + i * size);
		if (entry->GetKind() == 0x37) {
			break;
		}
	}
	return 0;
}

extern "C" s32 CmdAddObjFlagList(s32 cmd, s32 count, s32* values)
{
	for (s32 i = 0; i < count; i++) {
		((CCameraActionScroll2*)gGame.mCameraActions.mTable[4])->AddHideObj(values[i]);
	}
	return 0;
}
#else
#include "game/reltrash.h"
#include "game/relmodule.h"
#include "game/cgame.h"
#include "game/cjobjset.h"
#include "game/csound.h"

// in rel/trash.cpp and rel/fafa.cpp
extern "C" void SetFafaPartVisibility(s32 mode);

// one child step then path[i] sibling steps per level
static inline HSD_JObj* GetJObjByPath(HSD_JObj* jobj, s32* path, s32 depth)
{
	s32 i;
	s32 j;

	if (jobj == NULL) {
		return NULL;
	}
	for (i = 0; i < depth; i++, path++) {
		jobj = jobj == NULL ? NULL : jobj->child;
		for (j = 0; j < *path; j++) {
			jobj = jobj == NULL ? NULL : jobj->next;
			if (jobj == NULL) {
				return NULL;
			}
		}
	}
	return jobj;
}

extern "C" s32 CmdAddObjFlagList(s32 cmd, s32 count, s32* values);
extern "C" void SetObj37PartVisibility(s32 mode);
extern "C" s32 CmdSetObj37PartVisibility(s32 cmd, s32 count, s32* args);
extern "C" s32 CmdSetFafaPartVisibility(s32 cmd, s32 count, s32* args);
extern "C" s32 CmdNop(s32 cmd, s32 count, s32* args);
extern "C" void DrawStage(s32 arg);
extern "C" void UpdateStage(s32 arg);
extern "C" void InitStage(s32 arg);

extern "C" {
RelSlotFunc _prolog_slots[7] = { InitStage, UpdateStage, DrawStage }; // only the first three slots are used
RelCmdFunc _prolog_cmds[]    = {
    CmdNop,         CmdSetFafaPartVisibility, CmdSetObj37PartVisibility, CmdStartTrash, CmdGetTrashResult, CmdEndTrash, CmdAddObjFlagList,
    CmdCancelTrash, CmdIsNearTrashbox,
};
}

/**
 * @note Address: stage07 .text 0x0
 * @note Size: 0x78
 */
extern "C" s32 CmdAddObjFlagList(s32 cmd, s32 count, s32* values)
{
	s32 i;

	for (i = 0; i < count; i++) {
		CCameraActionScroll2* camera = static_cast<CCameraActionScroll2*>(gGame.mCameraActions.mTable[CAMACTION_SCROLL2]);
		camera->AddHideObj(values[i]);
	}
	return 0;
}

/**
 * @note Address: stage07 .text 0x78
 * @note Size: 0xF10
 */
extern "C" void SetObj37PartVisibility(s32 mode)
{
	CJObj jobj;
	const XObjPool& pool = gGame.mXObjs.mPools[22];
	s32 i;

	for (i = 0; i < 0x120; i++) {
		XObj* entry = pool.Get(i);
		if (entry->GetKind() != 0x37) { // only kind-0x37 placements
			continue;
		}
		// the mode picks which of the object's two parts shows
		switch (mode) {
		case 0: { // 0: both hidden
			s32 path0[2] = { 0, 1 };
			jobj.Attach(GetJObjByPath(entry->mJObj, path0, 2));
			HSD_JObjSetFlags(jobj.mJObj, HIDDEN);
			s32 path1[2] = { 0, 2 };
			jobj.Attach(GetJObjByPath(entry->mJObj, path1, 2));
			HSD_JObjSetFlags(jobj.mJObj, HIDDEN);
			break;
		}
		case 1: { // 1: the first part shows
			s32 path0[2] = { 0, 1 };
			jobj.Attach(GetJObjByPath(entry->mJObj, path0, 2));
			HSD_JObjClearFlags(jobj.mJObj, HIDDEN);
			s32 path1[2] = { 0, 2 };
			jobj.Attach(GetJObjByPath(entry->mJObj, path1, 2));
			HSD_JObjSetFlags(jobj.mJObj, HIDDEN);
			break;
		}
		case 2: { // 2: the second part shows
			s32 path0[2] = { 0, 1 };
			jobj.Attach(GetJObjByPath(entry->mJObj, path0, 2));
			HSD_JObjSetFlags(jobj.mJObj, HIDDEN);
			s32 path1[2] = { 0, 2 };
			jobj.Attach(GetJObjByPath(entry->mJObj, path1, 2));
			HSD_JObjClearFlags(jobj.mJObj, HIDDEN);
			break;
		}
		case 3: { // 3: both show
			s32 path0[2] = { 0, 1 };
			jobj.Attach(GetJObjByPath(entry->mJObj, path0, 2));
			HSD_JObjClearFlags(jobj.mJObj, HIDDEN);
			s32 path1[2] = { 0, 2 };
			jobj.Attach(GetJObjByPath(entry->mJObj, path1, 2));
			HSD_JObjClearFlags(jobj.mJObj, HIDDEN);
			break;
		}
		}
		break; // only the first kind-0x37 entry
	}
}

/**
 * @note Address: stage07 .text 0xF88
 * @note Size: 0x88
 */
// CJObj::~CJObj, inline

/**
 * @note Address: stage07 .text 0x1010
 * @note Size: 0x48
 */
// CVec::~CVec, inline

/**
 * @note Address: stage07 .text 0x1058
 * @note Size: 0x28
 */
extern "C" s32 CmdSetObj37PartVisibility(s32 cmd, s32 count, s32* args)
{
	SetObj37PartVisibility(args[0]);
	return 0;
}

/**
 * @note Address: stage07 .text 0x1080
 * @note Size: 0x28
 */
extern "C" s32 CmdSetFafaPartVisibility(s32 cmd, s32 count, s32* args)
{
	SetFafaPartVisibility(args[0]);
	return 0;
}

/**
 * @note Address: stage07 .text 0x10A8
 * @note Size: 0x8
 */
extern "C" s32 CmdNop(s32 cmd, s32 count, s32* args)
{
	return 0;
}

/**
 * @note Address: stage07 .text 0x10B0
 * @note Size: 0x20
 */
extern "C" void DrawStage(s32 arg)
{
	DrawTrashStack();
}

/**
 * @note Address: stage07 .text 0x10D0
 * @note Size: 0x110
 */
// NONMATCHING: retail makes a discarded PSVECDistance call before GetSoundVolumeAtPosition and uses a larger
// saved-register frame. The dead call stays removed; object filtering, volume update and UpdateTrash match.
extern "C" void UpdateStage(s32 arg)
{
	XSound* sound = &gGame.mSound;
	s32 i;

	if (IsGameSoundPlaying(sound, 0x1000D)) {
		for (i = 0; i < gGame.mXObjs.mObjectCount; i++) {
			XObj* actor = gGame.GetObject(i);
			// live, drawn objects of kind 0xD0 drive the loop's volume
			if ((actor->mStatus.mFlags & 1) && (actor->mStatus.mFlags & 0x10000) && actor->mCoord.mPlacement->mObjectId == 0xD0) {
				SetGameSfxVolume(sound, 0x1000D, GetSoundVolumeAtPosition(sound, &actor->mMoveTranslate, 1), 0);
			}
		}
	}
	UpdateTrash(arg);
}

/**
 * @note Address: stage07 .text 0x11E0
 * @note Size: 0x20
 */
extern "C" void InitStage(s32 arg)
{
	InitTrash();
}

/**
 * @note Address: stage07 .text 0x1200
 * @note Size: 0xD4
 */
// CJObj::SetTranslateX, inline

/**
 * @note Address: stage07 .text 0x12D4
 * @note Size: 0xD4
 */
// CJObj::SetTranslateY, inline

/**
 * @note Address: stage07 .text 0x13A8
 * @note Size: 0xD4
 */
// CJObj::SetTranslateZ, inline

/**
 * @note Address: stage07 .text 0x147C
 * @note Size: 0xD4
 */
// CJObj::SetScaleX, inline

/**
 * @note Address: stage07 .text 0x1550
 * @note Size: 0xD4
 */
// CJObj::SetScaleY, inline

/**
 * @note Address: stage07 .text 0x1624
 * @note Size: 0xD4
 */
// CJObj::SetScaleZ, inline

/**
 * @note Address: stage07 .text 0x16F8
 * @note Size: 0x1A8
 */
// CJObj::SetRotationX, inline

/**
 * @note Address: stage07 .text 0x18A0
 * @note Size: 0x1A8
 */
// CJObj::SetRotationY, inline

/**
 * @note Address: stage07 .text 0x1A48
 * @note Size: 0x1A8
 */
// CJObj::SetRotationZ, inline

/**
 * @note Address: stage07 .text 0x1BF0
 * @note Size: 0x178
 */
// CJObj::SetTranslate, the three float one, inline

/**
 * @note Address: stage07 .text 0x1D68
 * @note Size: 0x158
 */
// CJObj::SetTranslate, the vector one, inline

/**
 * @note Address: stage07 .text 0x1EC0
 * @note Size: 0x178
 */
// CJObj::SetScale, the three float one, inline

/**
 * @note Address: stage07 .text 0x2038
 * @note Size: 0x158
 */
// CJObj::SetScale, the vector one, inline

/**
 * @note Address: stage07 .text 0x2190
 * @note Size: 0x22C
 */
// CJObj::SetRotation, the three float one, inline

/**
 * @note Address: stage07 .text 0x23BC
 * @note Size: 0x20C
 */
// CJObj::SetRotation, the vector one, inline

/**
 * @note Address: stage07 .text 0x25C8
 * @note Size: 0x80
 */
// CJObj::SetAnimRate, inline
#endif
