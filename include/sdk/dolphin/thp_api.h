#ifndef DOLPHIN_THP_API_H
#define DOLPHIN_THP_API_H

#ifdef __cplusplus
extern "C" {
#endif

BOOL THPInit(void);
s32 THPVideoDecode(void* file, void* tileY, void* tileU, void* tileV, void* workArea);
u32 THPAudioDecode(s16* audioBuffer, u8* audioFrame, s32 flag);

#ifdef __cplusplus
}
#endif

#endif
