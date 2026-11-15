#ifndef _REVOLUTION_VEC_H
#define _REVOLUTION_VEC_H

// Wii variant of the VEC API: the Revolution SDK adds VECHalfAngle and
// VECSquareDistance.
#include "Dolphin/vec.h"

#ifdef __cplusplus
extern "C" {
#endif

void C_VECHalfAngle(const Vec*, const Vec*, Vec*);
f32 PSVECSquareDistance(const Vec*, const Vec*);

#ifdef __cplusplus
};
#endif

#endif
