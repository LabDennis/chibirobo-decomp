#ifndef _DOLPHIN_DSP_INTERNAL_H_
#define _DOLPHIN_DSP_INTERNAL_H_

#include <dolphin/dsp.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    DSP_TASK_STATE_NEW,
    DSP_TASK_STATE_RUNNING,
    DSP_TASK_STATE_YIELDED,
    DSP_TASK_STATE_DONE,
};

#define DSP_TASK_FLAG_LINKED 0x1U
#define DSP_TASK_FLAG_CANCEL 0x2U

static inline void __DSPSendMailAndWait(u32 mail) {
    DSPSendMailToDSP(mail);
    while (DSPCheckMailToDSP() != 0) {
    }
}

extern DSPTaskInfo* __DSP_first_task;
extern DSPTaskInfo* __DSP_last_task;
extern DSPTaskInfo* __DSP_curr_task;
extern DSPTaskInfo* __DSP_tmp_task;
extern DSPTaskInfo* __DSP_rude_task;
extern int __DSP_rude_task_pending;

DECL_WEAK void __DSPHandler(__OSInterrupt, OSContext*);
void __DSP_exec_task(DSPTaskInfo*, DSPTaskInfo*);
void __DSP_boot_task(DSPTaskInfo*);
void __DSP_insert_task(DSPTaskInfo*);
void __DSP_add_task(DSPTaskInfo* task);
void __DSP_remove_task(DSPTaskInfo* task);
void __DSP_debug_printf(const char* fmt, ...);

#ifdef __cplusplus
}
#endif

#endif
