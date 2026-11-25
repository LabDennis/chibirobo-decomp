#ifndef _DOLPHIN_AXFX_INTERNAL_H_
#define _DOLPHIN_AXFX_INTERNAL_H_

#include <dolphin/axfx.h>
#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void* (*__AXFXAlloc)(u32);
extern void (*__AXFXFree)(void*);

#ifndef __MWERKS__
f32 __fmadds(f32, f32, f32);
f32 __fnmsubs(f32, f32, f32);
#endif

#ifdef __cplusplus
}
#endif

static inline s32 AXFXTruncateSample(f32 sample)
{
    // Keep the signed saturation of the PowerPC float-to-word conversion.
    if (sample >= 2147483648.0f) {
        return INT_MAX;
    }
    if (!(sample >= -2147483648.0f)) {
        return INT_MIN;
    }
    return (s32)sample;
}

#endif
