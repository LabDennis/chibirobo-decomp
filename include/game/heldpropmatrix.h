#ifndef _GAME_HELDPROPMATRIX_H
#define _GAME_HELDPROPMATRIX_H

#include "game/xobj.h"

extern "C" void PSMTXTrans(Mtx matrix, f32 x, f32 y, f32 z);

static inline void SetHeldPropMatrix(XObj* prop, MtxPtr const& matrix)
{
	HSD_JObjCopyMtx(prop->mJObj, matrix);
}

static inline void RotateHeldProp(Mtx source, Mtx result, Mtx rotation, char axis, f32 angle)
{
	PSMTXRotRad(rotation, axis, angle);
	PSMTXConcat(source, rotation, result);
}

static inline void OffsetHeldProp(Mtx source, Mtx translation, f32 x, f32 y, f32 z)
{
	PSMTXTrans(translation, x, y, z);
	PSMTXConcat(source, translation, source);
}

#endif
