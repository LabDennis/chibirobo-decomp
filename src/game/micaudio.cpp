#include "game/cgame.h"
#include "game/mic.h"

extern "C" {
s32 M2SGetActiveChannel(void);
s32 M2SGetSamplesLeft(void);
void M2SGetSamples(s16** samples, s32 count);
void M2SAdvanceBuffer(s32 count);
void* memset(void* dst, int val, u32 size);
}

/**
 * @note Address: 0x8000A560
 * @note Size: 0x118
 */
// pulls the next samples from the mic into the buffer, a faster step when more are waiting,
// and keeps the loudest ones
extern "C" void FillMicrophoneSampleBuffer(s16* buffer)
{
	s32 count;
	s32 step;
	u32 next;
	memset(buffer, 0, 0x280);
	u32* pos = &gMicSamplePhase[M2SGetActiveChannel()];
	s32 left = M2SGetSamplesLeft();
	if (left < 0) {
		*pos = 0;
	} else if (left >= sMicBufferThresholds[0]) {
		if (left < sMicBufferThresholds[1]) {
			step = sMicSamplePhaseSteps[0];
		} else if (left < sMicBufferThresholds[2]) {
			step = sMicSamplePhaseSteps[1];
		} else {
			step = sMicSamplePhaseSteps[2];
		}
		next = *pos;
		next += step * 159;
		count = next >> 16;
		s16* samples;
		M2SGetSamples(&samples, count + 4);
		s16* sample = samples;
		for (s32 i = 0; i < count + 4; i++) {
			s32 value = *sample;
			if (value > gMicMeterPeak) {
				gMicMeterPeak = value;
			}
			if (value > 1000 && value > gMicObjectPeak) {
				gMicObjectPeak = value;
			}
			sample++;
		}
		next += step;
		*pos = next & 0xFFFF;
		M2SAdvanceBuffer(next >> 16);
	}
}

/**
 * @note Address: 0x8000A678
 * @note Size: 0x60
 */
extern "C" void OnMicAudioDMA(void)
{
	gPreviousAudioDMACallback();
	FillMicrophoneSampleBuffer(gMicDMABuffers[gMicDMABufferIndex]);
	DCFlushRange(gMicDMABuffers[gMicDMABufferIndex], 0x280);
	gMicDMABufferIndex ^= 1;
}
