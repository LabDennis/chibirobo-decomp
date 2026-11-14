#include "game/wii/csound.h"

#include <string.h>

struct WiiSoundBank {
	const char* name;
	s32 groupId;
};

static const char sBankKitchen[]     = "sfx_kitchen.ssm";
static const char sBankStage02[]     = "sfx_stage02.ssm";
static const char sBankSouko[]       = "sfx_souko.ssm";
static const char sBankJennyRoom[]   = "sfx_jennyroom.ssm";
static const char sBankStage05[]     = "sfx_stage05.ssm";
static const char sBankPapaMama[]    = "sfx_papamama.ssm";
static const char sBankStage07[]     = "sfx_stage07.ssm";
static const char sBankUraniwa[]     = "sfx_uraniwa.ssm";
static const char sBankPipe[]        = "sfx_pipe.ssm";
static const char sBankManual[]      = "sfx_manual.ssm";
static const char sBankUfo[]         = "sfx_ufo.ssm";
static const char sBankMartial[]     = "sfx_martial.ssm";
static const char sBankHock[]        = "sfx_hock.ssm";
static const char sBankGicco[]       = "sfx_gicco.ssm";
static const char sBankBb[]          = "sfx_bb.ssm";
static const char sBankSanpoo[]      = "sfx_sanpoo.ssm";
static const char sBankTrex[]        = "sfx_trex.ssm";
static const char sBankArmy[]        = "sfx_army.ssm";
static const char sBankKaeru[]       = "sfx_kaeru.ssm";
static const char sBankUraniwaAmb1[] = "sfx_uraniwa_ambient1.ssm";
static const char sBankUraniwaAmb2[] = "sfx_uraniwa_ambient2.ssm";
static const char sBankUraniwaAmb3[] = "sfx_uraniwa_ambient3.ssm";
static const char sBankConcert[]     = "sfx_concert.ssm";
static const char sBankEnding[]      = "sfx_ending.ssm";

WiiSoundBank gWiiSoundBanks[] = {
	{ sBankKitchen, 1 },      { sBankStage02, 2 },      { sBankSouko, 3 },    { sBankJennyRoom, 4 }, { sBankStage05, 5 },
	{ sBankPapaMama, 6 },     { sBankStage07, 7 },      { sBankUraniwa, 8 },  { sBankPipe, 9 },      { sBankManual, 10 },
	{ sBankUfo, 11 },         { sBankMartial, 12 },     { sBankHock, 13 },    { sBankGicco, 14 },    { sBankBb, 15 },
	{ sBankSanpoo, 16 },      { sBankTrex, 17 },        { sBankArmy, 18 },    { sBankKaeru, 19 },    { sBankUraniwaAmb1, 20 },
	{ sBankUraniwaAmb2, 21 }, { sBankUraniwaAmb3, 22 }, { sBankConcert, 23 }, { sBankEnding, 24 },   { NULL, 0 },
};

extern "C" BOOL LoadWiiSoundBankByName(XSound* sound, const char* name)
{
	for (int i = 0; gWiiSoundBanks[i].name; ++i) {
		if (strcmp(name, gWiiSoundBanks[i].name) == 0)
			return sound->LoadGroupAsync(gWiiSoundBanks[i].groupId);
	}
	return FALSE;
}
