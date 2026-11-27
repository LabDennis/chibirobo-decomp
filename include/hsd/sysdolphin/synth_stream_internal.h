#ifndef HSD_SYNTH_STREAM_INTERNAL_H
#define HSD_SYNTH_STREAM_INTERNAL_H

#include <dolphin/os.h>
#include <sysdolphin/dvdsche.h>
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

// a queued stream start, kicked off when the current stream goes away
typedef struct HSD_SynthPStreamRequest {
	struct HSD_SynthPStreamRequest* next;
	s32 entrynum;
	s16 vol;
	s16 vol2;
	u8 volMode;
	u8 vol2Mode;
	u8 macroPan;
	u8 userPan;
	u8 macroSPan;
	u8 userSPan;
	f32 pitch1;
	f32 pitch2;
	f32 mixMain;
	f32 mixAuxA;
	f32 mixAuxB;
	s16 channel;
	s16 useRear;
} HSD_SynthPStreamRequest;

#define HSD_SYNTHPSTREAM_MAX 4
#define PSTREAM_MAX_BUFFER_DEPTH 3
#define PSTREAM_MAX_BUFFERS (HSD_SYNTHPSTREAM_MAX * (PSTREAM_MAX_BUFFER_DEPTH + 1) + 1)

extern HSD_SynthPStream* sSynthActivePStreams;
extern HSD_SynthBeatMarkerBlock* sSynthFreeBeatMarkerBlocks;
extern HSD_SynthPStreamRequest* sSynthFreePStreamRequests;
extern HSD_SynthPStreamRequest* sSynthQueuedPStreamRequests;
extern HSD_SynthPStream* sSynthFreePStreams;
extern s32 sSynthStreamBufferAramBase;
extern s32 sSynthStreamBufferCount;
extern s32 sSynthStreamBufferNext[PSTREAM_MAX_BUFFERS];
extern HSD_SynthPStreamHako sSynthStreamHakoBuffers[PSTREAM_MAX_BUFFERS];
extern s32 sSynthNextFreeStreamBuffer;
extern s32 sSynthQueuedStreamPlaybackId;

s32 HSD_SynthTakeStreamBuffer(void);
u32 HSD_SynthStreamBufferAramAddress(s32 bufferIndex);

static inline void freeBeatMarkers(s32 buf)
{
	HSD_SynthBeatMarkerBlock* bmb = sSynthStreamHakoBuffers[buf].markers;
	HSD_SynthBeatMarkerBlock* next;

	while (bmb != NULL) {
		next                       = bmb->next;
		bmb->next                  = sSynthFreeBeatMarkerBlocks;
		sSynthFreeBeatMarkerBlocks = bmb;
		bmb                        = next;
	}
	sSynthStreamHakoBuffers[buf].markers = NULL;
}

static inline void recycleStreamBuffer(s32 bufferIndex)
{
	freeBeatMarkers(bufferIndex);
	sSynthStreamBufferNext[bufferIndex] = STREAM_BUFFER_FREE;
}

static inline void releaseStreamHead(HSD_SynthPStream* stream)
{
	s32 bufferIndex = stream->bufferQueueHead;

	stream->bufferQueueHead = sSynthStreamBufferNext[bufferIndex];
	recycleStreamBuffer(bufferIndex);
}

static inline void releasePStream(HSD_SynthPStream* pst)
{
	HSD_SynthPStream** pit;
	HSD_SynthPStreamRequest** rit;
	HSD_SynthPStreamRequest* req;
	BOOL enabled;

	pit = &sSynthActivePStreams;
	if (pst->schedulerSlot >= 0) {
		ReleaseDvdStreamingSlot(pst->schedulerSlot);
	}
	for (; *pit != NULL && *pit != pst; pit = &(*pit)->next) { }
	if (*pit != NULL) {
		// Only completed buffers belong to this queue.
		while (pst->bufferQueueHead != STREAM_BUFFER_END) {
			releaseStreamHead(pst);
		}
		*pit = (*pit)->next;
	}
	pst->next          = sSynthFreePStreams;
	sSynthFreePStreams = pst;

	if (pst->node->sfxID == sSynthQueuedStreamPlaybackId) {
		req = sSynthQueuedPStreamRequests;
		for (rit = &sSynthQueuedPStreamRequests; *rit != NULL; rit = &(*rit)->next) {
			if (*rit == req) {
				*rit = req->next;
				break;
			}
		}
		req->next                 = sSynthFreePStreamRequests;
		sSynthFreePStreamRequests = req;

		enabled = OSDisableInterrupts();
		req     = sSynthQueuedPStreamRequests;
		if (req != NULL) {
			sSynthQueuedStreamPlaybackId =
			    HSD_SynthPStreamStart(req->entrynum, req->vol, req->volMode, req->vol2, req->vol2Mode, req->macroPan, req->userPan,
			                          req->macroSPan, req->pitch1, req->pitch2, req->mixMain, req->mixAuxA, req->mixAuxB,
			                          req->userSPan, req->channel, req->useRear);
		} else {
			sSynthQueuedStreamPlaybackId = -1;
		}
		OSRestoreInterrupts(enabled);
	}
}
static inline HSD_SynthBeatMarkerBlock* allocBeatMarkerBlock(void)
{
	HSD_SynthBeatMarkerBlock* block = sSynthFreeBeatMarkerBlocks;
	s32 i;

	if (block != NULL) {
		sSynthFreeBeatMarkerBlocks = block->next;
	} else {
		block = HSD_AudioMalloc(PSTREAM_MARKER_BLOCKS_PER_ALLOC * sizeof(*block));
		for (i = 0; i < PSTREAM_MARKER_BLOCKS_PER_ALLOC - 1; i++) {
			block[i].next = sSynthFreeBeatMarkerBlocks;
			sSynthFreeBeatMarkerBlocks = &block[i];
		}
		block += PSTREAM_MARKER_BLOCKS_PER_ALLOC - 1;
	}
	return block;
}

static inline HSD_SynthBeatMarker* HSD_SynthGetBeatMarker(s32 bufferIndex, s32 markerIndex)
{
	HSD_SynthPStreamHako* hako = &sSynthStreamHakoBuffers[bufferIndex];
	HSD_SynthBeatMarkerBlock* block = hako->markers;
	HSD_SynthBeatMarkerBlock* next;
	s32 i;

	if (block == NULL) {
		block = allocBeatMarkerBlock();
		ASSERT_SYNTH_MSG(0x4BC, block, "bmb");
		hako->markers = block;
		block->next = NULL;
	}
	while (markerIndex >= PSTREAM_MARKERS_PER_BLOCK) {
		next = block->next;
		if (next == NULL) {
			next = allocBeatMarkerBlock();
			ASSERT_SYNTH_MSG(0x4C4, next, "bmbnext");
			block->next = next;
			next->next = NULL;
			for (i = 0; i < PSTREAM_MARKERS_PER_BLOCK; i++) {
				next->marker[i].sampleOffset = 0;
			}
		}
		block = next;
		markerIndex -= PSTREAM_MARKERS_PER_BLOCK;
	}
	return &block->marker[markerIndex];
}
void HSD_SynthPStreamNextHakoCallback(s32 playbackId);

static inline s32 lastQueuedStreamBuffer(HSD_SynthPStream* pst)
{
	s32* link = &pst->bufferQueueHead;

	while (sSynthStreamBufferNext[*link] != STREAM_BUFFER_END) {
		link = &sSynthStreamBufferNext[*link];
	}
	return *link;
}

static inline void HSD_SynthStoreBeatMarkers(HSD_SynthPStream* stream, const HSD_SynthBeatMarker* markers, s32 count)
{
	BOOL enabled = OSDisableInterrupts();
	s32 i;

	for (i = 0; i < count; i++) {
		*HSD_SynthGetBeatMarker(stream->loadingBufferIndex, i) = markers[i];
	}
	OSRestoreInterrupts(enabled);
}

#endif
