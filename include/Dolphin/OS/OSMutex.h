#ifndef _DOLPHIN_OS_OSMUTEX_H
#define _DOLPHIN_OS_OSMUTEX_H

#include "Dolphin/OS/OSThread.h"
#include <sdk/dolphin/os/OSMutex.h>

#ifdef __cplusplus
extern "C" {
#endif

void __OSUnlockAllMutex(OSThread* thread);

#ifdef __cplusplus
}
#endif

#endif
