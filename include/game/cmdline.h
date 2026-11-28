#ifdef VERSION_R24J01
#ifndef _GAME_CMDLINE_H
#define _GAME_CMDLINE_H
#include "types.h"

struct CmdLineArgs {
	s32 mStage;
	s32 mSlot;
	// u32 fields: the Wii's compiler reads them out of the whole word, the GameCube's a byte at a time
	u32 mAttachmentMenuType : 2;
	// Zero keeps the default level. Overrides store the configured level plus one.
	u32 mCopterLevelOverride : 2;
	u32 mPopperLevelOverride : 2;
	u32 mRadarLevelOverride : 2;
	u8 _09[3];
	u32 mLiftPlugFlags;
	s16 mAttachmentActionType;
	s16 mToolActionType;
	char mFontPath[8];
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
#endif
#else
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
#endif
#endif
