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

inline void CJObj::SetRotationX(f32 x, s32 force)
{
	if (force == 0 && x == mRotate.x) {
		return;
	}
	mRotate.x = x;
	if (mJObj->flags & USE_QUATERNION) {
		Quaternion q;
		EulerToQuat(&mRotate, &q);
		HSD_JObjSetRotation(mJObj, &q);
	} else {
		HSD_JObjSetRotationX(mJObj, x);
	}
}

inline void CJObj::SetRotationY(f32 y, s32 force)
{
	if (force == 0 && y == mRotate.y) {
		return;
	}
	mRotate.y = y;
	if (mJObj->flags & USE_QUATERNION) {
		Quaternion q;
		EulerToQuat(&mRotate, &q);
		HSD_JObjSetRotation(mJObj, &q);
	} else {
		HSD_JObjSetRotationY(mJObj, y);
	}
}

inline void CJObj::SetRotationZ(f32 z, s32 force)
{
	if (force == 0 && z == mRotate.z) {
		return;
	}
	mRotate.z = z;
	if (mJObj->flags & USE_QUATERNION) {
		Quaternion q;
		EulerToQuat(&mRotate, &q);
		HSD_JObjSetRotation(mJObj, &q);
	} else {
		HSD_JObjSetRotationZ(mJObj, z);
	}
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
