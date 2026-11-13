#ifndef _GAME_MIC_H
#define _GAME_MIC_H

#include "types.h"
#include "Dolphin/OS/OSUtil.h"

// the mic on the memory card slots and the m2s stream it feeds
struct MicChannel {
	s32 mResult;   // _00, set by the mount callback, -3 once the mic is pulled out
	u32 mDeviceID; // _04
	s32 mState;    // _08, 0 probe, 1 mount, 2 wait, 3 ready
	s16* mBuffer;  // _0C, the mount buffer
};
STATIC_ASSERT(sizeof(MicChannel) == 0x10);

extern "C" {
extern s16* gMicDMABuffers[2];
extern u8 gMicDMABufferIndex;
extern u32 gMicSamplePhase[2];
extern void (*gPreviousAudioDMACallback)(void);
void OnMicAudioDMA();

// one per slot
extern MicChannel gMicrophoneChannels[2];
// the slot in use, -1 for none
extern s32 gActiveMicChannel;
extern s32 gMicObjectPeak; // peak above 1000 delivered to stage objects
extern s32 gMicMeterPeak;  // largest positive sample since the level bar was drawn
extern s32 sMicSamplePhaseSteps[3];
extern s32 sMicBufferThresholds[3];
extern BOOL gMicRecording;
extern OSTime gMicButtonReleaseTime;
}

#endif
