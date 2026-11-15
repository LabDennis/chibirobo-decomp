#ifndef _REVOLUTION_MTX_H
#define _REVOLUTION_MTX_H

#include "Dolphin/mtx.h"

#ifdef __cplusplus
extern "C" {
#endif

// the 4x4 routines the Wii SDK adds to the GameCube ones
void PSMTX44Identity(Mtx44 m);
void PSMTX44Concat(Mtx44 a, Mtx44 b, Mtx44 out);
u32 C_MTX44Inverse(Mtx44 src, Mtx44 inverse);

// further Revolution SDK entry points NW4R's math wrappers call
void PSMTXConcatArray(const Mtx a, const Mtx srcBase, Mtx dstBase, u32 count);
u32 PSMTXInvXpose(const Mtx src, Mtx invX);
void C_QUATMtx(Quaternion* quat, const Mtx mtx);
void C_QUATSlerp(const Quaternion* a, const Quaternion* b, Quaternion* out, f32 t);

// the projection builders nw4r::g3d's camera calls
void C_MTXFrustum(Mtx44 m, f32 t, f32 b, f32 l, f32 r, f32 n, f32 f);
void C_MTXLightFrustum(Mtx m, f32 t, f32 b, f32 l, f32 r, f32 n, f32 scaleS, f32 scaleT, f32 transS, f32 transT);

#ifdef __cplusplus
}
#endif

#endif
