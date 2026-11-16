#ifndef _REVOSDK_MEM_API_H
#define _REVOSDK_MEM_API_H

// MEM names NW4R uses that revolution/mem*.h do not declare (after xenoblade's revolution/mem, CC0).

#include "revolution/mem_frame_heap.h"

#ifdef __cplusplus
extern "C" {
#endif

u32 MEMGetAllocatableSizeForFrmHeapEx(MEMiHeapHead* heap, s32 align);

static inline s32 MEMGetHeapTotalSize(MEMiHeapHead* heap) { return (s32)((u32)heap->end - (u32)heap); }

#ifdef __cplusplus
}
#endif

#endif
