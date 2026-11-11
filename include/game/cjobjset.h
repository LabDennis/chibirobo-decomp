#ifndef _GAME_CJOBJSET_H
#define _GAME_CJOBJSET_H

#include "game/cjobj.h"

// the three CJObj setters that compare against a Vec built on the stack. only the
// files that emit or inline these include this

inline void CJObj::SetRotation(f32 x, f32 y, f32 z, s32 force)
{
	if (force == 0) {
		// retail tests the cached translation here
		if (!(mTranslate != CVec(x, y, z))) {
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
		if (!(mScale != CVec(x, y, z))) {
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
		if (!(mTranslate != CVec(x, y, z))) {
			return;
		}
	}
	mTranslate.x = x;
	mTranslate.y = y;
	mTranslate.z = z;
	HSD_JObjSetTranslate(mJObj, &mTranslate);
}

#endif
