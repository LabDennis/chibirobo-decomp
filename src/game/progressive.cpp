#include "game/progressive.h"
#include "game/cgame.h"
#include "game/cmdline.h"

extern "C" {
u32 OSGetSoundMode(void);
char* strtok(char*, const char*);
int strcmp(const char*, const char*);
int atoi(const char*);
s32 GetStageNum();
const char* GetStageFileName(s32);
s32 GetDefaultStageIndex();
}

BootConfigEntry gBootConfigEntries[] = {
	{ "STAGE_NO", BootStage, 0 },
	{ "DRAW_XZGRID", BootFlag, 1ULL << 18 },
	{ "DRAW_WORLDAXIS", BootFlag, 1ULL << 19 },
	{ "DRAW_LIGHTMARK", BootFlag, 1ULL << 20 },
	{ "DRAW_SAFETYFRAME", BootFlag, 1ULL << 21 },
	{ "DRAW_FRAMEMETER", BootFlag, 1ULL << 22 },
	{ "DRAW_HEAPMETER", BootFlag, 1ULL << 23 },
	{ "DRAW_CROSSHAIR", BootCrosshair, 1ULL << 50 },
	{ "DRAW_DEBUGSTRING", BootFlag, 1ULL << 24 },
	{ "DRAW_COLLISION", BootFlag, 1ULL << 25 },
	{ "DRAW_CAMCOLLISION", BootFlag, 1ULL << 53 },
	{ "DRAW_OBJINFO", BootFlag, 1ULL << 26 },
	{ "DRAW_PLAYTIME", BootFlag, 1ULL << 27 },
	{ "PRINT_FILEINFO", BootFlag, 1ULL << 28 },
	{ "ENABLE_LOGO", BootFlag, 1ULL << 34 },
	{ "ENABLE_TITLE", BootFlag, 1ULL << 29 },
	{ "ENABLE_GAMEOVER", BootFlag, 1ULL << 30 },
	{ "ENABLE_TIMEUP", BootFlag, 1ULL << 46 },
	{ "ENABLE_DAYNIGHT", BootFlag, 1ULL << 49 },
	{ "ENABLE_DEBUGOPR", BootFlag, 1ULL << 31 },
	{ "ENABLE_ATC_COPTER", BootSetting, 1 },
	{ "ENABLE_ATC_POPPER", BootSetting, 2 },
	{ "ENABLE_ATC_RADAR", BootSetting, 3 },
	{ "ENABLE_RELMODULE", BootFlag, 1ULL << 32 },
	{ "ENABLE_ARAMCACHE", BootFlag, 1ULL << 41 },
	{ "ENABLE_PRECACHE", BootFlag, 1ULL << 42 },
	{ "ENABLE_ARCHIVER", BootUnrestrictedFlag, 1ULL << 33 },
	{ "ENABLE_SCREENSHOT", BootFlag, 1ULL << 52 },
	{ "ATC_MENU_TYPE", BootSetting, 0 },
	{ "LIFTPLUG_TYPE", BootSetting, 4 },
	{ "ATC_ACTION_TYPE", BootSetting, 5 },
	{ "TOOL_ACTION_TYPE", BootSetting, 6 },
	{ "TITLE_CURSOR", BootSetting, 7 },
	{ "EVENT_LOG", BootEventLog, 0 },
	{ NULL, BootFlag, 0 },
};

VideoModePrompt gVideoModePrompt = { 0, -1, 0.0f, 0.0f, 0, 0 };

/**
 * @note Address: 0x8001C89C
 * @note Size: 0x28
 */
extern "C" void ResetProgressiveScanPrompt(void)
{
	gVideoModePrompt.step              = 0;
	gVideoModePrompt.pendingMode       = -1;
	gVideoModePrompt.framesRemaining   = 0.0f;
	gVideoModePrompt.restoredSelection = 0;
	gVideoModePrompt.useStandardMode   = 0;
}

extern "C" void LoadBootConfig()
{
	CGame& game = gGame;
	u64& flags  = game.mStatus.mFlags;
	flags |= 1ULL << 34;
	flags |= 1ULL << 29;
	flags |= 1ULL << 30;
	flags |= 1ULL << 32;
	flags |= 1ULL << 33;
	if (OSGetSoundMode() == 1)
		flags |= 1ULL << 36;
	else
		flags &= ~(1ULL << 36);
	flags |= 1ULL << 35;
	flags |= 1ULL << 14;
	flags |= 1ULL << 16;
	flags |= 1ULL << 38;
	flags |= 1ULL << 40;
	flags |= 1ULL << 41;
	flags |= 1ULL << 42;
	flags |= 1ULL << 47;
	flags |= 1ULL << 48;
	flags |= 1ULL << 49;
	flags |= 1ULL << 54;

	CFileText file;
	file.mPos  = 0;
	s32 result = file.Read("/qp.ini", 0, 0);
	if (result == 0) {
		char line[256];
		BOOL restricted = FALSE;
		while (file.GetLine(line) == TRUE) {
			for (char* p = line; *p; ++p)
				if (*p == ';') {
					*p = 0;
					break;
				}
			char* key = strtok(line, " \t,=");
			if (!key)
				continue;
			if (strcmp(key, "&") == 0)
				restricted = TRUE;
			char* text = strtok(NULL, " \t,=");
			if (!text)
				continue;
			for (s32 i = 0; gBootConfigEntries[i].name; ++i) {
				const BootConfigEntry* entry = &gBootConfigEntries[i];
				if (strcmp(key, entry->name) != 0)
					continue;
				switch (entry->kind) {
				case BootFlag:
					if (restricted)
						break;
				case BootUnrestrictedFlag:
					if (atoi(text))
						flags |= entry->value;
					else
						flags &= ~entry->value;
					break;
				case BootStage:
					if (!restricted) {
						s32 stage = gCmdLineArgs.mStage >= 0 ? gCmdLineArgs.mStage : atoi(text);
						if (stage >= 100)
							stage += GetStageNum() - 99;
						gCmdLineArgs.mStage = stage;
						gGame.SetStage(stage);
						GetStageFileName(stage);
					}
					break;
				case BootCrosshair: {
					flags |= entry->value;
					s32 x = atoi(text);
					text  = strtok(NULL, " \t,=");
					if (text) {
						gCmdLineArgs.mCrosshairX = x;
						gCmdLineArgs.mCrosshairY = atoi(text);
						atoi(text);
					}
					break;
				}
				// restricted like the settings, but nothing is left of it in this build
				case BootEventLog:
					if (restricted)
						break;
					break;
				case BootSetting:
					if (restricted)
						break;
					switch (entry->value) {
					case 0:
						gCmdLineArgs.mAttachmentMenuType = atoi(text);
						break;
					case 1:
						gCmdLineArgs.mCopterLevelOverride = atoi(text) + 1;
						break;
					case 2:
						gCmdLineArgs.mPopperLevelOverride = atoi(text) + 1;
						break;
					case 3:
						gCmdLineArgs.mRadarLevelOverride = atoi(text) + 1;
						break;
					case 4:
						gCmdLineArgs.mLiftPlugFlags = atoi(text);
						break;
					case 5:
						gCmdLineArgs.mAttachmentActionType = atoi(text);
						break;
					case 6:
						gCmdLineArgs.mToolActionType = atoi(text);
						break;
					case 7:
						gCmdLineArgs.mTitleCursor = atoi(text);
						break;
					}
					atoi(text);
					break;
				}
			}
		}
		file.Close();
	} else if (result == -3) {
		s32 stage           = gCmdLineArgs.mStage >= 0 ? gCmdLineArgs.mStage : GetDefaultStageIndex();
		gCmdLineArgs.mStage = stage;
		gGame.SetStage(stage);
	}
}
