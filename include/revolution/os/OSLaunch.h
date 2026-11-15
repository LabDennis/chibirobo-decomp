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

// The ES types as IOS's own header declares them (after pik2wii's RevoSDK/esp.h). That header packs
// them to 4 bytes, which is part of the format: the 64-bit ids sit at offset 4 (retail reads
// ticketId at +0x04 and sysVersion at +0x04/+0x08), where a u64 cannot stand without it.
typedef u32 ESId;
typedef u32 ESContentId;
typedef u64 ESTitleId;
typedef u64 ESTicketId;
typedef u8 ESVersion;
typedef u16 ESTitleVersion;
typedef ESTitleId ESSysVersion;
typedef u32 ESTitleType;
typedef u16 ESContentType;
typedef u8 ESTmdReserved[62];
typedef u8 ESTicketReserved[48];
typedef u8 ESSysAccessMask[2];
typedef u8 ESCidxMask[64];

#pragma pack(push, 4)

typedef struct ESTmdViewHeader {
	ESVersion version;           // _00
	ESSysVersion sysVersion;     // _04
	ESTitleId titleId;           // _0C
	ESTitleType type;            // _14
	u16 groupId;                 // _18
	ESTmdReserved reserved;      // _1A, starts with the product area (u32), byte 0x18 holds the access flags
	ESTitleVersion titleVersion; // _58
	u16 numContents;             // _5A
} ESTmdViewHeader;

typedef struct ESCmdView {
	ESContentId cid;    // _00
	u16 index;          // _04
	ESContentType type; // _06
	u64 size;           // _08
} ESCmdView;

typedef struct ESLpEntry {
	u32 code;  // _00, 0 none, 1 play time in seconds, 4 number of launches
	u32 limit; // _04
} ESLpEntry;

typedef struct ESTicketView {
	ESVersion version;             // _00
	ESTicketId ticketId;           // _04
	ESId deviceId;                 // _0C
	ESTitleId titleId;             // _10
	ESSysAccessMask sysAccessMask; // _18
	u16 ticketVersion;             // _1A
	u32 accessTitleId;             // _1C
	u32 accessTitleMask;           // _20
	u8 licenseType;                // _24
	ESTicketReserved reserved;     // _25
	u8 audit;                      // _55
	ESCidxMask cidxMask;           // _56
	ESLpEntry limits[8];           // _98
} ESTicketView;

typedef struct ESTmdView {
	ESTmdViewHeader head;    // _00
	ESCmdView contents[512]; // _5C
} ESTmdView;

#pragma pack(pop)

#ifndef offsetof
#define offsetof(type, member) ((u32) & (((type*)0)->member))
#endif

STATIC_ASSERT(offsetof(ESTmdViewHeader, sysVersion) == 0x04);
STATIC_ASSERT(offsetof(ESTmdViewHeader, reserved) == 0x1A);
STATIC_ASSERT(sizeof(ESTmdViewHeader) == 0x5C);
STATIC_ASSERT(sizeof(ESCmdView) == 0x10);
STATIC_ASSERT(offsetof(ESTicketView, ticketId) == 0x04);
STATIC_ASSERT(offsetof(ESTicketView, titleId) == 0x10);
STATIC_ASSERT(offsetof(ESTicketView, sysAccessMask) == 0x18);
STATIC_ASSERT(offsetof(ESTicketView, limits) == 0x98);
STATIC_ASSERT(sizeof(ESTicketView) == 0xD8);
STATIC_ASSERT(offsetof(ESTmdView, contents) == 0x5C);

s32 ESP_InitLib(void);
s32 ESP_CloseLib(void);
s32 ESP_GetTitleId(u64* titleId);
s32 ESP_GetTicketViews(u64 titleId, ESTicketView* views, u32* numViews);
s32 ESP_LaunchTitle(u64 titleId, ESTicketView* view);
s32 ESP_DiGetTicketView(void* ticket, ESTicketView* view);
s32 ESP_GetTmdView(u64 titleId, ESTmdView* view, u32* size);
s32 ESP_GetConsumption(u64 ticketId, ESLpEntry* limits, u32* numLimits);

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
