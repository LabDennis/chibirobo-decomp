#include "game/THPSimple.h"
#include "game/THPDraw.h"
#include "Dolphin/ai.h"
#include "Dolphin/dvd.h"
#include "Dolphin/os.h"
#include "Dolphin/thp.h"
#include "Dolphin/gx.h"
#include <string.h>

typedef struct THPHeader {
	char magic[4];             // _00
	u32 version;               // _04
	u32 bufSize;               // _08
	u32 audioMaxSamples;       // _0C
	f32 frameRate;             // _10
	u32 numFrames;             // _14
	u32 firstFrameSize;        // _18
	u32 movieDataSize;         // _1C
	u32 compInfoDataOffsets;   // _20
	u32 offsetDataOffsets;     // _24
	u32 movieDataOffsets;      // _28
	u32 finalFrameDataOffsets; // _2C
} THPHeader;

#define THP_COMP_MAX 16

typedef struct THPFrameCompInfo {
	u32 numComponents;          // _00
	u8 frameComp[THP_COMP_MAX]; // _04
} THPFrameCompInfo;

typedef struct THPReadBuffer {
	u8* ptr;               // _00
	s32 frameNumber;       // _04
	volatile BOOL isValid; // _08
} THPReadBuffer;

typedef struct THPTextureSet {
	u8* ytexture;    // _00
	u8* utexture;    // _04
	u8* vtexture;    // _08
	s32 frameNumber; // _0C
} THPTextureSet;

typedef struct THPAudioBuffer {
	s16* buffer;     // _00
	s16* curPtr;     // _04
	u32 validSample; // _08
} THPAudioBuffer;

#define READ_BUFFER_NUM  10
#define AUDIO_BUFFER_NUM 3

// the sdk keeps a pointer to the file here, the game keeps the file itself
typedef struct THPSimple {
	DVDFileInfo fileInfo;                         // _00
	THPHeader header;                             // _3C
	THPFrameCompInfo compInfo;                    // _6C
	THPVideoInfo videoInfo;                       // _80
	THPAudioInfo audioInfo;                       // _8C
	void* thpWork;                                // _9C
	BOOL open;                                    // _A0
	u8 preFetchState;                             // _A4
	u8 audioState;                                // _A5
	u8 loop;                                      // _A6
	u8 audioExist;                                // _A7
	s32 curOffset;                                // _A8
	BOOL dvdError;                                // _AC
	volatile u32 readProgress;                    // _B0
	s32 nextDecodeIndex;                          // _B4
	s32 readIndex;                                // _B8
	s32 readSize;                                 // _BC
	u32 totalReadFrame;                           // _C0
	f32 curVolume;                                // _C4
	f32 targetVolume;                             // _C8
	f32 deltaVolume;                              // _CC
	s32 rampCount;                                // _D0
	THPReadBuffer readBuffer[READ_BUFFER_NUM];    // _D4
	THPTextureSet textureSet;                     // _14C
	THPAudioBuffer audioBuffer[AUDIO_BUFFER_NUM]; // _15C
	s32 audioDecodeIndex;                         // _180
	s32 audioOutputIndex;                         // _184
} THPSimple;
STATIC_ASSERT(sizeof(THPSimple) == 0x188);

#define SAMPLES_PER_AUDIO_FRAME 160 // 5ms at 32khz
#define BYTES_PER_AUDIO_FRAME   (SAMPLES_PER_AUDIO_FRAME * sizeof(s16) * 2)

// clang-format off
// 32768 * vol * vol / (127 * 127)
static u16 VolumeTable[] = {
	    0,     2,     8,    18,    32,    50,    73,    99,
	  130,   164,   203,   245,   292,   343,   398,   457,
	  520,   587,   658,   733,   812,   895,   983,  1074,
	 1170,  1269,  1373,  1481,  1592,  1708,  1828,  1952,
	 2080,  2212,  2348,  2488,  2632,  2781,  2933,  3090,
	 3250,  3415,  3583,  3756,  3933,  4114,  4298,  4487,
	 4680,  4877,  5079,  5284,  5493,  5706,  5924,  6145,
	 6371,  6600,  6834,  7072,  7313,  7559,  7809,  8063,
	 8321,  8583,  8849,  9119,  9394,  9672,  9954, 10241,
	10531, 10826, 11125, 11427, 11734, 12045, 12360, 12679,
	13002, 13329, 13660, 13995, 14335, 14678, 15025, 15377,
	15732, 16092, 16456, 16823, 17195, 17571, 17951, 18335,
	18723, 19115, 19511, 19911, 20316, 20724, 21136, 21553,
	21974, 22398, 22827, 23260, 23696, 24137, 24582, 25031,
	25484, 25941, 26402, 26868, 27337, 27810, 28288, 28769,
	29255, 29744, 30238, 30736, 31238, 31744, 32254, 32768,
};
// clang-format on

static THPSimple SimpleControl;
// the header read only asks for 0x120 of this
static u8 WorkBuffer[0x140] ATTRIBUTE_ALIGN(32);
static s16 SoundBuffer[2][SAMPLES_PER_AUDIO_FRAME * 2] ATTRIBUTE_ALIGN(32);

static s32 ReadDone = 1;

static BOOL Initialized;
static s32 SoundBufferIndex;
static AIDCallback OldAIDCallback;
static s16* LastAudioBuffer;
static s16* CurAudioBuffer;
static s32 AudioSystem;
// step of the open or preload in progress, both are polled from the game loop
static s32 LoadStep;
static s32 EntryNum;
static u32 PreLoadNum;
static s32 PreLoadLoop;
static BOOL SoundOff;

static void __THPSimpleDVDCallback(s32 result, DVDFileInfo* fileInfo);
static void THPAudioMixCallback(void);

/**
 * @note Address: 0x801B71C8
 * @note Size: 0x10
 */
void THPSimpleSetSound(BOOL on)
{
	SoundOff = !on;
}

/**
 * @note Address: 0x801B70B4
 * @note Size: 0x114
 */
BOOL THPSimpleInit(s32 audioSystem)
{
	memset(&SimpleControl, 0, sizeof(THPSimple));
	LCEnable();
	if (THPInit() == FALSE) {
		return FALSE;
	}

	{
		BOOL old = OSDisableInterrupts();

		AudioSystem      = audioSystem;
		SoundBufferIndex = 0;
		LastAudioBuffer  = NULL;
		CurAudioBuffer   = NULL;

		OldAIDCallback = AIRegisterDMACallback(THPAudioMixCallback);
		if (OldAIDCallback == NULL && AudioSystem != 0) {
			AIRegisterDMACallback(NULL);
			OSRestoreInterrupts(old);
			return FALSE;
		}

		OSRestoreInterrupts(old);
	}

	if (AudioSystem == 0) {
		memset(SoundBuffer, 0, BYTES_PER_AUDIO_FRAME << 1);
		DCFlushRange(SoundBuffer, BYTES_PER_AUDIO_FRAME << 1);
		AIInitDMA((u32)SoundBuffer[SoundBufferIndex], BYTES_PER_AUDIO_FRAME);
		AIStartDMA();
	}
	Initialized = TRUE;
	return TRUE;
}

/**
 * @note Address: 0x801B6FF0
 * @note Size: 0xC4
 */
s32 THPSimpleOpen(const char* path)
{
	if (!Initialized) {
		return -1;
	}

	if (SimpleControl.open) {
		return -1;
	}

	memset(&SimpleControl.videoInfo, 0, sizeof(THPVideoInfo));
	memset(&SimpleControl.audioInfo, 0, sizeof(THPAudioInfo));

	EntryNum = DVDConvertPathToEntrynum((char*)path);
	if (EntryNum == -1) {
		return -1;
	}

	if (!DVDFastOpen(EntryNum, &SimpleControl.fileInfo)) {
		return -1;
	}

	LoadStep = 1;
	return 1;
}

/**
 * @note Address: 0x801B6FD8
 * @note Size: 0x18
 */
static void ReadCallback(s32 result, DVDFileInfo* fileInfo)
{
	if (result >= 0) {
		ReadDone++;
	}
}

/**
 * @note Address: 0x801B6D4C
 * @note Size: 0x28C
 */
s32 THPSimpleOpenWait(void)
{
	s32 offset;
	u32 i;

	switch (LoadStep) {
	case 1: // the header first
		LoadStep++;
		ReadDone = 0;
		DVDReadAsyncPrio(&SimpleControl.fileInfo, WorkBuffer, 0x40, 0, ReadCallback, 2);
		return 0;

	case 2: // then the component info
		if (!ReadDone) {
			return 0;
		}

		memcpy(&SimpleControl.header, WorkBuffer, sizeof(THPHeader));
		if (strcmp(SimpleControl.header.magic, "THP") != 0) {
			DVDClose(&SimpleControl.fileInfo);
			return -1;
		}

		if (SimpleControl.header.version != 0x11000) { // thp version 1.0
			DVDClose(&SimpleControl.fileInfo);
			return -1;
		}

		LoadStep++;
		ReadDone = 0;
		DVDReadAsyncPrio(&SimpleControl.fileInfo, WorkBuffer, 0x120, SimpleControl.header.compInfoDataOffsets, ReadCallback, 2);
		return 0;

	case 3:
		if (!ReadDone) {
			return 0;
		}

		memcpy(&SimpleControl.compInfo, WorkBuffer, sizeof(THPFrameCompInfo));
		offset                   = sizeof(THPFrameCompInfo);
		SimpleControl.audioExist = 0;
		for (i = 0; i < SimpleControl.compInfo.numComponents; i++) {
			switch (SimpleControl.compInfo.frameComp[i]) {
			case 0: // the video component
				memcpy(&SimpleControl.videoInfo, WorkBuffer + offset, sizeof(THPVideoInfo));
				offset += sizeof(THPVideoInfo);
				break;
			case 1: // the audio component
				memcpy(&SimpleControl.audioInfo, WorkBuffer + offset, sizeof(THPAudioInfo));
				SimpleControl.audioExist = 1;
				offset += sizeof(THPAudioInfo);
				break;
			default:
				// retail never closes the file on this path
				return -1;
			}
		}

		SimpleControl.curOffset              = SimpleControl.header.movieDataOffsets;
		SimpleControl.readSize               = SimpleControl.header.firstFrameSize;
		SimpleControl.readIndex              = 0;
		SimpleControl.totalReadFrame         = 0;
		SimpleControl.dvdError               = FALSE;
		SimpleControl.textureSet.frameNumber = -1;
		SimpleControl.nextDecodeIndex        = 0;
		SimpleControl.audioDecodeIndex       = 0;
		SimpleControl.audioOutputIndex       = 0;
		SimpleControl.preFetchState          = FALSE;
		SimpleControl.audioState             = FALSE;
		SimpleControl.loop                   = 0;
		SimpleControl.open                   = TRUE;
		SimpleControl.curVolume              = 127.0f; // full volume
		SimpleControl.targetVolume           = SimpleControl.curVolume;
		SimpleControl.rampCount              = 0;
		LoadStep                             = 0;
		break;
	}

	return 1;
}

/**
 * @note Address: 0x801B6CBC
 * @note Size: 0x90
 */
BOOL THPSimpleClose(void)
{
	if (SimpleControl.open && SimpleControl.preFetchState == FALSE) {
		if (SimpleControl.audioExist) {
			if (SimpleControl.audioState == TRUE) {
				return FALSE;
			}
		} else {
			SimpleControl.audioState = FALSE;
		}

		if (!SimpleControl.readProgress) {
			SimpleControl.open = FALSE;
			DVDClose(&SimpleControl.fileInfo);
			return TRUE;
		}
	}
	return FALSE;
}

/**
 * @note Address: 0x801B6C38
 * @note Size: 0x84
 */
u32 THPSimpleCalcNeedMemory(void)
{
	u32 size;

	if (SimpleControl.open) {
		size = OSRoundUp32B(SimpleControl.header.bufSize) * READ_BUFFER_NUM;
		size += OSRoundUp32B(SimpleControl.videoInfo.xSize * SimpleControl.videoInfo.ySize);
		size += OSRoundUp32B(SimpleControl.videoInfo.xSize * SimpleControl.videoInfo.ySize / 4);
		size += OSRoundUp32B(SimpleControl.videoInfo.xSize * SimpleControl.videoInfo.ySize / 4);
		if (SimpleControl.audioExist) {
			size += OSRoundUp32B(SimpleControl.header.audioMaxSamples * 4) * AUDIO_BUFFER_NUM;
		}
		size += 0x1000;
		return size;
	}
	return 0;
}

/**
 * @note Address: 0x801B6A68
 * @note Size: 0x1D0
 */
BOOL THPSimpleSetBuffer(void* buffer)
{
	u32 i;
	u8* ptr;
	u32 ysize, uvsize;

	if (SimpleControl.open && SimpleControl.preFetchState == FALSE) {
		if (SimpleControl.audioState == TRUE) {
			return FALSE;
		}

		ysize  = OSRoundUp32B(SimpleControl.videoInfo.xSize * SimpleControl.videoInfo.ySize);
		uvsize = OSRoundUp32B(SimpleControl.videoInfo.xSize * SimpleControl.videoInfo.ySize / 4);
		ptr    = (u8*)buffer;

		SimpleControl.textureSet.ytexture = ptr;
		DCInvalidateRange(ptr, ysize);
		ptr += ysize;
		SimpleControl.textureSet.utexture = ptr;
		DCInvalidateRange(ptr, uvsize);
		ptr += uvsize;
		SimpleControl.textureSet.vtexture = ptr;
		DCInvalidateRange(ptr, uvsize);
		ptr += uvsize;

		for (i = 0; i < READ_BUFFER_NUM; i++) {
			SimpleControl.readBuffer[i].ptr = ptr;
			ptr += OSRoundUp32B(SimpleControl.header.bufSize);
			SimpleControl.readBuffer[i].isValid = FALSE;
		}

		if (SimpleControl.audioExist) {
			for (i = 0; i < AUDIO_BUFFER_NUM; i++) {
				SimpleControl.audioBuffer[i].buffer      = (s16*)ptr;
				SimpleControl.audioBuffer[i].curPtr      = (s16*)ptr;
				SimpleControl.audioBuffer[i].validSample = 0;
				ptr += OSRoundUp32B(SimpleControl.header.audioMaxSamples * 4);
			}
		}

		SimpleControl.thpWork = (void*)ptr;
	}

	return TRUE;
}

static inline void ReadFrameAsync(void)
{
	if (!SimpleControl.dvdError && SimpleControl.preFetchState == TRUE) {
		if (SimpleControl.totalReadFrame > SimpleControl.header.numFrames - 1) {
			if (SimpleControl.loop != 1) { // no loop: stop at the last frame
				return;
			}
			SimpleControl.totalReadFrame = 0;
			SimpleControl.curOffset      = SimpleControl.header.movieDataOffsets;
			SimpleControl.readSize       = SimpleControl.header.firstFrameSize;
		}

		SimpleControl.readProgress = TRUE;
		if (DVDReadAsyncPrio(&SimpleControl.fileInfo, SimpleControl.readBuffer[SimpleControl.readIndex].ptr, SimpleControl.readSize,
		                     SimpleControl.curOffset, __THPSimpleDVDCallback, 2)
		    != TRUE) {
			SimpleControl.readProgress = FALSE;
			SimpleControl.dvdError     = TRUE;
		}
	}
}

/**
 * @note Address: 0x801B68EC
 * @note Size: 0x17C
 */
static void __THPSimpleDVDCallback(s32 result, DVDFileInfo* fileInfo)
{
	if (result == DVD_RESULT_FATAL_ERROR) {
		SimpleControl.dvdError = TRUE;
	} else if (result != DVD_RESULT_CANCELED) {
		SimpleControl.readProgress                                    = FALSE;
		SimpleControl.readBuffer[SimpleControl.readIndex].frameNumber = SimpleControl.totalReadFrame;
		SimpleControl.totalReadFrame++;
		SimpleControl.readBuffer[SimpleControl.readIndex].isValid = TRUE;
		SimpleControl.curOffset += SimpleControl.readSize;
		// each frame starts with the size of the next
		SimpleControl.readSize  = *(u32*)SimpleControl.readBuffer[SimpleControl.readIndex].ptr;
		SimpleControl.readIndex = SimpleControl.readIndex + 1 >= READ_BUFFER_NUM ? 0 : SimpleControl.readIndex + 1;
		if (!SimpleControl.readBuffer[SimpleControl.readIndex].isValid) {
			ReadFrameAsync();
		}
	}
}

/**
 * @note Address: 0x801B6890
 * @note Size: 0x5C
 */
s32 THPSimplePreLoad(s32 loop)
{
	u32 readNum;

	if (SimpleControl.open && SimpleControl.preFetchState == FALSE) {
		readNum = READ_BUFFER_NUM;

		if (!loop) {
			if (SimpleControl.header.numFrames < READ_BUFFER_NUM) {
				readNum = SimpleControl.header.numFrames; // a short movie preloads whole
			}
		}

		PreLoadLoop = loop;
		PreLoadNum  = readNum;
		LoadStep    = 1;
		return 1;
	}

	return -1;
}

/**
 * @note Address: 0x801B6714
 * @note Size: 0x17C
 */
s32 THPSimplePreLoadWait(void)
{
	if (LoadStep > 0) {
		// odd steps start a read, even steps wait for it
		if (LoadStep & 1) {
			LoadStep++;
			ReadDone = 0;
			DVDReadAsyncPrio(&SimpleControl.fileInfo, SimpleControl.readBuffer[SimpleControl.readIndex].ptr, SimpleControl.readSize,
			                 SimpleControl.curOffset, ReadCallback, 2);
			return 0;
		}

		if (!ReadDone) {
			return 0;
		}

		SimpleControl.curOffset += SimpleControl.readSize;
		SimpleControl.readSize                                        = *(u32*)SimpleControl.readBuffer[SimpleControl.readIndex].ptr;
		SimpleControl.readBuffer[SimpleControl.readIndex].isValid     = TRUE;
		SimpleControl.readBuffer[SimpleControl.readIndex].frameNumber = SimpleControl.totalReadFrame;
		SimpleControl.readIndex = SimpleControl.readIndex + 1 >= READ_BUFFER_NUM ? 0 : SimpleControl.readIndex + 1;
		SimpleControl.totalReadFrame++;

		if (SimpleControl.totalReadFrame > SimpleControl.header.numFrames - 1) {
			if (SimpleControl.loop == 1) {
				SimpleControl.totalReadFrame = 0;
				SimpleControl.curOffset      = SimpleControl.header.movieDataOffsets;
				SimpleControl.readSize       = SimpleControl.header.firstFrameSize;
			}
		}

		LoadStep++;
		if (--PreLoadNum == 0) {
			SimpleControl.loop          = PreLoadLoop;
			SimpleControl.preFetchState = TRUE;
			return 1;
		}
	}

	return 0;
}

/**
 * @note Address: 0x801B6700
 * @note Size: 0x14
 */
void THPSimpleAudioStart(void)
{
	SimpleControl.audioState = TRUE;
}

/**
 * @note Address: 0x801B66EC
 * @note Size: 0x14
 */
void THPSimpleAudioStop(void)
{
	SimpleControl.audioState = FALSE;
}

/**
 * @note Address: 0x801B65E4
 * @note Size: 0x108
 */
BOOL THPSimpleLoadStop(void)
{
	s32 i;

	if (SimpleControl.open && SimpleControl.audioState == FALSE) {
		SimpleControl.preFetchState = FALSE;

		if (SimpleControl.readProgress) {
			DVDCancel(&SimpleControl.fileInfo.cBlock);
			SimpleControl.readProgress = FALSE;
		}

		for (i = 0; i < READ_BUFFER_NUM; i++) {
			SimpleControl.readBuffer[i].isValid = 0;
		}

		for (i = 0; i < AUDIO_BUFFER_NUM; i++) {
			SimpleControl.audioBuffer[i].validSample = 0;
		}

		SimpleControl.textureSet.frameNumber = -1;
		SimpleControl.curOffset              = SimpleControl.header.movieDataOffsets;
		SimpleControl.readSize               = SimpleControl.header.firstFrameSize;
		SimpleControl.readIndex              = 0;
		SimpleControl.totalReadFrame         = 0;
		SimpleControl.dvdError               = FALSE;
		SimpleControl.nextDecodeIndex        = 0;
		SimpleControl.audioDecodeIndex       = 0;
		SimpleControl.audioOutputIndex       = 0;
		SimpleControl.curVolume              = SimpleControl.targetVolume;
		SimpleControl.rampCount              = 0;

		return TRUE;
	}

	return FALSE;
}

static inline BOOL VideoDecode(u8* videoFrame)
{
	s32 ret = THPVideoDecode(videoFrame, SimpleControl.textureSet.ytexture, SimpleControl.textureSet.utexture,
	                         SimpleControl.textureSet.vtexture, SimpleControl.thpWork);

	if (ret == 0) {
		SimpleControl.textureSet.frameNumber = SimpleControl.readBuffer[SimpleControl.nextDecodeIndex].frameNumber;
		return TRUE;
	}
	return FALSE;
}

static inline void CheckPrefetch(void)
{
	BOOL enabled = OSDisableInterrupts();
	if (!SimpleControl.readBuffer[SimpleControl.readIndex].isValid && !SimpleControl.readProgress) {
		ReadFrameAsync();
	}
	OSRestoreInterrupts(enabled);
}

/**
 * @note Address: 0x801B626C
 * @note Size: 0x378
 */
s32 THPSimpleDecode(s32 audioTrack)
{
	u32 i;
	u8* ptr;
	u32 *compSizePtr, sample;

	if (SimpleControl.readBuffer[SimpleControl.nextDecodeIndex].isValid) {
		// a frame: an 8-byte header, per-component sizes, then the components
		compSizePtr = (u32*)(SimpleControl.readBuffer[SimpleControl.nextDecodeIndex].ptr + 8);
		ptr         = SimpleControl.readBuffer[SimpleControl.nextDecodeIndex].ptr + SimpleControl.compInfo.numComponents * 4 + 8;

		if (SimpleControl.audioExist) {
			if (audioTrack < 0 || audioTrack >= SimpleControl.audioInfo.sndNumTracks) {
				return 4; // bad track
			}

			if (SimpleControl.audioBuffer[SimpleControl.audioDecodeIndex].validSample == 0) {
				for (i = 0; i < SimpleControl.compInfo.numComponents; i++) {
					switch (SimpleControl.compInfo.frameComp[i]) {
					case 0:
						if (VideoDecode(ptr) == FALSE) {
							return 1; // video decode failed
						}
						break;

					case 1:
						sample = THPAudioDecode(SimpleControl.audioBuffer[SimpleControl.audioDecodeIndex].buffer,
						                        ptr + (*compSizePtr) * audioTrack, 0);
						{
							BOOL old = OSDisableInterrupts();

							SimpleControl.audioBuffer[SimpleControl.audioDecodeIndex].validSample = sample;
							SimpleControl.audioBuffer[SimpleControl.audioDecodeIndex].curPtr
							    = SimpleControl.audioBuffer[SimpleControl.audioDecodeIndex].buffer;

							OSRestoreInterrupts(old);
						}
						SimpleControl.audioDecodeIndex++;
						if (SimpleControl.audioDecodeIndex >= AUDIO_BUFFER_NUM) {
							SimpleControl.audioDecodeIndex = 0;
						}
						break;
					}
					ptr += *compSizePtr;
					compSizePtr++;
				}
			} else {
				return 3; // the audio queue is full
			}
		} else {
			for (i = 0; i < SimpleControl.compInfo.numComponents; i++) {
				switch (SimpleControl.compInfo.frameComp[i]) {
				case 0:
					if (VideoDecode(ptr) == FALSE) {
						return 1;
					}
					break;
				}
				ptr += *compSizePtr;
				compSizePtr++;
			}
		}

		SimpleControl.readBuffer[SimpleControl.nextDecodeIndex].isValid = FALSE;
		SimpleControl.nextDecodeIndex = SimpleControl.nextDecodeIndex + 1 >= READ_BUFFER_NUM ? 0 : SimpleControl.nextDecodeIndex + 1;

		CheckPrefetch();
		return 0;
	}

	return 2; // nothing to decode
}

/**
 * @note Address: 0x801B61F8
 * @note Size: 0x74
 */
s32 THPSimpleDrawBegin(BOOL alpha)
{
	if (SimpleControl.textureSet.frameNumber >= 0) {
		THPGXYuv2RgbSetup(alpha);
		THPGXSetupVtx();
		THPGXLoadYuvTex(SimpleControl.textureSet.ytexture, SimpleControl.textureSet.utexture, SimpleControl.textureSet.vtexture,
		                (s16)SimpleControl.videoInfo.xSize, (s16)SimpleControl.videoInfo.ySize);
		return SimpleControl.textureSet.frameNumber;
	}
	return -1;
}

/**
 * @note Address: 0x801B61D8
 * @note Size: 0x20
 */
void THPSimpleDrawEnd(void)
{
	THPGXRestore();
}

/**
 * @note Address: 0x801B6134
 * @note Size: 0xA4
 */
s32 THPSimpleDrawCurrentFrame(s32 x, s32 y, s32 polygonW, s32 polygonH, BOOL alpha)
{
	if (SimpleControl.textureSet.frameNumber >= 0) {
		THPSimpleDrawBegin(alpha);
		THPGXDrawQuad((s16)x, (s16)y, (s16)polygonW, (s16)polygonH);
		THPSimpleDrawEnd();
		return SimpleControl.textureSet.frameNumber;
	}
	return -1;
}

/**
 * @note Address: 0x801B6104
 * @note Size: 0x30
 */
void THPSimpleDrawQuad(s32 x, s32 y, s32 polygonW, s32 polygonH)
{
	THPGXDrawQuad((s16)x, (s16)y, (s16)polygonW, (s16)polygonH);
}

/**
 * @note Address: 0x801B5D48
 * @note Size: 0x3BC
 */
static void MixAudio(s16* destination, s16* source, u32 sample)
{
	u32 sampleNum, requestSample, i;
	s32 mix;
	s16 *dst, *libsrc, *thpsrc;
	u16 attenuation;

	if (source) {
		if (SimpleControl.open && SimpleControl.audioState == TRUE && SimpleControl.audioExist) {
			requestSample = sample;
			dst           = destination;
			libsrc        = source;

			while (TRUE) {
				if (SimpleControl.audioBuffer[SimpleControl.audioOutputIndex].validSample) {
					if (SimpleControl.audioBuffer[SimpleControl.audioOutputIndex].validSample >= requestSample) {
						sampleNum = requestSample;
					} else {
						sampleNum = SimpleControl.audioBuffer[SimpleControl.audioOutputIndex].validSample;
					}

					thpsrc = SimpleControl.audioBuffer[SimpleControl.audioOutputIndex].curPtr;
					for (i = 0; i < sampleNum; i++) {
						if (SimpleControl.rampCount) { // the ramp eases the volume to its target
							SimpleControl.rampCount--;
							SimpleControl.curVolume += SimpleControl.deltaVolume;
						} else {
							SimpleControl.curVolume = SimpleControl.targetVolume;
						}

						attenuation = VolumeTable[(s32)SimpleControl.curVolume];

						// the thp is attenuated in 15-bit fixed point, mixed, and clamped to s16
						mix = (*libsrc) + ((attenuation * (*thpsrc)) >> 15);
						if (mix < -32768) {
							mix = -32768;
						}
						if (mix > 32767) {
							mix = 32767;
						}
						*dst = (s16)mix;
						dst++;
						libsrc++;
						thpsrc++;

						mix = (*libsrc) + ((attenuation * (*thpsrc)) >> 15);
						if (mix < -32768) {
							mix = -32768;
						}
						if (mix > 32767) {
							mix = 32767;
						}
						*dst = (s16)mix;
						dst++;
						libsrc++;
						thpsrc++;

						// the game's own additions, mute and a mono downmix
						if (SoundOff) {
							dst[-2] = 0;
							dst[-1] = 0;
						} else if (OSGetSoundMode() == OS_SOUND_MODE_MONO) {
							mix     = (dst[-1] + dst[-2]) / 2;
							dst[-2] = mix;
							dst[-1] = mix;
						}
					}
					requestSample -= sampleNum;

					SimpleControl.audioBuffer[SimpleControl.audioOutputIndex].validSample -= sampleNum;
					SimpleControl.audioBuffer[SimpleControl.audioOutputIndex].curPtr = thpsrc;

					if (SimpleControl.audioBuffer[SimpleControl.audioOutputIndex].validSample == 0) {
						SimpleControl.audioOutputIndex++;
						if (SimpleControl.audioOutputIndex >= AUDIO_BUFFER_NUM) {
							SimpleControl.audioOutputIndex = 0;
						}
					}

					if (!requestSample) {
						break;
					}
				} else {
					memcpy(dst, libsrc, requestSample << 2);
					break;
				}
			}
		} else {
			memcpy(destination, source, sample << 2);
		}
	} else {
		if (SimpleControl.open && SimpleControl.audioState == TRUE && SimpleControl.audioExist) {
			requestSample = sample;
			dst           = destination;

			while (TRUE) {
				if (SimpleControl.audioBuffer[SimpleControl.audioOutputIndex].validSample) {
					if (SimpleControl.audioBuffer[SimpleControl.audioOutputIndex].validSample >= requestSample) {
						sampleNum = requestSample;
					} else {
						sampleNum = SimpleControl.audioBuffer[SimpleControl.audioOutputIndex].validSample;
					}

					thpsrc = SimpleControl.audioBuffer[SimpleControl.audioOutputIndex].curPtr;
					for (i = 0; i < sampleNum; i++) {
						if (SimpleControl.rampCount) {
							SimpleControl.rampCount--;
							SimpleControl.curVolume += SimpleControl.deltaVolume;
						} else {
							SimpleControl.curVolume = SimpleControl.targetVolume;
						}

						attenuation = VolumeTable[(s32)SimpleControl.curVolume];

						mix = (attenuation * (*thpsrc)) >> 15;
						if (mix < -32768) {
							mix = -32768;
						}
						if (mix > 32767) {
							mix = 32767;
						}
						*dst = (s16)mix;
						dst++;
						thpsrc++;

						mix = (attenuation * (*thpsrc)) >> 15;
						if (mix < -32768) {
							mix = -32768;
						}
						if (mix > 32767) {
							mix = 32767;
						}
						*dst = (s16)mix;
						dst++;
						thpsrc++;
					}

					requestSample -= sampleNum;

					SimpleControl.audioBuffer[SimpleControl.audioOutputIndex].validSample -= sampleNum;
					SimpleControl.audioBuffer[SimpleControl.audioOutputIndex].curPtr = thpsrc;

					if (SimpleControl.audioBuffer[SimpleControl.audioOutputIndex].validSample == 0) {
						SimpleControl.audioOutputIndex++;
						if (SimpleControl.audioOutputIndex >= AUDIO_BUFFER_NUM) {
							SimpleControl.audioOutputIndex = 0;
						}
					}

					if (!requestSample) {
						break;
					}
				} else {
					memset(dst, 0, requestSample << 2);
					break;
				}
			}
		} else {
			memset(destination, 0, sample << 2);
		}
	}
}

/**
 * @note Address: 0x801B5D00
 * @note Size: 0x48
 */
BOOL THPSimpleGetVideoInfo(THPVideoInfo* videoInfo)
{
	if (SimpleControl.open) {
		memcpy(videoInfo, &SimpleControl.videoInfo, sizeof(THPVideoInfo));
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x801B5CB8
 * @note Size: 0x48
 */
BOOL THPSimpleGetAudioInfo(THPAudioInfo* audioInfo)
{
	if (SimpleControl.open) {
		memcpy(audioInfo, &SimpleControl.audioInfo, sizeof(THPAudioInfo));
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x801B5C94
 * @note Size: 0x24
 */
u32 THPSimpleGetTotalFrame(void)
{
	if (SimpleControl.open) {
		return SimpleControl.header.numFrames;
	}
	return 0;
}

/**
 * @note Address: 0x801B5B1C
 * @note Size: 0x178
 */
static void THPAudioMixCallback(void)
{
	if (AudioSystem == 0) {
		SoundBufferIndex ^= 1; // ping-pong between the two dma buffers
		AIInitDMA((u32)SoundBuffer[SoundBufferIndex], BYTES_PER_AUDIO_FRAME);
		{
			BOOL old = OSEnableInterrupts();

			MixAudio(SoundBuffer[SoundBufferIndex], NULL, SAMPLES_PER_AUDIO_FRAME);
			DCFlushRange(SoundBuffer[SoundBufferIndex], BYTES_PER_AUDIO_FRAME);

			OSRestoreInterrupts(old);
		}
	} else {
		if (AudioSystem == 1) {
			if (LastAudioBuffer) {
				CurAudioBuffer = LastAudioBuffer;
			}

			OldAIDCallback();
			LastAudioBuffer = (s16*)OSPhysicalToCached(AIGetDMAStartAddr());
		} else {
			OldAIDCallback();
			CurAudioBuffer = (s16*)OSPhysicalToCached(AIGetDMAStartAddr());
		}

		SoundBufferIndex ^= 1;
		AIInitDMA((u32)SoundBuffer[SoundBufferIndex], BYTES_PER_AUDIO_FRAME);
		{
			BOOL old = OSEnableInterrupts();

			if (CurAudioBuffer) {
				DCInvalidateRange(CurAudioBuffer, BYTES_PER_AUDIO_FRAME);
			}
			MixAudio(SoundBuffer[SoundBufferIndex], CurAudioBuffer, SAMPLES_PER_AUDIO_FRAME);
			DCFlushRange(SoundBuffer[SoundBufferIndex], BYTES_PER_AUDIO_FRAME);

			OSRestoreInterrupts(old);
		}
	}
}
