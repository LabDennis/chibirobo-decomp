#include "Dolphin/os.h"

extern const u8 __OSSystemCallVectorStart[];
extern const u8 __OSSystemCallVectorEnd[];

/**
 * @note Address: 0x80169DCC
 * @note Size: 0x64
 */
void __OSInitSystemCall(void)
{
	void* handler = OS_SYS_CALL_HANDLER;
	// These linker labels delimit the machine-code template.
	size_t vectorSize = (u32)__OSSystemCallVectorEnd - (u32)__OSSystemCallVectorStart;
	memcpy(handler, __OSSystemCallVectorStart, vectorSize);

	DCFlushRangeNoSync(handler, OS_HANDLER_SLOT_SIZE);
	__sync();
	ICInvalidateRange(handler, OS_HANDLER_SLOT_SIZE);
}
