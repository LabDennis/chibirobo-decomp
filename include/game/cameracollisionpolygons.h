#ifndef _GAME_CAMERACOLLISIONPOLYGONS_H
#define _GAME_CAMERACOLLISIONPOLYGONS_H

#include "types.h"
#if defined(VERSION_R24J01)
#include "Dolphin/mtx.h"
#else
#include <dolphin/mtx/mtxtypes.h>
#endif

// the fifth point repeats the first so every edge is a pair
struct CamCollisionPolygon {
	Vec point[5]; // _00
};
STATIC_ASSERT(sizeof(CamCollisionPolygon) == 0x3C);

#endif
