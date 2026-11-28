#include "revolution/axfx.h"
#include "Dolphin/OS/OSAlloc.h"

static void* __AXFXAllocFunction(unsigned long size);
static void __AXFXFreeFunction(void* block);

AXFXAllocHook __AXFXAlloc = __AXFXAllocFunction;
AXFXFreeHook __AXFXFree   = __AXFXFreeFunction;

static void* __AXFXAllocFunction(unsigned long size)
{
	return OSAllocFromHeap(__OSCurrHeap, size);
}

static void __AXFXFreeFunction(void* block)
{
	OSFreeToHeap(__OSCurrHeap, block);
}

void AXFXSetHooks(AXFXAllocHook alloc, AXFXFreeHook free)
{
	__AXFXAlloc = alloc;
	__AXFXFree  = free;
}

void AXFXGetHooks(AXFXAllocHook* alloc, AXFXFreeHook* free)
{
	*alloc = __AXFXAlloc;
	*free  = __AXFXFree;
}
