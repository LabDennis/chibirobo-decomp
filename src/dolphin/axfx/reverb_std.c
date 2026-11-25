#include <dolphin/dolphin.h>
#include <dolphin/ax.h>
#include <dolphin/axfx.h>
#include <cstring>

#include "PowerPC_EABI_Support/MSL_C/PPC_EABI/math_ppc.h"

#include "__ax.h"
#include "__axfx.h"
#include "__axfx_reverb.h"

// prototypes
static void DLsetdelay(AXFX_REVSTD_DELAYLINE* dl, s32 lag);
static int DLcreate(AXFX_REVSTD_DELAYLINE* dl, s32 max_length);
static void DLdelete(AXFX_REVSTD_DELAYLINE* dl);
static int ReverbSTDCreate(AXFX_REVSTD_WORK* rv, f32 coloration, f32 time, f32 mix, f32 damping, f32 predelay);
static int ReverbSTDModify(AXFX_REVSTD_WORK* rv, f32 coloration, f32 time, f32 mix, f32 damping, f32 predelay);
static void ReverbSTDFree(AXFX_REVSTD_WORK* rv);

/**
 * @note Address: N/A
 * @note Size: ?
 */
static void DLsetdelay(AXFX_REVSTD_DELAYLINE* dl, s32 lag) {
    dl->outPoint = dl->inPoint - (lag * 4);
    while (dl->outPoint < 0) {
        dl->outPoint += dl->length;
    }
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static int DLcreate(AXFX_REVSTD_DELAYLINE* dl, s32 max_length) {
    dl->length = (max_length * 4);
    dl->inputs = __AXFXAlloc(max_length << 2);
	ASSERTMSGLINE(49, dl->inputs, "Can't allocate the memory.");
	if (dl->inputs == NULL) {
		return 0;
	}

    memset(dl->inputs, 0, max_length << 2);
    dl->lastOutput = 0.0f;
    DLsetdelay(dl, max_length >> 1);
    dl->inPoint = 0;
    dl->outPoint = 0;
	return 1;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static void DLdelete(AXFX_REVSTD_DELAYLINE* dl) {
    __AXFXFree(dl->inputs);
}

/**
 * @note Address: 0x8017B2EC
 * @note Size: 0x47C
 */
static int ReverbSTDCreate(AXFX_REVSTD_WORK* rv, f32 coloration, f32 time, f32 mix, f32 damping, f32 predelay) {
    u8 i;
    u8 k;
    static s32 lens[4] = {
        0x000006FD,
        0x000007CF,
        0x000001B1,
        0x00000095,
    };

    memset(rv, 0, sizeof(AXFX_REVSTD_WORK));

	ASSERTMSGLINE(109, coloration >= 0.0f && coloration <= 1.0f &&
				  time >= 0.01f && time <= 10.0f &&
				  mix >= 0.0f && mix <= 1.0f &&
				  damping >= 0.0f && damping <= 1.0f &&
				  predelay >= 0.0f && predelay <= 0.1f,
				  "The value of specified parameter is out of range.");

    if (!(coloration >= 0.0f && coloration <= 1.0f &&
          time >= 0.01f && time <= 10.0f &&
          mix >= 0.0f && mix <= 1.0f &&
          damping >= 0.0f && damping <= 1.0f &&
          predelay >= 0.0f && predelay <= 0.1f)) {
        return 0;
    }

    for (k = 0; k < 3; k++) {
        for (i = 0; i < 2; i++) {
            if (DLcreate(&rv->C[i + (k * 2)], lens[i] + 2) == 0) {
				ReverbSTDFree(rv);
				return 0;
			}
            DLsetdelay(&rv->C[i + (k * 2)], lens[i]);
            rv->combCoef[i + (k * 2)] = powf(10.0f, (lens[i] * -3) / (32000.0f * time));
        }

        for (i = 0; i < 2; i++) {
            if (DLcreate(&rv->AP[i + (k * 2)], lens[i + 2] + 2) == 0) {
				ReverbSTDFree(rv);
				return 0;
			}
            DLsetdelay(&rv->AP[i + (k * 2)], lens[i + 2]);
        }
        rv->lpLastout[k] = 0.0f;
    }

    rv->allPassCoeff = coloration;
    rv->level = mix;
    rv->damping = damping;
    if (rv->damping < 0.05f) {
        rv->damping = 0.05f;
    }
    rv->damping = (1.0f - (0.05f + (0.8f * rv->damping)));

    rv->preDelayTime = (s32)(32000.0f * predelay);
    if (rv->preDelayTime != 0) {
        for (i = 0; i < 3; i++) {
            rv->preDelayLine[i] = __AXFXAlloc(rv->preDelayTime * 4);
            ASSERTMSGLINE(162, rv->preDelayLine[i], "Can't allocate the memory.");
			if (rv->preDelayLine[i] == NULL) {
				ReverbSTDFree(rv);
				return 0;
			}
			
			memset(rv->preDelayLine[i], 0, rv->preDelayTime * 4);
            rv->preDelayPtr[i] = rv->preDelayLine[i];
        }
    }

    return 1;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static int ReverbSTDModify(AXFX_REVSTD_WORK* rv, f32 coloration, f32 time, f32 mix, f32 damping, f32 predelay) {
	ASSERTMSGLINE(196, coloration >= 0.0f && coloration <= 1.0f &&
				  time >= 0.01f && time <= 10.0f &&
				  mix >= 0.0f && mix <= 1.0f &&
				  damping >= 0.0f && damping <= 1.0f &&
				  predelay >= 0.0f && predelay <= 0.1f,
				  "The value of specified parameter is out of range.");

    if (!(coloration >= 0.0f && coloration <= 1.0f &&
          time >= 0.01f && time <= 10.0f &&
          mix >= 0.0f && mix <= 1.0f &&
          damping >= 0.0f && damping <= 1.0f &&
          predelay >= 0.0f && predelay <= 0.1f)) {
        return 0;
    }

    ReverbSTDFree(rv);
    return ReverbSTDCreate(rv, coloration, time, mix, damping, predelay);
}

static inline f32 DelayLinePush(AXFX_REVSTD_DELAYLINE* line, f32 sample)
{
    line->inputs[line->inPoint / sizeof(f32)] = sample;
    line->lastOutput = line->inputs[line->outPoint / sizeof(f32)];

    line->inPoint += sizeof(f32);
    if (line->inPoint == line->length) {
        line->inPoint = 0;
    }
    line->outPoint += sizeof(f32);
    if (line->outPoint == line->length) {
        line->outPoint = 0;
    }
    return line->lastOutput;
}

static inline f32 ApplyAllPass(AXFX_REVSTD_DELAYLINE* line, f32 coefficient, f32 sample)
{
    f32 previous = line->lastOutput;
    f32 feedback = __fmadds(coefficient, previous, sample);

    DelayLinePush(line, feedback);
    return __fnmsubs(coefficient, feedback, previous);
}

/**
 * @note Original HandleReverb address: 0x8017B768
 */
static void ProcessChannel(s32* samples, AXFX_REVSTD_WORK* rv, int channel)
{
    AXFX_REVSTD_DELAYLINE* comb = &rv->C[2 * channel];
    AXFX_REVSTD_DELAYLINE* allPass = &rv->AP[2 * channel];
    const f32* combCoefficients = &rv->combCoef[2 * channel];
    f32 allPassCoefficient = rv->allPassCoeff;
    f32 damping = rv->damping;
    f32 wetGain = rv->level * 0.6f;
    f32 dryGain = 0.6f - wetGain;
    f32 lowPass = rv->lpLastout[channel];
    f32* preDelayStart = rv->preDelayLine[channel];
    f32* preDelay = rv->preDelayPtr[channel];
    f32* preDelayEnd = AXFXPreDelayEnd(preDelayStart, rv->preDelayTime);
    int i;

    for (i = 0; i < AX_SAMPLES_PER_FRAME; i++) {
        f32 dry = (f32)samples[i];
        f32 input = dry;
        f32 comb0;
        f32 comb1;
        f32 wet;

        if (preDelayEnd != NULL) {
            input = *preDelay;
            *preDelay++ = dry;
            if (preDelay == preDelayEnd) {
                preDelay = preDelayStart;
            }
        }

        comb0 = DelayLinePush(&comb[0], __fmadds(combCoefficients[0], comb[0].lastOutput, input));
        comb1 = DelayLinePush(&comb[1], __fmadds(combCoefficients[1], comb[1].lastOutput, input));
        wet = ApplyAllPass(&allPass[0], allPassCoefficient, comb0 + comb1);
        lowPass = __fmadds(damping, lowPass, wet * 0.3f);
        wet = ApplyAllPass(&allPass[1], allPassCoefficient, lowPass);
        samples[i] = AXFXTruncateSample(__fmadds(wetGain, wet, dryGain * dry));
    }

    rv->lpLastout[channel] = lowPass;
    rv->preDelayPtr[channel] = preDelay;
}

/**
 * @note Address: 0x8017BB1C
 * @note Size: 0x124
 */
static void ReverbSTDFree(AXFX_REVSTD_WORK* rv) {
    u8 i;

    for (i = 0; i < 6; i++) {
		if (rv->AP[i].inputs != 0) {
			DLdelete(&rv->AP[i]);
			rv->AP[i].inputs = NULL;
		}
    }

    for (i = 0; i < 6; i++) {
        if (rv->C[i].inputs != 0) {
			DLdelete(&rv->C[i]);
			rv->C[i].inputs = NULL;
		}
    }

    if (rv->preDelayTime) {
        for (i = 0; i < 3; i++) {
            if (rv->preDelayLine[i] != 0) {
				__AXFXFree(rv->preDelayLine[i]);
				rv->preDelayLine[i] = NULL;
			}
        }
    }
}

/**
 * @note Address: 0x8017BC40
 * @note Size: 0x6C
 */
int AXFXReverbStdInit(AXFX_REVERBSTD* rev) {
    int ret;
    BOOL old;

    old = OSDisableInterrupts();
    rev->tempDisableFX = 1;
    ret = ReverbSTDCreate(&rev->rv, rev->coloration, rev->time, rev->mix, rev->damping, rev->preDelay);
    rev->tempDisableFX = !ret;
    OSRestoreInterrupts(old);
    return ret;
}

/**
 * @note Address: 0x8017BCAC
 * @note Size: 0x4C
 */
int AXFXReverbStdShutdown(AXFX_REVERBSTD* rev) {
    BOOL old;

    old = OSDisableInterrupts();
    rev->tempDisableFX = 1;
    ReverbSTDFree(&rev->rv);
    OSRestoreInterrupts(old);
    return 1;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
int AXFXReverbStdSettings(AXFX_REVERBSTD* rev) {
    int ret;
    BOOL old;

    old = OSDisableInterrupts();
    rev->tempDisableFX = 1;
    ret = ReverbSTDModify(&rev->rv, rev->coloration, rev->time, rev->mix, rev->damping, rev->preDelay);
    rev->tempDisableFX = rev->rv.C[0].inputs == NULL;
    OSRestoreInterrupts(old);
    return ret;
}

/**
 * @note Address: 0x8017BCF8
 * @note Size: 0x30
 */
void AXFXReverbStdCallback(AXFX_BUFFERUPDATE* bufferUpdate, AXFX_REVERBSTD* reverb) {
    if (reverb->tempDisableFX == 0) {
        ProcessChannel(bufferUpdate->left, &reverb->rv, 0);
        ProcessChannel(bufferUpdate->right, &reverb->rv, 1);
        ProcessChannel(bufferUpdate->surround, &reverb->rv, 2);
    }
}
