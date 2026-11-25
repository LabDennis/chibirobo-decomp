#include "game/relmodule.h"
#include "game/hsd_jobj.h"
#include "game/cgame.h"

extern "C" s32 CmdSetCameraScroll(s32 cmd, s32 count, s32* values);
extern "C" void InitStage(s32 arg);

extern "C" {
RelSlotFunc _prolog_slots[7] = { InitStage };
RelCmdFunc _prolog_cmds[]    = { CmdSetCameraScroll };
}

/**
 * @note Address: stage17 .text 0x0
 * @note Size: 0xC0
 */
extern "C" s32 CmdSetCameraScroll(s32 cmd, s32 count, s32* values)
{
	CCameraActions& actions = gGame.mCameraActions;
	s32 i;

	for (i = 0; i < count; i++) {
		f32 v = values[i] / 100.0f;
		if (v <= 0.0f) {
			continue;
		}
		switch (i) {
		case 0:
			static_cast<CCameraActionScroll*>(actions.mTable[CAMACTION_SCROLL])->mNearDistance = v;
			break;
		case 1:
			static_cast<CCameraActionScroll*>(actions.mTable[CAMACTION_SCROLL])->mFarDistance = v;
			break;
		case 2:
			static_cast<CCameraActionScroll*>(actions.mTable[CAMACTION_SCROLL])->mScrollFov = v;
			break;
		}
	}
	return 0;
}

/**
 * @note Address: stage17 .text 0xC0
 * @note Size: 0x4
 */
extern "C" void InitStage(s32 arg)
{
}
