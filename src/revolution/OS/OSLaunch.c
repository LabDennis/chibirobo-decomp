#include "revolution/hw_regs.h"
#include "revolution/os/OSLaunch.h"
#include "revolution/os/OSAlloc.h"
#include "revolution/os_state_flags.h"

#include "Dolphin/dvd.h"
#include "Dolphin/os.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define SYSTEM_MENU_TITLE_ID 0x0000000100000002ULL

// other files of the library
extern BOOL __OSInNandBoot;
u8 OSGetAppType(void);
s8 SCGetProductGameRegion(void);
u32 __DVDGetCoverStatus(void);
void __DVDResetWithNoSpinup(void);
u8 __OSGetDiscState(u8 last);
void __OSShutdownDevices(u32 event);
void __OSUnRegisterStateEvent(void);
void __OSLaunchMenu(void);
void __OSReturnToMenuForError(void);
void __OSHotResetForError(void);

// set by nothing in this game; when it is, the launching title is reported as the system menu
BOOL __OSLaunchAsSystemMenu;

static BOOL PackLaunchArgs(void* addr, s32 argc, char** argv)
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

// The title must belong to the company of the running one (the group of its data directory).
static BOOL CheckCompanyCode(u64 titleId, BOOL disc)
{
	char home[65];
	char path[65];
	NANDStatus status;
	u16 company;
	u16 other;

	if (NANDGetHomeDir(home) != NAND_RESULT_OK) {
		return FALSE;
	}

	if (NANDGetStatus(home, &status) != NAND_RESULT_OK) {
		return FALSE;
	}

	company = status.groupId;
	if (company == 2 && OSGetAppType() == 0x80 && (OSGetConsoleType() & 0xF0000000) == 0x10000000) {
		company = *(u16*)DVDGetCurrentDiskID()->company;
	}

	if (disc) {
		other = *(u16*)DVDGetCurrentDiskID()->company;
	} else {
		sprintf(path, "/title/%08x/%08x/data", (u32)((titleId >> 32) & 0xFFFFFFFF), (u32)(titleId & 0xFFFFFFFF));
		if (NANDGetStatus(path, &status) != NAND_RESULT_OK) {
			return FALSE;
		}
		other = status.groupId;
	}

	return company == other;
}

// The IOS the title asks for must have a ticket and its contents on the NAND. Written as in Petari's
// OSLaunch.c: with every local initialised the function is too big for -inline auto, and retail calls it.
static BOOL CheckTmdSysVersion(ESTmdView* tmd)
{
	ESSysVersion sysVersion = 0x0000000100000003ULL;
	char path[65] ATTRIBUTE_ALIGN(32);
	s32 err        = 0;
	u32 numTickets = 0;
	s32 blocks     = 0;
	s32 inodes     = 0;

	sysVersion = tmd->head.sysVersion;
	err        = ESP_GetTicketViews(sysVersion, NULL, &numTickets);
	if (err != 0 || numTickets != 1) {
		return FALSE;
	}

	sprintf(path, "/title/%08x/%08x/content", (u32)((sysVersion >> 32) & 0xFFFFFFFF), (u32)(sysVersion & 0xFFFFFFFF));
	err = NANDSecretGetUsage(path, &blocks, &inodes);
	if (err != 0 || (u32)inodes <= 2) {
		return FALSE;
	}

	return TRUE;
}

static inline BOOL CheckTmdCountryCode(ESTmdView* tmd)
{
	u32 area = *(u32*)&tmd->head.reserved[0];

	if (area == 3) {
		return TRUE;
	}

	if (area == SCGetProductGameRegion()) {
		return TRUE;
	}

	return FALSE;
}

static inline BOOL IsNandTitle(ESTmdView* tmd)
{
	if (tmd->head.reserved[0x18] & 1) {
		return TRUE;
	}

	return FALSE;
}

// Of several tickets the one without a limit and with the most rights wins, else the one with the
// most play time left.
static s32 GetValidTicketIndex(ESTicketView* tickets, u32 numTickets)
{
	u32 i;
	s32 index;
	BOOL unlimited;
	u32 maxRights;
	u32 maxRemaining;
	u32 j;
	u32 rights;
	u16 mask;
	s32 err;
	u32 remaining;
	s32 type;

	index        = 0;
	unlimited    = FALSE;
	maxRights    = 0;
	maxRemaining = 0;

	if (tickets == NULL) {
		OSReport("NULL pointer detected: line %d in %s\n", 407, __FILE__);
		return ES_ERR_INVALID;
	}

	for (i = 0; i < numTickets; i++) {
		err = __OSGetPlayTime(&tickets[i], &type, &remaining);
		if (err > 0) {
			continue;
		} else {
			switch (err) {
			case 0:
				break;
			default:
				return err;
			}
		}

		switch (type) {
		case 0:
			rights = 0;
			if (!unlimited) {
				index     = i;
				unlimited = TRUE;
			}

			mask = tickets[i].sysAccessMask[0] | (tickets[i].sysAccessMask[1] << 8);
			for (j = 0; j < 16; j++) {
				if (mask & (1 << j)) {
					rights++;
				}
			}

			if (rights > maxRights) {
				index     = i;
				maxRights = rights;
			}
			break;
		case 1:
			if (!unlimited && remaining > maxRemaining) {
				index        = i;
				maxRemaining = remaining;
			}
			break;
		case 9:
			break;
		default:
			break;
		}
	}

	if (!unlimited && maxRemaining == 0) {
		return -1;
	}

	return index;
}

void __OSRelaunchTitle(u32 resetCode)
{
	u64 titleId ATTRIBUTE_ALIGN(32);
	OSStateFlags state;
	u32 numTickets;
	s32 type;
	u32 remaining;
	ESTicketView* ticket;
	ESTicketView* tickets;
	OSNandbootInfo* info;
	void* bootInfo2;
	s32 err;

	numTickets = 1;

	OSSetArenaLo((void*)0x81280000);
	OSSetArenaHi((void*)0x812F0000);

	if (ESP_InitLib() != 0) {
		__OSReturnToMenuForError();
	}

	if (ESP_GetTitleId(&titleId) != 0) {
		__OSReturnToMenuForError();
	}

	ticket = OSAllocFromMEM1ArenaLo(0xE0, 32);
	if (ticket == NULL) {
		__OSReturnToMenuForError();
	}

	memset(ticket, 0, 0xE0);
	err = ESP_DiGetTicketView(NULL, ticket);
	if (err == ES_ERR_INVALID) {
		if (ESP_GetTicketViews(titleId, NULL, &numTickets) != 0) {
			__OSReturnToMenuForError();
		}

		tickets = OSAllocFromMEM1ArenaLo(OSRoundUp32B(numTickets * sizeof(ESTicketView)), 32);
		if (tickets == NULL) {
			__OSReturnToMenuForError();
		}

		if (ESP_GetTicketViews(titleId, tickets, &numTickets) != 0) {
			__OSReturnToMenuForError();
		}

		memcpy(ticket, tickets, sizeof(ESTicketView));
	} else if (err != 0) {
		__OSReturnToMenuForError();
	} else if (OSPlayTimeIsLimited()) {
		type      = 0;
		remaining = -1;
		__OSGetPlayTime(ticket, &type, &remaining);
		if (remaining == 0) {
			__OSWriteExpiredFlag();
			__OSReturnToMenuForError();
		}
	}

	bootInfo2 = OSAllocFromMEM1ArenaLo(0x2000, 64);
	info      = (OSNandbootInfo*)((u8*)bootInfo2 + 0x2000 - sizeof(OSNandbootInfo));
	memset(bootInfo2, 0, 0x2000);
	info->titleId    = titleId;
	info->appType    = OSGetAppType();
	info->launchType = 1;
	info->launchCode = resetCode | 0x80000000;
	__OSCreateNandbootInfo();
	__OSWriteNandbootInfo(info);

	__OSReadStateFlags(&state);
	state.data.fields.menuState = 3;
	__OSWriteStateFlags(&state);

	if (ESP_LaunchTitle(titleId, ticket) != 0) {
		__OSReturnToMenuForError();
	}

	while (TRUE) { }
}

static void LaunchTitle(u64 titleId)
{
	u32 numTickets;
	ESTicketView* ticket;
	ESTicketView* tickets;
	s32 index;

	numTickets = 1;

	if (ESP_InitLib() != 0) {
		return;
	}

	ticket = OSAllocFromMEM1ArenaLo(0xE0, 32);
	if (!ticket) {
		return;
	}

	memset(ticket, 0, 0xE0);
	if (ESP_GetTicketViews(titleId, NULL, &numTickets) != 0) {
		return;
	}

	tickets = OSAllocFromMEM1ArenaLo(OSRoundUp32B(numTickets * sizeof(ESTicketView)), 32);
	if (!tickets) {
		return;
	}

	if (ESP_GetTicketViews(titleId, tickets, &numTickets) != 0) {
		return;
	}

	index = GetValidTicketIndex(tickets, numTickets);
	if (index < 0) {
		return;
	}

	memcpy(ticket, &tickets[index], sizeof(ESTicketView));
	if (ESP_LaunchTitle(titleId, ticket) != 0) {
		return;
	}

	while (TRUE) { }
}

static void __OSLaunchTitle(u64 titleId, u32 launchCode, char** argv, BOOL check)
{
	u64 callerId ATTRIBUTE_ALIGN(32);
	u32 tmdSize ATTRIBUTE_ALIGN(32);
	OSStateFlags state;
	char titleName[20];
	ESTmdView* tmd;
	OSNandbootInfo* info;
	void* bootInfo2;
	char** list;
	s32 argc;
	s32 i;
	s32 err;

	tmd     = NULL;
	tmdSize = 0;

	if (ESP_InitLib() != 0) {
		__OSReturnToMenuForError();
	}

	if (ESP_GetTmdView(titleId, NULL, &tmdSize) != 0) {
		__OSReturnToMenuForError();
	}

	tmd = OSAllocFromMEM1ArenaLo(OSRoundUp32B(tmdSize), 64);
	if (tmd == NULL) {
		__OSReturnToMenuForError();
	}

	if (ESP_GetTmdView(titleId, tmd, &tmdSize) != 0) {
		__OSReturnToMenuForError();
	}

	if (!CheckTmdSysVersion(tmd)) {
		OSReport("OSLaunchTitle(): Firmware is not installed\n");
		__OSReturnToMenuForError();
	}

	if (check) {
		if (!CheckCompanyCode(titleId, FALSE)) {
			OSReport("OSLaunchTitle(): Company code is not correct\n");
			__OSReturnToMenuForError();
		}

		if (!CheckTmdCountryCode(tmd)) {
			OSReport("OSLaunchTitle(): Country code is not correct\n");
			__OSReturnToMenuForError();
		}
	}

	if (!IsNandTitle(tmd) && __DVDGetCoverStatus() == 2) {
		__DVDResetWithNoSpinup();
	}

	if (argv || launchCode) {
		bootInfo2 = OSAllocFromMEM1ArenaLo(0x2000, 64);
		info      = (OSNandbootInfo*)((u8*)bootInfo2 + 0x2000 - sizeof(OSNandbootInfo));
		sprintf(titleName, "%016llx", titleId);

		argc = 0;
		if (argv) {
			while (argv[argc]) {
				argc++;
			}
		}

		argc++;
		list    = OSAllocFromMEM1ArenaLo((argc + 1) * sizeof(char*), 1);
		list[0] = titleName;
		for (i = 1; i < argc; i++) {
			list[i] = argv[i - 1];
		}

		PackLaunchArgs(bootInfo2, argc, list);

		if (__OSInIPL || __OSLaunchAsSystemMenu) {
			callerId = SYSTEM_MENU_TITLE_ID;
		} else {
			err = ESP_InitLib();
			if (err != 0) {
				OSReport("\nOSExec(): Failed to exec %d in %d\n", err, 803);
				__OSHotResetForError();
			}

			err = ESP_GetTitleId(&callerId);
			if (err != 0) {
				OSReport("\nOSExec(): Failed to exec %d in %d\n", err, 811);
				__OSHotResetForError();
			}
		}

		info->titleId    = callerId;
		info->argsOffset = *(u32*)((u8*)bootInfo2 + 8);
		info->appType    = OSGetAppType();
		if (titleId == SYSTEM_MENU_TITLE_ID) {
			info->launchType = 4;
		} else {
			info->launchType = 2;
		}
		info->launchCode = launchCode;
		__OSCreateNandbootInfo();
		__OSWriteNandbootInfo(info);

		__OSReadStateFlags(&state);
		state.data.fields.menuState = 3;
		if (titleId == SYSTEM_MENU_TITLE_ID) {
			state.data.fields.discState = __OSGetDiscState(state.data.fields.discState);
			state.data.fields.unknown07 = 2;
		}
		__OSWriteStateFlags(&state);
	}

	OSDisableScheduler();
	__OSShutdownDevices(6);
	OSEnableScheduler();
	LaunchTitle(titleId);
	__OSLaunchMenu();
}

// not in the dol (only the system menu calls it): only its message survives (0x8031D2A0). Body as in wii-ipl's
// OSExec.c, where it is linked.
void __OSLaunchTitlevForSystem(u64 titleId, u32 launchCode, char** argv)
{
	__OSUnRegisterStateEvent();
	OSSetArenaLo((void*)0x81280000);
	OSSetArenaHi((void*)0x812F0000);

	if (__OSInIPL && !__OSInNandBoot) {
		__OSLaunchTitle(titleId, launchCode, argv, FALSE);
	} else {
		OSReport("__OSLaunchTitlevForSystem(): only system can call this function\n");
		__OSReturnToMenuForError();
	}
}

// not in the dol (only the system menu calls it): only its message survives (0x8031D2E4). Body as in wii-ipl's
// OSExec.c, where it is linked.
void __OSLaunchTitlelForSystem(u64 titleId, u32 launchCode, const char* arg, ...)
{
	va_list ap;
	char** argv;
	s32 i;

	__OSUnRegisterStateEvent();
	OSSetArenaLo((void*)0x81280000);
	OSSetArenaHi((void*)0x812F0000);

	argv = OSAllocFromMEM1ArenaLo(4, 0x1000);
	va_start(ap, arg);
	i = 0;
	while ((argv[i++] = (char*)arg) != NULL) {
		arg = va_arg(ap, const char*);
	}

	if (__OSInIPL && !__OSInNandBoot) {
		__OSLaunchTitle(titleId, launchCode, argv, FALSE);
	} else {
		OSReport("__OSLaunchTitlelForSystem(): only system can call this function\n");
		__OSReturnToMenuForError();
	}
}

void __OSReturnToMenuForSetting(u32 launchCode, const char* arg, ...)
{
	va_list ap;
	char** argv;
	s32 i;

	__OSUnRegisterStateEvent();
	OSSetArenaLo((void*)0x81280000);
	OSSetArenaHi((void*)0x812F0000);

	argv = OSAllocFromMEM1ArenaLo(4, 0x1000);
	va_start(ap, arg);
	i = 0;
	while ((argv[i++] = (char*)arg) != NULL) {
		arg = va_arg(ap, const char*);
	}

	__OSLaunchTitle(SYSTEM_MENU_TITLE_ID, launchCode, argv, FALSE);
	__OSReturnToMenuForError();
}

// not in the dol and in none of the other Wii decompilations: its message (0x8031D328) and the page it opens
// (0x8031D374) survive. That the message is for disc applications (application type 0x80) and that the page goes
// to the system menu as a launch argument is a reading of the two texts.
void OSLaunchManualViewer(void)
{
	if (OSGetAppType() == 0x80) {
		OSReport("OSLaunchManualViewer(): You can't call this API from DVD application.  \n");
		return;
	}

	__OSReturnToMenuForSetting(0, "/startup?initpage=showHelp", NULL);
}

// not in the dol and in none of the other Wii decompilations: only its five messages survive (0x8031D390-0x8031D4A4).
// The order of the areas is that of the texts, the two values of the last one are named by it. Which title it
// started and how is not known and not written.
void OSLaunchPDChannel(u32 titleId, u32 launchCode)
{
	if (OSGetAppType() == 0x80) {
		OSReport("OSLaunchPDChannel(): You can't call this API from DVD application.\n");
		return;
	}

	switch (SCGetProductGameRegion()) {
	case 0:
		OSReport("OSLaunchPDChannel(): product area is 'JPN'\n");
		break;
	case 1:
		OSReport("OSLaunchPDChannel(): product area is 'USA'\n");
		break;
	case 2:
		OSReport("OSLaunchPDChannel(): product area is 'EUR'\n");
		break;
	}

	OSReport("OSLaunchPDChannel(): Launch PDChannel, TitleID=0x%08x LaunchCode=0x%08x.\n", titleId, launchCode);
}
