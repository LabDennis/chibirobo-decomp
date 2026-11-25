#ifndef _memory_h_
#define _memory_h_

#include <dolphin/types.h>

typedef void* (*HSD_MemAllocCallback)(u32 size, u32 align, u32 flags);
typedef void (*HSD_MemFreeCallback)(void* ptr);
typedef void (*HSD_MemClearCallback)(void);
typedef u32 (*HSD_MemGetRemainCallback)(void);
typedef s32 (*HSD_MemCheckOwnCallback)(void* ptr);

typedef struct _HSD_MemCallbacks {
    HSD_MemAllocCallback alloc;
    HSD_MemFreeCallback free;
    HSD_MemClearCallback clear;
    HSD_MemGetRemainCallback get_remain;
    HSD_MemCheckOwnCallback check_own;
} HSD_MemCallbacks;

void _HSD_MemSetCallbacks(HSD_MemCallbacks* callbacks, u32 size);
void* _HSD_MemAlloc(u32 size, u32 align, u32 flags);
u32 _HSD_MemGetRemain(void);
void _HSD_MemClear(void);
s32 _HSD_MemCheckOwn(void* ptr);
void* HSD_Alloc(u32 size);
void HSD_Free(void* ptr);

#endif
