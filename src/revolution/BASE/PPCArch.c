#include "types.h"
#include "Dolphin/PPCArch.h"

// The special-purpose-register accessors have no C spelling. As in upstream's
// GameCube SDK reconstruction they live in the neighboring .s units; ogws,
// xenoblade and wiiplay keep the same Revolution SDK instruction streams.
// ogws: https://github.com/doldecomp/ogws/blob/master/src/revolution/BASE/PPCArch.c
// xenoblade: https://github.com/xbret/xenoblade/blob/main/libs/RVL_SDK/src/revolution/base/PPCArch.c
// wiiplay: https://github.com/Nikman800/wiiplay/blob/main/src/revolution/BASE/PPCArch.c

#define HID0_SPD 0x00000200 // Speculative cache access disable

/**
 * @note Address: 0x8017F530
 * @note Size: 0x28
 */
void PPCDisableSpeculation(void)
{
	PPCMthid0(PPCMfhid0() | HID0_SPD);
}
