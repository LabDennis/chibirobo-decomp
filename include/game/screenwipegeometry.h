#pragma once

#include "types.h"
#include <dolphin/mtx/mtxtypes.h>

struct WipeQuad {
	Vec vertices[4];
};

struct WipeQuadTemplate {
	WipeQuad corners;
	u8 _30[0x30];
};

struct WipeGeometry {
	Vec _00[6];
	WipeQuad colorCorners;
	u8 _78[0x30];
	WipeQuadTemplate depth, overlay, circleMask;
	WipeQuadTemplate tileMask;
	Vec tileEdgeVertices[18];
	u32 tileEdgeColors[18];
	WipeQuadTemplate textureMask;
};
extern "C" const WipeGeometry gScreenWipeGeometry;

STATIC_ASSERT(sizeof(WipeGeometry) == 0x3A8);
