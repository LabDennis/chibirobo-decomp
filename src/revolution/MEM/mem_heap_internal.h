#ifndef REVOLUTION_MEM_HEAP_INTERNAL_H
#define REVOLUTION_MEM_HEAP_INTERNAL_H
#include "revolution/mem_heap.h"
#include "sdk/msl/cpp/cstdint"
#include "string.h"

#define ROUND_UP(x, align)       (((x) + (align) - 1) & -(align))
#define ROUND_UP_PTR(x, align)   ((void*)(((uintptr_t)(x) + (align) - 1) & ~((align) - 1)))
#define ROUND_DOWN_PTR(x, align) ((void*)((uintptr_t)(x) & ~((align) - 1)))

static uintptr_t GetUIntPtr(const void* p)
{
	return (uintptr_t)p;
}

static void* AddU32ToPtr(const void* p, u32 ofs)
{
	return (void*)(GetUIntPtr(p) + ofs);
}

static void* SubU32ToPtr(const void* p, u32 ofs)
{
	return (void*)(GetUIntPtr(p) - ofs);
}

static s32 GetOffsetFromPtr(const void* start, const void* end)
{
	return GetUIntPtr(end) - GetUIntPtr(start);
}

static void LockHeap(MEMiHeapHead* heap)
{
	if (heap->opt & MEM_HEAP_OPT_CAN_LOCK) {
		OSLockMutex(&heap->mutex);
	}
}

static void UnlockHeap(MEMiHeapHead* heap)
{
	if (heap->opt & MEM_HEAP_OPT_CAN_LOCK) {
		OSUnlockMutex(&heap->mutex);
	}
}

static void FillAllocMemory(MEMiHeapHead* heap, void* memBlock, u32 size)
{
	if (heap->opt & MEM_HEAP_OPT_CLEAR_ALLOC) {
		memset(memBlock, 0, size);
	}
}
#endif
