#include <dolphin/dolphin.h>
#include <dolphin/ax.h>

#include "__ax.h"

static AXSPB __AXStudio ATTRIBUTE_ALIGN(32);

static s32 __AXSpbAL;
static s32 __AXSpbAR;
static s32 __AXSpbAS;
static s32 __AXSpbAAL;
static s32 __AXSpbAAR;
static s32 __AXSpbAAS;
static s32 __AXSpbABL;
static s32 __AXSpbABR;
static s32 __AXSpbABS;

/**
 * @note Address: 0x80178940
 * @note Size: 0xC
 */
u32 __AXGetStudio(void) {
    return (u32)&__AXStudio;
}

#define AX_MAX_DEPOP_DELTA 20

// The DSP stores the initial volume in two consecutive halfwords.
static s32 AXDepopFade(s32 volume, u16* volumeHi, u16* volumeLo, s16* dspDelta) {
    s32 step = volume / AX_SAMPLES_PER_FRAME;

    if (step == 0) {
        *volumeHi = 0;
        *volumeLo = 0;
        *dspDelta = 0;
        return 0;
    }

    if (step > AX_MAX_DEPOP_DELTA) {
        step = AX_MAX_DEPOP_DELTA;
    } else if (step < -AX_MAX_DEPOP_DELTA) {
        step = -AX_MAX_DEPOP_DELTA;
    }

    *volumeHi = (u16)((u32)volume >> 16);
    *volumeLo = (u16)volume;
    *dspDelta = (s16)-step;
    return volume - step * AX_SAMPLES_PER_FRAME;
}

/**
 * @note Address: 0x8017894C
 * @note Size: 0x3F8
 */
void __AXPrintStudio(void) {
    __AXSpbAL = AXDepopFade(__AXSpbAL, &__AXStudio.dpopLHi, &__AXStudio.dpopLLo, &__AXStudio.dpopLDelta);
    __AXSpbAR = AXDepopFade(__AXSpbAR, &__AXStudio.dpopRHi, &__AXStudio.dpopRLo, &__AXStudio.dpopRDelta);
    __AXSpbAS = AXDepopFade(__AXSpbAS, &__AXStudio.dpopSHi, &__AXStudio.dpopSLo, &__AXStudio.dpopSDelta);
    __AXSpbAAL = AXDepopFade(__AXSpbAAL, &__AXStudio.dpopALHi, &__AXStudio.dpopALLo, &__AXStudio.dpopALDelta);
    __AXSpbAAR = AXDepopFade(__AXSpbAAR, &__AXStudio.dpopARHi, &__AXStudio.dpopARLo, &__AXStudio.dpopARDelta);
    __AXSpbAAS = AXDepopFade(__AXSpbAAS, &__AXStudio.dpopASHi, &__AXStudio.dpopASLo, &__AXStudio.dpopASDelta);
    __AXSpbABL = AXDepopFade(__AXSpbABL, &__AXStudio.dpopBLHi, &__AXStudio.dpopBLLo, &__AXStudio.dpopBLDelta);
    __AXSpbABR = AXDepopFade(__AXSpbABR, &__AXStudio.dpopBRHi, &__AXStudio.dpopBRLo, &__AXStudio.dpopBRDelta);
    __AXSpbABS = AXDepopFade(__AXSpbABS, &__AXStudio.dpopBSHi, &__AXStudio.dpopBSLo, &__AXStudio.dpopBSDelta);
    DCFlushRange(&__AXStudio, sizeof(__AXStudio));
}

/**
 * @note Address: 0x80178D44
 * @note Size: 0x2C
 */
void __AXSPBInit(void) {
#if DEBUG
    OSReport("Initializing AXSPB code module\n");
#endif
    __AXSpbAL = 
    __AXSpbAR = 
    __AXSpbAS = 
    __AXSpbAAL = 
    __AXSpbAAR = 
    __AXSpbAAS = 
    __AXSpbABL = 
    __AXSpbABR = 
    __AXSpbABS = 0;
}

/**
 * @note Address: 0x80178D70
 * @note Size: 0x4
 */
void __AXSPBQuit(void) {
#if DEBUG
    OSReport("Shutting down AXSPB code module\n");
#endif
}

/**
 * @note Address: 0x80178D74
 * @note Size: 0x94
 */
void __AXDepopVoice(AXPB* p) {
    __AXSpbAL += p->dpop.aL;
    __AXSpbAAL += p->dpop.aAuxAL;
    __AXSpbABL += p->dpop.aAuxBL;
    __AXSpbAR += p->dpop.aR;
    __AXSpbAAR += p->dpop.aAuxAR;
    __AXSpbABR += p->dpop.aAuxBR;
    __AXSpbAS += p->dpop.aS;
    __AXSpbAAS += p->dpop.aAuxAS;
    __AXSpbABS += p->dpop.aAuxBS;
}
