#ifndef HSD_SYNTH_STREAM_INTERNAL_H
#define HSD_SYNTH_STREAM_INTERNAL_H

#include <sysdolphin/synth.h>
#include <sysdolphin/synth_internal.h>

enum {
	PSTREAM_MARKERS_PER_BLOCK = 3,
	PSTREAM_MARKER_BLOCKS_PER_ALLOC = 8,
};

typedef struct HSD_SynthBeatMarker {
	s32 sampleOffset; // _00
	s32 data;         // _04
} HSD_SynthBeatMarker;

typedef struct HSD_SynthBeatMarkerBlock {
	struct HSD_SynthBeatMarkerBlock* next;                // _00
	s32 reserved;                                        // _04
	HSD_SynthBeatMarker marker[PSTREAM_MARKERS_PER_BLOCK]; // _08
} HSD_SynthBeatMarkerBlock;

STATIC_ASSERT(sizeof(HSD_SynthBeatMarkerBlock) == 0x20);

// Each stream buffer has a 0x20-byte disc header followed by runtime state.
typedef struct HSD_SynthPStreamHako {
	s32 sampleBytes;                  // _00
	s32 endNibble;                    // _04
	s32 nextHeaderOffset;             // _08
	struct {
		AXPBADPCMLOOP adpcm;
		u16 padding;
	} loop[2];                         // _0C
	u8 markerCount;                   // _1C
	HSD_SynthBeatMarkerBlock* markers; // _20
	s32 nextMarkerIndex;              // _24
	u8 reserved[0x40 - 0x28];          // _28
} HSD_SynthPStreamHako;

enum { PSTREAM_HAKO_HEADER_SIZE = offsetof(HSD_SynthPStreamHako, markers) };

STATIC_ASSERT(PSTREAM_HAKO_HEADER_SIZE == 0x20);
STATIC_ASSERT(sizeof(HSD_SynthPStreamHako) == 0x40);

// An in-flight transfer is tracked by the stream, outside the playback queue.
enum {
	STREAM_BUFFER_FREE = -1,
	STREAM_BUFFER_END = -2,
};

enum {
	PSTREAM_LAST_HEADER = -1,
	PSTREAM_BUFFER_BYTES = 0x10000,
	PSTREAM_ADPCM_NIBBLES_PER_BYTE = 2,
	PSTREAM_BUFFER_NIBBLES = PSTREAM_BUFFER_BYTES * PSTREAM_ADPCM_NIBBLES_PER_BYTE,
	PSTREAM_ADPCM_FRAME_NIBBLES = 16,
	PSTREAM_ADPCM_FRAME_SAMPLES = 14,
	PSTREAM_ADPCM_HEADER_NIBBLES = 2,
	PSTREAM_REFILL_THRESHOLD_NIBBLES = 0xE60,
};

typedef struct HSD_SynthPStream {
	struct HSD_SynthPStream* next;
	HSD_SynthSFXNode* node;
	s32 entrynum;
	s32 schedulerSlot;
	s32 bufferQueueHead;
	s32 loopBufferIndex;
	s32 queuedBufferCount;
	s32 loadingBufferIndex;
	HSD_SynthBeatCallback beatCallback;
	void* beatContext;
	s32 stopAtLoop;
} HSD_SynthPStream;

STATIC_ASSERT(sizeof(HSD_SynthPStream) == 0x2C);

#define HSD_SYNTHPSTREAM_MAX 4
#define PSTREAM_MAX_BUFFER_DEPTH 3
#define PSTREAM_MAX_BUFFERS (HSD_SYNTHPSTREAM_MAX * (PSTREAM_MAX_BUFFER_DEPTH + 1) + 1)

extern HSD_SynthBeatMarkerBlock* sSynthFreeBeatMarkerBlocks;
extern s32 sSynthStreamBufferAramBase;
extern s32 sSynthStreamBufferCount;
extern s32 sSynthStreamBufferNext[PSTREAM_MAX_BUFFERS];
extern HSD_SynthPStreamHako sSynthStreamHakoBuffers[PSTREAM_MAX_BUFFERS];
extern s32 sSynthNextFreeStreamBuffer;

s32 HSD_SynthTakeStreamBuffer(void);
u32 HSD_SynthStreamBufferAramAddress(s32 bufferIndex);
HSD_SynthBeatMarker* HSD_SynthGetBeatMarker(s32 bufferIndex, s32 markerIndex);
void HSD_SynthStoreBeatMarkers(HSD_SynthPStream* stream, const HSD_SynthBeatMarker* markers, s32 count);
void HSD_SynthPStreamNextHakoCallback(s32 playbackId);

static inline s32 lastQueuedStreamBuffer(HSD_SynthPStream* pst)
{
	s32* link = &pst->bufferQueueHead;

	while (sSynthStreamBufferNext[*link] != STREAM_BUFFER_END) {
		link = &sSynthStreamBufferNext[*link];
	}
	return *link;
}

#endif
