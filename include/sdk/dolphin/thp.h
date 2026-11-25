#ifndef _DOLPHIN_THP_H_
#define _DOLPHIN_THP_H_

#include <dolphin/types.h>
#include <THP/THP.h>
#include <THP/THPBuffer.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct THPVideoInfo {
	u32 xSize;
	u32 ySize;
	u32 videoType;
} THPVideoInfo;

typedef struct THPAudioInfo {
	u32 sndChannels;
	u32 sndFrequency;
	u32 sndNumSamples;
	u32 sndNumTracks;
} THPAudioInfo;

typedef struct THPFrameCompInfo {
	u32 numComponents;
	u8 frameComp[16];
} THPFrameCompInfo;

typedef struct THPHeader {
	/* 0x00 */ char magic[4];
	/* 0x04 */ u32 version;
	/* 0x08 */ u32 bufsize;
	/* 0x0C */ u32 audioMaxSamples;
	/* 0x10 */ f32 frameRate;
	/* 0x14 */ u32 numFrames;
	/* 0x18 */ u32 firstFrameSize;
	/* 0x1C */ u32 movieDataSize;
	/* 0x20 */ u32 compInfoDataOffsets;
	/* 0x24 */ u32 offsetDataOffsets;
	/* 0x28 */ u32 movieDataOffsets;
	/* 0x2C */ u32 finalFrameDataOffsets;
} THPHeader;

#define THP_AUDIO_BUFFER_COUNT 3
#define THP_READ_BUFFER_COUNT  10
#define THP_TEXTURE_SET_COUNT  3

#ifdef __cplusplus
}
#endif

#endif // _DOLPHIN_THP_H_
