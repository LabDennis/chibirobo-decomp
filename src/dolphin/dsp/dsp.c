#include <cstddef>
#include <dolphin/dolphin.h>
#include <dolphin/hw_regs.h>

#include "__dsp.h"

#define BUILD_DATE "Apr  5 2004"
#if DEBUG
#define BUILD_TIME "03:56:49"
#else
#define BUILD_TIME "04:15:32"
#endif

#if DEBUG
const char* __DSPVersion = "<< Dolphin SDK - DSP\tdebug build: " BUILD_DATE " " BUILD_TIME " (0x2301) >>";
#else
const char* __DSPVersion = "<< Dolphin SDK - DSP\trelease build: " BUILD_DATE " " BUILD_TIME " (0x2301) >>";
#endif

static BOOL __DSP_init_flag;

/**
 * @note Address: 0x8017CD80
 * @note Size: 0x10
 */
u32 DSPCheckMailToDSP(void) {
    return (__DSPRegs[DSP_MAILBOX_IN_HI] & DSP_MAILBOX_FULL_HI) >> DSP_MAILBOX_FULL_BIT;
}

/**
 * @note Address: 0x8017CD90
 * @note Size: 0x10
 */
u32 DSPCheckMailFromDSP(void) {
    return (__DSPRegs[DSP_MAILBOX_OUT_HI] & DSP_MAILBOX_FULL_HI) >> DSP_MAILBOX_FULL_BIT;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
u32 DSPReadCPUToDSPMbox(void) {
    u32 high = __DSPRegs[DSP_MAILBOX_IN_HI];
    return (high << 16) | __DSPRegs[DSP_MAILBOX_IN_LO];
}

/**
 * @note Address: 0x8017CDA0
 * @note Size: 0x18
 */
u32 DSPReadMailFromDSP(void) {
    /* Reading the low half acknowledges the mail. */
    u32 high = __DSPRegs[DSP_MAILBOX_OUT_HI];
    return (high << 16) | __DSPRegs[DSP_MAILBOX_OUT_LO];
}

/**
 * @note Address: 0x8017CDB8
 * @note Size: 0x14
 */
void DSPSendMailToDSP(u32 mail) {
    __DSPRegs[DSP_MAILBOX_IN_HI] = mail >> 16;
    __DSPRegs[DSP_MAILBOX_IN_LO] = mail & 0xFFFF;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
void DSPAssertInt(void) {
    BOOL old;
    u16 tmp;

    old = OSDisableInterrupts();
    tmp = __DSPRegs[DSP_CONTROL_STATUS];
    tmp = (tmp & ~DSP_STATUS_INTERRUPTS) | DSP_CONTROL_ASSERT_INT;
    __DSPRegs[DSP_CONTROL_STATUS] = tmp;
    OSRestoreInterrupts(old);
}

/**
 * @note Address: 0x8017CDCC
 * @note Size: 0xC4
 */
void DSPInit(void) {
    BOOL old;
    u16 tmp;

    __DSP_debug_printf("DSPInit(): Build Date: %s %s\n", BUILD_DATE, BUILD_TIME);

    if (__DSP_init_flag == 1)
        return;

    OSRegisterVersion(__DSPVersion);

    old = OSDisableInterrupts();
    __OSSetInterruptHandler(__OS_INTERRUPT_DSP_DSP, __DSPHandler);
    __OSUnmaskInterrupts(OS_INTERRUPTMASK_DSP_DSP);

    tmp = __DSPRegs[DSP_CONTROL_STATUS];
    tmp = (tmp & ~DSP_STATUS_INTERRUPTS) | DSP_CONTROL_INIT;
    __DSPRegs[DSP_CONTROL_STATUS] = tmp;

    tmp = __DSPRegs[DSP_CONTROL_STATUS];
    __DSPRegs[DSP_CONTROL_STATUS] = tmp & ~(DSP_STATUS_INTERRUPTS | DSP_CONTROL_HALT);

    __DSP_first_task = __DSP_last_task = __DSP_curr_task = __DSP_tmp_task = NULL;
    __DSP_init_flag = 1;

    OSRestoreInterrupts(old);
}

/**
 * @note Address: 0x8017CE90
 * @note Size: 0x8
 */
BOOL DSPCheckInit(void) {
    return __DSP_init_flag;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
void DSPReset(void) {
    BOOL old;
    u16 tmp;

    old = OSDisableInterrupts();
    tmp = __DSPRegs[DSP_CONTROL_STATUS];
    tmp = (tmp & ~DSP_STATUS_INTERRUPTS) | DSP_CONTROL_INIT | DSP_CONTROL_RESET;
    __DSPRegs[DSP_CONTROL_STATUS] = tmp;
    __DSP_init_flag = 0;
    OSRestoreInterrupts(old);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
void DSPHalt(void) {
    BOOL old;
    u16 tmp;

    old = OSDisableInterrupts();
    tmp = __DSPRegs[DSP_CONTROL_STATUS];
    tmp = (tmp & ~DSP_STATUS_INTERRUPTS) | DSP_CONTROL_HALT;
    __DSPRegs[DSP_CONTROL_STATUS] = tmp;
    OSRestoreInterrupts(old);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
void DSPUnhalt(void) {
    BOOL old;
    u16 tmp;

    old = OSDisableInterrupts();
    tmp = __DSPRegs[DSP_CONTROL_STATUS];
    tmp = (tmp & ~(DSP_STATUS_INTERRUPTS | DSP_CONTROL_HALT));
    __DSPRegs[DSP_CONTROL_STATUS] = tmp;
    OSRestoreInterrupts(old);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
u32 DSPGetDMAStatus(void) {
    return __DSPRegs[DSP_CONTROL_STATUS] & DSP_STATUS_DMA_BUSY;
}

/**
 * @note Address: 0x8017CE98
 * @note Size: 0x70
 */
DSPTaskInfo* DSPAddTask(DSPTaskInfo* task) {
    BOOL old;

    ASSERTMSGLINE(556, __DSP_init_flag == 1, "DSPAddTask(): DSP driver not initialized!\n");

    old = OSDisableInterrupts();

    __DSP_insert_task(task);
    task->state = DSP_TASK_STATE_NEW;
    task->flags = DSP_TASK_FLAG_LINKED;

    OSRestoreInterrupts(old);
    if (task == __DSP_first_task)
        __DSP_boot_task(task);
    return task;
}

/**
 * @note Address: 0x8017CF08
 * @note Size: 0x40
 */
DSPTaskInfo* DSPCancelTask(DSPTaskInfo* task) {
    BOOL old;

    ASSERTMSGLINE(592, __DSP_init_flag == 1, "DSPCancelTask(): DSP driver not initialized!\n");

    old = OSDisableInterrupts();

    task->flags |= DSP_TASK_FLAG_CANCEL;

    OSRestoreInterrupts(old);
    return task;
}

/**
 * @note Address: 0x8017CF48
 * @note Size: 0xC8
 */
DSPTaskInfo* DSPAssertTask(DSPTaskInfo* task) {
    s32 old;

    ASSERTMSGLINE(623, __DSP_init_flag == 1, "DSPAssertTask(): DSP driver not initialized!\n");
    ASSERTMSGLINE(624, task->flags & DSP_TASK_FLAG_LINKED, "DSPAssertTask(): Specified task not in active task list!\n");

    old = OSDisableInterrupts();

    if (__DSP_curr_task == task) {
        __DSP_rude_task = task;
        __DSP_rude_task_pending = 1;
        OSRestoreInterrupts(old);
        return task;
    }

    if (task->priority < __DSP_curr_task->priority) {
        __DSP_rude_task = task;
        __DSP_rude_task_pending = 1;
        if (__DSP_curr_task->state == DSP_TASK_STATE_RUNNING) {
            DSPAssertInt();
        }
        OSRestoreInterrupts(old);
        return task;
    }

    OSRestoreInterrupts(old);
    return NULL;
}
