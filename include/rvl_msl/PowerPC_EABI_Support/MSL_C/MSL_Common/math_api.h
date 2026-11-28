#ifndef _MSL_MATH_API_H
#define _MSL_MATH_API_H

#include "types.h"
#include "stl/fdlibm.h"

#ifdef __cplusplus
extern "C" {
#endif // ifdef __cplusplus

// int __fpclassifyf(f32);
// int __signbitd(f64);

int __fpclassifyf(f32 x);
int __fpclassifyd(f64 x);

#define fpclassify(x) ((sizeof(x) == sizeof(f32)) ? __fpclassifyf((f32)(x)) : __fpclassifyd((f64)(x)))

#define FP_NAN       1
#define FP_INFINITE  2
#define FP_ZERO      3
#define FP_NORMAL    4
#define FP_SUBNORMAL 5

#define isinf(x)    ((fpclassify(x) == 2))
#define isfinite(x) ((fpclassify(x) > 2))

#ifdef __cplusplus
};
#endif // ifdef __cplusplus

#endif
