#ifndef __OS_H__
#define __OS_H__

#include <dolphin/types.h>

#include <Dolphin/OS/OSAlloc.h>
#include <Dolphin/OS/OSError.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef OSRoundUp32B
#define OSRoundUp32B(x) (((u32)(x) + 32 - 1) & ~(32 - 1))
#endif

BOOL OSDisableInterrupts(void);
BOOL OSRestoreInterrupts(BOOL level);
void* OSGetArenaLo(void);
void* OSGetArenaHi(void);
void OSSetArenaLo(void* newLo);
void* OSAllocFromArenaLo(u32 size, u32 align);
void OSPanic(const char* file, int line, const char* msg, ...);
void OSRegisterVersion(const char* version);
u32 __OSUnmaskInterrupts(u32 mask);
void DCFlushRange(void* addr, u32 nBytes);
void DCFlushRangeNoSync(void* addr, u32 nBytes);
void DCInvalidateRange(void* addr, u32 nBytes);
u32 OSGetPhysicalMemSize(void);
u16 OSGetWirelessID(s32 chan);
void OSSetWirelessID(s32 chan, u16 id);

#ifdef __cplusplus
}
#endif

#endif  // __OS_H__
