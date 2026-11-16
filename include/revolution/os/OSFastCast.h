// Wii additions to the SDK's OSFastCast.h: the 16-bit conversions NW4R's math headers use
// (after xenoblade's RVL_SDK header). The 8-bit and s16 output ones are in Dolphin/OS/OSFastCast.h.
#ifndef _REVOLUTION_OS_OSFASTCAST_H
#define _REVOLUTION_OS_OSFASTCAST_H

#include "Dolphin/OS/OSFastCast.h"

#ifdef __cplusplus
extern "C" {
#endif

static f32 __OSu16tof32(register u16* arg)
{
	register f32 ret;

	asm { psq_l ret, 0(arg), 1, 3 }

	return ret;
}

static void OSu16tof32(u16* in, volatile f32* out)
{
	*out = __OSu16tof32(in);
}

static f32 __OSs16tof32(register s16* arg)
{
	register f32 ret;

	asm { psq_l ret, 0(arg), 1, 5 }

	return ret;
}

static void OSs16tof32(s16* in, volatile f32* out)
{
	*out = __OSs16tof32(in);
}

static u16 __OSf32tou16(register f32 arg)
{
	f32 a;
	register f32* ptr = &a;
	u16 r;

	asm { psq_st arg, 0(ptr), 1, 3 }

	r = *(u16*)ptr;
	return r;
}

static void OSf32tou16(f32* in, volatile u16* out)
{
	*out = __OSf32tou16(*in);
}

#ifdef __cplusplus
}
#endif

#endif
