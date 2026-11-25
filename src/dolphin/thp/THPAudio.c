#include "THP/THP.h"
#include "stl/limits.h"

static s32 __THPAudioGetNewSample(THPAudioDecodeInfo* info);
static void __THPAudioInitialize(THPAudioDecodeInfo* info, u8* ptr);

/**
 * @note Address: 0x801FD1E4
 * @note Size: 0x3B0
 */
u32 THPAudioDecode(s16* audioBuffer, u8* audioFrame, s32 flag)
{
	THPAudioRecordHeader* header;
	THPAudioDecodeInfo decInfo;
	u8 *left, *right;
	s16 *decLeftPtr, *decRightPtr;
	s16 yn1, yn2;
	s32 i;
	s32 step;
	s32 sample;
	u64 yn; // Shift the sample bits unsigned and use signed values for clipping.

	if (audioBuffer == NULL || audioFrame == NULL) {
		return 0;
	}

	header = (THPAudioRecordHeader*)audioFrame;
	left   = audioFrame + sizeof(THPAudioRecordHeader);
	right  = left + header->offsetNextChannel;

	if (flag == 1) {
		decRightPtr = audioBuffer;
		decLeftPtr  = audioBuffer + header->sampleSize;
		step        = 1;
	} else {
		decRightPtr = audioBuffer;
		decLeftPtr  = audioBuffer + 1;
		step        = 2;
	}

	if (header->offsetNextChannel == 0) {
		__THPAudioInitialize(&decInfo, left);

		yn1 = header->lYn1;
		yn2 = header->lYn2;

		for (i = 0; i < header->sampleSize; i++) {
			sample = __THPAudioGetNewSample(&decInfo);
			yn     = header->lCoef[decInfo.predictor][1] * yn2;
			yn += header->lCoef[decInfo.predictor][0] * yn1;
			yn += (s32)((u32)sample << decInfo.scale << 11);
			yn <<= 5;

			yn += 0x8000;

			if ((s64)yn > INT_MAX) {
				yn = INT_MAX;
			}

			if ((s64)yn < INT_MIN) {
				yn = INT_MIN;
			}

			*decLeftPtr = (s16)((s64)yn >> 16);
			decLeftPtr += step;
			*decRightPtr = (s16)((s64)yn >> 16);
			decRightPtr += step;
			yn2 = yn1;
			yn1 = (s16)((s64)yn >> 16);
		}
	} else {
		__THPAudioInitialize(&decInfo, left);

		yn1 = header->lYn1;
		yn2 = header->lYn2;

		for (i = 0; i < header->sampleSize; i++) {
			sample = __THPAudioGetNewSample(&decInfo);
			yn     = header->lCoef[decInfo.predictor][1] * yn2;
			yn += header->lCoef[decInfo.predictor][0] * yn1;
			yn += (s32)((u32)sample << decInfo.scale << 11);
			yn <<= 5;
			yn += 0x8000;

			if ((s64)yn > INT_MAX) {
				yn = INT_MAX;
			}

			if ((s64)yn < INT_MIN) {
				yn = INT_MIN;
			}

			*decLeftPtr = (s16)((s64)yn >> 16);
			decLeftPtr += step;
			yn2 = yn1;
			yn1 = (s16)((s64)yn >> 16);
		}

		__THPAudioInitialize(&decInfo, right);

		yn1 = header->rYn1;
		yn2 = header->rYn2;

		for (i = 0; i < header->sampleSize; i++) {
			sample = __THPAudioGetNewSample(&decInfo);
			yn     = header->rCoef[decInfo.predictor][1] * yn2;
			yn += header->rCoef[decInfo.predictor][0] * yn1;
			yn += (s32)((u32)sample << decInfo.scale << 11);
			yn <<= 5;

			yn += 0x8000;

			if ((s64)yn > INT_MAX) {
				yn = INT_MAX;
			}

			if ((s64)yn < INT_MIN) {
				yn = INT_MIN;
			}

			*decRightPtr = (s16)((s64)yn >> 16);
			decRightPtr += step;
			yn2 = yn1;
			yn1 = (s16)((s64)yn >> 16);
		}
	}

	return header->sampleSize;
}

/**
 * @note Address: 0x801FD594
 * @note Size: 0x90
 */
static s32 __THPAudioGetNewSample(THPAudioDecodeInfo* info)
{
	s32 sample;

	if (!(info->offsetNibbles & 0x0f)) {
		info->predictor = (u8)((*(info->encodeData) & 0x70) >> 4);
		info->scale     = (u8)((*(info->encodeData) & 0xF));
		info->encodeData++;
		info->offsetNibbles += 2;
	}

	if (info->offsetNibbles & 0x1) {
		sample = (s32)((*(info->encodeData) & 0xFU) << 28) >> 28;
		info->encodeData++;
	} else {
		sample = (s32)((*(info->encodeData) & 0xF0U) << 24) >> 28;
	}

	info->offsetNibbles++;
	return sample;
}

/**
 * @note Address: 0x801FD624
 * @note Size: 0x3C
 */
static void __THPAudioInitialize(THPAudioDecodeInfo* info, u8* ptr)
{
	info->encodeData    = ptr;
	info->offsetNibbles = 2;
	info->predictor     = (u8)((*(info->encodeData) & 0x70) >> 4);
	info->scale         = (u8)((*(info->encodeData) & 0xF));
	info->encodeData++;
}
