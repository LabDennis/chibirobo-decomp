#include "game/cgame.h"
#include "game/mic.h"
#include "game/cgameutil.h"

#include "Dolphin/mic.h"

/**
 * @note Address: 0x8000A858
 * @note Size: 0x18
 */
extern "C" void OnMicrophoneDetached(s32 index, s32)
{
	gMicrophoneChannels[index].mResult = MIC_RESULT_NOCARD;
}

extern "C" {
void UpdateMicrophoneMount(void);
u32 CheckStageFlag(s32 stage, u32 mask);
BOOL M2SSetActiveChannel(s32 chan);
BOOL M2SSetMode(s32 mode);
BOOL M2SSetPrerecordSamples(s32 milliseconds);
void M2SOpen(void);
void M2SClose(void);
void M2SStart(void);
void M2SStop(void);
}

// whether the stage takes the mic, which ended up true either way
static inline BOOL IsMicStage()
{
	if (CheckStageFlag(gGame.mStage, 2)) {
		return TRUE;
	}
	return TRUE;
}

/**
 * @note Address: 0x8000A870
 * @note Size: 0x31C
 */
// the mic: opens it in whichever slot has one, records while its button is held and a little
// after, and hands the loudest sample to every object
extern "C" void UpdateStageMicrophone(u32 frames)
{
	if (gGame.mStatus.mFlags & 1) {
		UpdateMicrophoneMount();
		if (IsMicStage()) {
			if (gActiveMicChannel < 0) {
				if (gMicrophoneChannels[0].mResult == 0) {
					gActiveMicChannel = 0;
				} else if (gMicrophoneChannels[1].mResult == 0) {
					gActiveMicChannel = 1;
				}
				if (gActiveMicChannel >= 0 && gMicrophoneChannels[gActiveMicChannel].mDeviceID == 0) {
					M2SSetActiveChannel(gActiveMicChannel);
					M2SSetMode(3);
					M2SSetPrerecordSamples(400);
					M2SOpen();
					MICSetGain(gActiveMicChannel, 0);
					MICStart(gActiveMicChannel);
				}
			} else if (gMicrophoneChannels[gActiveMicChannel].mResult != 0) {
				M2SClose();
				gActiveMicChannel = -1;
			}
		}

		if (gActiveMicChannel >= 0 && gMicrophoneChannels[gActiveMicChannel].mDeviceID == 0) {
			u32 button;
			MICGetButton(gActiveMicChannel, &button);
			if (button & MIC_BUTTON_TALK) {
				if (!gMicRecording && gMicButtonReleaseTime == 0) {
					M2SStart();
					gMicRecording = TRUE;
				}
			} else if (gMicRecording && gMicButtonReleaseTime == 0) {
				gMicButtonReleaseTime = OSGetTime();
			}
			if (OSGetTime() - gMicButtonReleaseTime > OSMillisecondsToTicks(100) && gMicButtonReleaseTime != 0 && gMicRecording) {
				M2SStop();
				gMicRecording         = FALSE;
				gMicButtonReleaseTime = 0;
			}
		}

		if (gMicObjectPeak != 0) {
			if ((gGame.mStatus.mFlags & 1) && !(gGame.mStatus.mFlags & 2)) {
				for (s32 i = 0; i < gGame.mDrawLists[0].mCount; i++) {
					if (gGame.mDrawLists[0].mObjects[i]->mStatus.mFlags & 1) {
						gGame.mDrawLists[0].mObjects[i]->OnMicrophonePeak(-1, gMicObjectPeak);
					}
				}
			}
			gMicObjectPeak = 0;
		}
	}
}

/**
 * @note Address: 0x8000AB8C
 * @note Size: 0x4
 */
void XObj::OnMicrophonePeak(s32, s32)
{
}
