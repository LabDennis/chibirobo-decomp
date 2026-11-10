#ifndef _GAME_CAMERACOLLISIONPOLYGONS_H
#define _GAME_CAMERACOLLISIONPOLYGONS_H

#include "types.h"
#include <dolphin/mtx/mtxtypes.h>

// the fifth point repeats the first so every edge is a pair
struct CamCollisionPolygon {
	Vec point[5]; // _00
};
STATIC_ASSERT(sizeof(CamCollisionPolygon) == 0x3C);
extern "C" CamCollisionPolygon gCameraCollisionPolygons[256];

#endif
