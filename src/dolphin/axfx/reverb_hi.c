#include <dolphin/dolphin.h>
#include <dolphin/ax.h>
#include <dolphin/axfx.h>
#include <cstring>

#include "PowerPC_EABI_Support/MSL_C/PPC_EABI/math_ppc.h"

#include "__ax.h"
#include "__axfx.h"
#include "__axfx_reverb.h"

// prototypes
static void DLsetdelay(AXFX_REVHI_DELAYLINE* dl, s32 lag);
static int DLcreate(AXFX_REVHI_DELAYLINE* dl, s32 max_length);
static void DLdelete(AXFX_REVHI_DELAYLINE* dl);
static int ReverbHICreate(AXFX_REVHI_WORK* rv, f32 coloration, f32 time, f32 mix, f32 damping, f32 preDelay, f32 crosstalk);
static int ReverbHIModify(AXFX_REVHI_WORK* rv, f32 coloration, f32 time, f32 mix, f32 damping, f32 preDelay, f32 crosstalk);
static void ReverbHIFree(AXFX_REVHI_WORK* rv);

/**
 * @note Address: N/A
 * @note Size: ?
 */
static void DLsetdelay(AXFX_REVHI_DELAYLINE* dl, s32 lag) {
    dl->outPoint = dl->inPoint - (lag * 4);
    while (dl->outPoint < 0) {
        dl->outPoint += dl->length;
    }
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static int DLcreate(AXFX_REVHI_DELAYLINE* dl, s32 max_length) {
    dl->length = (max_length * 4);
    dl->inputs = __AXFXAlloc(max_length << 2);
	ASSERTMSGLINE(51, dl->inputs, "Can't allocate the memory.");
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
static void DLdelete(AXFX_REVHI_DELAYLINE* dl) {
    __AXFXFree(dl->inputs);
}

/**
 * @note Address: 0x8017A3E4
 * @note Size: 0x588
 */
static int ReverbHICreate(AXFX_REVHI_WORK* rv, f32 coloration, f32 time, f32 mix, f32 damping, f32 preDelay, f32 crosstalk) {
    u8 i;
    u8 k;
    static s32 lens[8] = {
        0x000006FD,
        0x000007CF,
        0x0000091D,
        0x000001B1,
        0x00000095,
        0x0000002F,
        0x00000049,
        0x00000043
    };

    memset(rv, 0, sizeof(AXFX_REVHI_WORK));

	ASSERTMSGLINE(105, coloration >= 0.0f && coloration <= 1.0f &&
				  time >= 0.01f && time <= 10.0f &&
				  mix >= 0.0f && mix <= 1.0f &&
				  crosstalk >= 0.0f && crosstalk <= 1.0f &&
				  damping >= 0.0f && damping <= 1.0f &&
				  preDelay >= 0.0f && preDelay <= 0.1f,
				  "The value of specified parameter is out of range.");

    if (!(coloration >= 0.0f && coloration <= 1.0f &&
          time >= 0.01f && time <= 10.0f &&
          mix >= 0.0f && mix <= 1.0f &&
          crosstalk >= 0.0f && crosstalk <= 1.0f &&
          damping >= 0.0f && damping <= 1.0f &&
          preDelay >= 0.0f && preDelay <= 0.1f)) {
        return 0;
    }

    for (k = 0; k < 3; k++) {
        for (i = 0; i < 3; i++) {
            if (DLcreate(&rv->C[i + (k * 3)], lens[i] + 2) == 0) {
				ReverbHIFree(rv);
				return 0;
			}

            DLsetdelay(&rv->C[i + (k * 3)], lens[i]);
            rv->combCoef[i + (k * 3)] = powf(10.0f, (lens[i] * -3) / (32000.0f * time));
        }

        for (i = 0; i < 2; i++) {
            if (DLcreate(&rv->AP[i + (k * 3)], lens[i + 3] + 2) == 0) {
				ReverbHIFree(rv);
				return 0;
			}
            DLsetdelay(&rv->AP[i + (k * 3)], lens[i + 3]);
        }

        if (DLcreate(&rv->AP[2 + (k * 3)], lens[k + 5] + 2) == 0) {
			ReverbHIFree(rv);
			return 0;
		}
        DLsetdelay(&rv->AP[2 + (k * 3)], lens[k + 5]);
        rv->lpLastout[k] = 0.0f;
    }

    rv->allPassCoeff = coloration;
    rv->level = mix;
    rv->crosstalk = crosstalk;
    rv->damping = damping;
    if (rv->damping < 0.05f) {
        rv->damping = 0.05f;
    }
    rv->damping = (1.0f - (0.05f + (0.8f * rv->damping)));

    rv->preDelayTime = (s32)(32000.0f * preDelay);
    if (rv->preDelayTime != 0) {
        for(i = 0; i < 3; i++) {
            rv->preDelayLine[i] = __AXFXAlloc(rv->preDelayTime * 4);
			ASSERTMSGLINE(173, rv->preDelayLine[i], "Can't allocate the memory.");
			if (rv->preDelayLine[i] == NULL) {
				ReverbHIFree(rv);
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
static int ReverbHIModify(AXFX_REVHI_WORK* rv, f32 coloration, f32 time, f32 mix, f32 damping, f32 preDelay, f32 crosstalk) {
	ASSERTMSGLINE(209, coloration >= 0.0f && coloration <= 1.0f &&
				  time >= 0.01f && time <= 10.0f &&
				  mix >= 0.0f && mix <= 1.0f &&
				  crosstalk >= 0.0f && crosstalk <= 1.0f &&
				  damping >= 0.0f && damping <= 1.0f &&
				  preDelay >= 0.0f && preDelay <= 0.1f,
				  "The value of specified parameter is out of range.");

    if (!(coloration >= 0.0f && coloration <= 1.0f &&
          time >= 0.01f && time <= 10.0f &&
          mix >= 0.0f && mix <= 1.0f &&
          crosstalk >= 0.0f && crosstalk <= 1.0f &&
          damping >= 0.0f && damping <= 1.0f &&
          preDelay >= 0.0f && preDelay <= 0.1f)) {
        return 0;
    }

    ReverbHIFree(rv);
    return ReverbHICreate(rv, coloration, time, mix, damping, preDelay, crosstalk);
}

enum {
    AXFX_ROUND_NEAREST,
    AXFX_ROUND_ZERO,
    AXFX_ROUND_UP,
    AXFX_ROUND_DOWN,
    AXFX_ROUND_MASK = 3,
};

static inline s32 RoundSample(f32 sample, u32 mode)
{
    s32 whole = AXFXTruncateSample(sample);
    f32 fraction;

    // At this magnitude, a single-precision sample has no fractional bits.
    if (!(sample > -8388608.0f && sample < 8388608.0f)) {
        return whole;
    }

    fraction = sample - (f32)whole;
    switch (mode) {
    case AXFX_ROUND_NEAREST:
        if (fraction > 0.5f || (fraction == 0.5f && (whole & 1))) {
            return whole + 1;
        }
        if (fraction < -0.5f || (fraction == -0.5f && (whole & 1))) {
            return whole - 1;
        }
        break;
    case AXFX_ROUND_UP:
        return whole + (fraction > 0.0f);
    case AXFX_ROUND_DOWN:
        return whole - (fraction < 0.0f);
    case AXFX_ROUND_ZERO:
        break;
    }
    return whole;
}

/**
 * @note Original DoCrossTalk address: 0x8017A96C
 */
static void MixCrossTalk(s32* left, s32* right, f32 crosstalk)
{
    f32 cross = 0.5f * crosstalk;
    f32 direct = 1.0f - cross;
    u32 rounding = PPCMffpscr() & AXFX_ROUND_MASK;
    int i;

    for (i = 0; i < AX_SAMPLES_PER_FRAME; i++) {
        f32 l = (f32)left[i];
        f32 r = (f32)right[i];

        left[i] = RoundSample(l * direct + r * cross, rounding);
        right[i] = RoundSample(l * cross + r * direct, rounding);
    }
}

static inline f32 DelayLinePush(AXFX_REVHI_DELAYLINE* line, f32 sample)
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

static inline f32 ApplyAllPass(AXFX_REVHI_DELAYLINE* line, f32 coefficient, f32 sample)
{
    f32 previous = line->lastOutput;
    f32 feedback = __fmadds(coefficient, previous, sample);

    DelayLinePush(line, feedback);
    return __fnmsubs(coefficient, feedback, previous);
}

/**
 * @note Original HandleReverb address: 0x8017AAFC
 */
static void ProcessChannel(s32* samples, AXFX_REVHI_WORK* rv, int channel)
{
    AXFX_REVHI_DELAYLINE* comb = &rv->C[3 * channel];
    AXFX_REVHI_DELAYLINE* allPass = &rv->AP[3 * channel];
    const f32* combCoefficients = &rv->combCoef[3 * channel];
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
        f32 comb2;
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
        comb2 = DelayLinePush(&comb[2], __fmadds(combCoefficients[2], comb[2].lastOutput, input));
        wet = ApplyAllPass(&allPass[0], allPassCoefficient, (comb0 + comb1) + comb2);
        wet = ApplyAllPass(&allPass[1], allPassCoefficient, wet);
        lowPass = __fmadds(damping, lowPass, wet * 0.3f);
        wet = ApplyAllPass(&allPass[2], allPassCoefficient, lowPass);
        samples[i] = AXFXTruncateSample(__fmadds(wetGain, wet, dryGain * dry));
    }

    rv->lpLastout[channel] = lowPass;
    rv->preDelayPtr[channel] = preDelay;
}

/**
 * @note Address: 0x8017B008
 * @note Size: 0x124
 */
static void ReverbHIFree(AXFX_REVHI_WORK* rv) {
    u8 i;

    for (i = 0; i < 9; i++) {
		if (rv->AP[i].inputs != 0) {
			DLdelete(&rv->AP[i]);
			rv->AP[i].inputs = NULL;
		}
    }

    for (i = 0; i < 9; i++) {
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
 * @note Address: 0x8017B12C
 * @note Size: 0x70
 */
int AXFXReverbHiInit(AXFX_REVERBHI* rev) {
    int ret;
    BOOL old;

    old = OSDisableInterrupts();
    rev->tempDisableFX = 1;
    ret = ReverbHICreate(&rev->rv, rev->coloration, rev->time, rev->mix, rev->damping, rev->preDelay, rev->crosstalk);
    rev->tempDisableFX = !ret;
    OSRestoreInterrupts(old);
    return ret;
}

/**
 * @note Address: 0x8017B19C
 * @note Size: 0x4C
 */
int AXFXReverbHiShutdown(AXFX_REVERBHI* rev) {
    BOOL old;

    old = OSDisableInterrupts();
    rev->tempDisableFX = 1;
    ReverbHIFree(&rev->rv);
    OSRestoreInterrupts(old);
    return 1;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
int AXFXReverbHiSettings(AXFX_REVERBHI* rev) {
    int ret;
    BOOL old;

    old = OSDisableInterrupts();
    rev->tempDisableFX = 1;
    ret = ReverbHIModify(&rev->rv, rev->coloration, rev->time, rev->mix, rev->damping, rev->preDelay, rev->crosstalk);
    rev->tempDisableFX = rev->rv.C[0].inputs == NULL;
    OSRestoreInterrupts(old);
    return ret;
}

/**
 * @note Address: 0x8017B1E8
 * @note Size: 0x104
 */
void AXFXReverbHiCallback(AXFX_BUFFERUPDATE* bufferUpdate, AXFX_REVERBHI* reverb) {
    AXFX_REVHI_WORK* rv = &reverb->rv;

    if (reverb->tempDisableFX != 0) {
        return;
    }
    if (rv->crosstalk != 0.0f) {
        MixCrossTalk(bufferUpdate->left, bufferUpdate->right, rv->crosstalk);
    }
    ProcessChannel(bufferUpdate->left, rv, 0);
    ProcessChannel(bufferUpdate->right, rv, 1);
    ProcessChannel(bufferUpdate->surround, rv, 2);
}
