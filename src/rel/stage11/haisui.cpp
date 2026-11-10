#include "types.h"
#include "Version.h"

// CGame
extern "C" u8 gGame[];

extern "C" s32 CmdSetCameraScroll(s32 cmd, s32 count, s32* values);
extern "C" void InitStage(void* arg);

struct CCameraActionScroll {
	u8 _00[0x44]; // _00
	f32 _44;      // _44
	f32 _48;      // _48
	f32 _4C;      // _4C
};

// past the end of CGame's symbol size
#if defined(VERSION_JP)
// 0xCE8 earlier in japan, its CGame is shorter
#define STAGE11_UNK() (*(CCameraActionScroll**)(gGame + 0xE7130))
#else
#define STAGE11_UNK() (*(CCameraActionScroll**)(gGame + 0xE7E18))
#endif

typedef void (*RelSlotFunc)(void* arg);
typedef s32 (*RelCmdFunc)(s32 cmd, s32 arg, void* arg2);

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
			STAGE11_UNK()->_48 = v;
			break;
		case 1:
			STAGE11_UNK()->_44 = v;
			break;
		case 2:
			STAGE11_UNK()->_4C = v;
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
