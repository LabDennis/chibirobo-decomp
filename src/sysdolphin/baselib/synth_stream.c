#include <Dolphin/ai.h>
#include <Dolphin/ar.h>
#include <sysdolphin/devcom.h>
#include <sysdolphin/dvdsche.h>
#include <dolphin/os.h>
#include <math.h>
#include <sdk/dolphin/ax.h>
#include <sdk/dolphin/os/OSRtc.h>
#include <sysdolphin/synth.h>
#include <sysdolphin/synth_bank.h>
#include <sysdolphin/synth_stream_internal.h>

typedef struct HSD_SynthPStreamChannel {
	AXPBADDR addr;
	AXPBADPCM adpcm;
} HSD_SynthPStreamChannel;

typedef struct HSD_SynthPStreamHeader {
	u32 unknown[2];
	u32 sampleRate;
	u32 channelCount;
	HSD_SynthPStreamChannel channels[2];
} HSD_SynthPStreamHeader;

STATIC_ASSERT(offsetof(HSD_SynthPStreamHeader, channels) == 0x10);
STATIC_ASSERT(sizeof(HSD_SynthPStreamHeader) == 0x80);

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

static inline u32 getAXEndAddress(const AXVPB* voice)
{
	return readAXAddress(&voice->pb.addr.endAddress);
}

static HSD_SynthPStream* sSynthActivePStreams = NULL; /* pstream entry active list */
HSD_SynthBeatMarkerBlock* sSynthFreeBeatMarkerBlocks = NULL; /* bmb free list */
static HSD_SynthPStreamRequest* sSynthFreePStreamRequests = NULL; /* request free list */
static HSD_SynthPStreamRequest* sSynthQueuedPStreamRequests = NULL; /* request queue */
static HSD_SynthPStream* sSynthFreePStreams = NULL; /* pstream entry free list */
static BOOL sSynthStreamBufferUrgent = FALSE;
static s32 sSynthStreamVolumeScale = 0;
s32 sSynthStreamBufferAramBase = 0;
s32 sSynthStreamBufferCount = 0;
s32 sSynthNextFreeStreamBuffer = 0;
static u32 sSynthSilentSampleAddr = 0;
s32 sSynthStreamBufferNext[PSTREAM_MAX_BUFFERS];
HSD_SynthPStreamHako sSynthStreamHakoBuffers[PSTREAM_MAX_BUFFERS] ATTRIBUTE_ALIGN(32);
static HSD_SynthPStream sSynthPStreams[HSD_SYNTHPSTREAM_MAX];
static s32 sSynthQueuedStreamPlaybackId = -1;

void HSD_SynthPStreamFirstHakoDataCallback(int requestID, void* context, void* buffer, BOOL cancelled);
void HSD_SynthPStreamFirstHakoHeaderCallback(int requestID, void* context, void* buffer, BOOL cancelled);
void HSD_SynthPStreamHeaderCallback(int requestID, void* context, void* buffer, BOOL cancelled);

static void freeBeatMarkers(s32 buf)
{
	HSD_SynthBeatMarkerBlock* bmb = sSynthStreamHakoBuffers[buf].markers;
	HSD_SynthBeatMarkerBlock* next;

	while (bmb != NULL) {
		next         = bmb->next;
		bmb->next    = sSynthFreeBeatMarkerBlocks;
		sSynthFreeBeatMarkerBlocks = bmb;
		bmb          = next;
	}
	sSynthStreamHakoBuffers[buf].markers = NULL;
}

static void recycleStreamBuffer(s32 bufferIndex)
{
	freeBeatMarkers(bufferIndex);
	sSynthStreamBufferNext[bufferIndex] = STREAM_BUFFER_FREE;
}

static void releaseStreamHead(HSD_SynthPStream* stream)
{
	s32 bufferIndex = stream->bufferQueueHead;

	stream->bufferQueueHead = sSynthStreamBufferNext[bufferIndex];
	recycleStreamBuffer(bufferIndex);
}

void releasePStream(HSD_SynthPStream* pst)
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
	pst->next    = sSynthFreePStreams;
	sSynthFreePStreams = pst;

	if (pst->node->sfxID == sSynthQueuedStreamPlaybackId) {
		req = sSynthQueuedPStreamRequests;
		for (rit = &sSynthQueuedPStreamRequests; *rit != NULL; rit = &(*rit)->next) {
			if (*rit == req) {
				*rit = req->next;
				break;
			}
		}
		req->next    = sSynthFreePStreamRequests;
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

static inline u32 streamChannelNibbleAddress(s32 bufferIndex, s32 channel)
{
	const HSD_SynthPStreamHako* hako = &sSynthStreamHakoBuffers[bufferIndex];

	// Stereo stores two equal-sized channels, so the stride in nibbles
	// equals the combined transfer size in bytes.
	return HSD_SynthStreamBufferAramAddress(bufferIndex) * PSTREAM_ADPCM_NIBBLES_PER_BYTE + channel * hako->sampleBytes;
}

static inline BOOL isPStreamActive(const HSD_SynthPStream* stream, s32 playbackId)
{
	HSD_SynthPStream* p;

	for (p = sSynthActivePStreams; p != NULL; p = p->next) {
		if (p == stream) {
			return p->node->sfxID == playbackId;
		}
	}
	return FALSE;
}

/**
 * @note Address: 0x801F1078
 * @note Size: 0x934
 */
void HSD_SynthPStreamMasterClockCallback(void)
{
	HSD_SynthSFXNode* node;
	HSD_SynthPStream* pstream;
	HSD_SynthPStream* next;
	HSD_SynthSFXNode* n;
	HSD_SynthBeatMarker* marker;
	u32 streamBaseNibbles;
	u32 currentAddress;
	u32 offsetNibbles;
	s32 playingBuffer;
	s32 playbackId;
	s32 i;
	s32 j;

	sSynthStreamBufferUrgent = FALSE;
	for (pstream = sSynthActivePStreams; pstream != NULL; pstream = next) {
		node = pstream->node;
		next = pstream->next;
		if (node->flags & SYNTH_VOICE_STREAM_PENDING) {
			continue;
		}

		streamBaseNibbles = (u32)sSynthStreamBufferAramBase * PSTREAM_ADPCM_NIBBLES_PER_BYTE;
		currentAddress = getAXCurrentAddress(node->voice[0]);
		if (currentAddress < streamBaseNibbles) {
			continue;
		}
		offsetNibbles = currentAddress - streamBaseNibbles;
		playingBuffer = offsetNibbles / PSTREAM_BUFFER_NIBBLES;
		playbackId = node->sfxID;
		if (playingBuffer == pstream->bufferQueueHead) {
			HSD_SynthPStreamHako* hako = &sSynthStreamHakoBuffers[playingBuffer];
			s32 playedNibbles = offsetNibbles % PSTREAM_BUFFER_NIBBLES;
			s32 playedSamples = playedNibbles * PSTREAM_ADPCM_FRAME_SAMPLES / PSTREAM_ADPCM_FRAME_NIBBLES;

			if (pstream->queuedBufferCount == 1 && hako->nextHeaderOffset != PSTREAM_LAST_HEADER
			    && getAXEndAddress(node->voice[0]) - currentAddress < PSTREAM_REFILL_THRESHOLD_NIBBLES) {
				sSynthStreamBufferUrgent = TRUE;
			}
			while (hako->nextMarkerIndex < hako->markerCount
			       && (marker = HSD_SynthGetBeatMarker(playingBuffer, hako->nextMarkerIndex))->sampleOffset < playedSamples) {
				if (pstream->beatCallback != NULL) {
					pstream->beatCallback(playbackId, marker->data, pstream->beatContext);
					// The callback can stop playback and reuse this stream slot.
					if (!isPStreamActive(pstream, playbackId)) {
						goto next_pstream;
					}
				}
				hako->nextMarkerIndex++;
			}
		} else {
			s32 oldHead = pstream->bufferQueueHead;
			HSD_SynthPStreamHako* hako = &sSynthStreamHakoBuffers[oldHead];

			while (hako->nextMarkerIndex < hako->markerCount) {
				marker = HSD_SynthGetBeatMarker(oldHead, hako->nextMarkerIndex);
				if (pstream->beatCallback != NULL) {
					pstream->beatCallback(playbackId, marker->data, pstream->beatContext);
					if (!isPStreamActive(pstream, playbackId)) {
						goto next_pstream;
					}
				}
				hako->nextMarkerIndex++;
			}
			releaseStreamHead(pstream);
			if (pstream->bufferQueueHead != playingBuffer) {
				OSReport("pst=%08x: pst->buftableheadix = %d, a=%d\n cadhi= %d pstbuf=%d\n", pstream, pstream->bufferQueueHead,
				         playingBuffer, currentAddress, streamBaseNibbles);
				__assert(__FILE__, 0xDCB, "pstream->bufTableHeadIx == a");
			}
			pstream->queuedBufferCount--;
			if (pstream->schedulerSlot >= 0) {
				SetDvdStreamingSlotKey(pstream->schedulerSlot, pstream->queuedBufferCount);
			}
			for (j = 0; j < (node->rearIndex != -1) + 1; j++) {
				if (j == 0) {
					n = node;
				} else {
					n = &sSynthSfxNodes[node->rearIndex];
				}
				for (i = 0; i < node->voice_count; i++) {
					AXSetVoiceEndAddr(n->voice[i], streamChannelNibbleAddress(playingBuffer, i)
					                                 + sSynthStreamHakoBuffers[playingBuffer].endNibble);
				}
			}
		}
		if (playingBuffer == pstream->loopBufferIndex) {
			if (pstream->queuedBufferCount >= 2) {
				pstream->loopBufferIndex = sSynthStreamBufferNext[pstream->bufferQueueHead];
				for (j = 0; j < (node->rearIndex != -1) + 1; j++) {
					if (j == 0) {
						n = node;
					} else {
						n = &sSynthSfxNodes[node->rearIndex];
					}
					for (i = 0; i < node->voice_count; i++) {
						AXSetVoiceLoopAddr(n->voice[i], streamChannelNibbleAddress(pstream->loopBufferIndex, i)
						                                  + PSTREAM_ADPCM_HEADER_NIBBLES);
						AXSetVoiceAdpcmLoop(n->voice[i], &sSynthStreamHakoBuffers[pstream->loopBufferIndex].loop[i].adpcm);
					}
				}
			} else if (sSynthStreamHakoBuffers[playingBuffer].nextHeaderOffset == PSTREAM_LAST_HEADER) {
				pstream->loopBufferIndex = STREAM_BUFFER_END;
				for (j = 0; j < (node->rearIndex != -1) + 1; j++) {
					if (j == 0) {
						n = node;
					} else {
						n = &sSynthSfxNodes[node->rearIndex];
					}
					for (i = 0; i < node->voice_count; i++) {
						AXSetVoiceLoopAddr(n->voice[i], sSynthSilentSampleAddr);
						AXSetVoiceLoop(n->voice[i], 0);
					}
				}
			}
		}
	next_pstream:;
	}
}

/**
 * @note Address: 0x801F19AC
 * @note Size: 0x408
 */
void HSD_SynthPStreamFirstHakoDataCallback(int requestID, void* context, void* buffer, BOOL cancelled)
{
	HSD_SynthSFXNode* node = getNode((s32)context);
	HSD_SynthPStream* stream;
	HSD_SynthPStreamHako* hako;
	HSD_SynthSFXNode* n;
	AXPBVE ve;
	s32 bufferIndex;
	s32 i;
	s32 j;

	if (node == NULL || node->link.pstream == NULL) {
		return;
	}
	stream = node->link.pstream;
	bufferIndex = stream->loadingBufferIndex;
	hako = &sSynthStreamHakoBuffers[bufferIndex];
	if (!(node->flags & SYNTH_VOICE_PAUSED)) {
		ve.currentVolume
		    = 32767.0f
		    * (node->u.user_vol[1].volume
		       * (node->u.user_vol[0].volume
		          * (sSynthChannelFaders[node->channel].volume * (node->channel < SYNTH_MASTER_BYPASS_CHANNEL ? sSynthStreamVolume * sSynthMasterFader.volume : sSynthStreamVolume))));
	} else {
		ve.currentVolume = 0;
	}
	ve.currentDelta = 0;
	node->curVol    = ve.currentVolume;
	stream->bufferQueueHead = bufferIndex;
	sSynthStreamBufferNext[bufferIndex] = STREAM_BUFFER_END;
	stream->loopBufferIndex = bufferIndex;
	for (j = 0; j < (node->rearIndex != -1) + 1; j++) {
		HSD_SynthSFXNode* rear;
		if (j == 1) {
			rear                   = &sSynthSfxNodes[node->rearIndex];
			rear->u.rear.lastVol   = ve.currentVolume;
			rear->u.rear.lastDelta = ve.currentDelta;
			HSD_SynthSFXLinkUpdateRearPitch(rear);
			if (node->flags & SYNTH_VOICE_PITCH_PAUSED) {
				rear->u.rear.pitch = 0.0f;
			} else {
				rear->u.rear.pitch = node->pitch[1] * (node->freqBase * node->pitch[0]);
			}
		}
		if (j == 0) {
			n = node;
		} else {
			n = rear;
		}
		for (i = 0; i < node->voice_count; i++) {
			u32 channelAddress = streamChannelNibbleAddress(bufferIndex, i);
			u32 sampleStart = channelAddress + PSTREAM_ADPCM_HEADER_NIBBLES;

			AXSetVoiceVe(n->voice[i], &ve);
			if (node->flags & SYNTH_VOICE_PITCH_PAUSED) {
				setSynthSrcRatio(0);
			} else {
				setSynthSrcRatio(node->pitch[1] * (node->freqBase * node->pitch[0]));
			}
			AXSetVoiceSrcType(n->voice[i], 2);
			AXSetVoiceSrc(n->voice[i], &sSynthSrc.params);
			AXSetVoiceCurrentAddr(n->voice[i], sampleStart);
			AXSetVoiceEndAddr(n->voice[i], channelAddress + hako->endNibble);
			AXSetVoiceLoopAddr(n->voice[i], sampleStart);
			if (sSynthSoundMode == 1) {
				AXSetVoiceItdOn(n->voice[i]);
			}
			if (j == 0) {
				AXSetVoiceState(n->voice[i], 1);
			}
		}
	}
	node->flags &= ~SYNTH_VOICE_STREAM_PENDING;
	HSD_SynthSFXUpdateVolume(node);
	HSD_SynthSFXInitMix(node);
	HSD_SynthSFXUpdateMix(node, 0);
	ApplySynthMixTargetImmediately(node);
	node->link.pstream->queuedBufferCount = 1;
	SetDvdStreamingSlotKey(node->link.pstream->schedulerSlot, 1);
	if (hako->nextHeaderOffset == PSTREAM_LAST_HEADER) {
		ReleaseDvdStreamingSlot(node->link.pstream->schedulerSlot);
		node->link.pstream->schedulerSlot = -1;
	} else {
		SetDvdStreamingSlotCallback(node->link.pstream->schedulerSlot, HSD_SynthPStreamNextHakoCallback);
	}
}

/**
 * @note Address: 0x801F1DB4
 * @note Size: 0x2D0
 */
void HSD_SynthPStreamFirstHakoMarkerCallback(int requestID, void* context, void* buffer, BOOL cancelled)
{
	HSD_SynthSFXNode* node = getNode((s32)context);
	HSD_SynthPStream* stream;
	HSD_SynthPStreamHako* hako;

	if (node == NULL || node->link.pstream == NULL || cancelled) {
		return;
	}
	stream = node->link.pstream;
	hako = &sSynthStreamHakoBuffers[stream->loadingBufferIndex];
	HSD_SynthStoreBeatMarkers(stream, buffer, hako->markerCount);
}

/**
 * @note Address: 0x801F2084
 * @note Size: 0x14C
 */
void HSD_SynthPStreamFirstHakoHeaderCallback(int requestID, void* context, void* buffer, BOOL cancelled)
{
	HSD_SynthSFXNode* node = getNode((s32)context);
	HSD_SynthPStream* stream;
	HSD_SynthPStreamHako* hako;
	s32 bufferIndex;
	s32 markerOffset = sizeof(HSD_SynthPStreamHeader) + PSTREAM_HAKO_HEADER_SIZE;
	u32 markerBytes;

	if (node == NULL || node->link.pstream == NULL) {
		return;
	}
	stream = node->link.pstream;
	bufferIndex = stream->loadingBufferIndex;
	hako = &sSynthStreamHakoBuffers[bufferIndex];
	if (stream->stopAtLoop && hako->nextHeaderOffset <= markerOffset) {
		hako->nextHeaderOffset = PSTREAM_LAST_HEADER;
	}
	markerBytes = OSRoundUp32B(hako->markerCount * sizeof(HSD_SynthBeatMarker));
	if (markerBytes != 0) {
		HSD_DevComRequest(node->link.pstream->entrynum, markerOffset, 0, markerBytes, HSD_DEVCOM_DVD_TO_BUFFER, 0,
		                  HSD_SynthPStreamFirstHakoMarkerCallback, context);
	}
	HSD_DevComRequest(node->link.pstream->entrynum, markerOffset + markerBytes, HSD_SynthStreamBufferAramAddress(bufferIndex),
	                  hako->sampleBytes, HSD_DEVCOM_DVD_TO_ARAM, 0, HSD_SynthPStreamFirstHakoDataCallback, context);
	hako->nextMarkerIndex = 0;
}

/**
 * @note Address: 0x801F21D0
 * @note Size: 0x2CC
 */
void HSD_SynthPStreamHeaderCallback(int requestID, void* context, void* buffer, BOOL cancelled)
{
	HSD_SynthSFXNode* entry = getNode((s32)context);
	HSD_SynthPStreamHeader* header = buffer;
	s32 i;
	s32 bufferIndex;

	if (entry == NULL || entry->link.pstream == NULL) {
		return;
	}
	entry->voice_count = header->channelCount;
	if (entry->voice_count == 2) {
		entry->voice[1] = AXAcquireVoice(0x1F, dropcallback, 0);
		ASSERT_SYNTH(0xE94, entry->voice[1]);
		if (entry->rearIndex != -1) {
			AXVPB* voice            = AXAcquireVoice(0x1F, dropcallback, 0);
			HSD_SynthSFXNode* rear2 = &sSynthSfxNodes[voice->index];

			rear2->sfxID                               = -3;
			rear2->voice[0]                            = sSynthSfxNodes[entry->rearIndex].voice[0];
			sSynthSfxNodes[entry->rearIndex].voice[1]    = voice;
			sSynthSfxNodes[entry->rearIndex].voice_count = 2;
			rear2->voice_count                         = 2;
		}
	}
	entry->freqBase = (1.0f / SYNTH_SAMPLE_RATE) * header->sampleRate;
	for (i = 0; i < entry->voice_count; i++) {
		setSynthSrcRatio(entry->freqBase);
		AXSetVoiceAddr(entry->voice[i], &header->channels[i].addr);
		AXSetVoiceAdpcm(entry->voice[i], &header->channels[i].adpcm);
	}
	if (entry->rearIndex != -1) {
		HSD_SynthSFXNode* rear = &sSynthSfxNodes[entry->rearIndex];

		for (i = 0; i < entry->voice_count; i++) {
			AXSetVoiceAddr(rear->voice[i], &header->channels[i].addr);
			AXSetVoiceAdpcm(rear->voice[i], &header->channels[i].adpcm);
		}
	}
	bufferIndex = HSD_SynthTakeStreamBuffer();
	entry->link.pstream->loadingBufferIndex = bufferIndex;
	ASSERT_SYNTH(0xEB4, bufferIndex != STREAM_BUFFER_FREE);
	HSD_DevComRequest(entry->link.pstream->entrynum, sizeof(HSD_SynthPStreamHeader), (u32)&sSynthStreamHakoBuffers[bufferIndex],
	                  PSTREAM_HAKO_HEADER_SIZE, HSD_DEVCOM_DVD_TO_MRAM, 0, HSD_SynthPStreamFirstHakoHeaderCallback, context);
}

/**
 * @note Address: 0x801F249C
 * @note Size: 0xA0
 */
void HSD_SynthPStreamStartCallback(s32 pstreamID)
{
	HSD_SynthSFXNode* node = getNode(pstreamID);

	ASSERT_SYNTH_MSG(0xEC2, node != NULL, "entry");
	HSD_DevComRequest(node->link.pstream->entrynum, 0, 0, sizeof(HSD_SynthPStreamHeader), HSD_DEVCOM_DVD_TO_BUFFER, 0,
	                  HSD_SynthPStreamHeaderCallback, (void*)(u32)pstreamID);
}

/**
 * @note Address: N/A
 * @note Size: 0x3B8
 */
static inline HSD_SynthPStream* allocPStream(void)
{
	HSD_SynthPStream* pst = sSynthFreePStreams;

	if (pst != NULL) {
		sSynthFreePStreams = pst->next;
		return pst;
	}
	return NULL;
}

/**
 * @note Address: 0x801F253C
 * @note Size: 0x3C0
 */
s32 HSD_SynthPStreamStart(s32 entrynum, s32 vol, s32 volMode, s32 vol2, s32 vol2Mode, u8 macroPan, u8 userPan, u8 macroSPan, f32 pitch1,
                          f32 pitch2, f32 mixMain, f32 mixAuxA, f32 mixAuxB, s32 userSPan, s32 channel, s32 useRear)
{
	u32 index;
	s32 playbackId;
	BOOL enabled;
	AXVPB* voice;
	HSD_SynthSFXNode* node;
	HSD_SynthPStream* pst;

	if (entrynum < 0) {
		return 0;
	}
	if (channel < 0 || channel >= SYNTH_CHANNEL_COUNT) {
		return 0;
	}
	enabled = OSDisableInterrupts();
	if (sSynthSoundMode != 4) {
		useRear = 0;
	}
	voice = AXAcquireVoice(0x1F, dropcallback, 0);
	index = voice->index;
	node  = &sSynthSfxNodes[index];

	playbackId = nextPlaybackId(index);

	node->link.pstream = allocPStream();
	node->link.pstream->entrynum = entrynum;
	node->link.pstream->node = node;
	node->link.pstream->bufferQueueHead = STREAM_BUFFER_END;
	node->link.pstream->loadingBufferIndex = -1;
	node->link.pstream->queuedBufferCount = 0;
	node->link.pstream->beatCallback = NULL;
	node->link.pstream->beatContext = NULL;
	node->link.pstream->stopAtLoop = 0;
	pst = node->link.pstream;
	pst->next = sSynthActivePStreams;
	sSynthActivePStreams = pst;

	node->sfxID       = playbackId;
	node->sampleId    = 0;
	node->flags       = SYNTH_VOICE_STREAM_PENDING;
	node->voice_count = 1;
	node->channel     = channel;
	node->voice[0]    = voice;
	node->rearIndex   = -1;
	node->pitch[0]    = pitch1;
	node->pitch[1]    = pitch2;

	if (volMode == 0) {
		node->u.user_vol[0].volume   = 0.003921569f * vol;
		node->u.user_vol[0].target.f = node->u.user_vol[0].volume;
	} else {
		s32 raw                      = decibelsToDBx(vol);
		node->u.user_vol[0].volume   = HSD_SynthDBxToVol(raw);
		node->u.user_vol[0].target.i = raw;
		node->u.user_vol[0].dbxRaw   = raw;
	}
	node->u.user_vol[0].mode      = volMode;
	node->u.user_vol[0].duration = 0;

	if (vol2Mode == 0) {
		node->u.user_vol[1].volume   = 0.003921569f * vol2;
		node->u.user_vol[1].target.f = node->u.user_vol[1].volume;
	} else {
		s32 raw                      = decibelsToDBx(vol2);
		node->u.user_vol[1].volume   = HSD_SynthDBxToVol(raw);
		node->u.user_vol[1].target.i = raw;
		node->u.user_vol[1].dbxRaw   = raw;
	}
	node->u.user_vol[1].mode      = vol2Mode;
	node->u.user_vol[1].duration = 0;

	node->userPan[0]  = macroPan;
	node->userPan[1]  = userPan;
	node->userSPan[0] = macroSPan;
	node->userSPan[1] = userSPan;
	node->mixMain     = mixMain;
	node->mixAuxA     = mixAuxA;
	node->mixAuxB     = mixAuxB;

	if (useRear != 0) {
		AXVPB* rearVoice       = AXAcquireVoice(0x1F, dropcallback, 0);
		u32 rearIdx            = rearVoice->index;
		HSD_SynthSFXNode* rear = &sSynthSfxNodes[rearIdx];

		rear->sfxID       = -2;
		rear->rearIndex   = index;
		node->rearIndex   = rearIdx;
		rear->voice_count = 1;
		rear->voice[0]    = rearVoice;
	} else {
		node->rearIndex = -1;
	}
	node->link.pstream->schedulerSlot = AcquireDvdStreamingSlot(HSD_SynthPStreamStartCallback, node->sfxID);
	OSRestoreInterrupts(enabled);
	return node->sfxID;
}

/**
 * @note Address: N/A
 * @note Size: 0x24
 */
void HSD_SynthStreamPause(void)
{
	AISetStreamPlayState(0);
}

/**
 * @note Address: N/A
 * @note Size: 0x24
 */
void HSD_SynthStreamResume(void)
{
	AISetStreamPlayState(1);
}

/**
 * @note Address: N/A
 * @note Size: 0x118
 */
void HSD_SynthStreamSetVolume(f32 volume)
{
	s32 i;
	BOOL enabled;

	sSynthStreamVolume = volume;
	AISetStreamVolLeft(volume * (f32)sSynthStreamVolumeScale);
	AISetStreamVolRight(sSynthStreamVolume * (f32)sSynthStreamVolumeScale);
	enabled = OSDisableInterrupts();
	for (i = 0; i < HSD_SYNTHSFXNODE_MAX; i++) {
		HSD_SynthSFXNode* node = &sSynthSfxNodes[i];
		if (node->sfxID > 0) {
			HSD_SynthSFXUpdateVolume(node);
		}
	}
	OSRestoreInterrupts(enabled);
}

/**
 * @note Address: 0x801F28FC
 * @note Size: 0x14
 */
s32 HSD_SynthPStreamIsActive(void)
{
	return sSynthActivePStreams != NULL;
}

/**
 * @note Address: 0x801F2910
 * @note Size: 0x98
 */
void HSD_SynthPStreamSetBeatCallback(s32 id, HSD_SynthBeatCallback callback, void* context)
{
	HSD_SynthSFXNode* node;
	BOOL enabled = OSDisableInterrupts();

	node = getNode(id);
	if (node != NULL && node->link.pstream != NULL) {
		node->link.pstream->beatCallback = callback;
		node->link.pstream->beatContext = context;
	}
	OSRestoreInterrupts(enabled);
}

/**
 * @note Address: 0x801F29A8
 * @note Size: 0x468
 */
void HSD_SynthInit(s32 heapSize, s32 pstNum, s32 pstBufNum)
{
	s32 i;

	AXInit();
	ASSERT_SYNTH_REPORT(0xFDC, pstNum >= 1 && pstNum <= HSD_SYNTHPSTREAM_MAX, "HSD_AudioInit*: pstNum out of range.\n");
	ASSERT_SYNTH_REPORT(0xFDD, pstBufNum >= 1 && pstBufNum <= PSTREAM_MAX_BUFFER_DEPTH, "HSD_AudioInit*: pstBufNum out of range.\n");
	SetDvdStreamingLimits(pstNum, pstBufNum);
	sSynthStreamBufferCount = pstNum * (pstBufNum + 1) + 1;
	AISetDSPSampleRate(0);
	sSynthSilentSampleAddr = ARAlloc(0x500);
	HSD_DevComRequest(0, 0, sSynthSilentSampleAddr, 0x500, HSD_DEVCOM_CLEAR_ARAM, 0, NULL, NULL);
	sSynthSilentSampleAddr *= 2; // AX uses nibble addresses for ADPCM samples.
	hsd_SynthSFXBankHead[0] = (s32)ARAlloc(heapSize);
	AXRegisterCallback(HSD_SynthCallback);
	sSynthStreamVolumeScale = 0xFF;
	AISetStreamVolLeft(sSynthStreamVolume * (f32)sSynthStreamVolumeScale);
	AISetStreamVolRight(sSynthStreamVolume * (f32)sSynthStreamVolumeScale);

	sSynthMasterFader.volume   = 1.0f;
	sSynthMasterFader.target.f = 1.0f;
	sSynthMasterFader.duration = 0;
	for (i = 0; i < ARRAY_SIZE(sSynthChannelFaders); i++) {
		sSynthChannelFaders[i].volume   = 1.0f;
		sSynthChannelFaders[i].target.f = 1.0f;
		sSynthChannelFaders[i].duration = 0;
	}

	sSynthSfxBankHeapEnd = hsd_SynthSFXBankHead[0] + heapSize;
	sSynthStreamBufferAramBase = (s32)ARAlloc(sSynthStreamBufferCount * PSTREAM_BUFFER_BYTES);
	sSynthSoundMode = OSGetSoundMode();

	for (i = 0; i <= 0xFE; i++) {
		sSynthSqrtLookup[i] = sqrt(0.003937008f * i);
	}
	sSynthSqrtLookup[0xFF] = sSynthSqrtLookup[0xFE];

	for (i = 0; i < pstNum; i++) {
		sSynthPStreams[i].next = sSynthFreePStreams;
		sSynthFreePStreams = &sSynthPStreams[i];
	}

	for (i = 0; i < sSynthStreamBufferCount; i++) {
		sSynthStreamBufferNext[i] = STREAM_BUFFER_FREE;
	}
}
