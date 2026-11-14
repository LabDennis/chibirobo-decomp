#ifndef SYSDOLPHIN_DEVCOM_H
#define SYSDOLPHIN_DEVCOM_H

#include <sysdolphin/forward.h>

#ifdef __cplusplus
extern "C" {
#endif

int HSD_DevComRequest(int file, u32 src, u32 dest, u32 size, int type, int pri, HSD_DevComCallback callback, s32 args);

#ifdef __cplusplus
}
#endif

#endif
