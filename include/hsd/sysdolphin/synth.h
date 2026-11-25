#ifndef _synth_h_
#define _synth_h_

#include <types.h>

#ifdef __cplusplus
extern "C" {
#endif

BOOL HSD_AudioInit(u32 heapSize, u32 voices, u32 groups);
void HSD_AudioSetMallocFreeProc(void* (*mallocProc)(s32), void (*freeProc)(void*));
void HSD_SynthInit(s32 heapSize, s32 pstNum, s32 pstBufNum);
void* HSD_AudioMalloc(u32 size);
s32 HSD_SynthVolToDBx(f32 vol);
f32 HSD_SynthDBxToVol(s32 dbx);
extern s32 sSynthDecibelFloor;

s32 HSD_SynthSFXNewBank(s32 size);
s32 HSD_SynthSFXBankClear(s32 bankID);
s32 HSD_SynthSFXLoad(const char* filename, s32 bankID, void (*callback)(s32, s32), s32 arg);

s32 StartSynthSoundEffect(s32 sfxID, s32 vol, s32 volMode, s32 vol2, s32 vol2Mode, u8 macroPan, u8 userPan, u8 macroSPan, f32 mixMain,
                          f32 mixAuxA, f32 mixAuxB, u8 userSPan, s32 priority, s32 channel, s32 rear, s32 useItd);
void HSD_SynthSFXKeyOff(s32 id);
void HSD_SynthSFXPause(s32 id);
void HSD_SynthSFXResume(s32 id);
s32 HSD_SynthSFXCheck(s32 id);
void HSD_SynthSetVolFader(s32 id, s32 vol, s32 fadeFrames, s32 flag, s32 dbMode);
void HSD_SynthSFXSetPan(s32 id, u8 pan, s32 flag);
void HSD_SynthSFXSetSPan(s32 id, u8 span, s32 flag);
void HSD_SynthSFXSetMix(s32 id, f32 mixMain, f32 mixAuxA, f32 mixAuxB);
void HSD_SynthSFXSetPitch(s32 id, s32 flag, f32 ratio);
void HSD_SynthSFXSetPriority(s32 id, s32 prio);
void HSD_SynthSFXSetDriverInactivatedCallback(void (*callback)(s32));
void HSD_SynthSFXSetDriverMasterClockCallback(void (*callback)(u32));
void HSD_SynthSFXSetDriverPauseCallback(void (*callback)(s32));

typedef void (*HSD_SynthBeatCallback)(s32 id, s32 beat, void* context);

s32 HSD_SynthPStreamStart(s32 entrynum, s32 vol, s32 volMode, s32 vol2, s32 vol2Mode, u8 macroPan, u8 userPan, u8 macroSPan, f32 pitch1,
                        f32 pitch2, f32 mixMain, f32 mixAuxA, f32 mixAuxB, s32 userSPan, s32 channel, s32 useRear);
s32 HSD_SynthPStreamIsActive(void);
void HSD_SynthPStreamSetBeatCallback(s32 id, HSD_SynthBeatCallback callback, void* context);

#ifdef __cplusplus
}
#endif

#endif
