#ifndef GAME_HUDCOUNTERHELPERS_H
#define GAME_HUDCOUNTERHELPERS_H

#include "game/cgame.h"

static inline f32 GetHudBottomEdge()
{
	return (gGame.mWipeTile1.mHeight / 2 + gGame.mWipeTile1.mY >= 418.0f) ? 418.0f : gGame.mWipeTile1.mHeight / 2 + gGame.mWipeTile1.mY;
}

// moves cur toward target by step, not while the game is paused
static inline void Approach(f32* cur, f32* target, f32 step)
{
	if (gGame.mStatus.mFlags & 0x200000000000ULL) {
		return;
	}
	if (*cur > *target && (*cur -= step) < *target) {
		*cur = *target;
	}
	if (*cur < *target && (*cur += step) > *target) {
		*cur = *target;
	}
}

#endif
