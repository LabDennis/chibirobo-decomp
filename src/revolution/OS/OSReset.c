#include "revolution/hw_regs.h"
#include "revolution/os/OSReset.h"
#include "revolution/os/OSExec.h"
#include "revolution/os_state_flags.h"

#include "Dolphin/os.h"

typedef struct OSShutdownFunctionQueue {
	OSShutdownFunctionInfo* head;
	OSShutdownFunctionInfo* tail;
} OSShutdownFunctionQueue;

typedef struct SCIdleModeInfo {
	u8 mode;
	u8 led;
} SCIdleModeInfo;

typedef struct OSIOSRev {
	u8 reserved;
	u8 major;
	u8 minor;
	u8 micro;
	u8 month;
	u8 date;
	u16 year;
} OSIOSRev;

// other files of the library
extern BOOL __OSInNandBoot;
extern BOOL __OSInReboot;
BOOL __PADDisableRecalibration(BOOL disable);
u32 __DVDGetCoverStatus(void);
void __DVDPrepareReset(void);
BOOL __OSGetRTCFlags(u32* flags);
BOOL __OSClearRTCFlags(void);
void __OSStopPlayRecord(void);
void __OSUnRegisterStateEvent(void);
void __OSInitSTM(void);
void __OSHotReset(void);
void __OSShutdownToSBY(void);
void __OSLaunchMenu(void);
void __OSRelaunchTitle(u32 resetCode);
void __OSReboot(u32 resetCode, u32 bootDol);
void __OSGetIOSRev(OSIOSRev* rev);
void __OSWriteExpiredFlagIfSet(void);
s32 __OSGetPlayTime(void* ticket, u32* playTime, s32* remaining);
BOOL OSPlayTimeIsLimited(void);
u8 OSGetAppType(void);
void* OSAllocFromMEM1ArenaLo(u32 size, u32 align);
void __VISetRGBModeImm(void);
void SCInit(void);
u32 SCCheckStatus(void);
BOOL SCGetIdleMode(SCIdleModeInfo* info);
s32 ESP_InitLib(void);
s32 ESP_DiGetTicketView(void* ticket, void* view);
// 0x8018E4E4: takes the title kind and a NULL-terminated list of arguments for the menu
void __OSReturnToMenuForSetting(u32 kind, ...);

#define ENQUEUE_INFO(info, queue)                \
	do {                                         \
		OSShutdownFunctionInfo* __prev = (queue)->tail; \
		if (__prev == 0) {                       \
			(queue)->head = (info);              \
		} else {                                 \
			__prev->next = (info);               \
		}                                        \
		(info)->prev  = __prev;                  \
		(info)->next  = 0;                       \
		(queue)->tail = (info);                  \
	} while (0);

#define ENQUEUE_INFO_PRIO(info, queue)                                                             \
	do {                                                                                           \
		OSShutdownFunctionInfo* __prev;                                                            \
		OSShutdownFunctionInfo* __next;                                                            \
		for (__next = (queue)->head; __next && (__next->priority <= (info)->priority); __next = __next->next) \
			;                                                                                      \
                                                                                                   \
		if (__next == 0) {                                                                         \
			ENQUEUE_INFO(info, queue);                                                             \
		} else {                                                                                   \
			(info)->next = __next;                                                                 \
			__prev       = __next->prev;                                                           \
			__next->prev = (info);                                                                 \
			(info)->prev = __prev;                                                                 \
			if (__prev == 0) {                                                                     \
				(queue)->head = (info);                                                            \
			} else {                                                                               \
				__prev->next = (info);                                                             \
			}                                                                                      \
		}                                                                                          \
	} while (0);

static OSShutdownFunctionQueue ShutdownFunctionQueue;
BOOL __OSIsReturnToIdle;
static u32 bootThisDol;

static void KillThreads(void);
void __OSHotResetForError(void);
void __OSReturnToMenuForError(void);

void OSRegisterShutdownFunction(OSShutdownFunctionInfo* info)
{
	ENQUEUE_INFO_PRIO(info, &ShutdownFunctionQueue);
}

int __OSCallResetFunctions(BOOL final, u32 event)
{
	OSShutdownFunctionInfo* info;
	int err;
	u32 priority;

	priority = 0;
	err      = 0;

	for (info = ShutdownFunctionQueue.head; info != 0;) {
		if (err != 0 && priority != info->priority)
			break;
		err |= !info->func(final, event);
		priority = info->priority;
		info     = info->next;
	}

	err |= !__OSSyncSram();
	return err ? FALSE : TRUE;
}

static void KillThreads(void)
{
	OSThread* thread;
	OSThread* next;

	for (thread = __OSActiveThreadQueue.head; thread; thread = next) {
		next = thread->linkActive.next;
		switch (thread->state) {
		case 1:
		case 4:
			OSCancelThread(thread);
			break;
		}
	}
}

void __OSShutdownDevices(u32 event)
{
	BOOL padIntr;
	BOOL keepEnable;

	switch (event) {
	case 0:
	case 5:
	case 6:
		keepEnable = FALSE;
		break;
	case 1:
	case 2:
	case 3:
	case 4:
	default:
		keepEnable = TRUE;
		break;
	}

	__OSStopAudioSystem();

	if (!keepEnable) {
		padIntr = __PADDisableRecalibration(TRUE);
	}

	while (!__OSCallResetFunctions(FALSE, event)) { }

	while (!__OSSyncSram()) { }

	OSDisableInterrupts();
	__OSCallResetFunctions(TRUE, event);
	LCDisable();

	if (!keepEnable) {
		__PADDisableRecalibration(padIntr);
	}

	KillThreads();
}

// What the system menu is told about the disc: 3 without a disc, 1 when the last state was 1 and
// no RTC flag is set, 2 otherwise.
u8 __OSGetDiscState(u8 last)
{
	u32 flags;

	if (__DVDGetCoverStatus() != 2) {
		return 3;
	}

	if (last == 1 && __OSGetRTCFlags(&flags) && flags == 0) {
		return 1;
	}

	return 2;
}

void OSRebootSystem(void)
{
	OSStateFlags state;

	__OSStopPlayRecord();
	__OSUnRegisterStateEvent();
	__DVDPrepareReset();
	__OSReadStateFlags(&state);
	state.data.fields.discState = __OSGetDiscState(state.data.fields.discState);
	state.data.fields.menuState = 2;
	__OSClearRTCFlags();
	__OSWriteStateFlags(&state);
	OSDisableScheduler();
	__OSShutdownDevices(1);
	__OSHotResetForError();
}

void OSShutdownSystem(void)
{
	SCIdleModeInfo idleMode;
	OSStateFlags state;
	OSIOSRev iosRev;

	memset(&idleMode, 0, sizeof(idleMode));
	SCInit();
	while (SCCheckStatus() == 1) { }
	SCGetIdleMode(&idleMode);

	__OSStopPlayRecord();
	__OSUnRegisterStateEvent();
	__DVDPrepareReset();
	__OSReadStateFlags(&state);
	state.data.fields.discState = __OSGetDiscState(state.data.fields.discState);

	if (idleMode.mode == 1) {
		state.data.fields.menuState = 5;
	} else {
		state.data.fields.menuState = 1;
	}

	__OSClearRTCFlags();
	__OSWriteStateFlags(&state);
	__OSGetIOSRev(&iosRev);

	if (idleMode.mode == 1) {
		__OSIsReturnToIdle = TRUE;
		OSDisableScheduler();
		__OSShutdownDevices(5);
		OSEnableScheduler();
		__OSLaunchMenu();
	} else {
		OSDisableScheduler();
		__OSShutdownDevices(2);
		__OSShutdownToSBY();
	}
}

void OSRestart(u32 resetCode)
{
	u8 type = OSGetAppType();

	__OSStopPlayRecord();
	__OSUnRegisterStateEvent();

	if (type == 0x81) {
		OSDisableScheduler();
		__OSShutdownDevices(4);
		OSEnableScheduler();
		__OSRelaunchTitle(resetCode);
	} else if (type == 0x80) {
		OSDisableScheduler();
		__OSShutdownDevices(4);
		OSEnableScheduler();
		__OSReboot(resetCode, bootThisDol);
	}

	OSDisableScheduler();
	__OSShutdownDevices(1);
	__OSHotResetForError();
}

static void __OSReturnToMenu(u8 timeChange)
{
	OSStateFlags state;
	void* ticket;
	u32 playTime;
	s32 remaining;

	__OSStopPlayRecord();
	__OSUnRegisterStateEvent();
	__DVDPrepareReset();
	__OSReadStateFlags(&state);
	state.data.fields.discState = __OSGetDiscState(state.data.fields.discState);
	state.data.fields.menuState = 3;
	state.data.fields.unknown07 = timeChange;
	__OSClearRTCFlags();
	__OSWriteStateFlags(&state);

	OSSetArenaLo((void*)0x81280000);
	OSSetArenaHi((void*)0x812F0000);

	if (ESP_InitLib() != 0) {
		__OSReturnToMenuForError();
	}

	ticket = OSAllocFromMEM1ArenaLo(0xE0, 32);
	if (ticket == NULL) {
		__OSReturnToMenuForError();
	}

	memset(ticket, 0, 0xE0);
	if (ESP_DiGetTicketView(NULL, ticket) == 0 && OSPlayTimeIsLimited()) {
		playTime  = 0;
		remaining = -1;
		__OSGetPlayTime(ticket, &playTime, &remaining);
		if (remaining == 0) {
			__OSWriteExpiredFlagIfSet();
		}
	}

	OSDisableScheduler();
	__OSShutdownDevices(5);
	OSEnableScheduler();
	__OSLaunchMenu();
	OSDisableScheduler();
	__VISetRGBModeImm();
	__OSHotResetForError();
}

void OSReturnToMenu(void)
{
	__OSReturnToMenu(0);
	OSPanic(__FILE__, 0x37F, "OSReturnToMenu(): Falied to boot system menu.\n");
}

void OSReturnToDataManager(void)
{
	__OSReturnToMenu(1);
	OSPanic(__FILE__, 0x391, "OSReturnToDataManager(): Falied to boot system menu.\n");
}

void OSReturnToSetting(u32 page)
{
	const char* path;

	switch (page) {
	case 1:
		path = "Calendar/Calendar_index.html";
		break;
	case 2:
		path = "Display/Display_index.html";
		break;
	case 3:
		path = "Sound/Sound_index.html";
		break;
	case 4:
		path = "Parental_Control/Parental_Control_index.html";
		break;
	case 5:
		path = "Internet/Internet_index.html";
		break;
	case 6:
		path = "WiiConnect24/Wiiconnect24_index.html";
		break;
	case 7:
		path = "Update/Update_index.html";
		break;
	default:
		OSReport("OSReturnToSetting(): You can't specify %d.  \n", page);
		OSPanic(__FILE__, 0x3CB, "");
		break;
	}

	__OSReturnToMenuForSetting(1, path, NULL);
}

void __OSReturnToMenuForError(void)
{
	OSStateFlags state;

	__OSReadStateFlags(&state);
	state.data.fields.discState = 2;
	state.data.fields.menuState = 3;
	__OSClearRTCFlags();
	__OSWriteStateFlags(&state);
	__OSLaunchMenu();
	OSDisableScheduler();
	__VISetRGBModeImm();
	__OSHotResetForError();
	OSPanic(__FILE__, 0x3F2, "__OSReturnToMenu(): Falied to boot system menu.\n");
}

void __OSHotResetForError(void)
{
	if (__OSInNandBoot || __OSInReboot) {
		__OSInitSTM();
	}

	__OSHotReset();
	OSPanic(__FILE__, 0x40A, "__OSHotReset(): Falied to reset system.\n");
}

u32 OSGetResetCode(void)
{
	u32 resetCode;
	if (__OSRebootParams.valid)
		resetCode = 0x80000000 | __OSRebootParams.restartCode;
	else
		resetCode = (__PIRegs[9] & 0xFFFFFFF8) / 8;

	return resetCode;
}

void OSResetSystem(int reset, u32 resetCode, BOOL forceMenu)
{
	OSPanic(__FILE__, 0x4A1, "OSResetSystem() is obsoleted. It doesn't work any longer.\n");
}

// not in the dol (nothing calls it): only its message survives, behind OSResetSystem's (0x80310F88). Body as in
// Petari's OSReset.c; the line it reported is not known.
void OSSetBootDol(u32 dolOffset)
{
	OSPanic(__FILE__, __LINE__, "OSSetBootDol() is obsoleted. It doesn't work any longer.\n");
}
