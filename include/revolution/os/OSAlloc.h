#ifndef _REVOLUTION_OS_OSALLOC_H
#define _REVOLUTION_OS_OSALLOC_H

// Wii variant of OSAlloc.h: the Revolution SDK allocates the fatal-display
// buffers out of MEM1's arena. It also defines the Dolphin and sdk guard
// macros so the GameCube variants are skipped when this header is included
// first in a Wii unit.
#ifndef _DOLPHIN_OS_OSALLOC_H
#define _DOLPHIN_OS_OSALLOC_H
#endif
#ifndef _DOLPHIN_OSALLOC_H_
#define _DOLPHIN_OSALLOC_H_
#endif

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif // ifdef __cplusplus

/////////// HEAP TYPES ///////////
// Useful typedef for heap locations.
typedef int OSHeapHandle;

// Alloc visitor function type.
typedef void (*OSAllocVisitor)(void* obj, u32 size);

// Current heap.
extern volatile OSHeapHandle __OSCurrHeap;

//////////////////////////////////

///////// HEAP FUNCTIONS /////////
// Heap functions.
void* OSInitAlloc(void* arenaStart, void* arenaEnd, int maxHeaps);
OSHeapHandle OSCreateHeap(void* start, void* end);
OSHeapHandle OSSetCurrentHeap(OSHeapHandle heap);
void OSFreeToHeap(OSHeapHandle heap, void* ptr);

void* OSAllocFromMEM1ArenaLo(u32 size, u32 align);

// Unused/inlined in P2.
void* OSAllocFromHeap(OSHeapHandle heap, u32 size);
void* OSAllocFixed(void** rStart, void** rEnd);
void OSDestroyHeap(OSHeapHandle heap);
void OSAddToHeap(OSHeapHandle heap, void* start, void* end);
s32 OSCheckHeap(OSHeapHandle heap);
void OSDumpHeap(OSHeapHandle heap);
u32 OSReferentSize(void* ptr);
void OSVisitAllocated(OSAllocVisitor visitor);

//////////////////////////////////

////////// HEAP MACROS ///////////
// Allocate 'size' bytes from current heap (unused in P2).
// Returns ptr to allocated space (32-aligned).
#define OSAlloc(size) OSAllocFromHeap(__OSCurrHeap, (size))

// Deallocates 'ptr' to current heap.
#define OSFree(ptr) OSFreeToHeap(__OSCurrHeap, (ptr))

//////////////////////////////////

#ifdef __cplusplus
};
#endif // ifdef __cplusplus

#endif
