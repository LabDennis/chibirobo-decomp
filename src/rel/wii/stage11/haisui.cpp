#if defined(VERSION_R24J01)
#include "game/wii/cgame.h"

#define STAGE11_SCROLL() ((CCameraActionScroll*)gGame.mCameraActions.mTable[11])

extern "C" s32 Stage11_Init(void* arg)
{
	return 1;
}

extern "C" s32 Stage11_CmdSetCameraScroll(s32 cmd, s32 count, s32* values)
{
	for (s32 i = 0; i < count; i++) {
		f32 v = values[i] / 100.0f;
		if (v <= 0.0f) {
			continue;
		}
		switch (i) {
		case 0:
			STAGE11_SCROLL()->mNearDistance = v;
			break;
		case 1:
			STAGE11_SCROLL()->mFarDistance = v;
			break;
		case 2:
			STAGE11_SCROLL()->mScrollFov = v;
			break;
		}
	}
	return 0;
}
#else
#include "types.h"
#include "Version.h"
#include "game/cgame.h"

extern "C" s32 CmdSetCameraScroll(s32 cmd, s32 count, s32* values);
extern "C" void InitStage(void* arg);

#define STAGE11_SCROLL() ((CCameraActionScroll*)gGame.mCameraActions.mTable[11])

typedef void (*RelSlotFunc)(void* arg);
typedef s32 (*RelCmdFunc)(s32 cmd, s32 count, s32* args);

extern "C" {
RelSlotFunc _prolog_slots[7] = { InitStage };
RelCmdFunc _prolog_cmds[]    = { (RelCmdFunc)CmdSetCameraScroll };
}

/**
 * @note Address: stage11 .text 0x0
 * @note Size: 0xC0
 */
extern "C" s32 CmdSetCameraScroll(s32 cmd, s32 count, s32* values)
{
	s32 i;

	for (i = 0; i < count; i++) {
		f32 v = values[i] / 100.0f;
		if (v <= 0.0f) {
			continue;
		}
		switch (i) {
		case 0:
			STAGE11_SCROLL()->mNearDistance = v;
			break;
		case 1:
			STAGE11_SCROLL()->mFarDistance = v;
			break;
		case 2:
			STAGE11_SCROLL()->mScrollFov = v;
			break;
		}
	}
	return 0;
}

/**
 * @note Address: stage11 .text 0xC0
 * @note Size: 0x4
 */
extern "C" void InitStage(void* arg)
{
}
#endif
