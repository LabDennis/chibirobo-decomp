#include "game/cgame.h"
#include "game/xobjmap.h"
#include "game/cgameutil.h"

extern "C" {
char* strtok(char* str, const char* delim);
int strcmp(const char* a, const char* b);
}

/**
 * @note Address: 0x80013580
 * @note Size: 0xCC
 */
// reads /stage:name and /slot:letter off the command line
extern "C" void Initialize__5CGameFiPPc(CGame* game, int argc, char** argv)
{
	for (s32 i = 1; i < argc; i++) {
		switch (argv[i][0]) {
		case '/':
			char* name = strtok(argv[i] + 1, ":");
			if (strcmp(name, "stage") == 0) {
				gCmdLineArgs.mStage = FindStageIndexByName(strtok(NULL, " "));
			} else if (strcmp(name, "slot") == 0) {
				gCmdLineArgs.mSlot = *strtok(NULL, " ") - 'A';
			}
			break;
		}
	}
}

/**
 * @note Address: 0x8001357C
 * @note Size: 0x4
 */
void CScrnFade::Reset()
{
}
