#ifndef _GAME_EVTTIME_H
#define _GAME_EVTTIME_H

#include "types.h"

// the time of day evt's TIME command drives, mode 1 is night
struct EvtTimeState {
	s32 mMode;     // _00
	f32 mTime;     // _04
	f32 mDuration; // _08
	s32 mRunning;  // _0C
};
STATIC_ASSERT(sizeof(EvtTimeState) == 0x10);

#endif
