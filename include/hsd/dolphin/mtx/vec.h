#ifndef _VEC_H_
#define _VEC_H_

#include <dolphin/mtx/mtxtypes.h>

void PSVECSubtract(Vec* a, Vec* b, Vec* result);
void PSVECAdd(Vec* a, Vec* b, Vec* result);
void PSVECNormalize(Vec* src, Vec* unit);
f32 PSVECSquareMag(Vec* v);
f32 PSVECMag(const Vec* v);
f32 PSVECDotProduct(Vec* a, Vec* b);
void C_VECCrossProduct(const Vec* a, const Vec* b, Vec* axb);
void PSVECCrossProduct(Vec* a, Vec* b, Vec* result);
void C_VECReflect(const Vec* src, const Vec* normal, Vec* dst);
void PSVECReflect(Vec* src, Vec* normal, Vec* dst);
f32 PSVECDistance(Vec* a, Vec* b);

#ifdef __cplusplus
extern "C"
#endif
void PSVECScale(const Vec* src, Vec* dst, f32 scale);

#endif
