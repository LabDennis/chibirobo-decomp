#ifndef GAME_WII_ARITHMETIC_H
#define GAME_WII_ARITHMETIC_H

#include "types.h"

extern f32 WiiReciprocalSquareRoot(f32 value);
extern void WiiReportMathError(const char* file, int line, const char* format, ...);

inline f32 FSqrt(f32 value)
{
	static char sMessage[] = "FSqrt: Input is out of the domain.";
	static char sFile[]    = __FILE__;
	if (!(value >= 0.0f)) {
		WiiReportMathError(sFile, 627, sMessage);
	}
	if (value <= 0.0f) {
		return 0.0f;
	}
	return value * WiiReciprocalSquareRoot(value);
}

#endif
