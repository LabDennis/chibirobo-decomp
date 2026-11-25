#include "types.h"
#include "Dolphin/PPCArch.h"

#define HID0_SPD 0x00000200 // Speculative cache access disable

/**
 * @note Address: 0x80161234
 * @note Size: 0x28
 */
void PPCDisableSpeculation(void)
{
	PPCMthid0(PPCMfhid0() | HID0_SPD);
}
