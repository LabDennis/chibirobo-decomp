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
