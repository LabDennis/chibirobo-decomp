#ifndef _REVOLUTION_OS_OSLAUNCH_H
#define _REVOLUTION_OS_OSLAUNCH_H

#include "types.h"
#include "revolution/esp.h"
#include "revolution/nand.h"

#ifdef __cplusplus
extern "C" {
#endif // ifdef __cplusplus

// What OSNandbootInfo.c, OSPlayTime.c and OSLaunch.c share: the boot information handed to the next title.

// /shared2/sys/NANDBOOTINFO
typedef struct OSNandbootInfo {
	u32 checksum;      // _00, sum of all words that follow
	u32 argsOffset;    // _04
	u8 unknown08;      // _08
	u8 unknown09;      // _09
	u8 appType;        // _0A
	u8 launchType;     // _0B, 1 relaunch, 2 launch, 4 launch of the system menu
	u32 launchCode;    // _0C
	u32 unknown10;     // _10
	u32 unknown14;     // _14
	u64 titleId;       // _18, the title that launches
	char args[0x1000]; // _20
} OSNandbootInfo;

BOOL __OSCreateNandbootInfo(void);
BOOL __OSWriteNandbootInfo(OSNandbootInfo* info);

BOOL OSPlayTimeIsLimited(void);
BOOL __OSWriteExpiredFlag(void);
BOOL __OSWriteExpiredFlagIfSet(void);
s32 __OSGetPlayTime(ESTicketView* ticket, s32* type, u32* remaining);
void __OSInitPlayTime(void);

void __OSRelaunchTitle(u32 resetCode);
void __OSReturnToMenuForSetting(u32 launchCode, const char* arg, ...);

#ifdef __cplusplus
}
#endif // ifdef __cplusplus

#endif
