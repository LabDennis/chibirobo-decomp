#include <dolphin/ar.h>

/**
 * @note Address: 0x80190AB0
 * @note Size: 0x40
 */
// how much aram is still free, alloc a block and see where it lands
extern "C" u32 GetFreeARAMSize(void)
{
	u32 start = ARAlloc(0x20);
	ARFree(NULL);
	return ARGetSize() - start;
}
