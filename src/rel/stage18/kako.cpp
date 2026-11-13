#include "types.h"
#include "Version.h"
#include "dolphin/mtx.h"

extern "C" u8 gGame[];

extern "C" s32 IsGameSoundPlaying(void* obj, s32 id);
extern "C" void* GetSoundVolumeAtPosition(void* obj, Vec* pos, s32 arg);
extern "C" void SetGameSfxVolume(void* obj, s32 id, void* arg, s32 arg2);

// rel/ana.cpp
extern "C" s32 CmdDropHeldObj(s32 cmd, s32 count, s32* args);

extern "C" void UpdateStage(void* arg);

// past the end of CGame's symbol size
#if defined(VERSION_JP)
// 0xCE8 earlier in japan, its CGame is shorter
#define SOUND() ((void*)(gGame + 0xE713C))
#else
#define SOUND() ((void*)(gGame + 0xE7E24))
#endif

typedef void (*RelSlotFunc)(void* arg);
typedef s32 (*RelCmdFunc)(s32 cmd, s32 count, s32* args);

extern "C" {
RelSlotFunc _prolog_slots[7] = { 0, UpdateStage };
RelCmdFunc _prolog_cmds[]    = { CmdDropHeldObj };
}

/**
 * @note Address: stage18 .text 0x0
 * @note Size: 0x98
 */
extern "C" void UpdateStage(void* arg)
{
	Vec pos;

	pos.x = -278.67f;
	pos.y = 0.0f;
	pos.z = 265.43f;
	if (IsGameSoundPlaying(SOUND(), 0x9001F)) {
		SetGameSfxVolume(SOUND(), 0x9001F, GetSoundVolumeAtPosition(SOUND(), &pos, 0), 0);
	}
}
