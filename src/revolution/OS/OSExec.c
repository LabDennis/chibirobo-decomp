#include "revolution/hw_regs.h"
#include "revolution/os/OSAlloc.h"
#include "revolution/os/OSExec.h"
#include "revolution/os/OSReset.h"

#include "Dolphin/os.h"
#include "Dolphin/dvd.h"
#include "stl/stdio.h"
#include "stl/string.h"
#include "stl/wchar.h"

typedef struct {
	char date[16];
	u32 entry;
	u32 size;
	u32 rebootSize;
	u32 reserved2;
} AppLoaderStruct;

typedef void (*appInitCallback)(void (*report)(const char*, ...));
typedef BOOL (*appGetNextCallback)(void** addr, u32* length, u32* offset);
typedef void* (*appGetEntryCallback)(void);
typedef void (*AppLoaderCallback)(appInitCallback* init, appGetNextCallback* getNext, appGetEntryCallback* getEntry);

// one entry of the disc's partition table
typedef struct {
	u32 offset;
	u32 type;
} PartitionEntry;

// the disc's table of partition tables, at 0x40000
typedef struct {
	u32 count;
	u32 offset;
} PartitionTable;

// four bytes every ReadDisc puts on its stack and never reads
typedef struct {
	u8 r, g, b, a;
} ReadDiscColor;

// other files of the library
extern u32 __DVDLayoutFormat;
void __DVDPrepareResetAsync(DVDCBCallback callback);
void DVDLowInit(void);
BOOL DVDLowFinish(void);
BOOL DVDLowReadDiskID(DVDDiskID* id, DVDLowCallback callback);
BOOL DVDLowOpenPartition(u32 offset, void* ticket, u32 numCerts, void* certs, void* tmd, DVDLowCallback callback);
BOOL DVDLowOpenPartitionWithTmdAndTicketView(u32 offset, void* ticketView, u32 tmdSize, void* tmd, u32 numCerts, void* certs,
                                             DVDLowCallback callback);
BOOL DVDLowClosePartition(DVDLowCallback callback);
BOOL DVDLowUnencryptedRead(void* addr, u32 length, u32 offset, DVDLowCallback callback);
s32 ESP_InitLib(void);
s32 ESP_CloseLib(void);
s32 ESP_GetTitleId(u64* titleId);
s32 ESP_GetTicketViews(u64 titleId, void* views, u32* numViews);
s32 ESP_LaunchTitle(u64 titleId, void* view);
s32 ESP_DiGetTicketView(void* ticket, void* view);
s32 ESP_DiGetTmd(void* tmd, u32* size);
s32 __OSGetPlayTime(void* ticket, u32* playTime, s32* remaining);
BOOL OSPlayTimeIsLimited(void);
void __OSWriteExpiredFlag(void);
void __OSReturnToMenuForError(void);
void __OSHotResetForError(void);
void __OSRestoreMEM1Config(u32 key);
void __OSInitMemoryProtection(void);
void __OSInitIPCBuffer(void);
void IPCReInit(void);
void IPCCltReInit(void);
void OSGetSaveRegion(void** start, void** end);

OSExecParams* __OSExecParams AT_ADDRESS(0x800030F0);

static DVDDiskID DiskID ATTRIBUTE_ALIGN(32);

static volatile BOOL Prepared;
BOOL __OSInReboot;
static volatile u32 LowIntType;
// Partition type handed to the next program. Descriptive name, not from the SDK.
// Explicitly initialised: retail has it at .sbss+0, ahead of the function static apploaderPosition (+4).
// Without the initialiser it comes out behind it.
u32 __OSNextPartitionType = 0;

static BOOL PackArgs(void* addr, s32 argc, char** argv)
{
	s32 numArgs;
	char* bootInfo2;
	char* ptr;
	char** list;
	u32 i;

	bootInfo2 = (char*)addr;
	memset(bootInfo2, 0, 0x2000);

	if (argc == 0) {
		*(u32*)(bootInfo2 + 8) = 0;
	} else {
		numArgs = argc;
		ptr     = bootInfo2 + 0x2000;
		while (--argc >= 0) {
			ptr -= strlen(argv[argc]) + 1;
			strcpy(ptr, argv[argc]);
			argv[argc] = (char*)(ptr - bootInfo2);
		}

		ptr = bootInfo2 + ((ptr - bootInfo2) & ~0x3);
		ptr -= ((numArgs + 1) * 4);
		list = (char**)ptr;

		for (i = 0; i < numArgs + 1; i++) {
			list[i] = argv[i];
		}

		ptr -= 4;
		*(u32*)ptr = numArgs;

		*(u32*)(bootInfo2 + 8) = (ptr - bootInfo2);
	}

	return TRUE;
}

// writes the bytes of a 16-bit string as lower-case hex digits
static BOOL EncodeHex(char* dst, const char* src)
{
	char* ptr;
	u32 i;
	u8 shift;
	s32 mask;
	s32 digit;

	if (src != NULL) {
		ptr = dst;
		while (src[0] != 0 || src[1] != 0) {
			for (i = 0; i < 4; i++) {
				mask  = (i % 2) ? 0x0F : 0xF0;
				shift = (i % 2) ? 0 : 4;
				digit = (*src & mask) >> shift;

				if (digit >= 0 && digit < 10) {
					*ptr = digit + '0';
				} else if (digit >= 10 && digit < 16) {
					*ptr = digit + 'a' - 10;
				} else {
					return FALSE;
				}

				ptr++;
				if (i % 2) {
					src++;
				}
			}
		}

		*ptr = '\0';
		return TRUE;
	}

	return FALSE;
}

// PackArgs for partition type 2: every second argument is a 16-bit string and goes in as hex
static BOOL PackArgsEncoded(void* addr, s32 argc, char** argv)
{
	s32 numArgs;
	char* bootInfo2;
	char* ptr;
	char** list;
	char* wstr;
	u32 i;

	bootInfo2 = (char*)addr;
	memset(bootInfo2, 0, 0x2000);

	if (argc == 0) {
		*(u32*)(bootInfo2 + 8) = 0;
	} else {
		numArgs = argc;
		ptr     = bootInfo2 + 0x2000;
		while (--argc >= 0) {
			if (argc < 2 || argc % 2 != 0) {
				ptr -= strlen(argv[argc]) + 1;
				strcpy(ptr, argv[argc]);
				argv[argc] = (char*)(ptr - bootInfo2);
			} else {
				wstr = argv[argc];
				ptr -= wcslen((wchar_t*)wstr) * 4 + 1;
				EncodeHex(ptr, wstr);
				argv[argc] = (char*)(ptr - bootInfo2);
			}
		}

		ptr = bootInfo2 + ((ptr - bootInfo2) & ~0x3);
		ptr -= ((numArgs + 1) * 4);
		list = (char**)ptr;

		for (i = 0; i < numArgs + 1; i++) {
			list[i] = argv[i];
		}

		ptr -= 4;
		*(u32*)ptr = numArgs;

		*(u32*)(bootInfo2 + 8) = (ptr - bootInfo2);
	}

	return TRUE;
}

static asm void Run(register void* entryPoint)
{
	// clang-format off
	fralloc
	bl ICFlashInvalidate
	sync
	isync
	mtctr entryPoint
	bctr
	frfree
	blr
	// clang-format on
}

static void StartDol(const OSExecParams* params, void* entry)
{
	OSExecParams* paramsWork = OSAllocFromMEM1ArenaLo(sizeof(OSExecParams), 1);

	__OSSetExecParams(params, paramsWork);
	__PIRegs[9] = 7;

	OSDisableInterrupts();
	Run(entry);
}

static void ReadDisc(void* addr, s32 length, s32 offset)
{
	DVDCommandBlock block;
	ReadDiscColor color = { 0xFF, 0xFF, 0xFF, 0x00 };

	DVDReadAbsAsyncPrio(&block, addr, length, offset, NULL, 0);

	while (DVDGetCommandBlockStatus(&block)) {
		if (DVDGetCommandBlockStatus(&block) > 2 || DVDGetCommandBlockStatus(&block) < 0) {
			__OSReturnToMenuForError();
		}
	}
}

static void ExecCallback(s32 result, DVDCommandBlock* block)
{
	Prepared = TRUE;
}

void __OSGetExecParams(OSExecParams* params)
{
	if (0x80000000 <= (u32)__OSExecParams) {
		memcpy(params, __OSExecParams, sizeof(OSExecParams));
	} else {
		params->valid = FALSE;
	}
}

void __OSSetExecParams(const OSExecParams* params, OSExecParams* addr)
{
	memcpy(addr, params, sizeof(OSExecParams));
	__OSExecParams = addr;
}

static s32 GetApploaderPosition(void)
{
	static s32 apploaderPosition;

	u32* tgcHeader;
	s32 apploaderOffsetInTGC;

	if (apploaderPosition != 0) {
		return apploaderPosition;
	}

	if (*(s32*)0x800030F4 != 0) {
		tgcHeader = OSAllocFromMEM1ArenaLo(0x40, 32);
		ReadDisc(tgcHeader, 0x40, *(s32*)0x800030F4 >> 2);
		apploaderOffsetInTGC = tgcHeader[14];
		apploaderPosition    = (*(s32*)0x800030F4 + apploaderOffsetInTGC) >> 2;
	} else {
		apploaderPosition = 0x2440 >> 2;
	}
	return apploaderPosition;
}

static AppLoaderStruct* LoadApploader(void)
{
	AppLoaderStruct* header;

	header = (AppLoaderStruct*)OSAllocFromMEM1ArenaLo(sizeof(AppLoaderStruct), 32);
	ReadDisc(header, sizeof(AppLoaderStruct), GetApploaderPosition());

	ReadDisc((void*)0x81200000, OSRoundUp32B(header->size), GetApploaderPosition() + (0x20 >> 2));
	ICInvalidateRange((void*)0x81200000, OSRoundUp32B(header->size));
	return header;
}

static void* LoadDol(const OSExecParams* params, AppLoaderCallback getInterface)
{
	appInitCallback appInit;
	appGetNextCallback appGetNext;
	appGetEntryCallback appGetEntry;
	void* addr;
	u32 length;
	u32 offset;
	OSExecParams* paramsWork;

	getInterface(&appInit, &appGetNext, &appGetEntry);
	paramsWork = (OSExecParams*)OSAllocFromMEM1ArenaLo(sizeof(OSExecParams), 1);
	__OSSetExecParams(params, paramsWork);
	appInit(OSReport);
	OSSetArenaLo(paramsWork);

	while (appGetNext(&addr, &length, &offset) != 0) {
		ReadDisc(addr, length, offset >> __DVDLayoutFormat);
	}

	return appGetEntry();
}

static BOOL IsNewApploader(AppLoaderStruct* header)
{
	return strncmp(header->date, "2004/02/01", 10) > 0 ? TRUE : FALSE;
}

static void LowCallback(u32 intType)
{
	LowIntType = intType;
}

#define WAIT_LOW(line)                                                       \
	while (LowIntType == 0) { }                                              \
	if (LowIntType != 1) {                                                   \
		OSReport("\nOSExec(): Failed to exec %d in %d\n", LowIntType, line); \
		__OSReturnToMenuForError();                                          \
	}

#define CHECK_ES(cond, line)                                         \
	if (cond) {                                                      \
		OSReport("\nOSExec(): Failed to exec %d in %d\n", rc, line); \
		__OSHotResetForError();                                      \
	}

// Opens the partition of the next program, starts the IOS its title asks for and opens the partition again.
// Descriptive name, not from the SDK.
static void LaunchIOSForPartition(void)
{
	void* views;
	s32 rc;
	void* ticketView;
	PartitionEntry* entries;
	PartitionTable* tables;
	PartitionEntry* partition;
	void* tmd;
	u32 mem1End;
	u32 arenaHi;
	u8 i;
	PartitionEntry* entry;
	u64 iosTitleId;
	u32 numViews;
	u32 tmdSize;
	u32 playTime;
	s32 remaining;

	numViews = 1;
	tmdSize  = 0;
	rc       = -1;

	tables     = OSAllocFromMEM1ArenaLo(0x20, 32);
	entries    = OSAllocFromMEM1ArenaLo(0x800, 32);
	tmd        = OSAllocFromMEM1ArenaLo(0x4A00, 64);
	ticketView = OSAllocFromMEM1ArenaLo(0xE0, 32);

	if (__OSNextPartitionType == *(u32*)0x80003194 && *(u32*)0x80003198 != 0) {
		rc = ESP_InitLib();
		if (rc == 0) {
			rc = ESP_DiGetTicketView(NULL, ticketView);
		}
		if (rc == 0) {
			rc = ESP_DiGetTmd(NULL, &tmdSize);
		}
		if (rc == 0) {
			rc = ESP_DiGetTmd(tmd, &tmdSize);
		}
		ESP_CloseLib();

		if (OSPlayTimeIsLimited()) {
			playTime  = 0;
			remaining = -1;
			__OSGetPlayTime(ticketView, &playTime, &remaining);
			if (remaining == 0) {
				__OSWriteExpiredFlag();
				__OSReturnToMenuForError();
			}
		}
	}

	if (rc == 0) {
		partition         = entries;
		partition->type   = *(u32*)0x80003194;
		partition->offset = *(u32*)0x80003198;
	} else {
		LowIntType = 0;
		DVDLowClosePartition(LowCallback);
		WAIT_LOW(920);

		LowIntType = 0;
		DVDLowUnencryptedRead(tables, 0x20, 0x40000 >> 2, LowCallback);
		WAIT_LOW(930);

		LowIntType = 0;
		DVDLowUnencryptedRead(entries, 0x800, tables[0].offset, LowCallback);
		WAIT_LOW(942);

		partition = NULL;
		for (i = 0, entry = entries; i < tables[0].count; i++, entry++) {
			if (entry->type == __OSNextPartitionType) {
				partition = entry;
			}
		}

		if (!partition) {
			for (i = 0, entry = entries + 4; i < tables[1].count; i++, entry++) {
				if (entry->type == __OSNextPartitionType) {
					partition = entry;
				}
			}
		}

		if (!partition) {
			OSReport("\nOSExec(): The specified game doesn't exist in the disc\n");
			__OSReturnToMenuForError();
		}

		*(u32*)0x80003194 = partition->type;
		*(u32*)0x80003198 = partition->offset;

		LowIntType = 0;
		if (*(u8*)0x80003187 == 0x80) {
			DVDLowOpenPartitionWithTmdAndTicketView(partition->offset, ticketView, tmdSize, tmd, 0, NULL, LowCallback);
		} else {
			DVDLowOpenPartition(partition->offset, NULL, 0, NULL, tmd, LowCallback);
		}
		WAIT_LOW(1003);

		LowIntType = 0;
		DVDLowClosePartition(LowCallback);
		WAIT_LOW(1013);
	}

	iosTitleId = *(u64*)((u8*)tmd + 0x184);

	rc = ESP_InitLib();
	CHECK_ES(rc != 0, 1024);

	rc = ESP_GetTicketViews(iosTitleId, NULL, &numViews);
	CHECK_ES(numViews != 1 || rc != 0, 1033);

	views = OSAllocFromMEM1ArenaLo(OSRoundUp32B(numViews * 0xD8), 32);
	rc    = ESP_GetTicketViews(iosTitleId, views, &numViews);
	CHECK_ES(rc != 0, 1042);

	DVDLowFinish();
	arenaHi = *(u32*)0x8000311C;
	mem1End = *(u32*)0x80003120;
	DCStoreRange((void*)0x80003100, 0x100);

	rc = ESP_LaunchTitle(iosTitleId, views);
	CHECK_ES(rc != 0, 1058);
	ESP_CloseLib();

	DCInvalidateRange((void*)0x80003100, 0x100);
	if (arenaHi < *(u32*)0x8000311C) {
		*(u32*)0x80003120 = arenaHi - (*(u32*)0x8000311C - *(u32*)0x80003120);
		*(u32*)0x80003128 = arenaHi - (*(u32*)0x8000311C - *(u32*)0x80003128);
		*(u32*)0x80003130 = arenaHi - (*(u32*)0x8000311C - *(u32*)0x80003130);
		*(u32*)0x80003134 = arenaHi - (*(u32*)0x8000311C - *(u32*)0x80003134);
		*(u32*)0x8000311C = arenaHi;
	}

	if (mem1End < *(u32*)0x80003120) {
		__OSInitMemoryProtection();
	}

	__OSInitIPCBuffer();
	IPCReInit();
	IPCCltReInit();
	DVDLowInit();

	LowIntType = 0;
	DVDLowReadDiskID(&DiskID, LowCallback);
	WAIT_LOW(1102);

	LowIntType = 0;
	if (*(u8*)0x80003187 == 0x80) {
		DVDLowOpenPartitionWithTmdAndTicketView(partition->offset, ticketView, tmdSize, tmd, 0, NULL, LowCallback);
	} else {
		DVDLowOpenPartition(partition->offset, NULL, 0, NULL, tmd, LowCallback);
	}
	WAIT_LOW(1125);
}

void __OSLaunchMenu(void)
{
	u32 numViews;
	void* views;
	s32 rc;

	numViews = 1;
	OSSetArenaLo((void*)0x81280000);
	OSSetArenaHi((void*)0x812F0000);

	if (ESP_InitLib() != 0) {
		return;
	}

	rc = ESP_GetTicketViews(0x0000000100000002, NULL, &numViews);
	if (numViews != 1 || rc != 0) {
		return;
	}

	views = OSAllocFromMEM1ArenaLo(OSRoundUp32B(numViews * 0xD8), 32);
	if (ESP_GetTicketViews(0x0000000100000002, views, &numViews) != 0) {
		return;
	}
	if (ESP_LaunchTitle(0x0000000100000002, views) != 0) {
		return;
	}

	while (TRUE) { }
}

void __OSBootDolSimple(u32 doloffset, u32 restartCode, void* regionStart, void* regionEnd, BOOL argsUseDefault, s32 argc, char** argv)
{
	OSExecParams* params;
	void* dolEntry;
	AppLoaderStruct* header;
	u8* lowMem;
	u64 titleId ATTRIBUTE_ALIGN(32);

	OSDisableInterrupts();
	if (__OSInReboot) {
		__OSNextPartitionType = *(u32*)0x80003194;
	}
	__OSRestoreMEM1Config(0xBA2CF);

	params                 = (OSExecParams*)OSAllocFromMEM1ArenaLo(sizeof(OSExecParams), 1);
	params->valid          = TRUE;
	params->restartCode    = restartCode;
	params->regionStart    = regionStart;
	params->regionEnd      = regionEnd;
	params->argsUseDefault = argsUseDefault;

	if (!argsUseDefault) {
		params->argsAddr = OSAllocFromMEM1ArenaLo(0x2000, 1);
		if (__OSNextPartitionType == 2 && !__OSInReboot) {
			PackArgsEncoded(params->argsAddr, argc, argv);
		} else {
			PackArgs(params->argsAddr, argc, argv);
		}
	}

	DVDInit();
	DVDSetAutoInvalidation(TRUE);
	DVDResume();

	Prepared = FALSE;

	__DVDPrepareResetAsync(ExecCallback);
	__OSMaskInterrupts(0xFFFFFFF0);
	__OSUnmaskInterrupts(0x10);
	OSEnableInterrupts();

	while (Prepared != TRUE) { }

	LaunchIOSForPartition();

	if (restartCode == 0xA0000000 && !__OSInReboot) {
		if (ESP_InitLib() != 0) {
			return;
		}
		if (ESP_GetTitleId(&titleId) != 0) {
			return;
		}
		if (ESP_CloseLib() != 0) {
			return;
		}
		snprintf((char*)params->argsAddr + (u32)argv[1], 17, "%016llx", titleId);
	}

	header = LoadApploader();
	if (IsNewApploader(header)) {
		if (doloffset == 0xFFFFFFFF) {
			doloffset = GetApploaderPosition() + ((header->size + 0x20) >> 2);
		}

		params->bootDol = doloffset;
		dolEntry        = LoadDol(params, (AppLoaderCallback)header->entry);

		// the game code of the disc becomes the code of the running program
		lowMem                   = (u8*)0x80000000;
		*(u32*)(lowMem + 0x3180) = *(u32*)lowMem;
		lowMem[0x3184]           = 0x80;
		StartDol(params, dolEntry);
	} else {
		*(u32*)0x812FDFF0 = (u32)regionStart;
		*(u32*)0x812FDFEC = (u32)regionEnd;
		*(u8*)0x800030E2  = 1;

		ReadDisc((void*)0x81330000, OSRoundUp32B(header->rebootSize), GetApploaderPosition() + ((header->size + 0x20) >> 2));
		ICInvalidateRange((void*)0x81330000, OSRoundUp32B(header->rebootSize));
		OSDisableInterrupts();
		ICFlashInvalidate();
		Run((void*)0x81330000);
	}
}

void __OSBootDol(u32 doloffset, u32 restartCode, const char** argv)
{
	char doloffInString[20];
	s32 argvlen;
	char** argvToPass;
	s32 i;
	void* saveStart;
	void* saveEnd;

	OSGetSaveRegion(&saveStart, &saveEnd);
	sprintf(doloffInString, "%d", doloffset);
	argvlen = 0;

	if (argv != 0) {
		while (argv[argvlen] != 0) {
			argvlen++;
		}
	}

	argvlen++;
	argvToPass  = OSAllocFromMEM1ArenaLo((argvlen + 1) * 4, 1);
	*argvToPass = doloffInString;

	for (i = 1; i < argvlen; i++) {
		argvToPass[i] = (char*)argv[i - 1];
	}

	__OSBootDolSimple(-1, restartCode, saveStart, saveEnd, FALSE, argvlen, argvToPass);
}
