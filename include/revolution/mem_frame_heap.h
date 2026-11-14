#ifndef RVL_SDK_MEM_FRAME_HEAP_H
#define RVL_SDK_MEM_FRAME_HEAP_H
#include "revolution/mem_heap.h"
#ifdef __cplusplus
extern "C" {
#endif

#define MEM_FRM_HEAP_MIN_SIZE (sizeof(MEMiHeapHead) + sizeof(MEMiFrmHeapHead))

// Specify how to free memory
typedef enum {
	MEM_FRM_HEAP_FREE_TO_HEAD = (1 << 0),
	MEM_FRM_HEAP_FREE_TO_TAIL = (1 << 1),
	MEM_FRM_HEAP_FREE_ALL     = MEM_FRM_HEAP_FREE_TO_HEAD | MEM_FRM_HEAP_FREE_TO_TAIL
} MEMiFrmFreeFlag;

typedef struct MEMiFrmHeapState {
	u32 id;                        // at 0x0
	u8* head;                      // at 0x4
	u8* tail;                      // at 0x8
	struct MEMiFrmHeapState* next; // at 0xC
} MEMiFrmHeapState;

// Placed in heap after base heap head
typedef struct MEMiFrmHeapHead {
	u8* head;                 // at 0x0
	u8* tail;                 // at 0x4
	MEMiFrmHeapState* states; // at 0x8
} MEMiFrmHeapHead;

MEMiHeapHead* MEMCreateFrmHeapEx(void* start, u32 size, u16 opt);
MEMiHeapHead* MEMDestroyFrmHeap(MEMiHeapHead* heap);
void* MEMAllocFromFrmHeapEx(MEMiHeapHead* heap, u32 size, s32 align);
void MEMFreeToFrmHeap(MEMiHeapHead* heap, u32 flags);
BOOL MEMRecordStateForFrmHeap(MEMiHeapHead* heap, u32 id);
BOOL MEMFreeByStateToFrmHeap(MEMiHeapHead* heap, u32 id);

#ifdef __cplusplus
}
#endif
#endif
