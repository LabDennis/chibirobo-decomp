#ifndef _GAME_CJOBJSET_H
#define _GAME_CJOBJSET_H

#include "game/cjobj.h"

// the three CJObj setters that compare against a Vec built on the stack. retail copies it
// from a zeroed Vec in .bss and then fills in x, y and z. only the files that emit or
// inline these include this

#pragma push
#pragma pool_data on

// a Vec from its parts, starting from zero
static inline Vec MakeJObjVec(const Vec& zero, f32 x, f32 y, f32 z)
{
	Vec v = zero;
	v.x   = x;
	v.y   = y;
	v.z   = z;
	return v;
}

inline void CJObj::SetRotation(f32 x, f32 y, f32 z, s32 force)
{
	if (force == 0) {
		static Vec zero;
		// retail tests the cached translation here
		if (!(mTranslate != MakeJObjVec(zero, x, y, z))) {
			return;
		}
	}
	mRotate.x = x;
	mRotate.y = y;
	mRotate.z = z;

	if (mJObj->flags & USE_QUATERNION) {
		Quaternion q;
		EulerToQuat(&mRotate, &q);
		HSD_JObjSetRotation(mJObj, &q);
	} else {
		// a CVec passed off as a quaternion, w ends up being its vtable pointer
		HSD_JObjSetRotation(mJObj, (Quaternion*)&mRotate);
	}
}

inline void CJObj::SetScale(f32 x, f32 y, f32 z, s32 force)
{
	if (force == 0) {
		static Vec zero;
		if (!(mScale != MakeJObjVec(zero, x, y, z))) {
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
		static Vec zero;
		if (!(mTranslate != MakeJObjVec(zero, x, y, z))) {
			return;
		}
	}
	mTranslate.x = x;
	mTranslate.y = y;
	mTranslate.z = z;
	HSD_JObjSetTranslate(mJObj, &mTranslate);
}

#pragma pop

#endif
