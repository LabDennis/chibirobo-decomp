#include "types.h"
#include "Version.h"
#include "game/cgame.h"

extern "C" s32 CmdNop(s32 cmd, s32 count, s32* args);
extern "C" void Stage20NoOpSlot5(void* arg);
extern "C" void UpdateStage(void* arg);
extern "C" void InitStage(void* arg);

typedef void (*RelSlotFunc)(void* arg);
typedef s32 (*RelCmdFunc)(s32 cmd, s32 count, s32* args);

extern "C" {
RelSlotFunc _prolog_slots[7] = { InitStage, UpdateStage, 0, 0, 0, Stage20NoOpSlot5 };
RelCmdFunc _prolog_cmds[]    = { CmdNop };
}

static XObj* timeAnimObj;

/**
 * @note Address: stage20 .text 0x0
 * @note Size: 0x8
 */
extern "C" s32 CmdNop(s32 cmd, s32 count, s32* args)
{
	return 0;
}

/**
 * @note Address: stage20 .text 0x8
 * @note Size: 0x4
 */
extern "C" void Stage20NoOpSlot5(void* arg)
{
}

/**
 * @note Address: stage20 .text 0xC
 * @note Size: 0xA0
 */
extern "C" void UpdateStage(void* arg)
{
	if (timeAnimObj != NULL) {
		f32 x = 150.0f * gGame.mTimeState.mTime;
		if (gGame.mTimeState.mMode == 1) {
			x += 150.0f;
		}
		timeAnimObj->SetAnim(0, 1, 0, 0, 0, x, 1.0f, 0.0f, 10.0f);
	}
}

/**
 * @note Address: stage20 .text 0xAC
 * @note Size: 0x78
 */
extern "C" void InitStage(void* arg)
{
	XObj* obj;
	s32 i;

	timeAnimObj = 0;
	for (i = 0; i < gGame.mXObjs.mObjectCount; i++) {
		obj = gGame.GetObject(i);
		if ((obj->mStatus.mFlags & 1) != 0 && obj->mCoord.mPlacement->mObjectId == 0x2CD) {
			timeAnimObj = obj;
		}
	}
}
