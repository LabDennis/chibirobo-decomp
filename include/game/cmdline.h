#ifndef _GAME_CMDLINE_H
#define _GAME_CMDLINE_H
#include "types.h"

struct CmdLineArgs {
	s32 mStage;
	s32 mSlot;
	u8 mAttachmentMenuType : 2;
	// Zero keeps the default level. Overrides store the configured level plus one.
	u8 mCopterLevelOverride : 2;
	u8 mPopperLevelOverride : 2;
	u8 mRadarLevelOverride : 2;
	u8 _09[3];
	u32 mLiftPlugFlags;
	s16 mAttachmentActionType;
	s16 mToolActionType;
	s32 mCrosshairX, mCrosshairY;
	s32 mTitleCursor;
};
STATIC_ASSERT(sizeof(CmdLineArgs) == 0x20);
extern "C" CmdLineArgs gCmdLineArgs;

enum BootConfigKind {
	BootFlag,
	BootUnrestrictedFlag,
	BootStage,
	BootEventLog,
	BootSetting,
	BootCrosshair,
};

struct BootConfigEntry {
	const char* name;
	BootConfigKind kind;
	u64 value;
};
STATIC_ASSERT(sizeof(BootConfigEntry) == 0x10);
extern "C" const char sBootConfigPath[8];
extern "C" const char sBootConfigDelimiters[5];
extern "C" const char sBootConfigRestrictionMarker[2];
extern "C" const BootConfigEntry gBootConfigEntries[];
#endif
