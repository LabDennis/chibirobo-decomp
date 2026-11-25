#ifndef _DOLPHIN_OS_OSTHREAD_H
#define _DOLPHIN_OS_OSTHREAD_H

#include "Dolphin/OS/OSUtil.h"
#include <sdk/dolphin/os/OSThread.h>

#ifdef __cplusplus
extern "C" {
#endif

// Thread global addresses.
volatile OSContext* __OSCurrentContext AT_ADDRESS((u32)OSPhysicalToCached(0x00D4));
volatile OSContext* __OSFPUContext AT_ADDRESS((u32)OSPhysicalToCached(0x00D8));
OSThreadQueue __OSActiveThreadQueue AT_ADDRESS((u32)OSPhysicalToCached(0x00DC));
OSThread* __OSCurrentThread AT_ADDRESS((u32)OSPhysicalToCached(0x00E4));


#ifdef __cplusplus
}
#endif

#endif
