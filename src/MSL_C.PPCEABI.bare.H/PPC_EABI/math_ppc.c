#include "PowerPC_EABI_Support/MSL_C/PPC_EABI/math_ppc.h"
#include "fdlibm.h"
#include "PowerPC_EABI_Support/MSL_C/MSL_Common/math_api.h"

extern int __float_nan[];

/**
 * @note Address: 0x8015CF00
 * @note Size: 0x24
 */
WEAK f32 cosf(f32 __x)
{
	return cos((f64)__x);
}

/**
 * @note Address: 0x8015CEDC
 * @note Size: 0x24
 */
WEAK f32 sinf(f32 __x)
{
	return sin((f64)__x);
}

/**
 * @note Address: 0x8015CEB8
 * @note Size: 0x24
 */
WEAK f32 tanf(f32 __x)
{
	return tan((f64)__x);
}

/**
 * @note Address: 0x8015CE94
 * @note Size: 0x24
 */
f32 powf(f32 x, f32 y)
{
	return pow(x, y);
}

/**
 * @note Address: 0x8015CDB0
 * @note Size: 0xE4
 */
f32 sqrtf(f32 x)
{
	static const f64 _half  = .5;
	static const f64 _three = 3.0;

	if (x > 0.0f) {
		f64 guess = __frsqrte((f64)x);
		guess     = _half * guess * (_three - guess * guess * x);
		guess     = _half * guess * (_three - guess * guess * x);
		guess     = _half * guess * (_three - guess * guess * x);
		return (f32)(x * guess);
	} else if (x < 0.0) {
		return *(f32*)__float_nan;
	} else if (fpclassify(x) == FP_NAN) {
		return *(f32*)__float_nan;
	}
	return x;
}

/**
 * @note Address: 0x8015CDA8
 * @note Size: 0x8
 */
f64(fabs)(f64 x)
{
	return __fabs(x);
}
