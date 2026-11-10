#ifndef _GAME_PROGRESSIVE_H
#define _GAME_PROGRESSIVE_H

#include "types.h"

struct SavedVideoMode {
	s32 hasSelection;
	s32 useStandardMode;
};
STATIC_ASSERT(sizeof(SavedVideoMode) == 8);

struct VideoModePrompt {
	s32 step;
	s32 pendingMode; // -1 when no change is requested
	f32 framesRemaining;
	f32 alpha;
	s32 restoredSelection;
	s32 useStandardMode;
};
STATIC_ASSERT(sizeof(VideoModePrompt) == 0x18);

extern "C" SavedVideoMode* gVideoModeResetState;
extern "C" VideoModePrompt gVideoModePrompt;

#if defined(VERSION_PAL)
extern "C" f32 gProgressiveFadeFrames;
#endif

#endif
