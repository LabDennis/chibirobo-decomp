#include <dolphin/os.h>
#include <sysdolphin/devcom.h>
#include <sysdolphin/dvdsche.h>
#include <sysdolphin/synth_stream_internal.h>

static s32* streamBufferQueueTailLink(HSD_SynthPStream* stream)
{
	s32* link = &stream->bufferQueueHead;

	while (*link != STREAM_BUFFER_END) {
		link = &sSynthStreamBufferNext[*link];
	}
	return link;
}

u32 HSD_SynthStreamBufferAramAddress(s32 bufferIndex)
{
	return sSynthStreamBufferAramBase + (u32)bufferIndex * PSTREAM_BUFFER_BYTES;
}

s32 HSD_SynthTakeStreamBuffer(void)
{
	s32 bufferIndex = sSynthNextFreeStreamBuffer;
	s32 i;

	// Return the saved buffer and keep a spare for the next transfer.
	for (i = 0; i < sSynthStreamBufferCount; i++) {
		if (i != bufferIndex && sSynthStreamBufferNext[i] == STREAM_BUFFER_FREE) {
			sSynthNextFreeStreamBuffer = i;
		}
	}
	return bufferIndex == sSynthNextFreeStreamBuffer ? STREAM_BUFFER_FREE : bufferIndex;
}

/**
 * @note Address: 0x801F09A8
 * @note Size: 0x10C
 */
// NONMATCHING: the remaining diff is queue traversal register allocation and global-base scheduling.
void QueueLoadedSynthStreamBuffer(int requestID, void* context, void* buffer, BOOL cancelled)
{
	HSD_SynthSFXNode* node = getNode((s32)context);
	s32 bufferIndex;

	if (node == NULL || node->link.pstream == NULL) {
		return;
	}

	bufferIndex = node->link.pstream->loadingBufferIndex;
	*streamBufferQueueTailLink(node->link.pstream) = bufferIndex;
	sSynthStreamBufferNext[bufferIndex] = STREAM_BUFFER_END;
	node->link.pstream->queuedBufferCount++;
	SetDvdStreamingSlotKey(node->link.pstream->schedulerSlot, node->link.pstream->queuedBufferCount);

	// The scheduler can service a callback while updating the slot.
	bufferIndex = node->link.pstream->loadingBufferIndex;
	if (sSynthStreamHakoBuffers[bufferIndex].nextHeaderOffset == PSTREAM_LAST_HEADER) {
		ReleaseDvdStreamingSlot(node->link.pstream->schedulerSlot);
		node->link.pstream->schedulerSlot = -1;
	}
	node->link.pstream->loadingBufferIndex = STREAM_BUFFER_FREE;
}

/**
 * @note Address: 0x801F0AB4
 * @note Size: 0x2EC
 */
void HSD_SynthPStreamHakoMarkerCallback(int requestID, void* context, void* buffer, BOOL cancelled)
{
	HSD_SynthSFXNode* node = getNode((s32)context);
	HSD_SynthPStream* stream;
	HSD_SynthPStreamHako* hako;

	if (node == NULL || node->link.pstream == NULL || cancelled) {
		return;
	}
	stream = node->link.pstream;
	if (stream->loadingBufferIndex == STREAM_BUFFER_FREE) {
		return;
	}

	hako = &sSynthStreamHakoBuffers[stream->loadingBufferIndex];
	HSD_SynthStoreBeatMarkers(stream, buffer, hako->markerCount);
	hako->nextMarkerIndex = 0;
}

/**
 * @note Address: 0x801F0DA0
 * @note Size: 0x16C
 */
void HSD_SynthPStreamHakoHeaderCallback(int requestID, void* context, void* buffer, BOOL cancelled)
{
	HSD_SynthSFXNode* node = getNode((s32)context);
	HSD_SynthPStream* stream;
	HSD_SynthPStreamHako* hako;
	s32 bufferIndex;
	s32 headerOffset;
	u32 markerOffset;
	u32 markerBytes;

	if (node == NULL || node->link.pstream == NULL) {
		return;
	}

	stream = node->link.pstream;
	bufferIndex = stream->loadingBufferIndex;
	if (bufferIndex == STREAM_BUFFER_FREE) {
		return;
	}
	headerOffset = sSynthStreamHakoBuffers[lastQueuedStreamBuffer(stream)].nextHeaderOffset;
	hako = &sSynthStreamHakoBuffers[bufferIndex];
	if (stream->stopAtLoop && hako->nextHeaderOffset <= headerOffset) {
		hako->nextHeaderOffset = PSTREAM_LAST_HEADER;
	}
	markerOffset = (u32)headerOffset + PSTREAM_HAKO_HEADER_SIZE;
	markerBytes = OSRoundUp32B(hako->markerCount * sizeof(HSD_SynthBeatMarker));
	if (markerBytes != 0) {
		HSD_DevComRequest(node->link.pstream->entrynum, markerOffset, 0, markerBytes, HSD_DEVCOM_DVD_TO_BUFFER, 0,
		                  HSD_SynthPStreamHakoMarkerCallback, context);
	}
	HSD_DevComRequest(node->link.pstream->entrynum, markerOffset + markerBytes, HSD_SynthStreamBufferAramAddress(bufferIndex),
	                  hako->sampleBytes, HSD_DEVCOM_DVD_TO_ARAM, 0, QueueLoadedSynthStreamBuffer, context);
}

/**
 * @note Address: 0x801F0F0C
 * @note Size: 0x16C
 */
void HSD_SynthPStreamNextHakoCallback(s32 playbackId)
{
	BOOL enabled = OSDisableInterrupts();
	HSD_SynthSFXNode* node = getNode(playbackId);

	if (node != NULL && node->link.pstream != NULL) {
		s32 headerOffset = sSynthStreamHakoBuffers[lastQueuedStreamBuffer(node->link.pstream)].nextHeaderOffset;
		s32 bufferIndex;

		ASSERT_SYNTH(0xD7B, headerOffset != PSTREAM_LAST_HEADER);
		bufferIndex = HSD_SynthTakeStreamBuffer();
		if (bufferIndex != STREAM_BUFFER_FREE) {
			node->link.pstream->loadingBufferIndex = bufferIndex;
			HSD_DevComRequest(node->link.pstream->entrynum, (u32)headerOffset, (u32)&sSynthStreamHakoBuffers[bufferIndex],
			                  PSTREAM_HAKO_HEADER_SIZE, HSD_DEVCOM_DVD_TO_MRAM, 0, HSD_SynthPStreamHakoHeaderCallback,
			                  (void*)playbackId);
		}
	}
	OSRestoreInterrupts(enabled);
}
