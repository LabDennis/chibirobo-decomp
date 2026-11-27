#ifndef _GAME_CJOBJSET_H
#define _GAME_CJOBJSET_H

#include "game/cjobj.h"

static inline Vec MakeVec(f32 x, f32 y, f32 z)
{
	Vec v = Vec();
	v.x   = x;
	v.y   = y;
	v.z   = z;
	return v;
}

inline void CJObj::SetRotation(f32 x, f32 y, f32 z, s32 force)
{
	if (force == 0) {
		// retail tests the cached translation here
		if (!(mTranslate != MakeVec(x, y, z))) {
			return;
		}
	}
	mRotate.Set(x, y, z);
	UpdateRotation();
}

inline void CJObj::SetScale(f32 x, f32 y, f32 z, s32 force)
{
	if (force == 0) {
		if (!(mScale != MakeVec(x, y, z))) {
			return;
		}
	}
	mScale.x = x;
	mScale.y = y;
	mScale.z = z;
	HSD_JObjSetScale(mJObj, &mScale);
}

inline void CJObj::SetTranslate(f32 x, f32 y, f32 z, s32 force)
{
	if (force == 0) {
		if (!(mTranslate != MakeVec(x, y, z))) {
			return;
		}
	}
	mTranslate.x = x;
	mTranslate.y = y;
	mTranslate.z = z;
	HSD_JObjSetTranslate(mJObj, &mTranslate);
}

#endif
