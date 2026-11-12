#include "game/cgame.h"
#include "game/mic.h"
#include "game/cgameutil.h"

#include "Dolphin/mic.h"

extern "C" void OnMicrophoneDetached(s32 index, s32 result);

extern "C" {
// one per slot
MicChannel gMicrophoneChannels[2] = {
	{ MIC_RESULT_NOCARD, 0, 0, NULL },
	{ MIC_RESULT_NOCARD, 0, 0, NULL },
};
s32 sMicSamplePhaseSteps[3] = { 22557, 22559, 22561 };
s32 sMicBufferThresholds[3] = { 80, 160, 240 };
// the slot in use, -1 for none
s32 gActiveMicChannel = -1;
BOOL gMicRecording;
OSTime gMicButtonReleaseTime;
s32 gMicObjectPeak; // peak above 1000 delivered to stage objects
s32 gMicMeterPeak;  // largest positive sample since the level bar was drawn
s16* gMicDMABuffers[2] ATTRIBUTE_ALIGN(32);
u8 gMicDMABufferIndex;
u32 gMicSamplePhase[2];
void (*gPreviousAudioDMACallback)(void);
}

/**
 * @note Address: 0x8000A6D8
 * @note Size: 0x180
 */
// NONMATCHING: the work area isn't read back for the mount
// mounts the mic in slot B and keeps it mounted
extern "C" void UpdateMicrophoneMount(void)
{
	MicChannel* slot = &gMicrophoneChannels[1];
	s32 result;
	switch (slot->mState) {
	case 0:
		if (MICProbeEx(1) == 0) {
			slot->mState = 1;
		}
		break;
	case 1:
		slot->mBuffer = (s16*)OSAllocFromHeap(__OSCurrHeap, MIC_RINGBUFF_SIZE);
		switch (result = MICMountAsync(1, slot->mBuffer, MIC_RINGBUFF_SIZE, OnMicrophoneDetached, NULL)) {
		case 0:
			slot->mState = 2;
			break;
		case -1:
			break;
		default:
			goto failed;
		}
		break;
	case 2:
		switch (result = MICGetResultCode(1)) {
		case 0:
			MICGetDeviceID(1, &slot->mDeviceID);
			slot->mResult = 0;
			slot->mState  = 3;
			break;
		case -1:
			break;
		default:
			goto failed;
		}
		break;
	case 3:
		if (slot->mResult != 0) {
			MICUnmount(1);
			OSFreeToHeap(__OSCurrHeap, slot->mBuffer);
			slot->mBuffer = NULL;
			slot->mState  = 0;
		}
		break;
	}
	return;

failed:
	switch (result) {
	case -2:
		OSFreeToHeap(__OSCurrHeap, slot->mBuffer);
		slot->mBuffer = NULL;
		slot->mState  = 0;
		break;
	case -3:
		OSFreeToHeap(__OSCurrHeap, slot->mBuffer);
		slot->mBuffer = NULL;
		slot->mState  = 0;
		break;
	}
}
