// the Wii model object, cobj.cpp by the file name its nw4r asserts carry: NW_Obj wraps the
// nw4r scene model and its animation objects, CObj is the GameCube CJObj and CXObj in one
// class on top of it. Only the functions that stay clear of the nw4r::g3d headers are here.
#include "game/wii/xobj.h"
#include "Dolphin/gx.h"
#include "Dolphin/mtx.h"
#include "game/gmath.h"
#include <math.h>

/**
 * @note Address: 0x802B95C0
 * @note Size: 0x180
 */
void CObj::Init()
{
	mRequest.Reset();
	mLoadState = 0;
	mMoveTranslate.Set(0.0f, 0.0f, 0.0f);
	mMoveTarget0.Set(0.0f, 0.0f, 0.0f);
	mMoveSpeed0.Set(0.0f, 0.0f, 0.0f);
	mMoveRotate.Set(0.0f, 0.0f, 0.0f);
	mMoveTarget1.Set(0.0f, 0.0f, 0.0f);
	mMoveSpeed1.Set(0.0f, 0.0f, 0.0f);
	mMoveScale.Set(1.0f, 1.0f, 1.0f);
	mMoveTarget2.Set(1.0f, 1.0f, 1.0f);
	mMoveSpeed2.Set(0.0f, 0.0f, 0.0f);
	mAnimId        = -1;
	mAnimRate[1]   = 0.0f;
	mAnimRate[0]   = 0.0f;
	mAnimRate[3]   = 0.0f;
	mAnimRate[2]   = 0.0f;
	GXColor add    = { 0, 0, 0, 0 };
	mColor0        = add;
	GXColor mul    = { 0xFF, 0xFF, 0xFF, 0 };
	mColor1        = mul;
	mColorsChanged = TRUE;
}

/**
 * @note Address: 0x802B9740
 * @note Size: 0x160
 */
// NONMATCHING: retail keeps the four components of the first color in r3..r6 in rising order, this
// loads them falling (r6..r3); the second color and everything else agree. Building the first color
// from a u32 0 gets the registers and swaps the two colors' stack slots instead.
BOOL CObj::Load(ResourceFile* file, const char* path, u32 model, u32 flags, u32 offset, u32 size)
{
	switch (mLoadState) {
	case 0:
		ReadAsync(file, path, model, flags, offset, size);
		// no break: the Wii checks the read it has just asked for
	case 1:
		ReadCheck();
		break;
	}
	if (mLoadState == 0) {
		GXColor add    = { 0, 0, 0, 0 };
		mColor0        = add;
		GXColor mul    = { 0xFF, 0xFF, 0xFF, 0 };
		mColor1        = mul;
		mColorsChanged = TRUE;
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x802B98A0
 * @note Size: 0x7C
 */
BOOL CObj::ReadAsync(ResourceFile* file, const char* path, u32 model, u32 flags, u32 offset, u32 size)
{
	if (mRequest.Request(file, path, flags, offset, size)) {
		mLoadState++;
		mModel     = model;
		mLoadFlags = flags;
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x802B9AE4
 * @note Size: 0x80
 */
void CObj::Release()
{
	if (mObject.m_pScnMdl != NULL) {
		ClearAnim();
		mObject.Release((mRequest.mFile != NULL ? mRequest.mFile->mRefCount : 0) == 1);
	}
	mRequest.Release();
	mLoadState = 0;
}

/**
 * @note Address: 0x802B9C50
 * @note Size: 0x124
 */
// NONMATCHING: the two temporaries swap stack slots, retail has the scaled direction at 0x8 and
// the Vec operator- takes at 0x18. Leaving the product unnamed gets the slots and loses the order
// its components are loaded in.
void CObj::SetMoveDir(const Vec* dir, f32 speed)
{
	mMoveTarget0.Set(dir->x, dir->y, dir->z);
	CVec d = mMoveTranslate - *dir;
	if (PSVECMag(&d) > 0.0f) {
		// the float fabs is nw4r::math::FAbs's, as in fbeffect.cpp
		d.x = __fabsf(d.x);
		d.y = __fabsf(d.y);
		d.z = __fabsf(d.z);
		// each axis gets its share of the distance, so all three arrive on the same frame
		PSVECNormalize(&d, &d);
		CVec step = d * speed;
		mMoveSpeed0.Set(step.x, step.y, step.z);
		if (mMoveSpeed0.x > 0.0f && mMoveSpeed0.x < 0.001f) {
			mMoveSpeed0.x = 0.001f;
		}
		if (mMoveSpeed0.z > 0.0f && mMoveSpeed0.z < 0.001f) {
			mMoveSpeed0.z = 0.001f;
		}
	}
}

/**
 * @note Address: 0x802B9D74
 * @note Size: 0xDC
 */
void CObj::SetRotateY(f32 target, f32 speed)
{
	mMoveTarget1.y = target;
	mMoveSpeed1.y  = speed;
	if (speed == 0.0f) {
		mMoveRotate.y = target;
		return;
	}
	f32 cur        = WrapAngle(mMoveRotate.y);
	f32 tgt        = WrapAngle(mMoveTarget1.y);
	mMoveRotate.y  = cur;
	mMoveTarget1.y = tgt;
	if (tgt == cur) {
		return;
	}
	f32 diff = tgt - cur;
	diff     = __fabsf(diff);
	if (diff >= TWO_PI) {
		diff -= TWO_PI;
	}
	// go the short way round
	if (diff > PI) {
		if (tgt < cur) {
			mMoveTarget1.y = TWO_PI + tgt;
		} else {
			mMoveTarget1.y = tgt - TWO_PI;
		}
	}
}

/**
 * @note Address: 0x802B9E50
 * @note Size: 0x9C
 */
void CObj::SetAnim(s32 id, BOOL force, f32 frame, f32 rate, f32 blend, f32 blendRate)
{
	if (force || id != mAnimId) {
		mAnimId = id;
		_148    = 0;
		mObject.SetAnim(NULL, id, frame, blend, blendRate);
		// a new anim always takes the rate too
		force = TRUE;
	}
	if (rate != mAnimRate[0] || force) {
		mAnimRate[0] = rate;
		mAnimRate[1] = 0.01f + rate;
	}
}

/**
 * @note Address: 0x802BA110
 * @note Size: 0x10
 */
void CObj::ClearAnim()
{
	mAnimId = -1;
	mObject.RemoveAnimAll();
}

/**
 * @note Address: 0x802BA120
 * @note Size: 0x14
 */
void CObj::SetAnimRate(f32 rate)
{
	mAnimRate[0] = rate;
	// off by a bit so the next update always sets it
	mAnimRate[1] = 0.01f + rate;
}

/**
 * @note Address: 0x802BA34C
 * @note Size: 0x48
 */
void CObj::SetAnmScn(u32 id, f32 frame, f32 rate)
{
	mObject.SetAnmScn(id, frame);
	SetAnmScnRate(rate);
}

/**
 * @note Address: 0x802BA394
 * @note Size: 0x14
 */
void CObj::SetAnmScnRate(f32 rate)
{
	mAnimRate[2] = rate;
	mAnimRate[3] = 0.01f + rate;
}

/**
 * @note Address: 0x802BA3A8
 * @note Size: 0x94
 */
// the root node takes the object's own rotation, translation and scale
void CObj::ExecCallbackA(nw4r::g3d::ChrAnmResult* pResult, nw4r::g3d::ResMdl mdl, nw4r::g3d::FuncObjCalcWorld* pFuncObj)
{
	if (pFuncObj->GetCallbackNodeID() == 0) {
		nw4r::math::MTX34 rot;
		nw4r::math::MTX34 rotTrans;
		nw4r::math::MTX34RotXYZRad(&rot, mMoveRotate.x, mMoveRotate.y, mMoveRotate.z);
		PSMTXTransApply(rot.m, rotTrans.m, mMoveTranslate.x, mMoveTranslate.y, mMoveTranslate.z);
		pResult->SetRotTrans(&rotTrans);
		pResult->SetScale((const nw4r::math::VEC3*)&mMoveScale);
	}
}

/**
 * @note Address: 0x802BA43C
 * @note Size: 0xA4
 */
void CObj::ExecCallback_CALC_MAT(int timing, nw4r::g3d::ScnObj* obj, u32 param, void* info)
{
	switch (timing) {
	case 4:
		if (!(mLoadFlags & 0x20000) && mColorsChanged) {
			mObject.SetColors(mColor0, mColor1);
			mColorsChanged = FALSE;
		}
		break;
	}
}

/**
 * @note Address: 0x802BA4E0
 * @note Size: 0x4
 */
void CObj::ExecCallbackB()
{
}
