#ifndef _VEC_H_
#define _VEC_H_

#include <dolphin/mtx/mtxtypes.h>

void PSVECSubtract(Vec* a, Vec* b, Vec* result);
void PSVECAdd(Vec* a, Vec* b, Vec* result);
void PSVECNormalize(Vec* src, Vec* unit);
f32 PSVECSquareMag(Vec* v);
f32 PSVECMag(const Vec* v);
f32 PSVECDotProduct(Vec* a, Vec* b);
void PSVECCrossProduct(Vec* a, Vec* b, Vec* result);
void PSVECReflect(Vec* src, Vec* normal, Vec* dst);
f32 PSVECSquareDistance(Vec* a, Vec* b);
f32 PSVECDistance(const Vec* a, const Vec* b);

#ifdef __cplusplus
// the game's own spelling, the library's c files declare it locally
extern "C" void PSVECScale(const Vec* src, Vec* dst, f32 scale);
#endif

#endif
