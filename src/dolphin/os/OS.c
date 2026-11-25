#include <dolphin/dolphin.h>
#include <dolphin/exi.h>
#include <dolphin/os.h>
#include <dolphin/si.h>
#include <dolphin/db.h>

#include "__os.h"

#define NOP 0x60000000

// external functions
extern void EnableMetroTRKInterrupts(void);
extern void __OSInitMemoryProtection(void);

#define OS_EXCEPTIONTABLE_ADDR 0x3000
#define OS_DBJUMPPOINT_ADDR 0x60

#if SDK_REVISION < 1
#define BUILD_DATE  "Apr  5 2004"
#define DBUILD_TIME "03:55:13"
#define RBUILD_TIME "04:13:58"
#elif SDK_REVISION < 2
#define BUILD_DATE  "May 21 2004"
#define DBUILD_TIME "09:15:32"
#define RBUILD_TIME "09:28:09"
#else
#define BUILD_DATE  "Nov 10 2004"
#define DBUILD_TIME "06:08:19"
#define RBUILD_TIME "06:26:41"
#endif

#if DEBUG
const char* __OSVersion = "<< Dolphin SDK - OS\tdebug build: "BUILD_DATE" "DBUILD_TIME" (0x2301) >>";
#else
const char* __OSVersion = "<< Dolphin SDK - OS\trelease build: "BUILD_DATE" "RBUILD_TIME" (0x2301) >>";
#endif

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

static OSBootInfo* BootInfo;
static u32* BI2DebugFlag;
static u32 BI2DebugFlagHolder;

OSTime __OSStartTime;
BOOL __OSInIPL;
void (**OSExceptionTable)(u8, OSContext*);
BOOL AreWeInitialized;
f32 ZeroPS[2];
f64 ZeroF;
BOOL __OSIsGcam;

static void OSExceptionInit(void);
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

/**
 * @note Address: N/A
 * @note Size: ?
 */
u32 __OSIsDebuggerPresent(void) {
    return *(u32*)OSPhysicalToCached(0x40);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static void DisableWriteGatherPipe(void) {
    u32 hid2;

    hid2 = PPCMfhid2();
    hid2 &= ~0x40000000;
    PPCMthid2(hid2);
}

/**
 * @note Address: 0x80161478
 * @note Size: 0x28
 */
u32 OSGetConsoleType(void) {
    if (!BootInfo || BootInfo->consoleType == 0) {
        return OS_CONSOLE_ARTHUR;
    }
    return BootInfo->consoleType;
}

// needed for assert
#undef NULL
#define NULL 0

/**
 * @note Address: N/A
 * @note Size: ?
 */
static void ClearArena(void) {
    if (!((OSGetResetCode() & 0x80000000) ? TRUE : FALSE)) {
        memset(OSGetArenaLo(), 0, (u32)OSGetArenaHi() - (u32)OSGetArenaLo());
        return;
    }

    if (__OSRebootParams.regionStart == NULL) {
        memset(OSGetArenaLo(), 0, (u32)OSGetArenaHi() - (u32)OSGetArenaLo());
        return;
    }

    ASSERTLINE(683, __OSRebootParams.regionEnd != NULL);

    if ((u32)OSGetArenaLo() < (u32)__OSRebootParams.regionStart) {
        if ((u32)OSGetArenaHi() <= (u32)__OSRebootParams.regionStart) {
            memset(OSGetArenaLo(), 0, (u32)OSGetArenaHi() - (u32)OSGetArenaLo());
            return;
        }

        memset(OSGetArenaLo(), 0, (u32)__OSRebootParams.regionStart - (u32)OSGetArenaLo());

        if ((u32)OSGetArenaHi() > (u32)__OSRebootParams.regionEnd) {
            memset(__OSRebootParams.regionEnd, 0, (u32)OSGetArenaHi() - (u32)__OSRebootParams.regionEnd);
        }
    }
}

/**
 * @note Address: 0x801614A0
 * @note Size: 0x3C
 */
static void InquiryCallback(s32, DVDCommandBlock* block) {
    switch (block->state) {
    case 0:
        __OSDeviceCode = (u16)(0x8000 | DriveInfo.deviceCode);
        break;
    default:
        __OSDeviceCode = 1;
        break;
    }
}

/**
 * @note Address: 0x801614DC
 * @note Size: 0x4E0
 */
void OSInit(void) {
    u32 consoleType;
    void* bi2StartAddr;

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
        PPCDisableSpeculation();
        PPCSetFpNonIEEEMode();

        BootInfo = (OSBootInfo*)OSPhysicalToCached(0);
        BI2DebugFlag = 0;
        __DVDLongFileNameFlag = 0;

        bi2StartAddr = (void*)(*(u32*)OSPhysicalToCached(0xF4));
        if (bi2StartAddr) {
            BI2DebugFlag = (void*)((char*)bi2StartAddr + 0xC);
            __PADSpec = ((u32*)bi2StartAddr)[9];
            *(u8*)OSPhysicalToCached(0x30E8) = *BI2DebugFlag;
            *(u8*)OSPhysicalToCached(0x30E9) = __PADSpec;
        } else if (BootInfo->arenaHi) {
            BI2DebugFlagHolder = *(u8*)OSPhysicalToCached(0x30E8);
            BI2DebugFlag = &BI2DebugFlagHolder;
            __PADSpec = *(u8*)OSPhysicalToCached(0x30E9);
        }

        __DVDLongFileNameFlag = 1;

        OSSetArenaLo((!BootInfo->arenaLo) ? &__ArenaLo : BootInfo->arenaLo);
        if ((!BootInfo->arenaLo) && (BI2DebugFlag) && (*(u32*)BI2DebugFlag < 2)) {
            OSSetArenaLo((void*)(((u32)(char*)&_stack_addr + 0x1F) & 0xFFFFFFE0));
        }
        OSSetArenaHi((!BootInfo->arenaHi) ? &__ArenaHi : BootInfo->arenaHi);

        OSExceptionInit();
        __OSInitSystemCall();
        OSInitAlarm();
        __OSModuleInit();
        __OSInterruptInit();
        __OSSetInterruptHandler(0x16, &__OSResetSWInterruptHandler);
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

        OSReport("\nDolphin OS\n");
#if DEBUG
        OSReport("Kernel built : %s %s\n", BUILD_DATE, DBUILD_TIME);
#else
        OSReport("Kernel built : %s %s\n", BUILD_DATE, RBUILD_TIME);
#endif
        OSReport("Console Type : ");

        consoleType = OSGetConsoleType();
        switch (consoleType & 0xF0000000) {
        case OS_CONSOLE_RETAIL:
            OSReport("Retail %d\n", consoleType);
            break;
        case OS_CONSOLE_DEVELOPMENT:
        case OS_CONSOLE_TDEV:
            switch (consoleType & 0x0FFFFFFF) {
            case OS_CONSOLE_EMULATOR:
                OSReport("Mac Emulator\n");
                break;
            case OS_CONSOLE_PC_EMULATOR:
                OSReport("PC Emulator\n");
                break;
            case OS_CONSOLE_ARTHUR:
                OSReport("EPPC Arthur\n");
                break;
            case OS_CONSOLE_MINNOW:
                OSReport("EPPC Minnow\n");
                break;
            default:
                OSReport("Development HW%d (%08x)\n", (consoleType & 0xFFFFFFF) - 3, consoleType);
                break;
            }
            break;
        default:
            OSReport("%08x\n", consoleType);
            break;
        }

        OSReport("Memory %d MB\n", (u32)BootInfo->memorySize >> 0x14U);
        OSReport("Arena : 0x%x - 0x%x\n", OSGetArenaLo(), OSGetArenaHi());
        OSRegisterVersion(__OSVersion);

        // if location of debug flag exists, and flag is >= 2, enable MetroTRKInterrupts
        if (BI2DebugFlag && ((*BI2DebugFlag) >= 2)) {
            EnableMetroTRKInterrupts();
        }

        ClearArena();
        OSEnableInterrupts();

        if (!__OSInIPL) {
            DVDInit();

            if (__OSIsGcam) {
                __OSDeviceCode = 0x9000;
                return;
            }

            DCInvalidateRange(&DriveInfo, sizeof(DriveInfo));
            DVDInquiryAsync(&DriveBlock, &DriveInfo, InquiryCallback);
        }
    }
}

static u32 __OSExceptionLocations[] = {
    0x00000100, 0x00000200, 0x00000300, 0x00000400, 0x00000500, 0x00000600, 0x00000700, 0x00000800,
    0x00000900, 0x00000C00, 0x00000D00, 0x00000F00, 0x00001300, 0x00001400, 0x00001700,
};

#if DEBUG
char * __OSExceptionNames[17] = {
	"System reset",
	"MachineCheck",
	"DSI",
	"ISI",
	"External Int.",
	"Alignment",
	"Program",
	"FP Unavailable",
	"Decrementer",
	"System call",
	"Trace",
	"Perf mon",
	"IABR",
	"SMI",
	"Thermal Int.",
    "Protection error",
    "FP Exception",
};
#endif

/**
 * @note Address: 0x801619BC
 * @note Size: 0x280
 */
static void OSExceptionInit(void) {
    __OSException exception;
    void* destAddr;
    
    // The exception number is patched into an instruction before each copy.
    u32 oldOpCode = *__OSEVSetNumber;
    u32 handlerSize = (u32)__OSEVEnd - (u32)__OSEVStart;
    u32 dbJumpSize = (u32)__OSDBJUMPEND - (u32)__OSDBJUMPSTART;

    ASSERTMSGLINE(1063, handlerSize <= 0x100, "OSExceptionInit(): too big exception vector code.");

    // Install the debugger integrator if boot has left its slot empty.
    destAddr = (void*)OSPhysicalToCached(OS_DBJUMPPOINT_ADDR);
    if (*(u32*)destAddr == 0) {
        u32 dbIntegratorSize = (u32)__OSDBINTEND - (u32)__OSDBINTSTART;

        DBPrintf("Installing OSDBIntegrator\n");
        memcpy(destAddr, __OSDBINTSTART, dbIntegratorSize);
        DCFlushRangeNoSync(destAddr, dbIntegratorSize);
        __sync();
        ICInvalidateRange(destAddr, dbIntegratorSize);
    }

    // Copy the right vector into the table
    for (exception = 0; exception < __OS_EXCEPTION_MAX; exception++) {
        if (BI2DebugFlag && (*BI2DebugFlag >= 2) && __DBIsExceptionMarked(exception)) {
            DBPrintf(">>> OSINIT: exception %d commandeered by TRK\n", exception);
            continue;
        }
        
        // Modify the copy of code in text before transferring
        // to the exception table.
        *__OSEVSetNumber = oldOpCode | exception;
        
        // Modify opcodes at __DBVECTOR if necessary
        if (__DBIsExceptionMarked(exception)) {
            DBPrintf(">>> OSINIT: exception %d vectored to debugger\n", exception);
            memcpy(__DBVECTOR, __OSDBJUMPSTART, dbJumpSize);
        } else {
            // make sure the opcodes are still nop
            u32 offset;

            for (offset = 0; offset < dbJumpSize; offset += sizeof(u32)) {
                __DBVECTOR[offset / sizeof(u32)] = NOP;
            }
        }
        
        // Install the modified handler.
        destAddr = (void*)OSPhysicalToCached(__OSExceptionLocations[(u32)exception]);
        memcpy(destAddr, __OSEVStart, handlerSize);
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
    *__OSEVSetNumber = oldOpCode;
    
    DBPrintf("Exceptions initialized...\n");
}

/**
 * @note Address: 0x80161C64
 * @note Size: 0x1C
 */
__OSExceptionHandler __OSSetExceptionHandler(__OSException exception, __OSExceptionHandler handler) {
    __OSExceptionHandler oldHandler;
    
    ASSERTMSGLINE(1205, exception < __OS_EXCEPTION_MAX, "__OSSetExceptionHandler(): unknown exception."); 
    
    oldHandler = OSExceptionTable[exception];
    OSExceptionTable[exception] = handler;
    return oldHandler;
}

/**
 * @note Address: 0x80161C80
 * @note Size: 0x14
 */
__OSExceptionHandler __OSGetExceptionHandler(__OSException exception) {
    ASSERTMSGLINE(1228, exception < __OS_EXCEPTION_MAX, "__OSGetExceptionHandler(): unknown exception.");
    return OSExceptionTable[exception];
}

/**
 * @note Address: 0x80161D88
 * @note Size: 0x54
 */
void __OSPSInit(void) {
    PPCMthid2(PPCMfhid2() | HID2_LSQE | HID2_PSE);
    ICFlashInvalidate();
    __sync();
    __OSResetGQRs();
}

/**
 * @note Address: 0x80161DDC
 * @note Size: 0x14
 */
u32 __OSGetDIConfig(void) {
    return (__DIRegs[9] & 0xFF);
}

/**
 * @note Address: 0x80161DF0
 * @note Size: 0x2C
 */
void OSRegisterVersion(const char* id) {
    OSReport("%s\n", id);
}
