#include "game/cgame.h"
#include "game/cjobjset.h"

void operator delete(void* object);

extern "C" {
extern void* __vt__22CInterfaceCameraAction[];
extern void* __vt__23CInterfaceInterpolation[];
extern void* __vt__14CCameraMemento[];
extern void* __vt__8CMsgData[];
extern void* __vt__7CObjVec[];
extern void* __vt__5CJObj[];
}

// CJObj's weak copies, everything in it is inline

/**
 * @note Address: 0x80016ECC
 * @note Size: 0x88
 */
// todo: weak copy
extern "C" CInterfaceCameraAction* __dt__22CInterfaceCameraActionFv(CInterfaceCameraAction* action, s32 flags)
{
	if (action != NULL) {
		*(void***)((u8*)action + 0x3C) = __vt__22CInterfaceCameraAction; // _3C vtable
		action->mAngle.CVec::~CVec();
		action->mInterest.CVec::~CVec();
		action->mEye.CVec::~CVec();
		if ((s16)flags > 0) {
			::operator delete(action);
		}
	}
	return action;
}

/**
 * @note Address: 0x80016F54
 * @note Size: 0x48
 */
// todo: weak copy
extern "C" CInterfaceInterpolation* __dt__23CInterfaceInterpolationFv(CInterfaceInterpolation* interp, s32 flags)
{
	if (interp != NULL) {
		*(void***)interp = __vt__23CInterfaceInterpolation;
		if ((s16)flags > 0) {
			::operator delete(interp);
		}
	}
	return interp;
}

/**
 * @note Address: 0x80016F9C
 * @note Size: 0x88
 */
// todo: weak copy
extern "C" CCameraMemento* __dt__14CCameraMementoFv(CCameraMemento* memento, s32 flags)
{
	if (memento != NULL) {
		*(void***)memento = __vt__14CCameraMemento;
		memento->mAngle.CVec::~CVec();
		memento->mInterest.CVec::~CVec();
		memento->mEye.CVec::~CVec();
		if ((s16)flags > 0) {
			::operator delete(memento);
		}
	}
	return memento;
}

/**
 * @note Address: 0x80017024
 * @note Size: 0x48
 */
// todo: weak copy
extern "C" CMsgData* __dt__8CMsgDataFv(CMsgData* data, s32 flags)
{
	if (data != NULL) {
#if defined(VERSION_JP)
		*(void***)((u8*)data + 0x894) = __vt__8CMsgData; // _894 vtable
#else
		*(void***)((u8*)data + 0x898) = __vt__8CMsgData; // _898 vtable
#endif
		if ((s16)flags > 0) {
			::operator delete(data);
		}
	}
	return data;
}

/**
 * @note Address: 0x8001706C
 * @note Size: 0xC8
 */
// todo: weak copy
extern "C" CMsgFont* __dt__8CMsgFontFv(CMsgFont* font, s32 flags)
{
	if (font != NULL) {
		font->CMsgFont::~CMsgFont();
		if ((s16)flags > 0)
			::operator delete(font);
	}
	return font;
}

/**
 * @note Address: 0x80017134
 * @note Size: 0x88
 */
// todo: weak copy
extern "C" CObjVec* __dt__7CObjVecFv(CObjVec* obj, s32 flags)
{
	if (obj != NULL) {
		*(void***)((u8*)obj + 0xC) = __vt__7CObjVec; // _0C vtable
		obj->mSpeed.CVec::~CVec();
		obj->mTarget.CVec::~CVec();
		obj->CVec::~CVec();
		if ((s16)flags > 0) {
			::operator delete(obj);
		}
	}
	return obj;
}

/**
 * @note Address: 0x800171BC
 * @note Size: 0x88
 */
// todo: weak copy
extern "C" CJObj* __dt__5CJObjFv(CJObj* obj, s32 flags)
{
	if (obj != NULL) {
		*(void***)((u8*)obj + 0x34) = __vt__5CJObj; // _34 vtable
		obj->mScale.CVec::~CVec();
		obj->mRotate.CVec::~CVec();
		obj->mTranslate.CVec::~CVec();
		if ((s16)flags > 0) {
			::operator delete(obj);
		}
	}
	return obj;
}

/**
 * @note Address: 0x80017244
 * @note Size: 0xC4
 */
extern "C" void SetTranslateX__5CJObjFfl(CJObj* jobj, f32 x, s32 force)
{
	jobj->CJObj::SetTranslateX(x, force);
}

/**
 * @note Address: 0x80017308
 * @note Size: 0xC4
 */
extern "C" void SetTranslateY__5CJObjFfl(CJObj* jobj, f32 y, s32 force)
{
	jobj->CJObj::SetTranslateY(y, force);
}

/**
 * @note Address: 0x800173CC
 * @note Size: 0xC4
 */
extern "C" void SetTranslateZ__5CJObjFfl(CJObj* jobj, f32 z, s32 force)
{
	jobj->CJObj::SetTranslateZ(z, force);
}

/**
 * @note Address: 0x80017490
 * @note Size: 0xC4
 */
extern "C" void SetScaleX__5CJObjFfl(CJObj* jobj, f32 x, s32 force)
{
	jobj->CJObj::SetScaleX(x, force);
}

/**
 * @note Address: 0x80017554
 * @note Size: 0xC4
 */
extern "C" void SetScaleY__5CJObjFfl(CJObj* jobj, f32 y, s32 force)
{
	jobj->CJObj::SetScaleY(y, force);
}

/**
 * @note Address: 0x80017618
 * @note Size: 0xC4
 */
extern "C" void SetScaleZ__5CJObjFfl(CJObj* jobj, f32 z, s32 force)
{
	jobj->CJObj::SetScaleZ(z, force);
}

/**
 * @note Address: 0x800176DC
 * @note Size: 0x19C
 */
extern "C" void SetRotationX__5CJObjFfl(CJObj* jobj, f32 x, s32 force)
{
	jobj->CJObj::SetRotationX(x, force);
}

/**
 * @note Address: 0x80017878
 * @note Size: 0x19C
 */
extern "C" void SetRotationY__5CJObjFfl(CJObj* jobj, f32 y, s32 force)
{
	jobj->CJObj::SetRotationY(y, force);
}

/**
 * @note Address: 0x80017A14
 * @note Size: 0x19C
 */
extern "C" void SetRotationZ__5CJObjFfl(CJObj* jobj, f32 z, s32 force)
{
	jobj->CJObj::SetRotationZ(z, force);
}

/**
 * @note Address: 0x80017BB0
 * @note Size: 0x16C
 */
extern "C" void SetTranslate__5CJObjFfffl(CJObj* jobj, f32 x, f32 y, f32 z, s32 force)
{
	jobj->CJObj::SetTranslate(x, y, z, force);
}

/**
 * @note Address: 0x80017D1C
 * @note Size: 0x14C
 */
extern "C" void SetTranslate__5CJObjFRC3Vecl(CJObj* jobj, const Vec& v, s32 force)
{
	jobj->CJObj::SetTranslate(v, force);
}

/**
 * @note Address: 0x80017E68
 * @note Size: 0x168
 */
extern "C" void SetScale__5CJObjFfffl(CJObj* jobj, f32 x, f32 y, f32 z, s32 force)
{
	jobj->CJObj::SetScale(x, y, z, force);
}

/**
 * @note Address: 0x80017FD0
 * @note Size: 0x148
 */
extern "C" void SetScale__5CJObjFRC3Vecl(CJObj* jobj, const Vec& v, s32 force)
{
	jobj->CJObj::SetScale(v, force);
}

/**
 * @note Address: 0x80018118
 * @note Size: 0x21C
 */
extern "C" void SetRotation__5CJObjFfffl(CJObj* jobj, f32 x, f32 y, f32 z, s32 force)
{
	Quaternion q;

	if (force == 0) {
		// retail tests the cached translation here
		if (!(jobj->mTranslate != CVec(x, y, z))) {
			return;
		}
	}
	jobj->mRotate.x = x;
	jobj->mRotate.y = y;
	jobj->mRotate.z = z;
	if (jobj->mJObj->flags & USE_QUATERNION) {
		EulerToQuat(&jobj->mRotate, &q);
		HSD_JObjSetRotation(jobj->mJObj, &q);
	} else {
		HSD_JObjSetRotation(jobj->mJObj, (Quaternion*)&jobj->mRotate);
	}
}

/**
 * @note Address: 0x80018334
 * @note Size: 0x1FC
 */
extern "C" void SetRotation__5CJObjFRC3Vecl(CJObj* jobj, const Vec& v, s32 force)
{
	if (force == 0 && !(jobj->mRotate != v)) {
		return;
	}
	jobj->mRotate.x = v.x;
	jobj->mRotate.y = v.y;
	jobj->mRotate.z = v.z;
	if (jobj->mJObj->flags & USE_QUATERNION) {
		Quaternion q;
		EulerToQuat(&jobj->mRotate, &q);
		HSD_JObjSetRotation(jobj->mJObj, &q);
	} else {
		HSD_JObjSetRotation(jobj->mJObj, (Quaternion*)&jobj->mRotate);
	}
}

/**
 * @note Address: 0x80018530
 * @note Size: 0x80
 */
extern "C" void SetAnimRate__5CJObjFfUl(CJObj* jobj, f32 rate, u32 mask)
{
	jobj->CJObj::SetAnimRate(rate, mask);
}
