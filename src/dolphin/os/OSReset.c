#include <dolphin/dolphin.h>
#include <dolphin/os.h>

#include "__os.h"

extern void __OSResetHardware(u32 resetCode);

static OSResetFunctionQueue ResetFunctionQueue;
static u32 bootThisDol;

/**
 * @note Address: 0x80168690
 * @note Size: 0x84
 */
void OSRegisterResetFunction(OSResetFunctionInfo* info) {
    OSResetFunctionInfo* next = ResetFunctionQueue.head;
    OSResetFunctionInfo* prev;

    ASSERTLINE(208, info->func);

    // Equal-priority callbacks run in registration order.
    while (next && next->priority <= info->priority) {
        next = next->next;
    }

    prev = next ? next->prev : ResetFunctionQueue.tail;
    info->prev = prev;
    info->next = next;

    if (prev) {
        prev->next = info;
    } else {
        ResetFunctionQueue.head = info;
    }
    if (next) {
        next->prev = info;
    } else {
        ResetFunctionQueue.tail = info;
    }
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
void OSUnregisterResetFunction(OSResetFunctionInfo* info) {
    if (info->prev) {
        info->prev->next = info->next;
    } else {
        ResetFunctionQueue.head = info->next;
    }
    if (info->next) {
        info->next->prev = info->prev;
    } else {
        ResetFunctionQueue.tail = info->prev;
    }
}

/**
 * @note Address: 0x80168714
 * @note Size: 0xA8
 */
int __OSCallResetFunctions(BOOL final) {
    OSResetFunctionInfo* info;
    BOOL callbacksReady = TRUE;
    u32 priority = 0;

    // Finish the current priority group even when a callback needs another pass.
    for (info = ResetFunctionQueue.head; info != NULL; info = info->next) {
        if (!callbacksReady && priority != info->priority) {
            break;
        }
        if (!info->func(final)) {
            callbacksReady = FALSE;
        }
        priority = info->priority;
    }

    // SRAM synchronization still runs when a reset callback needs another pass.
    return __OSSyncSram() && callbacksReady;
}

/**
 * @note Address: 0x8016882C
 * @note Size: 0x68
 */
static void KillThreads(void) {
    OSThread* thread;
    OSThread* next;

    for (thread = __OSActiveThreadQueue.head; thread; thread = next) {
        next = thread->linkActive.next;
        if (thread->state == OS_THREAD_STATE_READY || thread->state == OS_THREAD_STATE_WAITING) {
            OSCancelThread(thread);
        }
    }
}

/**
 * @note Address: 0x80168894
 * @note Size: 0x48
 */
void __OSDoHotReset(u32 resetCode) {
    OSDisableInterrupts();
    __VIRegs[1] = 0;
    ICFlashInvalidate();
    __OSResetHardware(resetCode * 8);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
void __OSShutdownDevices(BOOL doRecal) {
    int rc;
    BOOL disableRecalibration;

    __OSStopAudioSystem();

    if (!doRecal) {
        disableRecalibration = __PADDisableRecalibration(TRUE);
    }

    do {} while (!__OSCallResetFunctions(FALSE));
    do {} while (!__OSSyncSram());

    OSDisableInterrupts();

    rc = __OSCallResetFunctions(TRUE);
    ASSERTLINE(408, rc);

    LCDisable();
    if (!doRecal) {
        __PADDisableRecalibration(disableRecalibration);
    }

    KillThreads();
}

/**
 * @note Address: 0x801688DC
 * @note Size: 0x200
 */
void OSResetSystem(int reset, u32 resetCode, BOOL forceMenu) {
    OSSram* sram;

    OSDisableScheduler();

    if (reset == OS_RESET_HOTRESET && forceMenu) {
        sram = __OSLockSram();
        sram->flags |= 0x40;
        __OSUnlockSram(TRUE);

        resetCode = 0;
    }

    if (reset == OS_RESET_SHUTDOWN ||
        (reset == OS_RESET_RESTART && (bootThisDol || resetCode == 0xC0010000)))
    {
        __OSShutdownDevices(FALSE);
    } else {
        __OSShutdownDevices(TRUE);
    }

    if (reset == OS_RESET_HOTRESET) {
        __OSDoHotReset(resetCode);
    } else if (reset == OS_RESET_RESTART) {
        if (forceMenu == TRUE) {
            OSReport("OSResetSystem(): You can't specify TRUE to forceMenu if you restart. Ignored\n");
        }
        OSEnableScheduler();
        __OSReboot(resetCode, bootThisDol);
    }

    memset(OSPhysicalToCached(0x40), 0, 0xcc - 0x40);
    memset(OSPhysicalToCached(0xd4), 0, 0xe8 - 0xd4);
    memset(OSPhysicalToCached(0xf4), 0, 0xf8 - 0xf4);
    memset(OSPhysicalToCached(0x3000), 0, 0xc0);
    memset(OSPhysicalToCached(0x30c8), 0, 0xd4 - 0xc8);
    memset(OSPhysicalToCached(0x30e2), 0, 1);
}

/**
 * @note Address: 0x80168ADC
 * @note Size: 0x38
 */
u32 OSGetResetCode() {
    u32 resetCode;
    if (__OSRebootParams.valid)
        resetCode = 0x80000000 | __OSRebootParams.restartCode;
    else
        resetCode = (__PIRegs[9] & 0xFFFFFFF8) / 8;

    return resetCode;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
u32 OSSetBootDol(u32 dolOffset) {
    u32 oldDol;

    oldDol = bootThisDol;
    bootThisDol = dolOffset;
    return oldDol;
}
