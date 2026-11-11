#ifndef _GAME_CCAMERA_H
#define _GAME_CCAMERA_H

#include "Version.h"
#include "game/hsd_cobj.h"
#include "game/ut.h"

// the game's camera, an HSD_CObj with its own eye, interest and angles on top. name and
// base from RTTI. the setters are inline, most of their copies are in game part 1

extern "C" {
// takes the roll back out of the cobj when it isn't using an up vector
f32 GetCameraRoll(HSD_CObj* cobj);
}

class CCamera;

extern "C" {
// sets the camera up for a render mode, cbasesetup.cpp
CCamera* InitGamePerspectiveCamera(CCamera* camera, GXRenderModeObj* mode);
// the same for CBase's second camera
CCamera* InitGameOrthoCamera(CCamera* camera, GXRenderModeObj* mode);
}

class CCamera : public HSD_CObj {
public:
	BOOL mUseMtx;           // _8C, the view comes from SetMtx, not from the eye and interest
	HSD_CObj* mLoadedCObj;  // _90, made by Init(desc)
	CVec mEye;              // _94
	CVec mInterest;         // _A4
	CVec mAngle;            // _B4, x up and down, y around, z tilts the up vector
	CVec mUp;               // _C4
	f32 mRoll;              // _D4
	f32 mFov;               // _D8
	f32 mNear;              // _DC
	f32 mFar;               // _E0
	f32 mDistance;          // _E4
	Mtx mViewMtx;           // _E8
	CVec mScale;            // _118
	Mtx mScaleMtx;          // _128
	HSD_AObj* mLongestAObj; // _158, whichever of the cobj, eye and interest anims ends last
	f32 mProjection[7];     // _15C, gx state saved by SetCurrent
	f32 mViewport[6];       // _178
	// _190 vtable

	virtual ~CCamera() { }                                                                                  // _08
	virtual void Init(f32 near, f32 far, f32 fov, GXRenderModeObj* rmode);                                  // _0C
	virtual void Init(f32 near, f32 far, f32 top, f32 bottom, f32 left, f32 right, GXRenderModeObj* rmode); // _10
	virtual void Init(f32 near, f32 far, GXRenderModeObj* rmode);                                           // _14
	virtual void Init(HSD_CObjDesc* desc);                                                                  // _18
	virtual void Release();                                                                                 // _1C
	virtual void SetAnim(HSD_CameraAnim* anim, f32 frame);                                                  // _20
	virtual void SetAnimRate(f32 rate)                                                                      // _24
	{
		HSD_ForeachAnim(this, COBJ_TYPE, (HSD_TypeMask)(COBJ_MASK | WOBJ_MASK), HSD_AObjSetRate, AOBJ_ARG_AF, rate);
	}
	virtual void Anim();                                                                                                           // _28
	virtual void SetViewport(f32 x, f32 y, GXRenderModeObj* rmode);                                                                // _2C
	virtual void SetViewport(f32 left, f32 right, f32 top, f32 bottom) { HSD_CObjSetViewportfx4(this, left, right, top, bottom); } // _30
	virtual void SetViewport(HSD_RectF32* viewport) { HSD_CObjSetViewportf(this, viewport); }                                      // _34
#if !defined(VERSION_JP)
	// japan doesn't have this one yet, so its slots after it are 4 lower
	virtual void SetViewport(HSD_RectS16* viewport) { HSD_CObjSetViewport(this, viewport); } // _38
#endif
	virtual void SetScissor(GXRenderModeObj* rmode);                                                                            // _3C
	virtual void SetScissor(s32 left, u16 right, s32 top, u16 bottom) { HSD_CObjSetScissorx4(this, left, right, top, bottom); } // _40
	virtual void SetScissor(Scissor* scissor) { HSD_CObjSetScissor(this, scissor); }                                            // _44
	virtual void SetFov(f32 fov)                                                                                                // _48
	{
		mFov = fov;
		HSD_CObjSetFov(this, fov);
	}
	virtual void SetFrustum(f32 top, f32 bottom, f32 left, f32 right) // _4C
	{
		HSD_CObjSetTop(this, top);
		HSD_CObjSetBottom(this, bottom);
		HSD_CObjSetLeft(this, left);
		HSD_CObjSetRight(this, right);
	}
	virtual void SetNear(f32 near) // _50
	{
		mNear = near;
		HSD_CObjSetNear(this, near);
	}
	virtual void SetFar(f32 far) // _54
	{
		mFar = far;
		HSD_CObjSetFar(this, far);
	}
	virtual void SetEye(f32 x, f32 y, f32 z) // _58
	{
		mEye.x = x;
		mEye.y = y;
		mEye.z = z;
	}
	virtual void SetInterest(f32 x, f32 y, f32 z) // _5C
	{
		mInterest.x = x;
		mInterest.y = y;
		mInterest.z = z;
	}
	virtual void SetAngle(f32 x, f32 y, f32 z) // _60
	{
		mAngle.x = x;
		mAngle.y = y;
		mAngle.z = z;
	}
	virtual void SetUp(f32 x, f32 y, f32 z) // _64
	{
		mUp.x = x;
		mUp.y = y;
		mUp.z = z;
	}
	virtual void SetDistance(f32 distance) { mDistance = distance; }                                   // _68
	virtual void SetEye(const Vec& eye) { SetEye(eye.x, eye.y, eye.z); }                               // _6C
	virtual void SetInterest(const Vec& interest) { SetInterest(interest.x, interest.y, interest.z); } // _70
	virtual void SetAngle(const Vec& angle) { SetAngle(angle.x, angle.y, angle.z); }                   // _74
	virtual void SetUp(const Vec& up) { SetUp(up.x, up.y, up.z); }                                     // _78
	virtual void SetAngleY(f32 y) { mAngle.y = y; }                                                    // _7C
	virtual void SetAngleX(f32 x) { mAngle.x = x; }                                                    // _80
	virtual void SetAngleZ(f32 z) { mAngle.z = z; }                                                    // _84
	virtual void SetScale(f32 x, f32 y, f32 z)                                                         // _88
	{
		mScale.x = x;
		mScale.y = y;
		mScale.z = z;
		PSMTXScale(mScaleMtx, x, y, z);
	}
	virtual void SetMtx(Mtx mtx) // _8C
	{
		mUseMtx = TRUE;
		HSD_CObjSetViewingMtx(this, mtx);
		PSMTXCopy(mtx, mViewMtx);
	}
	virtual void ClearMtx() // _90
	{
		mUseMtx = FALSE;
		HSD_CObjClearViewingMtx(this);
	}
	virtual void CalcEye();      // _94
	virtual void CalcInterest(); // _98
	virtual void CalcAngle();    // _9C
	virtual void CalcUp();       // _A0
	virtual void CalcDistance(); // _A4

	MtxPtr SetCurrent();
	void EndCurrent();
};
typedef char CCamera_size_check[sizeof(CCamera) == 0x194 ? 1 : -1];

#endif
