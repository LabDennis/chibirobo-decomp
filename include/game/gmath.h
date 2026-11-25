#ifndef _GAME_GMATH_H
#define _GAME_GMATH_H

#include <math.h>

#define TWO_PI   6.2831855f
#define HALF_PI  1.5707964f
#define THIRD_PI 1.0471976f
#define DEG2RAD  0.017453292f
#define RAD2DEG  57.295776f // one ulp under 180 / pi, what the rodata has

#define ABS(x) ((x) < 0.0f ? -(x) : (x))

static inline f32 Abs(f32 x)
{
	return x < 0.0f ? -x : x;
}

// wraps radians into [0, 2pi), the camera actions and cobj all inline it
static inline f32 WrapAngle(f32 a)
{
	while (a < 0.0f) {
		a += TWO_PI;
	}
	while (a >= TWO_PI) {
		a -= TWO_PI;
	}
	return a;
}

static inline f32 sqrtf(f32 x)
{
	return msl_sqrtf(x);
}

#endif
