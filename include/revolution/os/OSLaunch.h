#ifndef _REVOLUTION_OS_OSLAUNCH_H
#define _REVOLUTION_OS_OSLAUNCH_H

#include "types.h"
#include "revolution/nand.h"

#ifdef __cplusplus
extern "C" {
#endif // ifdef __cplusplus

// What OSNandbootInfo.c, OSPlayTime.c and OSLaunch.c share: the ES (ticket) types and calls they use,
// the private NAND calls and the boot information handed to the next title.

#define ES_ERR_INVALID -1017

// IOS lays these out with 4-byte alignment: the 64-bit ids are not padded to 8.
#pragma pack(push, 4)

typedef struct ESLimit {
	u32 code;  // _00, 0 none, 1 play time in seconds, 4 number of launches
	u32 limit; // _04
} ESLimit;

typedef struct ESTicketView {
	u32 version;         // _00
	u64 ticketId;        // _04
	u32 deviceId;        // _0C
	u64 titleId;         // _10
	u8 sysAccessMask[2]; // _18
	u16 ticketVersion;   // _1A
	u32 accessTitleId;   // _1C
	u32 accessTitleMask; // _20
	u8 licenseType;      // _24
	u8 keyId;            // _25
	u8 reserved[0x72];   // _26
	ESLimit limits[8];   // _98
} ESTicketView;

typedef struct ESTmdView {
	u8 version;          // _00
	u8 caCrlVersion;     // _01
	u8 signerCrlVersion; // _02
	u8 padding;          // _03
	u64 sysVersion;      // _04
	u64 titleId;         // _0C
	u32 titleType;       // _14
	u16 groupId;         // _18
	u8 reserved[0x3E];   // _1A, starts with the product area (u32), byte 0x18 holds the access flags
} ESTmdView;

#pragma pack(pop)

s32 ESP_InitLib(void);
s32 ESP_CloseLib(void);
s32 ESP_GetTitleId(u64* titleId);
s32 ESP_GetTicketViews(u64 titleId, ESTicketView* views, u32* numViews);
s32 ESP_LaunchTitle(u64 titleId, ESTicketView* view);
s32 ESP_DiGetTicketView(void* ticket, ESTicketView* view);
s32 ESP_GetTmdView(u64 titleId, ESTmdView* view, u32* size);
s32 ESP_GetConsumption(u64 ticketId, ESLimit* limits, u32* numLimits);

typedef struct NANDStatus {
	u32 ownerId;   // _00
	u16 groupId;   // _04
	u8 attribute;  // _06
	u8 permission; // _07
} NANDStatus;

s32 NANDGetStatus(const char* path, NANDStatus* status);
s32 NANDPrivateCreate(const char* path, u8 permission, u8 attribute);
s32 NANDPrivateDelete(const char* path);
s32 NANDPrivateOpen(const char* path, NANDFileInfo* file, u8 access);
s32 NANDPrivateGetStatus(const char* path, NANDStatus* status);

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
