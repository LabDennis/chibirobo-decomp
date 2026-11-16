#pragma once

#include "types.h"
#if defined(VERSION_R24J01)
#include <Dolphin/vec.h>
#else
#include <dolphin/mtx/mtxtypes.h>
#endif

struct WipeQuad {
	Vec vertices[4];
};
