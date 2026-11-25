#ifndef _axdriver_h_
#define _axdriver_h_

#include <types.h>
#include <sysdolphin/synth.h>

#ifdef __cplusplus
extern "C" {
#endif

s32 HSD_AudioSFXStartParam(u32 sfx, u8 vol, u8 pan, s32 group, s32 track);
BOOL HSD_AudioSFXKeyOff(s32 vid);
BOOL HSD_AudioSFXKeyOffAll(void);
BOOL HSD_AudioSFXKeyOffTrack(s32 track);
BOOL HSD_AudioSFXCheck(s32 vid);
s32 HSD_AudioSFXCountByID(s32 sfx);
BOOL HSD_AudioSFXSetPanByID(s32 sfx, u8 pan);
BOOL HSD_AudioSFXSetVolumeByID(s32 sfx, u8 vol, u16 frames);
BOOL HSD_AudioSFXSetPitchByID(s32 sfx, s16 pitch);
BOOL AXDriverKeyOffSfx(s32 sfx);
BOOL AXDriverPauseSfx(s32 sfx);
BOOL AXDriverResumeSfx(s32 sfx);
BOOL AXDriverPauseTrack(s32 track);
BOOL AXDriverResumeTrack(s32 track);

s32 HSD_AudioPStreamStartChParam(const char* path, u8 vol, s32 channel);
s32 AXDriverIsStreamActive(void);
void AXDriverStopAllStreams(void);
BOOL AXDriverPauseAllStreams(void);
BOOL AXDriverResumeAllStreams(void);
BOOL AXDriverSetAllStreamVolumes(u8 vol, u16 frames);
BOOL AXDriverSetAllStreamPitches(s16 pitch);
BOOL AXDriverSetAllStreamParams(HSD_SynthBeatCallback callback, void* context);

BOOL LoadSoundSampleMetadata(void* data);

#ifdef __cplusplus
}
#endif

#endif
