#include <cstddef>

#include <dolphin/dolphin.h>
#include <dolphin/hw_regs.h>

#include "__dsp.h"

#define DSP_MAIL_BOOT_READY   0x8071FEEDU

#define DSP_MAIL_TASK_INIT    0xDCD10000U
#define DSP_MAIL_TASK_RESUME  0xDCD10001U
#define DSP_MAIL_TASK_YIELD   0xDCD10002U
#define DSP_MAIL_TASK_DONE    0xDCD10003U
#define DSP_MAIL_TASK_REQUEST 0xDCD10004U

#define DSP_CMD_SWITCH_TASK 0xCDD10001U
#define DSP_CMD_STOP        0xCDD10002U
#define DSP_CMD_RESUME_TASK 0xCDD10003U

DSPTaskInfo* __DSP_curr_task;
DSPTaskInfo* __DSP_first_task;
DSPTaskInfo* __DSP_last_task;
DSPTaskInfo* __DSP_tmp_task;
DSPTaskInfo* __DSP_rude_task;
int __DSP_rude_task_pending;

static void YieldCurrentTask(void) {
    BOOL rudePending = __DSP_rude_task_pending != 0;
    DSPTaskInfo* next;
    BOOL resume;

    if (rudePending) {
        next = __DSP_rude_task;
        resume = __DSP_curr_task == next;
    } else {
        next = __DSP_curr_task->next;
        resume = next == NULL && __DSP_curr_task == __DSP_first_task;
        if (next == NULL)
            next = __DSP_first_task;
    }

    if (resume) {
        __DSPSendMailAndWait(DSP_CMD_RESUME_TASK);
    } else {
        __DSPSendMailAndWait(DSP_CMD_SWITCH_TASK);
        __DSP_exec_task(__DSP_curr_task, next);
        __DSP_curr_task->state = DSP_TASK_STATE_YIELDED;
        __DSP_curr_task = next;
    }

    if (rudePending) {
        __DSP_rude_task = NULL;
        __DSP_rude_task_pending = 0;
    }

    if (resume && __DSP_curr_task->res_cb != NULL)
        __DSP_curr_task->res_cb(__DSP_curr_task);
}

static void CompleteCurrentTask(void) {
    BOOL rudePending = __DSP_rude_task_pending != 0;
    BOOL hasNext = FALSE;
    BOOL stop = FALSE;

    if (!rudePending) {
        hasNext = __DSP_curr_task->next != NULL;
        stop = !hasNext && __DSP_curr_task == __DSP_first_task;
    }

    // The completion callback can change the task queue.
    if (__DSP_curr_task->done_cb != NULL)
        __DSP_curr_task->done_cb(__DSP_curr_task);

    if (rudePending) {
        __DSPSendMailAndWait(DSP_CMD_SWITCH_TASK);
        __DSP_exec_task(NULL, __DSP_rude_task);
        __DSP_remove_task(__DSP_curr_task);
        __DSP_curr_task = __DSP_rude_task;
        __DSP_rude_task = NULL;
        __DSP_rude_task_pending = 0;
    } else if (stop) {
        __DSPSendMailAndWait(DSP_CMD_STOP);
        __DSP_curr_task->state = DSP_TASK_STATE_DONE;
        __DSP_remove_task(__DSP_curr_task);
    } else {
        __DSPSendMailAndWait(DSP_CMD_SWITCH_TASK);
        __DSP_curr_task->state = DSP_TASK_STATE_DONE;
        if (hasNext) {
            __DSP_exec_task(NULL, __DSP_curr_task->next);
            __DSP_curr_task = __DSP_curr_task->next;
            __DSP_remove_task(__DSP_curr_task->prev);
        } else {
            __DSP_exec_task(NULL, __DSP_first_task);
            __DSP_curr_task = __DSP_first_task;
            __DSP_remove_task(__DSP_last_task);
        }
    }
}

/**
 * @note Address: 0x8017D060
 * @note Size: 0x424
 */
void __DSPHandler(__OSInterrupt intr, OSContext* context) {
    u16 acknowledgedStatus;
    OSContext exceptionContext;
    u16 status;
    u32 mail;

    status = __DSPRegs[DSP_CONTROL_STATUS];
    acknowledgedStatus = (status & ~(DSP_STATUS_AI_INTERRUPT | DSP_STATUS_ARAM_INTERRUPT)) | DSP_STATUS_DSP_INTERRUPT;
    __DSPRegs[DSP_CONTROL_STATUS] = acknowledgedStatus;

    OSClearContext(&exceptionContext);
    OSSetCurrentContext(&exceptionContext);
    ASSERTMSGLINE(143, __DSP_curr_task != NULL, "__DSPHandler(): No current task! Someone set us up the bomb!\n");

    while (DSPCheckMailFromDSP() == 0)
        ;

    mail = DSPReadMailFromDSP();
    if ((__DSP_curr_task->flags & DSP_TASK_FLAG_CANCEL) && mail == DSP_MAIL_TASK_YIELD) {
        mail = DSP_MAIL_TASK_DONE;
    }

    switch (mail) {
    case DSP_MAIL_TASK_INIT:
        __DSP_curr_task->state = DSP_TASK_STATE_RUNNING;
        if (__DSP_curr_task->init_cb != NULL)
            __DSP_curr_task->init_cb(__DSP_curr_task);
        break;
    case DSP_MAIL_TASK_RESUME:
        __DSP_curr_task->state = DSP_TASK_STATE_RUNNING;
        if (__DSP_curr_task->res_cb != NULL)
            __DSP_curr_task->res_cb(__DSP_curr_task);
        break;
    case DSP_MAIL_TASK_YIELD:
        YieldCurrentTask();
        break;
    case DSP_MAIL_TASK_DONE:
        CompleteCurrentTask();
        break;
    case DSP_MAIL_TASK_REQUEST:
        if (__DSP_curr_task->req_cb != NULL)
            __DSP_curr_task->req_cb(__DSP_curr_task);
        break;
    default:
        ASSERTMSGLINEV(519, 0, "__DSPHandler(): Unknown msg from DSP 0x%08X - task sync failed!\n", mail);
    }

    OSClearContext(&exceptionContext);
    OSSetCurrentContext(context);
}

/**
 * @note Address: 0x8017D484
 * @note Size: 0x1A0
 */
void __DSP_exec_task(DSPTaskInfo* curr, DSPTaskInfo* next) {
    ASSERTMSGLINE(552, next != NULL, "__DSP_exec_task(): NULL task. It is to weep.\n");

    if (curr != NULL) {
        __DSPSendMailAndWait((u32)curr->dram_mmem_addr);
        __DSPSendMailAndWait(curr->dram_length);
        __DSPSendMailAndWait(curr->dram_addr);
    } else {
        __DSPSendMailAndWait(0);
        __DSPSendMailAndWait(0);
        __DSPSendMailAndWait(0);
    }

    __DSPSendMailAndWait((u32)next->iram_mmem_addr);
    __DSPSendMailAndWait(next->iram_length);
    __DSPSendMailAndWait(next->iram_addr);

    if (next->state == DSP_TASK_STATE_NEW) {
        __DSPSendMailAndWait(next->dsp_init_vector);
        __DSPSendMailAndWait(0);
        __DSPSendMailAndWait(0);
        __DSPSendMailAndWait(0);
    } else {
        __DSPSendMailAndWait(next->dsp_resume_vector);

        __DSPSendMailAndWait((u32)next->dram_mmem_addr);
        __DSPSendMailAndWait(next->dram_length);
        __DSPSendMailAndWait(next->dram_addr);
    }
}

/**
 * @note Address: 0x8017D624
 * @note Size: 0x18C
 */
void __DSP_boot_task(DSPTaskInfo* task) {
    u32 mail;

    ASSERTMSGLINE(634, task != NULL, "__DSP_boot_task(): NULL task!\n");
    while (DSPCheckMailFromDSP() == 0)
        ;

    // Reading the mailbox acknowledges the boot message in release builds too.
    mail = DSPReadMailFromDSP();
    ASSERTMSGLINEV(640, mail == DSP_MAIL_BOOT_READY, "__DSP_boot_task(): Failed to sync DSP on boot! (0x%08X)\n", mail);
    (void)mail;

    __DSPSendMailAndWait(0x80F3A001);
    __DSPSendMailAndWait((u32)task->iram_mmem_addr);

    __DSPSendMailAndWait(0x80F3C002);
    __DSPSendMailAndWait(task->iram_addr & 0xFFFF);

    __DSPSendMailAndWait(0x80F3A002);
    __DSPSendMailAndWait(task->iram_length);

    __DSPSendMailAndWait(0x80F3B002);
    __DSPSendMailAndWait(0);

    __DSPSendMailAndWait(0x80F3D001);
    __DSPSendMailAndWait(task->dsp_init_vector);

    __DSP_debug_printf("DSP is booting task: 0x%08X\n", (u32)task);
    __DSP_debug_printf("__DSP_boot_task()  : IRAM MMEM ADDR: 0x%08X\n", (u32)task->iram_mmem_addr);
    __DSP_debug_printf("__DSP_boot_task()  : IRAM DSP ADDR : 0x%08X\n", task->iram_addr);
    __DSP_debug_printf("__DSP_boot_task()  : IRAM LENGTH   : 0x%08X\n", task->iram_length);
    __DSP_debug_printf("__DSP_boot_task()  : DRAM MMEM ADDR: 0x%08X\n", task->dram_length);
    __DSP_debug_printf("__DSP_boot_task()  : Start Vector  : 0x%08X\n", task->dsp_init_vector);
}

/**
 * @note Address: 0x8017D7B0
 * @note Size: 0xA0
 */
void __DSP_insert_task(DSPTaskInfo* task) {
    DSPTaskInfo* temp;

    if (__DSP_first_task == NULL) {
        __DSP_curr_task = task;
        __DSP_first_task = __DSP_last_task = task;
        task->next = task->prev = NULL;
        return;
    }

    temp = __DSP_first_task;
    while (temp != NULL) {
        if (task->priority < temp->priority) {
            task->prev = temp->prev;
            temp->prev = task;
            task->next = temp;
            if (task->prev == NULL)
                __DSP_first_task = task;
            else
                task->prev->next = task;
            break;
        }
        temp = temp->next;
    }

    if (temp == NULL) {
        __DSP_last_task->next = task;
        task->next = NULL;
        task->prev = __DSP_last_task;
        __DSP_last_task = task;
    }
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
void __DSP_add_task(DSPTaskInfo* task) {
    ASSERTMSGLINE(771, task != NULL, "__DSP_add_task(): Why are you adding a NULL task?\n");

    if (__DSP_last_task == NULL) {
        __DSP_curr_task = task;
        __DSP_last_task = task;
        __DSP_first_task = task;
        task->next = task->prev = NULL;
    } else {
        __DSP_last_task->next = task;
        task->next = NULL;
        task->prev = __DSP_last_task;
        __DSP_last_task = task;
    }

    task->state = DSP_TASK_STATE_NEW;
    __DSP_debug_printf("__DSP_add_task() : Added task    : 0x%08X\n", (u32)task);
}

/**
 * @note Address: 0x8017D850
 * @note Size: 0x94
 */
void __DSP_remove_task(DSPTaskInfo* task) {
    ASSERTMSGLINE(813, task != NULL, "__DSP_remove_task(): NULL task! Why? WHY?!?!\n");
    task->flags = 0;
    task->state = DSP_TASK_STATE_DONE;

    if (__DSP_first_task == task) {
        if (task->next != NULL) {
            __DSP_first_task = task->next;
            task->next->prev = NULL;
        }
        else
            __DSP_first_task = __DSP_last_task = __DSP_curr_task = NULL;
        return;
    }

    if (__DSP_last_task == task) {
        __DSP_last_task = task->prev;
        task->prev->next = NULL;
        __DSP_curr_task = __DSP_first_task;
        return;
    }

    __DSP_curr_task = task->next;
    task->prev->next = task->next;
    task->next->prev = task->prev;
}
