#include "game/cgame.h"

extern "C" {
s32 InfoScrn_GetTimerHundredths(InfoScrn* screen);
void InfoScrn_SetTimerHundredths(InfoScrn* screen, s32 v);
void InfoScrn_HideTimer(InfoScrn* screen);
void InfoScrn_ShowTimer(InfoScrn* screen);
void InfoScrn_ApplyTimerHudCommand(InfoScrn* screen, u8 show, s32 mode);
}

/**
 * @note Address: stage02 .text 0x6170
 * @note Size: 0x30
 */
extern "C" s32 CmdGetTimer(s32 cmd, s32 argc, s32* argv)
{
	return InfoScrn_GetTimerHundredths(&gGame.mInfoScrn);
}

/**
 * @note Address: stage02 .text 0x61A0
 * @note Size: 0x38
 */
extern "C" s32 CmdSetTimer(s32 cmd, s32 argc, s32* argv)
{
	InfoScrn_SetTimerHundredths(&gGame.mInfoScrn, argv[0]);
	return 0;
}

/**
 * @note Address: stage02 .text 0x61D8
 * @note Size: 0x58
 */
extern "C" s32 CmdSetTimerRunning(s32 cmd, s32 argc, s32* argv)
{
	if (argv[0] != 0) {
		InfoScrn_ShowTimer(&gGame.mInfoScrn);
	} else {
		InfoScrn_HideTimer(&gGame.mInfoScrn);
	}
	return 0;
}

/**
 * @note Address: stage02 .text 0x6230
 * @note Size: 0x68
 */
extern "C" s32 CmdShowTimer(s32 cmd, s32 argc, s32* argv)
{
	if (argv[0] != 0) {
		InfoScrn_ApplyTimerHudCommand(&gGame.mInfoScrn, 1, 1);
	} else {
		InfoScrn_ApplyTimerHudCommand(&gGame.mInfoScrn, 0, 1);
	}
	return 0;
}
