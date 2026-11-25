#ifndef _AXFX_REVERB_H_
#define _AXFX_REVERB_H_

#include <dolphin/types.h>

static inline f32* AXFXPreDelayEnd(f32* start, s32 samples)
{
    if (samples <= 0) {
        return NULL;
    }
    // The original ring reserves the last allocated slot for delays longer than one sample.
    return start + (samples > 1 ? samples - 1 : 1);
}

#endif
