#include "game/wii/csound.h"

#include <string.h>

struct WiiSoundBank {
	const char* name;
	s32 groupId;
};

WiiSoundBank gWiiSoundBanks[] = {
	{ "sfx_kitchen.ssm", 1 },
	{ "sfx_stage02.ssm", 2 },
	{ "sfx_souko.ssm", 3 },
	{ "sfx_jennyroom.ssm", 4 },
	{ "sfx_stage05.ssm", 5 },
	{ "sfx_papamama.ssm", 6 },
	{ "sfx_stage07.ssm", 7 },
	{ "sfx_uraniwa.ssm", 8 },
	{ "sfx_pipe.ssm", 9 },
	{ "sfx_manual.ssm", 10 },
	{ "sfx_ufo.ssm", 11 },
	{ "sfx_martial.ssm", 12 },
	{ "sfx_hock.ssm", 13 },
	{ "sfx_gicco.ssm", 14 },
	{ "sfx_bb.ssm", 15 },
	{ "sfx_sanpoo.ssm", 16 },
	{ "sfx_trex.ssm", 17 },
	{ "sfx_army.ssm", 18 },
	{ "sfx_kaeru.ssm", 19 },
	{ "sfx_uraniwa_ambient1.ssm", 20 },
	{ "sfx_uraniwa_ambient2.ssm", 21 },
	{ "sfx_uraniwa_ambient3.ssm", 22 },
	{ "sfx_concert.ssm", 23 },
	{ "sfx_ending.ssm", 24 },
	{ NULL, 0 },
};

extern "C" BOOL LoadWiiSoundBankByName(XSound* sound, const char* name)
{
	for (const WiiSoundBank* bank = gWiiSoundBanks; bank->name; ++bank) {
		if (strcmp(name, bank->name) == 0)
			return sound->LoadGroupAsync(bank->groupId);
	}
	return FALSE;
}
