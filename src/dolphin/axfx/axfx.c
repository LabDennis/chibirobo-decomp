#include <dolphin/dolphin.h>
#include <dolphin/axfx.h>

/**
 * @note Address: 0x8017CD24
 * @note Size: 0x28
 */
static void* __AXFXAllocFunction(u32 bytes)
{
	return OSAlloc(bytes);
}

/**
 * @note Address: 0x8017CD4C
 * @note Size: 0x28
 */
static void __AXFXFreeFunction(void* p)
{
	OSFree(p);
}

void* (*__AXFXAlloc)(u32) = __AXFXAllocFunction;
void (*__AXFXFree)(void*) = __AXFXFreeFunction;

/**
 * @note Address: 0x8017CD74
 * @note Size: 0xC
 */
void AXFXSetHooks(void* (*alloc)(u32), void (*free)(void*))
{
	ASSERTLINE(46, alloc && free);

	__AXFXAlloc = alloc;
	__AXFXFree  = free;
}
