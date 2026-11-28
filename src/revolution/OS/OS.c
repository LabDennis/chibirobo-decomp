#include "revolution/hw_regs.h"
#include "revolution/os/OSContext.h"

#include <dolphin/dolphin.h>
#include <dolphin/exi.h>
#include <dolphin/os.h>
#include <dolphin/si.h>
#include <dolphin/db.h>

#include "revolution/os/__os.h"

// The exception vector and DB-interface entry points live in os_exception_vectors.s.
// tp documents the same Revolution SDK instruction stream:
// https://github.com/zeldaret/tp/blob/main/libs/revolution/src/os/OS.c

#define NOP 0x60000000

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
extern BOOL __OSInReboot;
extern void EnableMetroTRKInterrupts(void);
void PPCMthid4(u32 newHID4);
void* OSGetMEM1ArenaLo(void);
void* OSGetMEM1ArenaHi(void);
void* OSGetMEM2ArenaLo(void);
void* OSGetMEM2ArenaHi(void);
void OSSetMEM1ArenaLo(void* lo);
void OSSetMEM1ArenaHi(void* hi);
void OSSetMEM2ArenaLo(void* lo);
void OSSetMEM2ArenaHi(void* hi);
u32 OSGetPhysicalMem2Size(void);
u32 OSGetConsoleSimulatedMem1Size(void);
u32 OSGetConsoleSimulatedMem2Size(void);
void OSReturnToMenu(void);
void __OSInitAlarm(void);
void __OSInitIPCBuffer(void);
void __OSInitSTM(void);
void __OSInitNet(void);
void __OSInitPlayTime(void);
void __OSStartPlayRecord(void);
void IPCCltInit(void);
void SCInit(void);
u32 SCCheckStatus(void);
BOOL __DVDCheckDevice(void);

#define DB_EXCEPTIONRET_OFFSET  0xC
#define DB_EXCEPTIONDEST_OFFSET 0x8
#define OS_EXCEPTIONTABLE_ADDR  0x3000
#define OS_DBJUMPPOINT_ADDR     0x60

// low memory
#define OS_HOLLYWOOD_REV       (*(u32*)0x80003138)
#define OS_MEM1_ARENA_LO       (*(void**)0x8000310C)
#define OS_MEM1_ARENA_HI       (*(void**)0x80003110)
#define OS_MEM2_ARENA_LO       (*(void**)0x80003124)
#define OS_MEM2_ARENA_HI       (*(void**)0x80003128)
#define OS_IOS_VERSION         (*(u32*)0xC0003140)
#define OS_IOS_BUILD_DATE      (*(u32*)0xC0003144)
#define OS_BOOT_PROGRAM_TARGET (*(u8*)0x8000315C)
#define OS_APPLOADER_TARGET    (*(u8*)0x8000315D)
#define OS_APP_GAMENAME        ((const char*)0x80003180)
#define OS_APP_TYPE            (*(u8*)0x80003184)
#define OS_MIN_IOS_VERSION     (*(u32*)0x80003188)
#define OS_TITLE_GAMENAME      ((const char*)0x80003194)

#define OS_APP_TYPE_IPL 0x40
#define OS_APP_TYPE_DVD 0x80

// which memory an address lies in
#define OSIsMEM1Region(addr) (((u32)(addr) & 0x30000000) == 0x00000000)
#define OSIsMEM2Region(addr) (((u32)(addr) & 0x30000000) == 0x10000000)

#define OS_CONSOLE_RVL_PP_1       0x00000011
#define OS_CONSOLE_RVL_PP_2_1     0x00000012
#define OS_CONSOLE_RVL_PP_2_2     0x00000020
#define OS_CONSOLE_RVL_PP_2_3     0x00000021
#define OS_CONSOLE_RVL_PP_0       0x00000010
#define OS_CONSOLE_RVA_PROD_BOARD 0x00000100
#define OS_CONSOLE_RVL_EMULATOR   0x10000008
#define OS_CONSOLE_NDEV_1_0       0x10000010
#define OS_CONSOLE_NDEV_1_1       0x10000011
#define OS_CONSOLE_NDEV_1_2       0x10000012
#define OS_CONSOLE_NDEV_2_0       0x10000020
#define OS_CONSOLE_NDEV_2_1       0x10000021

#define BUILD_DATE "Oct  2 2008"
#define BUILD_TIME "03:35:15"

const char* __OSVersion = "<< RVL_SDK - OS \trelease build: " BUILD_DATE " " BUILD_TIME " (0x4199_60831) >>";

// DVDInquiryAsync requires a cache-line-aligned DMA destination.
static DVDDriveInfo DriveInfo ATTRIBUTE_ALIGN(32);
static DVDCommandBlock DriveBlock;
OSExecParams __OSRebootParams;

extern u32 __DVDLongFileNameFlag;
extern u32 __PADSpec;

// defined in link script
extern u8 __ArenaLo[];
extern char _stack_addr[];
extern u8 __ArenaHi[];

OSTime __OSStartTime;
static OSBootInfo* BootInfo;
static u32* BI2DebugFlag;
static u32 BI2DebugFlagHolder;
// buffer OSGetAppGamename returns
static char AppGamename[5];
f64 ZeroF;
f32 ZeroPS[2];
void (**OSExceptionTable)(u8, OSContext*);
BOOL AreWeInitialized;
BOOL __OSIsGcam;
BOOL __OSInNandBoot;
BOOL __OSInIPL;

// prototypes
static void __OSInitFPRs(void);
static void OSExceptionInit(void);
u8 OSGetAppType(void);

extern void __OSResetGQRs(void);

// Linker labels for the code copied into the low-memory exception slots.
extern u8 __OSEVStart[];
extern u8 __OSEVEnd[];
extern u32 __OSEVSetNumber[];
extern u32 __DBVECTOR[];
extern const u8 __OSDBINTSTART[];
extern const u8 __OSDBINTEND[];
extern const u8 __OSDBJUMPSTART[];
extern const u8 __OSDBJUMPEND[];

void __OSGetIOSRev(OSIOSRev* rev)
{
	u32 version   = OS_IOS_VERSION;
	u32 buildDate = OS_IOS_BUILD_DATE;

	rev->reserved = (version >> 24) & 0xFF;
	rev->major    = (version >> 16) & 0xFF;
	rev->minor    = (version >> 8) & 0xFF;
	rev->micro    = version & 0xFF;
	rev->month    = ((buildDate >> 20) & 0xF) * 10 + ((buildDate >> 16) & 0xF);
	rev->date     = ((buildDate >> 12) & 0xF) * 10 + ((buildDate >> 8) & 0xF);
	rev->year     = ((buildDate >> 4) & 0xF) * 10 + (buildDate & 0xF) + 2000;
}

static void DisableWriteGatherPipe(void)
{
	u32 hid2;

	hid2 = PPCMfhid2();
	hid2 &= ~0x40000000;
	PPCMthid2(hid2);
}

#define CONSOLE_TYPE_OF_REVISION(rev, RETURN) \
	switch (rev) {                            \
	case 0:                                   \
		RETURN(OS_CONSOLE_RVL_PP_0);          \
	case 1:                                   \
		RETURN(OS_CONSOLE_RVL_PP_1);          \
	case 2:                                   \
		RETURN(OS_CONSOLE_RVL_PP_2_1);        \
	case 0x10:                                \
		RETURN(OS_CONSOLE_RVL_PP_2_2);        \
	case 0x11:                                \
		RETURN(OS_CONSOLE_RVL_PP_2_3);        \
	default:                                  \
		if ((rev) > 0x11) {                   \
			RETURN(OS_CONSOLE_RVL_PP_2_3);    \
		}                                     \
		break;                                \
	}

// how the three users of the table above return the board
#define RETURN_RETAIL(type)      return (type)
#define RETURN_DEVELOPMENT(type) return OS_CONSOLE_DEVELOPMENT | (type)
#define RETURN_BY_MEM2(type)     return (mem2Size == 0x04000000) ? (type) : (OS_CONSOLE_DEVELOPMENT | (type))

u32 OSGetConsoleType(void)
{
	u32 hollywoodRev;
	u32 mem2Size;

	if (!BootInfo || BootInfo->consoleType == 0) {
		return OS_CONSOLE_ARTHUR;
	}

	hollywoodRev = OS_HOLLYWOOD_REV;

	if (__OSDeviceCode & 0x8000) {
		switch (__OSDeviceCode & ~0x8000) {
		case 0x0002:
		case 0x0003:
		case 0x0203:
			CONSOLE_TYPE_OF_REVISION(hollywoodRev, RETURN_RETAIL);
		case 0x0201:
		case 0x0202:
			CONSOLE_TYPE_OF_REVISION(hollywoodRev, RETURN_DEVELOPMENT);
		case 0x0300:
			return OS_CONSOLE_RVA_PROD_BOARD;
		}
	}

	mem2Size = OSGetPhysicalMem2Size();
	CONSOLE_TYPE_OF_REVISION(hollywoodRev, RETURN_BY_MEM2);

	return BootInfo->consoleType;
}

// zeroes a range through the data cache and writes its last 256 KB back to memory
static inline void ClearRange(void* addr, u32 size)
{
	void* flushStart = (size > 0x40000) ? (u8*)addr + size - 0x40000 : addr;

	DCZeroRange(addr, size);
	DCFlushRange(flushStart, 0x40000);
}

static void ClearArena(void)
{
	if (!((OSGetResetCode() & 0x80000000) ? TRUE : FALSE)) {
		ClearRange(OSGetArenaLo(), (u32)OSGetArenaHi() - (u32)OSGetArenaLo());
		return;
	}

	if (__OSRebootParams.regionStart == NULL || !OSIsMEM1Region(__OSRebootParams.regionStart)) {
		ClearRange(OSGetArenaLo(), (u32)OSGetArenaHi() - (u32)OSGetArenaLo());
		return;
	}

	// the line number is a guess; the release build keeps nothing of it
	ASSERTLINE(800, __OSRebootParams.regionEnd != NULL);

	if ((u32)OSGetArenaLo() < (u32)__OSRebootParams.regionStart) {
		if ((u32)OSGetArenaHi() <= (u32)__OSRebootParams.regionStart) {
			ClearRange(OSGetArenaLo(), (u32)OSGetArenaHi() - (u32)OSGetArenaLo());
			return;
		}

		ClearRange(OSGetArenaLo(), (u32)__OSRebootParams.regionStart - (u32)OSGetArenaLo());

		if ((u32)OSGetArenaHi() > (u32)__OSRebootParams.regionEnd) {
			ClearRange(__OSRebootParams.regionEnd, (u32)OSGetArenaHi() - (u32)__OSRebootParams.regionEnd);
		}
	}
}

static void ClearMEM2Arena(void)
{
	if (!((OSGetResetCode() & 0x80000000) ? TRUE : FALSE)) {
		ClearRange(OSGetMEM2ArenaLo(), (u32)OSGetMEM2ArenaHi() - (u32)OSGetMEM2ArenaLo());
		return;
	}

	if (__OSRebootParams.regionStart == NULL || !OSIsMEM2Region(__OSRebootParams.regionStart)) {
		ClearRange(OSGetMEM2ArenaLo(), (u32)OSGetMEM2ArenaHi() - (u32)OSGetMEM2ArenaLo());
		return;
	}

	ASSERTLINE(860, __OSRebootParams.regionEnd != NULL);

	if ((u32)OSGetMEM2ArenaLo() < (u32)__OSRebootParams.regionStart) {
		if ((u32)OSGetMEM2ArenaHi() <= (u32)__OSRebootParams.regionStart) {
			ClearRange(OSGetMEM2ArenaLo(), (u32)OSGetMEM2ArenaHi() - (u32)OSGetMEM2ArenaLo());
			return;
		}

		ClearRange(OSGetMEM2ArenaLo(), (u32)__OSRebootParams.regionStart - (u32)OSGetMEM2ArenaLo());

		if ((u32)OSGetMEM2ArenaHi() > (u32)__OSRebootParams.regionEnd) {
			ClearRange(__OSRebootParams.regionEnd, (u32)OSGetMEM2ArenaHi() - (u32)__OSRebootParams.regionEnd);
		}
	}
}

static void InquiryCallback(s32, DVDCommandBlock* block)
{
	switch (block->state) {
	case 0:
		__OSDeviceCode = (u16)(0x8000 | DriveInfo.deviceCode);
		break;
	default:
		__OSDeviceCode = 1;
		break;
	}
}

static void ReportOSInfo(void)
{
	u32 consoleType;
	OSIOSRev iosRev;

	OSReport("\nRevolution OS\n");
	OSReport("Kernel built : %s %s\n", BUILD_DATE, BUILD_TIME);
	OSReport("Console Type : ");

	consoleType = OSGetConsoleType();
	switch (consoleType & OS_CONSOLE_MASK) {
	case OS_CONSOLE_RETAIL:
		switch (consoleType) {
		case OS_CONSOLE_RVL_PP_1:
			OSReport("Pre-production board 1\n");
			break;
		case OS_CONSOLE_RVL_PP_2_1:
			OSReport("Pre-production board 2-1\n");
			break;
		case OS_CONSOLE_RVL_PP_2_2:
			OSReport("Pre-production board 2-2\n");
			break;
		case OS_CONSOLE_RVA_PROD_BOARD:
			OSReport("RVA 1\n");
			break;
		default:
			OSReport("Retail %d\n", consoleType);
			break;
		}
		break;
	case OS_CONSOLE_DEVELOPMENT:
		switch (consoleType) {
		case OS_CONSOLE_NDEV_2_1:
			OSReport("NDEV 2.1\n");
			break;
		case OS_CONSOLE_NDEV_2_0:
			OSReport("NDEV 2.0\n");
			break;
		case OS_CONSOLE_NDEV_1_2:
			OSReport("NDEV 1.2\n");
			break;
		case OS_CONSOLE_NDEV_1_1:
			OSReport("NDEV 1.1\n");
			break;
		case OS_CONSOLE_NDEV_1_0:
			OSReport("NDEV 1.0\n");
			break;
		case OS_CONSOLE_RVL_EMULATOR:
			OSReport("Revolution Emulator\n");
			break;
		default:
			OSReport("Emulation platform (%08x)\n", consoleType);
			break;
		}
		break;
	case OS_CONSOLE_TDEV:
		OSReport("TDEV-based emulation HW%d\n", (consoleType & ~OS_CONSOLE_MASK) - 3);
		break;
	default:
		OSReport("%08x\n", consoleType);
		break;
	}

	__OSGetIOSRev(&iosRev);
	OSReport("Firmware     : %d.%d.%d ", iosRev.major, iosRev.minor, iosRev.micro);
	OSReport("(%d/%d/%d)\n", iosRev.month, iosRev.date, iosRev.year);
	OSReport("Memory %d MB\n", (OSGetConsoleSimulatedMem1Size() + OSGetConsoleSimulatedMem2Size()) >> 20);
	OSReport("MEM1 Arena : 0x%x - 0x%x\n", OSGetMEM1ArenaLo(), OSGetMEM1ArenaHi());
	OSReport("MEM2 Arena : 0x%x - 0x%x\n", OSGetMEM2ArenaLo(), OSGetMEM2ArenaHi());
}

// stops with ERROR #002 when the IOS is older than the one the disc asks for, or of another major version
static inline void CheckFirmware(void)
{
	GXColor bg = { 0, 0, 255, 0 };
	GXColor fg = { 255, 255, 255, 0 };
	OSIOSRev iosRev;
	u32 version;
	__OSGetIOSRev(&iosRev);
	version = (iosRev.major << 16) | (iosRev.minor << 8) | iosRev.micro;
	if (iosRev.major != (OS_MIN_IOS_VERSION >> 16) || (iosRev.major == (OS_MIN_IOS_VERSION >> 16) && version < OS_MIN_IOS_VERSION)) {
		OSReport("OS ERROR: This firmware is an improper version for this SDK. Please use a correct Firmware.\n");
		OSFatal(fg, bg,
		        "\n\nERROR #002\nAn error has occurred.\nPress the Eject Button, remove the\nGame Disc, and turn off the power to \nthe "
		        "console. \nPlease read the Wii Operations Manual \nfor further instructions.\n");
		OSPanic(__FILE__, 1236, "Failed to run app");
	}
}

// NONMATCHING: the instruction bytes match retail. The strict report's residual is
// object-level relocation/symbolization: fixed memory constants such as 0x80004000
// are symbolic in the retail split but plain constants here, and the error-string
// relocations resolve through different object-local literal-pool ownership.
void OSInit(void)
{
	void* bi2StartAddr;
	void* arenaLo;
	void* arenaHi;

	if (AreWeInitialized == FALSE) {
		AreWeInitialized = TRUE;

		__OSStartTime = __OSGetSystemTime();
		OSDisableInterrupts();

		__OSGetExecParams(&__OSRebootParams);
		PPCMtmmcr0(0);
		PPCMtmmcr1(0);
		PPCMtpmc1(0);
		PPCMtpmc2(0);
		PPCMtpmc3(0);
		PPCMtpmc4(0);
		PPCMthid4(0x83900000);
		PPCDisableSpeculation();
		PPCSetFpNonIEEEMode();

		BootInfo              = (OSBootInfo*)OSPhysicalToCached(0);
		BI2DebugFlag          = 0;
		__DVDLongFileNameFlag = 0;

		bi2StartAddr = (void*)(*(u32*)OSPhysicalToCached(0xF4));
		if (bi2StartAddr) {
			BI2DebugFlag                     = (void*)((char*)bi2StartAddr + 0xC);
			__PADSpec                        = ((u32*)bi2StartAddr)[9];
			*(u8*)OSPhysicalToCached(0x30E8) = *BI2DebugFlag;
			*(u8*)OSPhysicalToCached(0x30E9) = __PADSpec;
		} else if (BootInfo->arenaHi) {
			BI2DebugFlagHolder = *(u8*)OSPhysicalToCached(0x30E8);
			BI2DebugFlag       = &BI2DebugFlagHolder;
			__PADSpec          = *(u8*)OSPhysicalToCached(0x30E9);
		}

		__DVDLongFileNameFlag = 1;

		arenaLo = OS_MEM1_ARENA_LO;
		if (!arenaLo) {
			if (OSIsMEM1Region(__ArenaLo)) {
				arenaLo = (!BootInfo->arenaLo) ? &__ArenaLo : BootInfo->arenaLo;
				if ((!BootInfo->arenaLo) && (BI2DebugFlag) && (*BI2DebugFlag < 2)) {
					arenaLo = (void*)(((u32)(char*)&_stack_addr + 0x1F) & 0xFFFFFFE0);
				}
			} else {
				arenaLo = (void*)0x80004000;
			}
		}
		OSSetMEM1ArenaLo(arenaLo);

		arenaHi = OS_MEM1_ARENA_HI;
		if (!arenaHi) {
			arenaHi = (!BootInfo->arenaHi) ? &__ArenaHi : BootInfo->arenaHi;
		}
		OSSetMEM1ArenaHi(arenaHi);

		arenaLo = OS_MEM2_ARENA_LO;
		if (arenaLo) {
			if (OSIsMEM2Region(__ArenaLo)) {
				arenaLo = &__ArenaLo;
				if ((BI2DebugFlag) && (*BI2DebugFlag < 2)) {
					arenaLo = (void*)(((u32)(char*)&_stack_addr + 0x1F) & 0xFFFFFFE0);
				}
			} else if ((u32)arenaLo >= 0x90000000 && (u32)arenaLo < 0x90000800) {
				arenaLo = (void*)0x90000800;
			}
			OSSetMEM2ArenaLo(arenaLo);
		}

		arenaHi = OS_MEM2_ARENA_HI;
		if (arenaHi) {
			OSSetMEM2ArenaHi(arenaHi);
		}

		__OSInitIPCBuffer();
		OSExceptionInit();
		__OSInitSystemCall();
		__OSInitAlarm();
		__OSModuleInit();
		__OSInterruptInit();
		__OSContextInit();
		__OSCacheInit();
		EXIInit();
		SIInit();
		__OSInitSram();
		__OSThreadInit();
		__OSInitAudioSystem();

		DisableWriteGatherPipe();

		if (!__OSInIPL) {
			__OSInitMemoryProtection();
		}

		ReportOSInfo();
		OSRegisterVersion(__OSVersion);

		// if location of debug flag exists, and flag is >= 2, enable MetroTRKInterrupts
		if (BI2DebugFlag && ((*BI2DebugFlag) >= 2)) {
			EnableMetroTRKInterrupts();
		}

		if (!__OSInNandBoot && !__OSInReboot) {
			ClearArena();
			ClearMEM2Arena();
		}

		OSEnableInterrupts();
		IPCCltInit();

		if (!__OSInNandBoot && !__OSInReboot) {
			__OSInitSTM();
			SCInit();
			while (SCCheckStatus() == 1) { }
			__OSInitNet();
		}

		if (!__OSInIPL) {
			switch (OS_BOOT_PROGRAM_TARGET) {
			case 0x80:
				break;
			case 0x81:
				OSReport("OS ERROR: boot program is not for RVL target. Please use correct boot program.\n");
				OSPanic(__FILE__, 1153, "Failed to run app");
				break;
			}

			switch (OS_APPLOADER_TARGET) {
			case 0x80:
				break;
			case 0x81:
				OSReport("OS ERROR: apploader[D].img is not for RVL target. Please use correct apploader[D].img.\n");
				OSPanic(__FILE__, 1171, "Failed to run app");
				break;
			}

			CheckFirmware();

			DVDInit();

			if (__OSIsGcam) {
				__OSDeviceCode = 0x9000;
			} else if (__OSDeviceCode == 0) {
				DCInvalidateRange(&DriveInfo, sizeof(DriveInfo));
				DVDInquiryAsync(&DriveBlock, &DriveInfo, InquiryCallback);
			}

			if (OSGetAppType() == OS_APP_TYPE_DVD && !__OSInReboot && !__DVDCheckDevice()) {
				OSReturnToMenu();
			}
		}

		if (!__OSInIPL && !__OSInNandBoot) {
			__OSInitPlayTime();
		}

		if (!__OSInIPL && !__OSInNandBoot && !__OSInReboot) {
			__OSStartPlayRecord();
		}
	}
}

static u32 __OSExceptionLocations[] = {
	0x00000100, 0x00000200, 0x00000300, 0x00000400, 0x00000500, 0x00000600, 0x00000700, 0x00000800,
	0x00000900, 0x00000C00, 0x00000D00, 0x00000F00, 0x00001300, 0x00001400, 0x00001700,
};

#if DEBUG
char* __OSExceptionNames[17] = {
	"System reset",   "MachineCheck",     "DSI",          "ISI",   "External Int.", "Alignment", "Program",
	"FP Unavailable", "Decrementer",      "System call",  "Trace", "Perf mon",      "IABR",      "SMI",
	"Thermal Int.",   "Protection error", "FP Exception",
};
#endif

static void OSExceptionInit(void)
{
	__OSException exception;
	void* destAddr;

	// These two vars help us change the exception number embedded
	// in the exception handler code.
	u32* opCodeAddr;
	u32 oldOpCode;

	// Address range of the actual code to be copied.
	u8* handlerStart;
	u32 handlerSize;

	ASSERTMSGLINE(1063, (u32)__OSEVEnd - (u32)__OSEVStart <= 0x100, "OSExceptionInit(): too big exception vector code.");

	// Install the first level exception vector.
	opCodeAddr   = (u32*)__OSEVSetNumber;
	oldOpCode    = *opCodeAddr;
	handlerStart = (u8*)__OSEVStart;
	handlerSize  = (u32)((u8*)__OSEVEnd - (u8*)__OSEVStart);

	// Install the DB integrator, only if we are the first OSInit to be run
	destAddr = (void*)OSPhysicalToCached(OS_DBJUMPPOINT_ADDR);
	if (*(u32*)destAddr == 0) // Lomem should be zero cleared only once by BS2
	{
		DBPrintf("Installing OSDBIntegrator\n");
		memcpy(destAddr, __OSDBINTSTART, (u32)__OSDBINTEND - (u32)__OSDBINTSTART);
		DCFlushRangeNoSync(destAddr, (u32)__OSDBINTEND - (u32)__OSDBINTSTART);
		__sync();
		ICInvalidateRange(destAddr, (u32)__OSDBINTEND - (u32)__OSDBINTSTART);
	}

	// Copy the right vector into the table
	for (exception = 0; exception < __OS_EXCEPTION_MAX; exception++) {
		if (BI2DebugFlag && (*BI2DebugFlag >= 2) && __DBIsExceptionMarked(exception)) {
			// this DBPrintf is suspicious.
			DBPrintf(">>> OSINIT: exception %d commandeered by TRK\n", exception);
			continue;
		}

		// Modify the copy of code in text before transferring
		// to the exception table.
		*opCodeAddr = oldOpCode | exception;

		// Modify opcodes at __DBVECTOR if necessary
		if (__DBIsExceptionMarked(exception)) {
			DBPrintf(">>> OSINIT: exception %d vectored to debugger\n", exception);
			// Petari's RVL SDK uses __OSDBINTEND here; it is the same address as __OSDBJUMPSTART.
			memcpy(__DBVECTOR, __OSDBINTEND, (u32)__OSDBJUMPEND - (u32)__OSDBINTEND);
		} else {
			// make sure the opcodes are still nop
			u32* ops = (u32*)__DBVECTOR;
			int cb;

			for (cb = 0; cb < (u32)__OSDBJUMPEND - (u32)__OSDBINTEND; cb += sizeof(u32)) {
				*ops++ = NOP;
			}
		}

		// Install the modified handler.
		destAddr = (void*)OSPhysicalToCached(__OSExceptionLocations[(u32)exception]);
		memcpy(destAddr, handlerStart, handlerSize);
		DCFlushRangeNoSync(destAddr, handlerSize);
		__sync();
		ICInvalidateRange(destAddr, handlerSize);
	}
	// initialize pointer to exception table
	OSExceptionTable = (void*)OSPhysicalToCached(OS_EXCEPTIONTABLE_ADDR);

	// install default exception handlers
	for (exception = 0; exception < __OS_EXCEPTION_MAX; exception++) {
		__OSSetExceptionHandler(exception, OSDefaultExceptionHandler);
	}

	// restore the old opcode, so that we can re-start an application without
	// downloading the text segments
	*opCodeAddr = oldOpCode;

	DBPrintf("Exceptions initialized...\n");
}

__OSExceptionHandler __OSSetExceptionHandler(__OSException exception, __OSExceptionHandler handler)
{
	__OSExceptionHandler oldHandler;

	ASSERTMSGLINE(1205, exception < __OS_EXCEPTION_MAX, "__OSSetExceptionHandler(): unknown exception.");

	oldHandler                  = OSExceptionTable[exception];
	OSExceptionTable[exception] = handler;
	return oldHandler;
}

__OSExceptionHandler __OSGetExceptionHandler(__OSException exception)
{
	ASSERTMSGLINE(1228, exception < __OS_EXCEPTION_MAX, "__OSGetExceptionHandler(): unknown exception.");
	return OSExceptionTable[exception];
}

void __OSPSInit(void)
{
	PPCMthid2(PPCMfhid2() | 0x80000000 | 0x20000000);
	ICFlashInvalidate();
	__sync();
	__OSResetGQRs();
}

u32 __OSGetDIConfig(void)
{
	return (__DIRegs[9] & 0xFF);
}

void OSRegisterVersion(const char* id)
{
	OSReport("%s\n", id);
}

// game code the system menu reports for itself
static const char* IPLGamename = "HAEA";

// the title's four-letter game code as a string
const char* OSGetAppGamename(void)
{
	const s8* src = (const s8*)OS_TITLE_GAMENAME;

	if (__OSInIPL) {
		src = (const s8*)IPLGamename;
	} else if (src[0] < '0' || (src[0] > '9' && src[0] < 'A') || src[0] > 'Z') {
		src = (const s8*)OS_APP_GAMENAME;
	}

	AppGamename[0] = src[0];
	AppGamename[1] = src[1];
	AppGamename[2] = src[2];
	AppGamename[3] = src[3];
	AppGamename[4] = '\0';
	return AppGamename;
}

u8 OSGetAppType(void)
{
	if (__OSInIPL) {
		return OS_APP_TYPE_IPL;
	}
	return OS_APP_TYPE;
}
