#ifndef _axdriver_h_
#define _axdriver_h_

#include <dolphin/types.h>

typedef void (*HSD_AudioBeatCallback)(s32 id, s32 beat, void* context);

BOOL HSD_AudioSFXKeyOff(s32 vid);
BOOL HSD_AudioSFXCheck(s32 vid);

BOOL AXDriverSetAllStreamParams(HSD_AudioBeatCallback callback, void* context);
void HSD_SynthPStreamSetBeatCallback(s32 id, HSD_AudioBeatCallback callback, void* context);
void HSD_SynthSFXSetDriverMasterClockCallback(void (*callback)(u32));

#endif
