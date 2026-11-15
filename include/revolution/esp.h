#ifndef REVOLUTION_ESP_H
#define REVOLUTION_ESP_H
#include "types.h"

BEGIN_SCOPE_EXTERN_C

#define ES_ERR_OK      0
#define ES_ERR_INVALID -1017

typedef u64 ESTitleId;

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
s32 ESP_GetTitleId(ESTitleId* titleId);
s32 ESP_GetDataDir(ESTitleId titleId, char* dataDir);
s32 ESP_GetTicketViews(u64 titleId, ESTicketView* views, u32* numViews);
s32 ESP_LaunchTitle(u64 titleId, ESTicketView* view);
s32 ESP_DiGetTicketView(void* ticket, ESTicketView* view);
s32 ESP_GetTmdView(u64 titleId, ESTmdView* view, u32* size);
s32 ESP_GetConsumption(u64 ticketId, ESLimit* limits, u32* numLimits);

END_SCOPE_EXTERN_C

#endif
