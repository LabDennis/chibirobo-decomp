// The Revolution SDK's OSFastCast.h additions g3d uses: setting the two free quantisation
// registers and the u8 input conversion (after xenoblade's RVL_SDK header, CC0).
#ifndef _REVOLUTION_OS_OSFASTCASTGQR_H
#define _REVOLUTION_OS_OSFASTCASTGQR_H

#include "revolution/os/OSFastCast.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OS_GQR_TYPE_U8  4
#define OS_GQR_TYPE_U16 5
#define OS_GQR_TYPE_S8  6
#define OS_GQR_TYPE_S16 7

static void OSSetGQR6(register u32 type, register u32 scale)
{
	register u32 val = ((scale << 8 | type) << 16) | ((scale << 8) | type);

	asm { mtspr 0x396, val }
}

static void OSSetGQR7(register u32 type, register u32 scale)
{
	register u32 val = ((scale << 8 | type) << 16) | ((scale << 8) | type);

	asm { mtspr 0x397, val }
}

static f32 __OSu8tof32(register u8* in)
{
	register f32 ret;

	asm { psq_l ret, 0(in), 1, 2 }

	return ret;
}

static void OSu8tof32(u8* in, volatile f32* out)
{
	*out = __OSu8tof32(in);
}

#ifdef __cplusplus
}
#endif

#endif
