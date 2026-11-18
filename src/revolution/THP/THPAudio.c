#include "revolution/thp.h"

// The Wii SDK of July 2008 decodes both channels in one loop, rounds with 0x400 before the shift and can mix
// the two channels down to one (bit 1 of flag). Older THPAudio.c (pik1wii, Petari) only served as a template.

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
		sample = (s32)((*(info->encodeData) & 0xF) << 28) >> 28;
		info->encodeData++;
	} else {
		sample = (s32)((*(info->encodeData) & 0xF0) << 24) >> 28;
	}

	info->offsetNibbles++;
	return sample;
}

static void __THPAudioInitialize(THPAudioDecodeInfo* info, u8* ptr)
{
	info->encodeData    = ptr;
	info->offsetNibbles = 2;
	info->predictor     = (u8)((*(info->encodeData) & 0x70) >> 4);
	info->scale         = (u8)((*(info->encodeData) & 0xF));
	info->encodeData++;
}

u32 THPAudioDecode(s16* audioBuffer, u8* audioFrame, s32 flag)
{
	THPAudioRecordHeader* header;
	THPAudioDecodeInfo decLeftInfo, decRightInfo;
	u8 *left, *right;
	s16 *decLeftPtr, *decRightPtr;
	s16 lyn1, lyn2, ryn1, ryn2;
	s32 i;
	s32 step;
	s32 sample;
	s64 yn, ynRight;
	BOOL mix;

	if (audioBuffer == NULL || audioFrame == NULL) {
		return 0;
	}

	header = (THPAudioRecordHeader*)audioFrame;
	left   = audioFrame + sizeof(THPAudioRecordHeader);
	right  = left + header->offsetNextChannel;

	if (flag & 1) {
		decRightPtr = audioBuffer;
		decLeftPtr  = audioBuffer + header->sampleSize;
		step        = 1;
	} else {
		decRightPtr = audioBuffer;
		decLeftPtr  = audioBuffer + 1;
		step        = 2;
	}

	mix = FALSE;
	if (flag & 2) {
		mix = TRUE;
	}

	if (header->offsetNextChannel == 0) {
		__THPAudioInitialize(&decLeftInfo, left);

		lyn1 = header->lYn1;
		lyn2 = header->lYn2;

		for (i = 0; i < header->sampleSize; i++) {
			sample = __THPAudioGetNewSample(&decLeftInfo);
			yn     = header->lCoef[decLeftInfo.predictor][1] * lyn2;
			yn += header->lCoef[decLeftInfo.predictor][0] * lyn1;
			yn += (sample << decLeftInfo.scale) << 11;
			yn += 0x400;
			yn >>= 11;

			if (yn > 32767) {
				yn = 32767;
			} else if (yn < -32768) {
				yn = -32768;
			}

			lyn2 = lyn1;
			lyn1 = (s16)yn;

			if (mix) {
				yn = ((yn << 1) * 0x5A82 + 0x4000) >> 15;

				if (yn > 32767) {
					yn = 32767;
				} else if (yn < -32768) {
					yn = -32768;
				}
			}

			*decLeftPtr = (s16)yn;
			decLeftPtr += step;
			*decRightPtr = (s16)yn;
			decRightPtr += step;
		}
	} else {
		__THPAudioInitialize(&decLeftInfo, left);
		__THPAudioInitialize(&decRightInfo, right);

		lyn1 = header->lYn1;
		lyn2 = header->lYn2;
		ryn1 = header->rYn1;
		ryn2 = header->rYn2;

		for (i = 0; i < header->sampleSize; i++) {
			sample = __THPAudioGetNewSample(&decLeftInfo);
			yn     = header->lCoef[decLeftInfo.predictor][1] * lyn2;
			yn += header->lCoef[decLeftInfo.predictor][0] * lyn1;
			yn += (sample << decLeftInfo.scale) << 11;
			yn += 0x400;
			yn >>= 11;

			if (yn > 32767) {
				yn = 32767;
			} else if (yn < -32768) {
				yn = -32768;
			}

			lyn2 = lyn1;
			lyn1 = (s16)yn;

			sample  = __THPAudioGetNewSample(&decRightInfo);
			ynRight = header->rCoef[decRightInfo.predictor][1] * ryn2;
			ynRight += header->rCoef[decRightInfo.predictor][0] * ryn1;
			ynRight += (sample << decRightInfo.scale) << 11;
			ynRight += 0x400;
			ynRight >>= 11;

			if (ynRight > 32767) {
				ynRight = 32767;
			} else if (ynRight < -32768) {
				ynRight = -32768;
			}

			ryn2 = ryn1;
			ryn1 = (s16)ynRight;

			if (mix) {
				yn = ((yn + ynRight) * 0x5A82 + 0x4000) >> 15;

				if (yn > 32767) {
					yn = 32767;
				} else if (yn < -32768) {
					yn = -32768;
				}

				ynRight = yn;
			}

			*decLeftPtr = (s16)yn;
			decLeftPtr += step;
			*decRightPtr = (s16)ynRight;
			decRightPtr += step;
		}
	}

	return header->sampleSize;
}
