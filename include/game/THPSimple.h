#ifndef _GAME_THPSIMPLE_H
#define _GAME_THPSIMPLE_H

#include "types.h"

// the sdk's THPSimple movie player, rebuilt with the game as c++. skip made opening and
// preloading async, added a mute switch and split drawing into begin, quad and end.
// names follow the sdk wherever it does the same job

#ifdef __cplusplus
extern "C" {
#endif // ifdef __cplusplus

typedef struct THPVideoInfo {
	u32 xSize;     // _00
	u32 ySize;     // _04
	u32 videoType; // _08
} THPVideoInfo;

typedef struct THPAudioInfo {
	u32 sndChannels;   // _00
	u32 sndFrequency;  // _04
	u32 sndNumSamples; // _08
	u32 sndNumTracks;  // _0C
} THPAudioInfo;

void THPSimpleSetSound(BOOL on);
BOOL THPSimpleInit(s32 audioSystem);
s32 THPSimpleOpen(const char* path);
s32 THPSimpleOpenWait(void);
BOOL THPSimpleClose(void);
u32 THPSimpleCalcNeedMemory(void);
BOOL THPSimpleSetBuffer(void* buffer);
s32 THPSimplePreLoad(s32 loop);
s32 THPSimplePreLoadWait(void);
void THPSimpleAudioStart(void);
void THPSimpleAudioStop(void);
BOOL THPSimpleLoadStop(void);
s32 THPSimpleDecode(s32 audioTrack);
s32 THPSimpleDrawBegin(BOOL alpha);
void THPSimpleDrawEnd(void);
#ifdef VERSION_R24J01
s32 THPSimpleDrawCurrentFrame(s32 x, s32 y, s32 z, s32 polygonW, s32 polygonH, BOOL alpha);
void THPSimpleDrawQuad(s32 x, s32 y, s32 z, s32 polygonW, s32 polygonH);
#else
s32 THPSimpleDrawCurrentFrame(s32 x, s32 y, s32 polygonW, s32 polygonH, BOOL alpha);
void THPSimpleDrawQuad(s32 x, s32 y, s32 polygonW, s32 polygonH);
#endif
BOOL THPSimpleGetVideoInfo(THPVideoInfo* videoInfo);
BOOL THPSimpleGetAudioInfo(THPAudioInfo* audioInfo);
u32 THPSimpleGetTotalFrame(void);
#ifdef VERSION_R24J01
BOOL THPSimpleSetVolume(s32 vol, s32 time);
#endif

#ifdef __cplusplus
}
#endif // ifdef __cplusplus

#endif
