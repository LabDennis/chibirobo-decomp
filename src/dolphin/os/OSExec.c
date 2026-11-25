#include <dolphin/dolphin.h>
#include <dolphin/os.h>

#include "__os.h"
#include "__dvd.h"

extern volatile u32 BOOT_REGION_START AT_ADDRESS(0x812FDFF0);
extern volatile u32 BOOT_REGION_END AT_ADDRESS(0x812FDFEC);
extern volatile u8 g_unk_800030E2 AT_ADDRESS(0x800030E2);

extern void __OSRunExecutable(void* entryPoint);

static int Prepared;

enum {
    EXEC_ARG_BLOCK_SIZE = 0x2000,
    EXEC_ARGV_ALIGNMENT = 0x1000,
    EXEC_ARGV_LIMIT = 0x400,
    EXEC_ARG_LIST_FIELD = 8,
    EXEC_ARG_HEADER_SIZE = EXEC_ARG_LIST_FIELD + sizeof(u32),
};

/**
 * @note Address: 0x801645AC
 * @note Size: 0x188
 */
static void PackArgs(void* addr, s32 argc, const char* const* argv) {
    char* bootInfo2 = addr;
    u32 stringOffset = EXEC_ARG_BLOCK_SIZE;
    u32 listOffset;
    u32 listSize;
    u32 minimumStringOffset;
    u32* list;
    s32 i;

    memset(bootInfo2, 0, EXEC_ARG_BLOCK_SIZE);
    if (argc <= 0 || (u32)argc > (EXEC_ARG_BLOCK_SIZE - EXEC_ARG_HEADER_SIZE) / sizeof(*list) - 2) {
        return;
    }

    listSize = (argc + 2) * sizeof(*list);
    minimumStringOffset = EXEC_ARG_HEADER_SIZE + listSize;

    // The strings occupy the end of the BI2 block, after argc and argv's offsets.
    for (i = 0; i < argc; i++) {
        size_t length = strlen(argv[i]) + 1;
        if (length > stringOffset - minimumStringOffset) {
            return;
        }
        stringOffset -= length;
    }

    listOffset = (stringOffset & ~3u) - listSize;
    ASSERTMSGLINE(LINE(129, 138, 140), listOffset >= 0x1000U, "OSExec: Argument list is too long");
    *(u32*)(bootInfo2 + EXEC_ARG_LIST_FIELD) = listOffset;

    list = (u32*)(bootInfo2 + listOffset);
    *list++ = argc;
    for (i = 0; i < argc; i++) {
        size_t length = strlen(argv[i]) + 1;
        list[i] = stringOffset;
        memcpy(bootInfo2 + stringOffset, argv[i], length);
        stringOffset += length;
    }
    list[argc] = 0;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static void StartDol(const OSExecParams* params, void* entry) {
    OSExecParams* paramsWork = OSAllocFromArenaLo(sizeof(OSExecParams), 1);

    __OSSetExecParams(params, paramsWork);
    __PIRegs[9] = 7;

    OSDisableInterrupts();
    __OSRunExecutable(entry);
}

/**
 * @note Address: 0x80164770
 * @note Size: 0x6C
 */
static void ReadDisc(void* addr, s32 length, s32 offset) {
    DVDCommandBlock block;
#if SDK_REVISION < 1
    OSTime start;
#endif

    DVDReadAbsAsyncPrio(&block, addr, length, offset, NULL, 0);

#if SDK_REVISION < 1    
    start = OSGetTime();
#endif

    while (DVDGetCommandBlockStatus(&block)) {
#if SDK_REVISION < 1
        if (!DVDCheckDisk() || OS_TIMER_CLOCK < (OSGetTime() - start))
#else
        if (!DVDCheckDisk())
#endif
        {
            __OSDoHotReset(0);
        }
    }
}

/**
 * @note Address: 0x801647DC
 * @note Size: 0xC
 */
static void Callback(s32, DVDCommandBlock*) {
    Prepared = TRUE;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static int IsStreamEnabled() {
    if (DVDGetCurrentDiskID()->streaming) {
        return TRUE;
    }

    return FALSE;
}

/**
 * @note Address: 0x801647E8
 * @note Size: 0x40
 */
void __OSGetExecParams(OSExecParams* params) {
    if (0x80000000 <= (u32)__OSExecParams) {
        memcpy(params, __OSExecParams, sizeof(OSExecParams));
    } else {
        params->valid = FALSE;
    }
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
void __OSSetExecParams(const OSExecParams* params, OSExecParams* addr) {
    memcpy(addr, params, sizeof(OSExecParams));
    __OSExecParams = addr;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static void StopStreaming() {
    DVDCommandBlock block;
#if SDK_REVISION < 1
    OSTime start;
#endif

    if (!__OSIsGcam && IsStreamEnabled()) {
        AISetStreamVolLeft(0);
        AISetStreamVolRight(0);
        DVDCancelStreamAsync(&block, NULL);

#if SDK_REVISION < 1
        start = OSGetTime();
#endif

        while (DVDGetCommandBlockStatus(&block)) {
#if SDK_REVISION < 1
            if (!DVDCheckDisk() || OS_TIMER_CLOCK < (OSGetTime() - start))
#else
            if (!DVDCheckDisk())
#endif
            {
                __OSDoHotReset(0);
            }
        }

        AISetStreamPlayState(0);
    }
}

/**
 * @note Address: 0x80164828
 * @note Size: 0xC4
 */
static int GetApploaderPosition(void) {
    static s32 apploaderPosition;

    u32* tgcHeader;
    s32 apploaderOffsetInTGC;

    if (apploaderPosition != 0) {
        return apploaderPosition;
    }

    if (__OSAppLoaderOffset != 0) {
        tgcHeader = OSAllocFromArenaLo(0x40, DOLPHIN_ALIGNMENT);
        ReadDisc(tgcHeader, 0x40, __OSAppLoaderOffset);
        apploaderOffsetInTGC = tgcHeader[14];
        ASSERTMSGLINE(LINE(370, 376, 378), apploaderOffsetInTGC != 0, "OSExec() or OSResetSystem(): Wrong apploader offset. Maybe converted by an\nolder version of gcm2tgc. Use gcm2tgc v.1.20 or later.");

        apploaderPosition = __OSAppLoaderOffset + apploaderOffsetInTGC;
    } else {
        apploaderPosition = 0x2440;
    }
    return apploaderPosition;
}

typedef struct {
    char date[16];
    u32 entry;
    u32 size;
    u32 rebootSize;
    u32 reserved2;
} AppLoaderStruct;

/**
 * @note Address: N/A
 * @note Size: ?
 */
static AppLoaderStruct* LoadApploader() {
    AppLoaderStruct* header;

    header = (AppLoaderStruct*)OSAllocFromArenaLo(sizeof(AppLoaderStruct), DOLPHIN_ALIGNMENT);
    ReadDisc(header, sizeof(AppLoaderStruct), GetApploaderPosition());
    ASSERTMSGLINE(LINE(401, 407, 409), header->rebootSize != 0, "OSResetSystem(): old apploader");

    ReadDisc((void*)0x81200000, OSRoundUp32B(header->size), GetApploaderPosition() + 0x20);
    ICInvalidateRange((void*)0x81200000, OSRoundUp32B(header->size));
    return header;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static void* LoadDol(const OSExecParams* params, AppLoaderCallback getInterface) {
    appInitCallback appInit;
    appGetNextCallback appGetNext;
    appGetEntryCallback appGetEntry;
    void* addr;
    u32 length;
    u32 offset;
    OSExecParams* paramsWork;

    getInterface(&appInit, &appGetNext, &appGetEntry);
    paramsWork = (OSExecParams*)OSAllocFromArenaLo(sizeof(OSExecParams), 1);
    __OSSetExecParams(params, paramsWork);
    appInit((void(*)(char*))OSReport);
    OSSetArenaLo(paramsWork);

    while (appGetNext(&addr, &length, &offset) != 0) {
        ReadDisc(addr, length, offset);
    }

    return appGetEntry();
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static BOOL IsNewApploader(AppLoaderStruct* header) {
    return strncmp(header->date, "2004/02/01", 10) > 0 ? TRUE : FALSE;
}

/**
 * @note Address: 0x801648EC
 * @note Size: 0x484
 */
void __OSBootDolSimple(u32 doloffset, u32 restartCode, void* regionStart, void* regionEnd, BOOL argsUseDefault, s32 argc, const char* const* argv) {
    OSExecParams* params;
    void* dolEntry;
    AppLoaderStruct* header;

#if SDK_REVISION < 1
    OSTime start;
#endif

    OSDisableInterrupts();
    params = (OSExecParams*)OSAllocFromArenaLo(sizeof(OSExecParams), 1);
    params->valid = TRUE;
    params->restartCode = restartCode;
    params->regionStart = regionStart;
    params->regionEnd = regionEnd;
    params->argsUseDefault = argsUseDefault;

    if (!argsUseDefault) {
        params->argsAddr = OSAllocFromArenaLo(EXEC_ARG_BLOCK_SIZE, 1);
        PackArgs(params->argsAddr, argc, argv);
    }

    DVDInit();
    DVDSetAutoInvalidation(TRUE);
    DVDResume();

    Prepared = FALSE;

    __DVDPrepareResetAsync(Callback);
    __OSMaskInterrupts(0xFFFFFFE0);
    __OSUnmaskInterrupts(0x400);
    OSEnableInterrupts();

#if SDK_REVISION < 1
    start = OSGetTime();
#endif

    while (Prepared != TRUE) {
#if SDK_REVISION < 1
        if (!DVDCheckDisk() || OS_TIMER_CLOCK < (OSGetTime() - start))
#else
        if (!DVDCheckDisk())
#endif
        {
            __OSDoHotReset(0);
        }
    }

    StopStreaming();

    header = LoadApploader();
    if (IsNewApploader(header)) {
        if (doloffset == 0xFFFFFFFF) {
            doloffset = (GetApploaderPosition() + 0x20) + header->size;
        }

        params->bootDol = doloffset;
        dolEntry = LoadDol(params, (AppLoaderCallback)header->entry);
        StartDol(params, dolEntry);
    } else {
        BOOT_REGION_START = (u32)regionStart;
        BOOT_REGION_END = (u32)regionEnd;
        g_unk_800030E2 = 1;

        ReadDisc((void*)0x81300000, OSRoundUp32B(header->rebootSize), (GetApploaderPosition() + 0x20) + header->size);
        ICInvalidateRange((void*)0x81300000, OSRoundUp32B(header->rebootSize));
        OSDisableInterrupts();
        ICFlashInvalidate();
        __OSRunExecutable((void*)0x81300000);
    }
}

/**
 * @note Address: 0x80164D70
 * @note Size: 0x19C
 */
void __OSBootDol(u32 doloffset, u32 restartCode, const char** argv) {
    char doloffInString[20];
    s32 argvlen;
    const char** argvToPass;
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
    argvToPass = OSAllocFromArenaLo((argvlen + 1) * 4, 1);
    *argvToPass = doloffInString;

    for (i = 1; i < argvlen; i++) {
        argvToPass[i] = argv[i - 1];
    }

    __OSBootDolSimple(-1, restartCode, saveStart, saveEnd, FALSE, argvlen, argvToPass);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static void ExecCommon(const char* dolfile, const char** argv) {
    DVDFileInfo fileInfo;
    u32 doloff;

    if ((s8)*dolfile == '\0') {
        doloff = 0;
    } else if (DVDOpen((char*)dolfile, &fileInfo)) {
        doloff = fileInfo.startAddr;
    } else {
        ASSERTMSGLINE(LINE(689, 695, 697), 0, "OSExec(): The specified file doesn't exist");
        return;
    }

    __OSBootDol(doloff, 0xC0000000, argv);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
void OSExecv(const char* dolfile, const char** argv) {
    ASSERTMSGLINE(LINE(718, 724, 726), dolfile != 0, "OSExecv(): null pointer was specified for the dol file name.");
    ASSERTMSGLINE(LINE(719, 725, 727), argv != 0, "OSExecv(): null pointer was specified for argv.");

    OSDisableScheduler();
    __OSShutdownDevices(FALSE);
    OSEnableScheduler();
    OSSetArenaLo((void*)0x81280000);
    OSSetArenaHi((void*)0x812F0000);
    ExecCommon(dolfile, argv);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
void OSExecl(const char* dolfile, const char* arg0, ...) {
    va_list vl;
    const char* arg;
    u32 argc = 0;
    const char** argv;
    const char** next;

    ASSERTMSGLINE(LINE(759, 765, 767), dolfile != 0, "OSExecl(): null pointer was specified for the dol file name.");

    // The SDK limit is exclusive and includes the terminating null.
    va_start(vl, arg0);
    for (arg = arg0; arg != NULL && argc < EXEC_ARGV_LIMIT - 2; arg = va_arg(vl, const char*)) {
        ++argc;
    }
    va_end(vl);
    ASSERTMSGLINE(LINE(787, 793, 794), arg == NULL, "OSExecl(): Arguments too long");
    if (arg != NULL) {
        return;
    }

    OSDisableScheduler();
    __OSShutdownDevices(FALSE);
    OSEnableScheduler();
    OSSetArenaLo((void*)0x81280000);
    OSSetArenaHi((void*)0x812F0000);

    argv = OSAllocFromArenaLo((argc + 1) * sizeof(*argv), EXEC_ARGV_ALIGNMENT);
    va_start(vl, arg0);
    for (next = argv, arg = arg0; arg != NULL; arg = va_arg(vl, const char*)) {
        *next++ = arg;
    }
    va_end(vl);
    *next = NULL;

    ExecCommon(dolfile, argv);
}
