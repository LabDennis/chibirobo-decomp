#ifndef _DOLPHIN_OS_OSFASTCAST_H
#define _DOLPHIN_OS_OSFASTCAST_H

#include "types.h"
#include "Dolphin/OS/OSUtil.h"

#ifdef __cplusplus
extern "C" {
#endif // ifdef __cplusplus

/////// FAST CAST DEFINES ////////
// GQR formats.
#define OS_GQR_U8  (0x0004) // GQR 1
#define OS_GQR_U16 (0x0005) // GQR 2
#define OS_GQR_S8  (0x0006) // GQR 3
#define OS_GQR_S16 (0x0007) // GQR 4

// GQRs for fast casting.
#define OS_FASTCAST_U8  (2)
#define OS_FASTCAST_U16 (3)
#define OS_FASTCAST_S8  (4)
#define OS_FASTCAST_S16 (5)

//////////////////////////////////

void OSInitFastCast(void);
s16 __OSf32tos16(f32 value);
u8 __OSf32tou8(f32 value);
s8 __OSf32tos8(f32 value);

static inline void OSf32tos16(f32* f, s16* out)
{
	*out = __OSf32tos16(*f);
}

static inline void OSf32tou8(f32* f, u8* out)
{
	*out = __OSf32tou8(*f);
}

static inline void OSf32tos8(f32* f, s8* out)
{
	*out = __OSf32tos8(*f);
}

#ifdef __cplusplus
};
#endif // ifdef __cplusplus

#endif
