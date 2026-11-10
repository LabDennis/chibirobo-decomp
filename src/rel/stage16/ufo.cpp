#include "types.h"
#include "Version.h"
#include "game/cgame.h"
#include "game/xchr.h"

extern "C" f64 atan2(f64 y, f64 x);

extern "C" s32 CmdNop(s32 cmd, void* arg, void* arg2);
extern "C" void UpdateStage(void* arg);
extern "C" void InitStage(void* arg);

typedef void (*RelSlotFunc)(void* arg);
typedef s32 (*RelCmdFunc)(s32 cmd, void* arg, void* arg2);

extern "C" {
RelSlotFunc _prolog_slots[7] = { InitStage, UpdateStage };
RelCmdFunc _prolog_cmds[]    = { CmdNop };
}

static XObj* gameBotan2;

/**
 * @note Address: stage16 .text 0x0
 * @note Size: 0x8
 */
extern "C" s32 CmdNop(s32 cmd, void* arg, void* arg2)
{
	return 0;
}

/**
 * @note Address: stage16 .text 0x8
 * @note Size: 0x114
 */
extern "C" void UpdateStage(void* arg)
{
	if (gameBotan2 != NULL) {
		XObj* other = gGame.GetPlayer();
		f32 angle   = atan2(other->mMoveTranslate.x - gameBotan2->mMoveTranslate.x, other->mMoveTranslate.z - gameBotan2->mMoveTranslate.z);
		gameBotan2->SetRotateY(angle, 0.0f);
	} else {
		XObj* obj;
		s32 i;
		for (i = 0; i < gGame.mXObjs.mObjectCount && gameBotan2 == NULL; i++) {
			obj = gGame.GetObject(i);
			if ((obj->mStatus.mFlags & 1) != 0 && obj->GetKind() == 0x2E9) {
				gameBotan2 = obj;
			}
		}
	}
}

/**
 * @note Address: stage16 .text 0x11C
 * @note Size: 0x10
 */
extern "C" void InitStage(void* arg)
{
	gameBotan2 = 0;
}
