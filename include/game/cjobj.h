#ifndef _GAME_CJOBJ_H
#define _GAME_CJOBJ_H

// the HSD headers have to come first, see game/hsd_jobj.h
#include "game/hsd_jobj.h"
#include "game/ut.h"

// the dol's jobj wrapper, name from RTTI in shun and stage07. the three setters that build
// a Vec on the stack are in game/cjobjset.h, only the files that use them include it

extern "C" s32 EulerToQuat(Vec* euler, Quaternion* q);

// CJObj methods pool data addresses in shun and stage07, the REL code around them doesn't
#pragma push
#pragma pool_data on

#ifdef CJOINT_SETTER_SECTION
#define CJOINT_SETTER
#else
#define CJOINT_SETTER
#endif

#ifdef CJOINT_AXIS_SETTER_SECTION
#define CJOINT_ANIM_RATE
#define CJOINT_AXIS_SETTER
#else
#define CJOINT_ANIM_RATE   CJOINT_SETTER
#define CJOINT_AXIS_SETTER CJOINT_SETTER
#endif

class CJObj {
public:
	HSD_JObj* mJObj; // _00
	CVec mTranslate; // _04
	CVec mRotate;    // _14
	CVec mScale;     // _24
	                 // _34 vtable

#ifdef ACTOR_TOY_JOINT_CTOR_SECTION
#endif
	CJObj()
	{
		mJObj        = NULL;
		mTranslate.x = 0.0f;
		mTranslate.y = 0.0f;
		mTranslate.z = 0.0f;
		mRotate.x    = 0.0f;
		mRotate.y    = 0.0f;
		mRotate.z    = 0.0f;
		mScale.x     = 0.0f;
		mScale.y     = 0.0f;
		mScale.z     = 0.0f;
	}

	CJObj(HSD_JObj* jobj); // in the dol, 0x80084C78, game/msg.cpp

#ifdef CJOINT_SETTER_SECTION
#endif
	virtual ~CJObj(); // _08

	CJOINT_AXIS_SETTER virtual void SetTranslateX(f32 x, s32 force) // _0C
	{
		if (force == 0 && x == mTranslate.x) {
			return;
		}
		mTranslate.x = x;
		HSD_JObjSetTranslateX(mJObj, x);
	}
	CJOINT_AXIS_SETTER virtual void SetTranslateY(f32 y, s32 force) // _10
	{
		if (force == 0 && y == mTranslate.y) {
			return;
		}
		mTranslate.y = y;
		HSD_JObjSetTranslateY(mJObj, y);
	}
	CJOINT_AXIS_SETTER virtual void SetTranslateZ(f32 z, s32 force) // _14
	{
		if (force == 0 && z == mTranslate.z) {
			return;
		}
		mTranslate.z = z;
		HSD_JObjSetTranslateZ(mJObj, z);
	}
	CJOINT_AXIS_SETTER virtual void SetScaleX(f32 x, s32 force) // _18
	{
		if (force == 0 && x == mScale.x) {
			return;
		}
		mScale.x = x;
		HSD_JObjSetScaleX(mJObj, x);
	}
	CJOINT_AXIS_SETTER virtual void SetScaleY(f32 y, s32 force) // _1C
	{
		if (force == 0 && y == mScale.y) {
			return;
		}
		mScale.y = y;
		HSD_JObjSetScaleY(mJObj, y);
	}
	CJOINT_AXIS_SETTER virtual void SetScaleZ(f32 z, s32 force) // _20
	{
		if (force == 0 && z == mScale.z) {
			return;
		}
		mScale.z = z;
		HSD_JObjSetScaleZ(mJObj, z);
	}
	CJOINT_SETTER virtual void SetRotationX(f32 x, s32 force) // _24
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
	CJOINT_SETTER virtual void SetRotationY(f32 y, s32 force) // _28
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
	CJOINT_SETTER virtual void SetRotationZ(f32 z, s32 force) // _2C
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
	CJOINT_SETTER virtual void SetTranslate(const Vec& v, s32 force) // _30
	{
		if (force == 0 && !(mTranslate != v)) {
			return;
		}
		mTranslate.x = v.x;
		mTranslate.y = v.y;
		mTranslate.z = v.z;
		HSD_JObjSetTranslate(mJObj, &mTranslate);
	}
	CJOINT_SETTER virtual void SetTranslate(f32 x, f32 y, f32 z, s32 force); // _34, in game/cjobjset.h
	CJOINT_SETTER virtual void SetScale(const Vec& v, s32 force)             // _38
	{
		if (force == 0 && !(mScale != v)) {
			return;
		}
		mScale.x = v.x;
		mScale.y = v.y;
		mScale.z = v.z;
		HSD_JObjSetScale(mJObj, &mScale);
	}
	CJOINT_SETTER virtual void SetScale(f32 x, f32 y, f32 z, s32 force); // _3C, in game/cjobjset.h
	CJOINT_SETTER virtual void SetRotation(const Vec& v, s32 force)      // _40
	{
		if (force == 0 && !(mRotate != v)) {
			return;
		}
		mRotate.x = v.x;
		mRotate.y = v.y;
		mRotate.z = v.z;
		if (mJObj->flags & USE_QUATERNION) {
			Quaternion q;
			EulerToQuat(&mRotate, &q);
			HSD_JObjSetRotation(mJObj, &q);
		} else {
			// a CVec passed off as a quaternion, w ends up being its vtable pointer
			HSD_JObjSetRotation(mJObj, (Quaternion*)&mRotate);
		}
	}
	CJOINT_SETTER virtual void SetRotation(f32 x, f32 y, f32 z, s32 force); // _44, in game/cjobjset.h
	CJOINT_ANIM_RATE virtual void SetAnimRate(f32 rate, u32 mask)           // _48
	{
		HSD_JObj* jobj;

		if (HSD_JObjGetFlags(mJObj) & INSTANCE) {
			jobj = mJObj->child;
		} else {
			jobj = mJObj;
		}
		HSD_ForeachAnim(jobj, JOBJ_TYPE, (HSD_TypeMask)mask, HSD_AObjSetRate, AOBJ_ARG_AF, rate);
	}

	void UpdateRotation()
	{
		if (mJObj->flags & USE_QUATERNION) {
			Quaternion q;
			EulerToQuat(&mRotate, &q);
			HSD_JObjSetRotation(mJObj, &q);
		} else {
			// a CVec passed off as a quaternion, w ends up being its vtable pointer
			HSD_JObjSetRotation(mJObj, (Quaternion*)&mRotate);
		}
	}

	void Attach(HSD_JObj* jobj)
	{
		mJObj = jobj;
		HSD_JObjGetTranslation(mJObj, &mTranslate);
		HSD_JObjGetRotation(mJObj, (Quaternion*)&mRotate);
		HSD_JObjGetScale(mJObj, &mScale);
	}

	// the joint's own values, read straight from it
	f32 GetTranslateX() const { return HSD_JObjGetTranslationX(mJObj); }
	f32 GetTranslateY() const { return HSD_JObjGetTranslationY(mJObj); }
	f32 GetTranslateZ() const { return HSD_JObjGetTranslationZ(mJObj); }
	f32 GetScaleX() const { return HSD_JObjGetScaleX(mJObj); }
	f32 GetScaleY() const { return HSD_JObjGetScaleY(mJObj); }
	f32 GetScaleZ() const { return HSD_JObjGetScaleZ(mJObj); }
	f32 GetRotationY() const { return HSD_JObjGetRotationY(mJObj); }
	MtxPtr GetMtx() const { return HSD_JObjGetMtxPtr(mJObj); }
	Vec GetPos() const { return GetJointPosition(mJObj); }
};
#undef CJOINT_ANIM_RATE
#undef CJOINT_AXIS_SETTER
#undef CJOINT_SETTER

#ifndef DEFER_OBJECT_DESTRUCTORS
inline CJObj::~CJObj()
{
}
#endif
typedef char CJObj_size_check[sizeof(CJObj) == 0x38 ? 1 : -1];

#pragma pop

#endif
