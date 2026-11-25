#include <types.h>
#include "Dolphin/OS/OSReset.h"
#include "Dolphin/mic.h"
#include "game/csound.h"
#include "game/mic.h"
#include "game/cgame.h"

extern "C" {
void AXQuit();
void AIReset();
void AIStopDMA();
void HSD_VISetUserGXDrawDoneCallback(void (*)());
void GXDrawDone();
void GXAbortFrame();
void VISetBlack(BOOL);
void VIFlush();
void VIWaitForRetrace();
}

/**
 * @note Address: 0x8000FA40
 * @note Size: 0x8C
 */
// CGame's _48, CBase's with the mic reset first
void CGame::Reset(u32 code)
{
	s32 channel = gActiveMicChannel;
	if (channel >= 0 && !gMicrophoneChannels[channel].mDeviceID)
		MICReset(channel);
#if defined(VERSION_PAL)
	AXDriverStopAllStreams();
	HSD_AudioSFXKeyOffAll();
#else
	AXQuit();
	AIReset();
#endif
	HSD_VISetUserGXDrawDoneCallback(NULL);
	GXDrawDone();
	GXAbortFrame();
	VISetBlack(TRUE);
	VIFlush();
	VIWaitForRetrace();
#if defined(VERSION_PAL)
	AXQuit();
	AIStopDMA();
#endif
	OSResetSystem(code, 0, FALSE);
}
