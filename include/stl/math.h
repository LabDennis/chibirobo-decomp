#ifndef _MATH_H
#define _MATH_H

#include "PowerPC_EABI_Support/MSL_C/MSL_Common/math_api.h"
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif // ifdef __cplusplus

#define SIGNBIT(x) ((int)(__HI(x) & 0x80000000))

#define INFINITY (*(f32*)__float_huge)
#define NAN      (*(f32*)__float_nan)
#define HUGE_VAL (*(f64*)__double_huge)

#define PI      3.1415927f
#define M_SQRT3 1.73205f

extern int __float_nan[];
extern int __float_huge[];
extern int __double_huge[];

f64 cos(f64);
f32 cosf(f32);
f64 sin(f64);
f32 sinf(f32);
f64 tan(f64);
f32 tanf(f32);

f64 acos(f64);
f64 asin(f64);
f64 atan(f64);
f64 atan2(f64, f64);

f64 ceil(f64);
f64 floor(f64);
f64 frexp(f64, int*);
f64 ldexp(f64, int);
f64 sqrt(f64);

f64 pow(f64, f64);
f64 log(f64);
f64 log10(f64);
f64 fmod(f64, f64);
f64 scalbn(f64, int);

f64 __ieee754_acos(f64);
f64 __ieee754_fmod(f64, f64);
f64 __ieee754_log(f64);
f64 __ieee754_log10(f64);
f64 __ieee754_pow(f64, f64);
f64 __ieee754_sqrt(f64);
f64 __ieee754_atan2(f64, f64);
f64 __ieee754_asin(f64);
int __ieee754_rem_pio2(f64, f64*);

f64 __kernel_sin(f64, f64, int);
f64 __kernel_cos(f64, f64);
f64 __kernel_tan(f64, f64, int);

f64 __fabs(f64);
f32 __fabsf(f32);
f64 __fnabs(f64);
f32 __fnabsf(f32);
f64 __fmadd(f64, f64, f64);
f64 __fmsub(f64, f64, f64);
f64 __fnmadd(f64, f64, f64);
f64 __fnmsub(f64, f64, f64);
f32 __fmadds(f32, f32, f32);
f32 __fmsubs(f32, f32, f32);
f32 __fnmadds(f32, f32, f32);
f32 __fnmsubs(f32, f32, f32);
f64 __fsel(f64, f64, f64);
f32 __fsels(f32, f32, f32);
f64 __frsqrte(f64);
f32 __fres(f32);
f64 __fsqrt(f64);
f32 __fsqrts(f32);
s64 __fctid(f64);
s64 __fctiw(f64);
f64 __fcfid(s64);
f64 __mffs(void);
void __mtfsf(int, f64);
void __mtfsfi(int, int);
void __mtfsb0(int);
void __mtfsb1(int);
f64 __setflm(f64);

#define FABS(x) (f32) __fabs(x)
#define fabs(x) __fabs(x)

inline f128 fabsl(f128 x)
{
	return __fabs((f64)x);
}

#ifdef __cplusplus
};
#endif // ifdef __cplusplus

static inline f32 dolsqrtf(f32 x)
{
	static const f64 _half  = .5;
	static const f64 _three = 3.0;
	vf32 y;
	if (x > 0.0f) {

		f64 guess = __frsqrte((f64)x);                            // returns an approximation to
		guess     = _half * guess * (_three - guess * guess * x); // now have 12 sig bits
		guess     = _half * guess * (_three - guess * guess * x); // now have 24 sig bits
		guess     = _half * guess * (_three - guess * guess * x); // now have 32 sig bits
		y         = (f32)(x * guess);
		return y;
	}
	return x;
}

#ifdef __cplusplus
// MSL's C++ float forms: inlines over the double routines, apart from the C sinf/cosf above
namespace std {
inline f32 sinf(f32 x)
{
	return sin(x);
}
inline f32 cosf(f32 x)
{
	return cos(x);
}
} // namespace std
#endif

#endif
