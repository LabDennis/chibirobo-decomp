#ifndef REVOLUTION_ESP_H
#define REVOLUTION_ESP_H
#include "types.h"

BEGIN_SCOPE_EXTERN_C

#define ES_ERR_OK      0
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

typedef u8 IOSCName[64];
typedef u8 IOSCSigDummy[60];
typedef u8 IOSCHash[20];
typedef u8 IOSCEccPublicKey[60];
typedef u8 IOSCRsaSig2048[256];
typedef u8 IOSCAesKey[16];

typedef struct IOSCSigRsa2048 {
	u32 sigType;
	IOSCRsaSig2048 sig;
	IOSCSigDummy dummy;
	IOSCName issuer;
} IOSCSigRsa2048;

// pack(4) on the ES structs is the SDK's own line: PrimeDecomp-sdk, xenoblade,
// pik1wii, pik2wii and Petari carry the same pragma.
#pragma pack(push, 4)

typedef struct ESLpEntry {
	u32 code;
	u32 limit;
} ESLpEntry;

typedef struct ESTicketView {
	ESVersion version;
	ESTicketId ticketID;
	ESId devID;
	ESTitleId titleID;
	ESSysAccessMask sysAccessMask;
	u16 ticketVer;
	u32 accTitleID;
	u32 accTitleMask;
	u8 license;
	ESTicketReserved reserved;
	u8 audit;
	ESCidxMask cidxMask;
	ESLpEntry limits[8];
} ESTicketView;

typedef struct ESContentMeta {
	ESContentId cid;
	u16 index;
	ESContentType type;
	u64 size;
	IOSCHash hash;
} ESContentMeta;

typedef struct ESTitleMetaHeader {
	ESVersion version;
	ESVersion caCrlVersion;
	ESVersion signerCrlVersion;
	ESSysVersion sysVersion;
	ESTitleId titleId;
	ESTitleType type;
	u16 groupId;
	ESTmdReserved reserved;
	u32 accessRights;
	ESTitleVersion titleVersion;
	u16 numContents;
	u16 bootIndex;
} ESTitleMetaHeader;

typedef struct ESTitleMeta {
	IOSCSigRsa2048 sig;
	ESTitleMetaHeader head;
	ESContentMeta contents[512];
} ESTitleMeta;

typedef struct ESTicket {
	IOSCSigRsa2048 sig;
	IOSCEccPublicKey serverPubKey;
	ESVersion version;
	ESVersion caCrlVersion;
	ESVersion signerCrlVersion;
	IOSCAesKey titleKey;
	ESTicketId ticketId;
	ESId deviceId;
	ESTitleId titleId;
	ESSysAccessMask sysAccessMask;
	u16 ticketVersion;
	u32 accessTitleId;
	u32 accessTitleMask;
	u8 licenseType;
	ESTicketReserved reserved;
	u8 audit;
	ESCidxMask cidxMask;
	ESLpEntry limits[8];
} ESTicket;

typedef struct ESTmdViewHeader {
	ESVersion version;
	ESSysVersion sysVersion;
	ESTitleId titleId;
	ESTitleType type;
	u16 groupId;
	ESTmdReserved reserved;
	ESTitleVersion titleVersion;
	u16 numContents;
} ESTmdViewHeader;

typedef struct ESCmdView {
	ESContentId cid;
	u16 index;
	ESContentType type;
	u64 size;
} ESCmdView;

typedef struct ESTmdView {
	ESTmdViewHeader head;
	ESCmdView contents[512];
} ESTmdView;

#pragma pack(pop)

#ifndef offsetof
#define offsetof(type, member) ((u32) & (((type*)0)->member))
#endif

STATIC_ASSERT(offsetof(ESTmdViewHeader, sysVersion) == 0x04);
STATIC_ASSERT(offsetof(ESTmdViewHeader, reserved) == 0x1A);
STATIC_ASSERT(sizeof(ESTmdViewHeader) == 0x5C);
STATIC_ASSERT(sizeof(ESCmdView) == 0x10);
STATIC_ASSERT(offsetof(ESTicketView, ticketID) == 0x04);
STATIC_ASSERT(offsetof(ESTicketView, titleID) == 0x10);
STATIC_ASSERT(offsetof(ESTicketView, sysAccessMask) == 0x18);
STATIC_ASSERT(offsetof(ESTicketView, limits) == 0x98);
STATIC_ASSERT(sizeof(ESTicketView) == 0xD8);
STATIC_ASSERT(sizeof(ESTicket) == 0x2A4);
STATIC_ASSERT(sizeof(ESTitleMeta) == 0x49E4);
STATIC_ASSERT(offsetof(ESTmdView, contents) == 0x5C);

s32 ESP_InitLib(void);
s32 ESP_CloseLib(void);
s32 ESP_GetTitleId(ESTitleId* titleId);
s32 ESP_GetDataDir(ESTitleId titleId, char* dataDir);
s32 ESP_GetTicketViews(u64 titleId, ESTicketView* views, u32* numViews);
s32 ESP_LaunchTitle(u64 titleId, ESTicketView* view);
s32 ESP_DiGetTicketView(const void* ticket, ESTicketView* view);
s32 ESP_DiGetTmd(ESTitleMeta* tmd, u32* size);
s32 ESP_GetTmdView(u64 titleId, ESTmdView* view, u32* size);
s32 ESP_GetConsumption(u64 ticketId, ESLpEntry* limits, u32* numLimits);

END_SCOPE_EXTERN_C

#endif
