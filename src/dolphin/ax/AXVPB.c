#include <dolphin/dolphin.h>
#include <dolphin/ax.h>
#include <cstring>
#include "PowerPC_EABI_Support/MSL_C/PPC_EABI/math_ppc.h"

#include "__ax.h"
#include "global.h"

static const u32 __AXSrcCycles[5] = {
    0x00000DF8,
    0x00000F78,
    0x000014B8,
    0x000019F8,
    0x000019F8
};

static const u32 __AXMainMixCycles[16] = {
    0x00000000, 0x000002F8, 0x000002F8,
	0x000005BE, 0x000002F8, 0x000005F0,
	0x000005F0, 0x000008B6, 0x00000000,
	0x000004F1, 0x000004F1, 0x000009A6,
	0x000004F1, 0x000009E2, 0x000009E2,
	0x00000E97
};

static const u32 __AXAuxMixCycles[32] = {
    0x00000000, 0x000002F8, 0x000002F8,
	0x000005BE, 0x00000000, 0x000004F1,
	0x000004F1, 0x000009A6, 0x000002F8,
	0x000005F0, 0x000005F0, 0x000008B6,
	0x000002F8, 0x000007E9, 0x000007E9,
	0x00000C9E, 0x00000000, 0x000002F8,
	0x000002F8, 0x000005BE, 0x00000000,
	0x000004F1, 0x000004F1, 0x000009A6,
	0x000004F1, 0x000007E9, 0x000007E9,
	0x00000AAF, 0x000004F1, 0x000009E2,
	0x000009E2, 0x00000E97
};

/* The DSP sync prefix includes the first seven reserved halfwords. */
#define AX_PB_SYNC_BYTES 0xD0
#define AX_FRAME_BASE_DSP_CYCLES 43536U

STATIC_ASSERT(sizeof(AXAddressPair) == 4);
STATIC_ASSERT(sizeof(AXPBADDR) == 0x10);
STATIC_ASSERT(offsetof(AXPBADDR, loopAddress) == 4);
STATIC_ASSERT(offsetof(AXPBADDR, endAddress) == 8);
STATIC_ASSERT(offsetof(AXPBADDR, currentAddress) == 12);
STATIC_ASSERT(offsetof(AXPB, addr) == 0x6E);
STATIC_ASSERT(sizeof(AXPB) == 0xF4);
STATIC_ASSERT(offsetof(AXVPB, updateData) == 0x34);
STATIC_ASSERT(offsetof(AXVPB, pb) == 0x138);
STATIC_ASSERT(sizeof(AXVPB) == 0x22C);

static AXPB __AXPB[AX_MAX_VOICES] ATTRIBUTE_ALIGN(AX_DSP_BUFFER_ALIGNMENT);
static AXPBITDBUFFER __AXITD[AX_MAX_VOICES] ATTRIBUTE_ALIGN(AX_DSP_BUFFER_ALIGNMENT);
static AXPBU __AXUpdates[AX_MAX_VOICES] ATTRIBUTE_ALIGN(AX_DSP_BUFFER_ALIGNMENT);
static AXVPB __AXVPB[AX_MAX_VOICES];

static u32 __AXMaxDspCycles;
static u32 __AXRecDspCycles;
static u32 __AXNumVoices;

/**
 * @note Address: 0x80178E08
 * @note Size: 0x8
 */
u32 __AXGetNumVoices(void) {
    return __AXNumVoices;
}

/* The PB prefix and update entries are copied between separate, word-aligned buffers. */
static inline void CopyWords(void* dst, const void* src, u32 wordCount) {
    u32* dstWords = dst;
    const u32* srcWords = src;

    while (wordCount != 0) {
        *dstWords++ = *srcWords++;
        wordCount--;
    }
}

static inline void ClearITDBuffer(AXPBITDBUFFER* buffer) {
    u32 i;

    for (i = 0; i < ARRAY_SIZEU(buffer->data); i++) {
        buffer->data[i] = 0;
    }
}

/**
 * @note Address: 0x80178E10
 * @note Size: 0x74C
 */
void __AXServiceVPB(AXVPB* pvpb) {
    AXPB* ppbDsp;
    AXPB* ppbUser;
    u32 sync;

    ASSERTLINE(238, (pvpb->index >= 0) && (pvpb->index < AX_MAX_VOICES));

    __AXNumVoices += 1;
    ppbDsp = &__AXPB[pvpb->index];
    ppbUser = &pvpb->pb;
    sync = pvpb->sync;
    if (sync == 0) {
        ppbUser->state = ppbDsp->state;
        ppbUser->ve.currentVolume = ppbDsp->ve.currentVolume;
        ppbUser->addr.currentAddress.hi = ppbDsp->addr.currentAddress.hi;
        ppbUser->addr.currentAddress.lo = ppbDsp->addr.currentAddress.lo;
        return;
    }
    if (sync & AX_SYNC_FLAG_COPYALL) {
        CopyWords(ppbDsp, ppbUser, AX_PB_SYNC_BYTES / sizeof(u32));

        if (pvpb->updateCounter != 0) {
            CopyWords(pvpb->updateData, __AXUpdates[pvpb->index].data, pvpb->updateCounter);
        }
        return;
    }

    if (sync & AX_SYNC_FLAG_COPYSELECT) {
        ppbDsp->srcSelect = ppbUser->srcSelect;
        ppbDsp->coefSelect = ppbUser->coefSelect;
    }

    if (sync & AX_SYNC_FLAG_COPYMXRCTRL) {
        ppbDsp->mixerCtrl = ppbUser->mixerCtrl;
    }

    if (sync & AX_SYNC_FLAG_COPYSTATE) {
        ppbDsp->state = ppbUser->state;
    } else {
        ppbUser->state = ppbDsp->state;
    }

    if (sync & AX_SYNC_FLAG_COPYTYPE) {
        ppbDsp->type = ppbUser->type;
    }

    if (sync & AX_SYNC_FLAG_COPYAXPBMIX) {
        ppbDsp->mix = ppbUser->mix;
    }

    if (sync & AX_SYNC_FLAG_COPYTSHIFT) {
        ppbDsp->itd.targetShiftL = ppbUser->itd.targetShiftL;
        ppbDsp->itd.targetShiftR = ppbUser->itd.targetShiftR;
    } else if (sync & AX_SYNC_FLAG_COPYITD) {
        ppbDsp->itd = ppbUser->itd;
        ClearITDBuffer(pvpb->itdBuffer);
    }

    if (sync & AX_SYNC_FLAG_COPYUPDATE) {
        u32 i;

        /* The update buffer address is set when the voice is initialized. */
        for (i = 0; i < ARRAY_SIZEU(ppbDsp->update.updNum); i++) {
            ppbDsp->update.updNum[i] = ppbUser->update.updNum[i];
        }

        if (pvpb->updateCounter != 0) {
            CopyWords(__AXUpdates[pvpb->index].data, pvpb->updateData, pvpb->updateCounter);
        }
    }

    if (sync & AX_SYNC_FLAG_COPYDPOP) {
        ppbDsp->dpop = ppbUser->dpop;
    }

    if (sync & AX_SYNC_FLAG_SWAPVOL) {
        ppbUser->ve.currentVolume = ppbDsp->ve.currentVolume;
        ppbDsp->ve.currentDelta = ppbUser->ve.currentDelta;
    } else if (sync & AX_SYNC_USER_VE) {
        ppbDsp->ve.currentVolume = ppbUser->ve.currentVolume;
        ppbDsp->ve.currentDelta = ppbUser->ve.currentDelta;
    }

    if (sync & AX_SYNC_FLAG_COPYFIR) {
        ppbDsp->fir = ppbUser->fir;
    }

    if (sync & (AX_SYNC_FLAG_COPYLOOP | AX_SYNC_FLAG_COPYLOOPADDR | AX_SYNC_FLAG_COPYENDADDR | AX_SYNC_FLAG_COPYCURADDR)) {
        if (sync & AX_SYNC_FLAG_COPYLOOP) {
            ppbDsp->addr.loopFlag = ppbUser->addr.loopFlag;
        }
        if (sync & AX_SYNC_FLAG_COPYLOOPADDR) {
            ppbDsp->addr.loopAddress = ppbUser->addr.loopAddress;
        }
        if (sync & AX_SYNC_FLAG_COPYENDADDR) {
            ppbDsp->addr.endAddress = ppbUser->addr.endAddress;
        }
        if (sync & AX_SYNC_FLAG_COPYCURADDR) {
            ppbDsp->addr.currentAddress = ppbUser->addr.currentAddress;
        } else {
            ppbUser->addr.currentAddress = ppbDsp->addr.currentAddress;
        }
    } else if (sync & AX_SYNC_FLAG_COPYADDR) {
        ppbDsp->addr = ppbUser->addr;
    } else {
        ppbUser->addr.currentAddress.hi = ppbDsp->addr.currentAddress.hi;
        ppbUser->addr.currentAddress.lo = ppbDsp->addr.currentAddress.lo;
    }

    if (sync & AX_SYNC_FLAG_COPYADPCM) {
        ppbDsp->adpcm = ppbUser->adpcm;
    }

    if (sync & AX_SYNC_FLAG_COPYRATIO) {
        ppbDsp->src.ratioHi = ppbUser->src.ratioHi;
        ppbDsp->src.ratioLo = ppbUser->src.ratioLo;
    } else if (sync & AX_SYNC_FLAG_COPYSRC) {
        ppbDsp->src = ppbUser->src;
    }

    if (sync & AX_SYNC_FLAG_COPYADPCMLOOP) {
        ppbDsp->adpcmLoop = ppbUser->adpcmLoop;
    }

    if (sync & AX_SYNC_FLAG_COPYLPFCOEFS) {
        ppbDsp->lpf.a0 = ppbUser->lpf.a0;
        ppbDsp->lpf.b0 = ppbUser->lpf.b0;
        return;
    }

    if (sync & AX_SYNC_FLAG_COPYLPF) {
        ppbDsp->lpf = ppbUser->lpf;
    }
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
void __AXDumpVPB(AXVPB* pvpb) {
    AXPB* ppbDsp;

    ppbDsp = &__AXPB[pvpb->index];
    if (ppbDsp->state == 1) {
        __AXDepopVoice(ppbDsp);
    }
    pvpb->pb.state = ppbDsp->state = ppbDsp->update.updNum[0] 
        = ppbDsp->update.updNum[1] = ppbDsp->update.updNum[2] 
        = ppbDsp->update.updNum[3] = ppbDsp->update.updNum[4] = 0;
    __AXPushCallbackStack(pvpb);
}

/**
 * @note Address: 0x8017955C
 * @note Size: 0x278
 */
void __AXSyncPBs(u32 lessDspCycles) {
    u32 cycles;
    u32 i;
    AXVPB* pvpb;

    __AXNumVoices = 0;
    DCInvalidateRange(__AXPB, sizeof(__AXPB));
    DCInvalidateRange(__AXITD, sizeof(__AXITD));
    cycles = AX_FRAME_BASE_DSP_CYCLES + __AXGetCommandListCycles() + lessDspCycles;

    for (i = AX_PRIORITY_STACKS - 1; i != AX_FREE_VOICE_PRIORITY; i--) {
        for (pvpb = __AXGetStackHead(i); pvpb; pvpb = pvpb->next) {
            if (pvpb->depop != 0) {
                __AXDepopVoice(&__AXPB[pvpb->index]);
            }

            if ((pvpb->pb.state == 1) || (pvpb->updateCounter != 0)) {
                if (pvpb->pb.srcSelect != 2) {
                    cycles += __AXSrcCycles[pvpb->pb.src.ratioHi];
                }

                if (pvpb->pb.lpf.on) {
                    cycles += 555;
                }

                cycles += __AXMainMixCycles[pvpb->pb.mixerCtrl & 0xF] + __AXAuxMixCycles[(pvpb->pb.mixerCtrl >> 4) & 0x1F] + __AXAuxMixCycles[(pvpb->pb.mixerCtrl >> 9) & 0x1F] + 0x8C;
                if (__AXMaxDspCycles > cycles) {
                    __AXServiceVPB(pvpb);
                } else {
                    __AXDumpVPB(pvpb);
                }
            } else {
                __AXServiceVPB(pvpb);
            }

            pvpb->sync = 0;
            pvpb->depop = 0;
            pvpb->updateMS = pvpb->updateCounter = 0;
            pvpb->updateWrite = pvpb->updateData;
        }
    }

    __AXRecDspCycles = cycles;
    for (pvpb = __AXGetStackHead(AX_FREE_VOICE_PRIORITY); pvpb; pvpb = pvpb->next) {
        if (pvpb->depop != 0) {
            __AXDepopVoice(&__AXPB[pvpb->index]);
        }
        pvpb->depop = 0;
        __AXPB[pvpb->index].state = __AXPB[pvpb->index].update.updNum[0] = __AXPB[pvpb->index].update.updNum[1] 
                                  = __AXPB[pvpb->index].update.updNum[2] = __AXPB[pvpb->index].update.updNum[3] 
                                  = __AXPB[pvpb->index].update.updNum[4] = 0;
    }

    DCFlushRange(__AXPB, sizeof(__AXPB));
    DCFlushRange(__AXITD, sizeof(__AXITD));
    DCFlushRange(__AXUpdates, sizeof(__AXUpdates));
}

/**
 * @note Address: 0x801797D4
 * @note Size: 0xC
 */
AXPB* __AXGetPBs(void) {
    return __AXPB;
}

/**
 * @note Address: 0x801797E0
 * @note Size: 0x44
 */
void __AXSetPBDefault(AXVPB* p) {
    p->pb.state = 0;
    p->pb.itd.flag = 0;
    p->sync = AX_SYNC_FLAG_COPYLPF | AX_SYNC_FLAG_COPYUPDATE | AX_SYNC_FLAG_COPYITD | AX_SYNC_FLAG_COPYSTATE;
    p->updateMS = p->updateCounter = 0;
    p->updateWrite = p->updateData;
    p->pb.update.updNum[0] = p->pb.update.updNum[1] = p->pb.update.updNum[2] = p->pb.update.updNum[3] = p->pb.update.updNum[4] = 0;
    p->pb.lpf.on = 0;
}

/**
 * @note Address: 0x80179824
 * @note Size: 0x204
 */
void __AXVPBInit(void) {
    u32 i;
    AXPB* ppb;
    AXPBITDBUFFER* ppbi;
    AXPBU* ppbu;
    AXVPB* pvpb;

#if DEBUG
    OSReport("Initializing AXVPB code module\n");
#endif
    __AXMaxDspCycles = OS_BUS_CLOCK / 400;
    __AXRecDspCycles = 0;

    // The DSP buffers are aligned at their bases.
    ASSERTLINE(913, OFFSET(__AXPB, AX_DSP_BUFFER_ALIGNMENT) == 0);
    ASSERTLINE(914, OFFSET(__AXITD, AX_DSP_BUFFER_ALIGNMENT) == 0);
    ASSERTLINE(915, OFFSET(__AXUpdates, AX_DSP_BUFFER_ALIGNMENT) == 0);

    memset(__AXPB, 0, sizeof(__AXPB));
    memset(__AXITD, 0, sizeof(__AXITD));
    memset(__AXVPB, 0, sizeof(__AXVPB));

    for (i = 0; i < AX_MAX_VOICES; i++) {
        ppb = &__AXPB[i];
        ppbi = &__AXITD[i];
        ppbu = &__AXUpdates[i];
        pvpb = &__AXVPB[i];

        pvpb->index = i;
        pvpb->updateWrite = pvpb->updateData;
        pvpb->itdBuffer = ppbi;
        __AXSetPBDefault(pvpb);

        if (i == AX_MAX_VOICES - 1) {
            pvpb->pb.nextHi = pvpb->pb.nextLo = ppb->nextHi = ppb->nextLo = 0;
        } else {
            pvpb->pb.nextHi = (u16)((u32)(ppb + 1) >> 16);
            pvpb->pb.nextLo = (u16)(u32)(ppb + 1);
            ppb->nextHi = (u16)((u32)(ppb + 1) >> 16);
            ppb->nextLo = (u16)(u32)(ppb + 1);
        }

        pvpb->pb.currHi = (u16)((u32)ppb >> 16);
        pvpb->pb.currLo = (u16)(u32)ppb;
        ppb->currHi = (u16)((u32)ppb >> 16);
        ppb->currLo = (u16)(u32)ppb;
        pvpb->pb.itd.bufferHi = (u16)((u32)ppbi >> 16);
        pvpb->pb.itd.bufferLo = (u16)(u32)ppbi;
        ppb->itd.bufferHi = (u16)((u32)ppbi >> 16);
        ppb->itd.bufferLo = (u16)(u32)ppbi;
        pvpb->pb.update.dataHi = (u16)((u32)ppbu >> 16);
        pvpb->pb.update.dataLo = (u16)(u32)ppbu;
        ppb->update.dataHi = (u16)((u32)ppbu >> 16);
        ppb->update.dataLo = (u16)(u32)ppbu;

        pvpb->priority = AX_FREE_VOICE_PRIORITY + 1;
        __AXPushFreeStack(pvpb);
    }

    DCFlushRange(__AXPB, sizeof(__AXPB));
}

/**
 * @note Address: 0x80179A28
 * @note Size: 0x4
 */
void __AXVPBQuit(void) {
#if DEBUG
    OSReport("Shutting down AXVPB code module\n");
#endif
}

/**
 * @note Address: 0x80179A2C
 * @note Size: 0xC4
 */
void AXSetVoiceSrcType(AXVPB* p, u32 type) {
    BOOL old;
    AXPB* ppb;

    ASSERTLINE(1020, p);
    ASSERTLINE(1021, type <= AX_SRC_TYPE_4TAP_16K);

    old = OSDisableInterrupts();
    ppb = &p->pb;
    switch(type) {
    case AX_SRC_TYPE_NONE:
        ppb->srcSelect = 2;
        break;
    case AX_SRC_TYPE_LINEAR:
        ppb->srcSelect = 1;
        break;
    case AX_SRC_TYPE_4TAP_8K:
        ppb->srcSelect = 0;
        ppb->coefSelect = 0;
        break;
    case AX_SRC_TYPE_4TAP_12K:
        ppb->srcSelect = 0;
        ppb->coefSelect = 1;
        break;
    case AX_SRC_TYPE_4TAP_16K:
        ppb->srcSelect = 0;
        ppb->coefSelect = 2;
        break;
    }

    p->sync |= AX_SYNC_FLAG_COPYSELECT;
    OSRestoreInterrupts(old);
}

/**
 * @note Address: 0x80179AF0
 * @note Size: 0x5C
 */
void AXSetVoiceState(AXVPB* p, u16 state) {
    BOOL old;

    old = OSDisableInterrupts();
    p->pb.state = state;
    p->sync |= AX_SYNC_FLAG_COPYSTATE;
    if (state == 0) {
        p->depop = 1;
    }
    OSRestoreInterrupts(old);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
void AXSetVoiceType(AXVPB* p, u16 type) {
    BOOL old;

    old = OSDisableInterrupts();
    p->pb.type = type;
    p->sync |= AX_SYNC_FLAG_COPYTYPE;
    OSRestoreInterrupts(old);
}

/* DSP mixer control bits and the corresponding volume ramps. */
enum {
    AX_MIX_MAIN_L         = 0x0001,
    AX_MIX_MAIN_R         = 0x0002,
    AX_MIX_MAIN_S         = 0x0004,
    AX_MIX_MAIN_RAMP      = 0x0008,
    AX_MIX_AUX_A_L        = 0x0010,
    AX_MIX_AUX_A_R        = 0x0020,
    AX_MIX_AUX_A_LR_RAMP  = 0x0040,
    AX_MIX_AUX_A_S        = 0x0080,
    AX_MIX_AUX_A_S_RAMP   = 0x0100,
    AX_MIX_AUX_B_L        = 0x0200,
    AX_MIX_AUX_B_R        = 0x0400,
    AX_MIX_AUX_B_LR_RAMP  = 0x0800,
    AX_MIX_AUX_B_S        = 0x1000,
    AX_MIX_AUX_B_S_RAMP   = 0x2000,
};

/**
 * @note Address: 0x80179B4C
 * @note Size: 0x254
 */
void AXSetVoiceMix(AXVPB* p, AXPBMIX* mix) {
    BOOL old;
    u16 mixerCtrl;
    u16* dst;
    const u16* src;

    src = (const u16*)mix;
    dst = (u16*)&p->pb.mix;
    mixerCtrl = 0;

    old = OSDisableInterrupts();

#define COPY_MIX_VOLUME(bits) \
    do { \
        if ((*dst++ = *src++) != 0) { \
            mixerCtrl |= (bits); \
        } \
    } while (0)

    /* AXPBMIX stores each volume beside its ramp delta. */
    COPY_MIX_VOLUME(AX_MIX_MAIN_L);
    COPY_MIX_VOLUME(AX_MIX_MAIN_L | AX_MIX_MAIN_RAMP);
    COPY_MIX_VOLUME(AX_MIX_MAIN_R);
    COPY_MIX_VOLUME(AX_MIX_MAIN_R | AX_MIX_MAIN_RAMP);
    COPY_MIX_VOLUME(AX_MIX_AUX_A_L);
    COPY_MIX_VOLUME(AX_MIX_AUX_A_L | AX_MIX_AUX_A_LR_RAMP);
    COPY_MIX_VOLUME(AX_MIX_AUX_A_R);
    COPY_MIX_VOLUME(AX_MIX_AUX_A_R | AX_MIX_AUX_A_LR_RAMP);
    COPY_MIX_VOLUME(AX_MIX_AUX_B_L);
    COPY_MIX_VOLUME(AX_MIX_AUX_B_L | AX_MIX_AUX_B_LR_RAMP);
    COPY_MIX_VOLUME(AX_MIX_AUX_B_R);
    COPY_MIX_VOLUME(AX_MIX_AUX_B_R | AX_MIX_AUX_B_LR_RAMP);
    COPY_MIX_VOLUME(AX_MIX_AUX_B_S);
    COPY_MIX_VOLUME(AX_MIX_AUX_B_S | AX_MIX_AUX_B_S_RAMP);
    COPY_MIX_VOLUME(AX_MIX_MAIN_S);
    COPY_MIX_VOLUME(AX_MIX_MAIN_S | AX_MIX_MAIN_RAMP);
    COPY_MIX_VOLUME(AX_MIX_AUX_A_S);
    COPY_MIX_VOLUME(AX_MIX_AUX_A_S | AX_MIX_AUX_A_S_RAMP);

#undef COPY_MIX_VOLUME

    p->pb.mixerCtrl = mixerCtrl;
    p->sync |= (AX_SYNC_FLAG_COPYAXPBMIX | AX_SYNC_FLAG_COPYMXRCTRL);
    OSRestoreInterrupts(old);
}

/**
 * @note Address: 0x80179DA0
 * @note Size: 0x64
 */
void AXSetVoiceItdOn(AXVPB* p) {
    BOOL old;

    old = OSDisableInterrupts();
    p->pb.itd.flag = 1;
    p->pb.itd.shiftL = p->pb.itd.shiftR = p->pb.itd.targetShiftL = p->pb.itd.targetShiftR = 0;
    p->sync &= ~(AX_SYNC_FLAG_COPYTSHIFT);
    p->sync |= AX_SYNC_FLAG_COPYITD;
    OSRestoreInterrupts(old);
}

/**
 * @note Address: 0x80179E04
 * @note Size: 0x5C
 */
void AXSetVoiceItdTarget(AXVPB* p, u16 lShift, u16 rShift) {
    BOOL old;

    old = OSDisableInterrupts();
    p->pb.itd.targetShiftL = lShift;
    p->pb.itd.targetShiftR = rShift;
    p->sync |= AX_SYNC_FLAG_COPYTSHIFT;
    OSRestoreInterrupts(old);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
void AXSetVoiceUpdateIncrement(AXVPB* p) {
    BOOL old;

    old = OSDisableInterrupts();
    p->updateMS++;
    p->sync |= AX_SYNC_FLAG_COPYUPDATE;
    ASSERTMSGLINE(1191, p->updateMS < ARRAY_SIZEU(p->pb.update.updNum), "PB updates cannot exceed 5ms\n");
    OSRestoreInterrupts(old);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
void AXSetVoiceUpdateWrite(AXVPB* p, u16 param, u16 data) {
    BOOL old;

    old = OSDisableInterrupts();
    p->updateCounter += 2;
    ASSERTMSGLINE(1205, p->updateCounter <= ARRAY_SIZEU(p->updateData), "PB update block exceeded 128 words\n");

    *p->updateWrite = param;
    p->updateWrite++;
    *p->updateWrite = data;
    p->updateWrite++;
    p->sync |= AX_SYNC_FLAG_COPYUPDATE;
    OSRestoreInterrupts(old);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
void AXSetVoiceDpop(AXVPB* p, AXPBDPOP* dpop) {
    BOOL old = OSDisableInterrupts();

    p->pb.dpop = *dpop;
    p->sync |= AX_SYNC_FLAG_COPYDPOP;
    OSRestoreInterrupts(old);
}

/**
 * @note Address: 0x80179E60
 * @note Size: 0x58
 */
void AXSetVoiceVe(AXVPB* p, AXPBVE* ve) {
    BOOL old;

    old = OSDisableInterrupts();
    p->pb.ve.currentVolume = ve->currentVolume;
    p->pb.ve.currentDelta = ve->currentDelta;
    p->sync |= AX_SYNC_USER_VE;
    OSRestoreInterrupts(old);
}

/**
 * @note Address: 0x80179EB8
 * @note Size: 0x4C
 */
void AXSetVoiceVeDelta(AXVPB* p, s16 delta) {
    BOOL old;

    old = OSDisableInterrupts();
    p->pb.ve.currentDelta = delta;
    p->sync |= AX_SYNC_FLAG_SWAPVOL;
    OSRestoreInterrupts(old);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
void AXSetVoiceFir(AXVPB* p, AXPBFIR* fir) {
    BOOL old;

    old = OSDisableInterrupts();
    p->pb.fir.numCoefs = fir->numCoefs;
    p->pb.fir.coefsHi = fir->coefsHi;
    p->pb.fir.coefsLo = fir->coefsLo;
    p->sync |= AX_SYNC_FLAG_COPYFIR;
    OSRestoreInterrupts(old);
}

#define AX_PB_FORMAT_ADPCM 0x00
#define AX_PB_FORMAT_PCM16 0x0A
#define AX_PB_FORMAT_PCM8  0x19

#define AX_PCM16_GAIN 0x0800U
#define AX_PCM8_GAIN  0x0100U

/**
 * @note Address: 0x80179F04
 * @note Size: 0x110
 */
void AXSetVoiceAddr(AXVPB* p, AXPBADDR* addr) {
    BOOL old;
    static const AXPBADPCM pcm16State = { { { 0 } }, AX_PCM16_GAIN };
    static const AXPBADPCM pcm8State = { { { 0 } }, AX_PCM8_GAIN };

    old = OSDisableInterrupts();
    p->pb.addr = *addr;

    /* The PCM decoder state follows the address block in the DSP layout. */
    switch (addr->format) {
    case AX_PB_FORMAT_ADPCM:
        ASSERTMSGLINE(1335, (addr->loopAddress.lo & 0xF) > 1, "*** loop address on ADPCM frame header! ***\n");
        ASSERTMSGLINE(1340, (addr->endAddress.lo & 0xF) > 1, "*** end address on ADPCM frame header! ***\n");
        ASSERTMSGLINE(1345, (addr->currentAddress.lo & 0xF) > 1, "*** current address on ADPCM frame header! ***\n");
        break;
    case AX_PB_FORMAT_PCM16:
        p->pb.adpcm = pcm16State;
        break;
    case AX_PB_FORMAT_PCM8:
        p->pb.adpcm = pcm8State;
        break;
    default:
        ASSERTMSGLINE(1389, 0, "unknown addr->formaqt in PB\n");
        break;
    }

    p->sync &= ~(AX_SYNC_FLAG_COPYLOOP | AX_SYNC_FLAG_COPYLOOPADDR | AX_SYNC_FLAG_COPYENDADDR | AX_SYNC_FLAG_COPYCURADDR);
    p->sync |= (AX_SYNC_FLAG_COPYADDR | AX_SYNC_FLAG_COPYADPCM);
    OSRestoreInterrupts(old);
}

/**
 * @note Address: 0x8017A014
 * @note Size: 0x4C
 */
void AXSetVoiceLoop(AXVPB* p, u16 loop) {
    BOOL old;

    old = OSDisableInterrupts();
    p->pb.addr.loopFlag = loop;
    p->sync |= AX_SYNC_FLAG_COPYLOOP;
    OSRestoreInterrupts(old);
}

/**
 * @note Address: 0x8017A060
 * @note Size: 0x54
 */
void AXSetVoiceLoopAddr(AXVPB* p, u32 addr) {
    BOOL old;

    old = OSDisableInterrupts();
    p->pb.addr.loopAddress.hi = (addr >> 0x10);
    p->pb.addr.loopAddress.lo = (addr);
    p->sync |= AX_SYNC_FLAG_COPYLOOPADDR;
    OSRestoreInterrupts(old);
}

/**
 * @note Address: 0x8017A0B4
 * @note Size: 0x54
 */
void AXSetVoiceEndAddr(AXVPB* p, u32 addr) {
    BOOL old;

    old = OSDisableInterrupts();
    p->pb.addr.endAddress.hi = (addr >> 0x10);
    p->pb.addr.endAddress.lo = (addr);
    p->sync |= AX_SYNC_FLAG_COPYENDADDR;
    OSRestoreInterrupts(old);
}

/**
 * @note Address: 0x8017A108
 * @note Size: 0x54
 */
void AXSetVoiceCurrentAddr(AXVPB* p, u32 addr) {
    BOOL old;

    old = OSDisableInterrupts();
    p->pb.addr.currentAddress.hi = (addr >> 0x10);
    p->pb.addr.currentAddress.lo = (addr);
    p->sync |= AX_SYNC_FLAG_COPYCURADDR;
    OSRestoreInterrupts(old);
}

/**
 * @note Address: 0x8017A15C
 * @note Size: 0xA4
 */
void AXSetVoiceAdpcm(AXVPB* p, AXPBADPCM* adpcm) {
    BOOL old = OSDisableInterrupts();

    p->pb.adpcm = *adpcm;
    p->sync |= AX_SYNC_FLAG_COPYADPCM;
    OSRestoreInterrupts(old);
}

/**
 * @note Address: 0x8017A200
 * @note Size: 0x98
 */
void AXSetVoiceSrc(AXVPB* p, AXPBSRC* src_) {
    BOOL old = OSDisableInterrupts();

    p->pb.src = *src_;
    p->sync &= ~AX_SYNC_FLAG_COPYRATIO;
    p->sync |= AX_SYNC_FLAG_COPYSRC;
    OSRestoreInterrupts(old);
}

/**
 * @note Address: 0x8017A298
 * @note Size: 0x98
 */
void AXSetVoiceSrcRatio(AXVPB* p, float ratio) {
    u32 r;
    BOOL old;

    old = OSDisableInterrupts();
    r = 65536.0f* ratio;
    if (r > 0x40000) {
        r = 0x40000;
    }
    p->pb.src.ratioHi = ((u32)r >> 0x10);
    p->pb.src.ratioLo = ((u32)r);
    p->sync |= AX_SYNC_FLAG_COPYRATIO;
    OSRestoreInterrupts(old);
}

/**
 * @note Address: 0x8017A330
 * @note Size: 0x6C
 */
void AXSetVoiceAdpcmLoop(AXVPB* p, AXPBADPCMLOOP* adpcmloop) {
    BOOL old = OSDisableInterrupts();

    p->pb.adpcmLoop = *adpcmloop;
    p->sync |= AX_SYNC_FLAG_COPYADPCMLOOP;
    OSRestoreInterrupts(old);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
void AXSetVoiceLpf(AXVPB* p, AXPBLPF* lpf) {
    BOOL old = OSDisableInterrupts();

    p->pb.lpf = *lpf;
    p->sync |= AX_SYNC_FLAG_COPYLPF;

    OSRestoreInterrupts(old);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
void AXSetVoiceLpfCoefs(AXVPB* p, u16 a0, u16 b0) {
    BOOL old;
    old = OSDisableInterrupts();

    p->pb.lpf.a0 = a0;
    p->pb.lpf.b0 = b0;
    p->sync |= AX_SYNC_FLAG_COPYLPFCOEFS;

    OSRestoreInterrupts(old);
}

static inline float AXLPFSqrt(float x)
{
    static const double half = 0.5;
    static const double three = 3.0;

    if (x > 0.0f) {
        double estimate = __frsqrte((double)x);
        estimate = half * estimate * (three - estimate * estimate * x);
        estimate = half * estimate * (three - estimate * estimate * x);
        estimate = half * estimate * (three - estimate * estimate * x);
        estimate = half * estimate * (three - estimate * estimate * x);
        return (float)(x * estimate);
    }
    return x;
}

#define PI 3.14159265358979323846f

/**
 * @note Address: N/A
 * @note Size: ?
 */
void AXGetLpfCoefs(u16 freq, u16* a0, u16* b0) {
    f32 bb;
    f32 cc;

    ASSERTMSGLINE(1616, freq <= 16000, "freq is out of range");

    cc = 2.0f - cosf(((PI * 2) * (f32)freq) / 32000.0f);
    bb = AXLPFSqrt((cc * cc) - 1.0f) - cc;

    *b0 = 32768.0f * -bb;
    *a0 = 0x7FFF - *b0;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
void AXSetMaxDspCycles(u32 cycles) {
    __AXMaxDspCycles = cycles;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
u32 AXGetMaxDspCycles(void) {
    return __AXMaxDspCycles;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
u32 AXGetDspCycles(void) {
    return __AXRecDspCycles;
}
