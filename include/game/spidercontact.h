#ifndef _GAME_SPIDERCONTACT_H
#define _GAME_SPIDERCONTACT_H
#include "game/xchr.h"
#include "game/gmath.h"

static inline f32 SpiderContactDistance(const Vec* a, const Vec* b)
{
	f32 dx = a->x - b->x;
	f32 dy = a->y - b->y;
	f32 dz = a->z - b->z;
	return sqrtf(dx * dx + dy * dy + dz * dz);
}

static inline void DrainSpiderBattery(Params& params, f32 amount)
{
	f32 battery = params.mBattery - amount;
	if (battery < 0.0f) {
		battery = 0.0f;
	}
	params.SetBattery(battery);
}

#endif
