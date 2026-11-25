#ifndef _DOLPHIN_VEC_H
#define _DOLPHIN_VEC_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif // ifdef __cplusplus

typedef struct Vec {
	f32 x;
	f32 y;
	f32 z;
} Vec;

typedef struct SVec {
	s16 x;
	s16 y;
	s16 z;
} SVec;

void PSVECAdd(const Vec*, const Vec*, Vec*);
void PSVECSubtract(const Vec*, const Vec*, Vec*);
void PSVECScale(const Vec*, Vec*, f32);
void PSVECNormalize(const Vec*, Vec*);
f32 PSVECSquareMag(const Vec*);
f32 PSVECMag(const Vec*);
f32 PSVECDotProduct(const Vec*, const Vec*);
void C_VECCrossProduct(const Vec*, const Vec*, Vec*);
void PSVECCrossProduct(const Vec*, const Vec*, Vec*);
void C_VECReflect(const Vec*, const Vec*, Vec*);
f32 PSVECDistance(const Vec*, const Vec*);

#ifdef __cplusplus
}
#endif

#endif
