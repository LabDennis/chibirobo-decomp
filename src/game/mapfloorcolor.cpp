#include "game/xobjmap.h"

static inline u32 MakeRGB(s32 r, s32 g, s32 b)
{
	s32 cr = r;
	if (cr >= 256) {
		cr = 255;
	}
	s32 cg = g;
	if (cg >= 256) {
		cg = 255;
	}
	s32 cb = b;
	if (cb >= 256) {
		cb = 255;
	}
	return (cr << 16) | (cg << 8) | cb;
}

// the floor kind of a triangle, from its vertex color
/**
 * @note Address: 0x80093210
 * @note Size: 0x48C
 */
extern "C" s32 GetMapTriangleFloorType(XObjMap* map, s32 trigon, s32 tri)
{
	static u32 sMapFloorTypeColors[13] = {
		MakeRGB(0, 0, 0),       MakeRGB(256, 256, 128), MakeRGB(256, 0, 0), MakeRGB(0, 0, 0), MakeRGB(256, 256, 0),
		MakeRGB(256, 256, 256), MakeRGB(0, 0, 0),       MakeRGB(0, 0, 0),   MakeRGB(0, 0, 0), MakeRGB(0, 256, 256),
		MakeRGB(256, 0, 256),   MakeRGB(256, 0, 128),   MakeRGB(0, 0, 0),
	};
	u32 color = map->mTrigon[trigon].mTri[tri].mColor;
	for (s32 i = 1; i < 13; i++) {
		if (color == sMapFloorTypeColors[i]) {
			return i;
		}
	}
	return 0;
}
