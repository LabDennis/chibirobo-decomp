#ifndef _GAME_AIMPITCH_H
#define _GAME_AIMPITCH_H

#include "game/gmath.h"
#include <dolphin/mtx/mtxtypes.h>

static inline f32 TargetPitch(const Vec& from, const Vec& to)
{
	f32 dz = to.z - from.z, dx = to.x - from.x, dy = to.y - from.y;
	f32 pitch = atan2(sqrtf(dx * dx + dz * dz), sqrtf(dy * dy));
	return dy < 0 ? HALF_PI - pitch : pitch - HALF_PI;
}

#endif
