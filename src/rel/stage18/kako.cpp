#if defined(VERSION_R24J01)
#include "game/wii/cgame.h"

extern "C" f32 GetSoundVolumeAtPosition(XSound* sound, Vec* pos, s32 kind);

extern "C" s32 Stage18_Update(void* arg)
{
	Vec pos;
	pos.x = -278.67f;
	pos.y = 0.0f;
	pos.z = 265.43f;
	if (IsWiiSfxPlaying(&gGame.mSound, 0x453)) {
		SetWiiSfxVolume(&gGame.mSound, 0x453, GetSoundVolumeAtPosition(&gGame.mSound, &pos, 0), 0.0f);
	}
	return 1;
}
#else
#include "game/relana.h"
#include "game/relmodule.h"
#include "game/hsd_jobj.h"
#include "game/cgame.h"

extern "C" void UpdateStage(s32 arg);

extern "C" {
RelSlotFunc _prolog_slots[7] = { 0, UpdateStage };
RelCmdFunc _prolog_cmds[]    = { CmdDropHeldObj };
}

/**
 * @note Address: stage18 .text 0x0
 * @note Size: 0x98
 */
extern "C" void UpdateStage(s32 arg)
{
	Vec pos;

	pos.x = -278.67f;
	pos.y = 0.0f;
	pos.z = 265.43f;
	if (IsGameSoundPlaying(&gGame.mSound, 0x9001F)) {
		SetGameSfxVolume(&gGame.mSound, 0x9001F, GetSoundVolumeAtPosition(&gGame.mSound, &pos, 0), 0);
	}
}
#endif
