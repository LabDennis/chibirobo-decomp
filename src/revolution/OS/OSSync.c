#include "Dolphin/os.h"

extern const u8 __OSSystemCallVectorStart[];
extern const u8 __OSSystemCallVectorEnd[];

/**
 * @note Address: 0x80189DDC
 * @note Size: 0x60
 */
void __OSInitSystemCall()
{
	memcpy(OS_SYS_CALL_HANDLER, __OSSystemCallVectorStart, (u32)__OSSystemCallVectorEnd - (u32)__OSSystemCallVectorStart);

	DCFlushRangeNoSync(OS_SYS_CALL_HANDLER, OS_HANDLER_SLOT_SIZE);
	__sync();
	ICInvalidateRange(OS_SYS_CALL_HANDLER, OS_HANDLER_SLOT_SIZE);
}
