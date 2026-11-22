#ifndef _GAME_CJOBJSET_H
#define _GAME_CJOBJSET_H

#include "game/cjobj.h"

// the three CJObj setters that compare against a Vec built on the stack. retail copies it
// from a zeroed Vec in .bss (the Vec() temporary) and then fills in x, y and z. only the
// files that emit or inline these include this. the helper takes the Vec by value: that
// form gives retail's stack at every inlined site, and it keeps the three-float setters
// over 256 inline units, where CMsgFont_Draw calls them (US 0x80082970, limit 256: the
// CJObj constructor and SetRotationY are calls there too, SetScale(const Vec&) is expanded)

#pragma push
#pragma pool_data on

// a Vec from its parts, starting from zero
static inline Vec MakeJObjVec(Vec v, f32 x, f32 y, f32 z)
{
	v.x = x;
	v.y = y;
	v.z = z;
	return v;
}

// the setters stand in the header's original order: Translate, Scale, Rotation. The compiler emits
// the weak copies in the order it meets the definitions, and only this order gives the JP living
// REL its retail layout byte for byte; no function in any edition changes score with it
inline void CJObj::SetTranslate(f32 x, f32 y, f32 z, s32 force)
{
	if (force == 0) {
		if (!(mTranslate != MakeJObjVec(Vec(), x, y, z))) {
			return;
		}
	}
	mTranslate.x = x;
	mTranslate.y = y;
	mTranslate.z = z;
	HSD_JObjSetTranslate(mJObj, &mTranslate);
}

inline void CJObj::SetScale(f32 x, f32 y, f32 z, s32 force)
{
	if (force == 0) {
		if (!(mScale != MakeJObjVec(Vec(), x, y, z))) {
			return;
		}
	}
	mScale.x = x;
	mScale.y = y;
	mScale.z = z;
	HSD_JObjSetScale(mJObj, &mScale);
}

inline void CJObj::SetRotation(f32 x, f32 y, f32 z, s32 force)
{
	if (force == 0) {
		// retail tests the cached translation here
		if (!(mTranslate != MakeJObjVec(Vec(), x, y, z))) {
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

#pragma pop

#endif
