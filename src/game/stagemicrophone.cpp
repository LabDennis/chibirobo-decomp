#include "game/cgame.h"
#include "game/mic.h"
#include "game/cgameutil.h"

#include "Dolphin/m2s.h"
#include "Dolphin/mic.h"

/**
 * @note Address: 0x8000A858
 * @note Size: 0x18
 */
extern "C" void OnMicrophoneDetached(s32 index, s32)
{
	gMicrophoneChannels[index].mResult = MIC_RESULT_NOCARD;
}

extern "C" void UpdateMicrophoneMount(void);

/**
 * @note Address: 0x8000A870
 * @note Size: 0x31C
 */
// the mic: opens it in whichever slot has one, records while its button is held and a little
// after, and hands the loudest sample to every object
extern "C" void UpdateStageMicrophone(u32)
{
	if (gGame.mStatus.mFlags & 1) {
		UpdateMicrophoneMount();
		if (gActiveMicChannel < 0) {
			if (gMicrophoneChannels[0].mResult == MIC_RESULT_READY) {
				gActiveMicChannel = 0;
			} else if (gMicrophoneChannels[1].mResult == MIC_RESULT_READY) {
				gActiveMicChannel = 1;
			}
			if (gActiveMicChannel >= 0 && gMicrophoneChannels[gActiveMicChannel].mDeviceID == 0) {
				M2SSetActiveChannel(gActiveMicChannel);
				M2SSetMode(M2S_MODE_PRERECORD | M2S_MODE_KEEP_MIC_RUNNING);
				M2SSetPrerecordSamples(400);
				M2SOpen();
				MICSetGain(gActiveMicChannel, 0);
				MICStart(gActiveMicChannel);
			}
		} else if (gMicrophoneChannels[gActiveMicChannel].mResult != MIC_RESULT_READY) {
			M2SClose();
			gActiveMicChannel = -1;
		}

		if (gActiveMicChannel >= 0 && gMicrophoneChannels[gActiveMicChannel].mDeviceID == 0) {
			u32 button = 0;
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
