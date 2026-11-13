#include "types.h"
#include "Version.h"

extern "C" u8 gGame[];

extern "C" s32 CmdSetCameraScroll(s32 cmd, s32 count, s32* values);
extern "C" void InitStage(void* arg);

struct CCameraActionScroll {
	u8 _00[0x44];      // _00
	f32 mFarDistance;  // _44, CCameraActionScroll::mFarDistance
	f32 mNearDistance; // _48, CCameraActionScroll::mNearDistance
	f32 mScrollFov;    // _4C, CCameraActionScroll::mScrollFov
};

#if defined(VERSION_JP)
// 0xCE8 earlier in japan, its CGame is shorter
#define STAGE17_UNK() (*(CCameraActionScroll**)(gGame + 0xE7130))
#else
#define STAGE17_UNK() (*(CCameraActionScroll**)(gGame + 0xE7E18))
#endif

typedef void (*RelSlotFunc)(void* arg);
typedef s32 (*RelCmdFunc)(s32 cmd, s32 count, s32* args);

extern "C" {
RelSlotFunc _prolog_slots[7] = { InitStage };
RelCmdFunc _prolog_cmds[]    = { (RelCmdFunc)CmdSetCameraScroll };
}

/**
 * @note Address: stage17 .text 0x0
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
			STAGE17_UNK()->mNearDistance = v;
			break;
		case 1:
			STAGE17_UNK()->mFarDistance = v;
			break;
		case 2:
			STAGE17_UNK()->mScrollFov = v;
			break;
		}
	}
	return 0;
}

/**
 * @note Address: stage17 .text 0xC0
 * @note Size: 0x4
 */
extern "C" void InitStage(void* arg)
{
}
