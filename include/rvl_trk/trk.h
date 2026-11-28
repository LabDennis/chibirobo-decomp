#ifndef RVL_TRK_H
#define RVL_TRK_H

// What the Wii's MetroTRK has on top of the shared GameCube header.
#include "PowerPC_EABI_Support/MetroTRK/trk.h"

#ifdef __cplusplus
extern "C" {
#endif

void* TRK_memset(void* dst, int val, size_t n);
size_t TRK_strlen(const char* str);

#ifdef __cplusplus
}
#endif

#endif
