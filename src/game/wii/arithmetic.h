#ifndef GAME_WII_ARITHMETIC_H
#define GAME_WII_ARITHMETIC_H

#include "types.h"

namespace nw4r {
namespace math {
f32 FrSqrt(f32 x);
}
namespace db {
void Warning(const char* file, int line, const char* msg, ...);
}
} // namespace nw4r

inline f32 FSqrt(f32 value)
{
	static char sMessage[] = "FSqrt: Input is out of the domain.";
	static char sFile[]    = __FILE__;
	if (!(value >= 0.0f)) {
		nw4r::db::Warning(sFile, 627, sMessage);
	}
	if (value <= 0.0f) {
		return 0.0f;
	}
	return value * nw4r::math::FrSqrt(value);
}

#endif
