#include "Dolphin/os.h"

/**
 * @note Address: 0x80169DAC
 * @note Size: 0x0
 */
ASM static void SystemCallVector()
{
#ifdef __MWERKS__ // clang-format off
	nofralloc

	entry __OSSystemCallVectorStart
	mfspr r9, HID0
	ori r10, r9, 8
	mtspr HID0, r10
	isync
	sync
	mtspr HID0, r9
	rfi

	entry __OSSystemCallVectorEnd
	nop
#endif // clang-format on
}

/**
 * @note Address: 0x80169DCC
 * @note Size: 0x64
 */
void __OSInitSystemCall()
{
	memcpy(OS_SYS_CALL_HANDLER, __OSSystemCallVectorStart, (u32)__OSSystemCallVectorEnd - (u32)__OSSystemCallVectorStart);

	DCFlushRangeNoSync(OS_SYS_CALL_HANDLER, OS_HANDLER_SLOT_SIZE);
	asm { sync }
	ICInvalidateRange(OS_SYS_CALL_HANDLER, OS_HANDLER_SLOT_SIZE);
}
