#pragma once

#include <dolphin/mtx/mtxtypes.h>

struct DustDrawDefaults {
	Vec _00[8];
	Vec texturedRotation, firstTriangleRotation, secondTriangleRotation;
};
extern "C" {
extern const DustDrawDefaults gDust2DrawDefaults;
extern const Vec gDust3TexturedRotation, gDust3CircleRotation;
}
