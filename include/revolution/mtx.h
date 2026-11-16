#ifndef _REVOLUTION_MTX_H
#define _REVOLUTION_MTX_H

#include "Dolphin/mtx.h"

#define MTXRowCol(m, r, c) ((m)[(r)][(c)])

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

#ifdef __cplusplus
}
#endif

#endif
