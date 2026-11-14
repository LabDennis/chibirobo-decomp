#ifndef REVOLUTION_MEM_HEAP_H
#define REVOLUTION_MEM_HEAP_H

#include "Dolphin/OS/OSMutex.h"
#include "revolution/mem.h"
#include "revolution/mem_list.h"

BEGIN_SCOPE_EXTERN_C
typedef enum { MEM_HEAP_OPT_CLEAR_ALLOC = (1 << 0), MEM_HEAP_OPT_DEBUG_FILL = (1 << 1), MEM_HEAP_OPT_CAN_LOCK = (1 << 2) } MEMHeapOpt;

struct MEMiHeapHead {
	u32 magic;     // at 0x0
	MEMLink link;  // at 0x4
	MEMList list;  // at 0xC
	u8* start;     // at 0x18
	u8* end;       // at 0x1C
	OSMutex mutex; // at 0x20

	union {
		u32 attribute;
		struct {
			u32 attribute_0_24 : 24;
			u32 opt : 8;
		};
	}; // at 0x38
};

void MEMiInitHeapHead(MEMiHeapHead* heap, u32 magic, void* start, void* end, u16 opt);
void MEMiFinalizeHeap(MEMiHeapHead* heap);
END_SCOPE_EXTERN_C
STATIC_ASSERT(sizeof(MEMiHeapHead) == 0x3c);
#endif
