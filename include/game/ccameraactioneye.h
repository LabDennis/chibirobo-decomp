#ifndef _GAME_CCAMERAACTIONEYE_H
#define _GAME_CCAMERAACTIONEYE_H

#include "game/ccameraaction.h"
#if defined(VERSION_R24J01)
#include "game/ut.h"
#else
#include "game/cobj.h"
#endif

// split out since cobj.h puts .rodata in every file that includes it

#if defined(VERSION_R24J01)

// a value with a target and a per frame speed to step it there, the same class
// wii/xobj.h declares (the decl copied since cobj.h pulls the GameCube-only HSD
// headers; same pattern as gxa_shared.h). when wii/xobj.h is already in, its copy
// stays out; both are the member model the retail ctor emits: the TVec vtable
// goes to the first member and CVec vtables to the two behind it
#ifndef GAME_WII_XOBJ_H
class CObjVec {
public:
	// the camera constructors inline the member ctors (the retail aggregate
	// ctors store the vtables themselves); the object side's copy declares it
	// for xobj_ut.cpp's out-of-line definition, whose weak copy XObjChr calls
	CObjVec() { }
	TVec mValue;  // _00
	CVec mTarget; // _10
	CVec mSpeed;  // _20

	BOOL Step(f32 frames);
	bool IsMoving() const;

	// puts it at a value with nowhere to go
	void Reset(f32 x, f32 y, f32 z)
	{
		mValue.x  = x;
		mValue.y  = y;
		mValue.z  = z;
		mTarget.x = x;
		mTarget.y = y;
		mTarget.z = z;
		mSpeed.x  = 0.0f;
		mSpeed.y  = 0.0f;
		mSpeed.z  = 0.0f;
	}
	void Reset(const Vec& v) { Reset(v.x, v.y, v.z); }
	void SetTarget(f32 x, f32 y, f32 z)
	{
		mTarget.x = x;
		mTarget.y = y;
		mTarget.z = z;
	}
	void SetTarget(const Vec& v) { SetTarget(v.x, v.y, v.z); }
	// puts it at a value with its target there too, the speed stays
	void SetNow(const Vec& v)
	{
		mValue.Set(v);
		SetTarget(v);
	}
};
STATIC_ASSERT(sizeof(CObjVec) == 0x30);
#endif

// a two-point cubic in x and y: the end points at +00, the tangents at +10;
// the pointer position runs through it for the look response (fn_8016A844
// evaluates it in cmotioncontrolcamera.cpp)
struct EyeCurve {
	f32 mP0X, mP0Y, mP1X, mP1Y;
	f32 mT0X, mT0Y, mT1X, mT1Y;
};
STATIC_ASSERT(sizeof(EyeCurve) == 0x20);

// the first person view, the remote's pointer aims the look; the Wii class
// carries the pointer tracker and the response curves the GameCube one lacks
class CCameraActionEye : public CInterfaceCameraAction {
public:
	CObjVec mLook;    // _40, x up and down, y around, z the fov offset
	CVec mOverride;   // _70, a non zero part replaces the target the pointer gives
	CObjVec mVec80;   // _80, the pointer presence blend: 1 aims, 0 keeps the last aim
	f32 mB0;          // _B0
	Vec mVecB4;       // _B4
	Vec mVecC0;       // _C0, a copy of one of the file's static limit vectors
	EyeCurve mCurveX; // _CC, the pitch response
	EyeCurve mCurveY; // _EC, the yaw response
	CVec mVec10C;     // _10C, the pointer tracker's smoothed point
	f32 mRange;       // _11C, the screen band the pointer may leave the center by
	u8 mFirstFrame;   // _120, set until the tracker has its first pointer sample

	virtual void Update(u32 step); // _10
	virtual void Reset();          // _14
	virtual void Init();           // _1C
	virtual void DrawDebug2d();    // _28
	virtual u32 GetCameraFlags();  // _58
	virtual ~CCameraActionEye() { }

	f32 TrackPointer(u32 step, f32 low, f32 high); // the dead band the pointer's aim is kept in
};
STATIC_ASSERT(sizeof(CCameraActionEye) == 0x124);

#else

// the first person view, the player looks around with the stick
class CCameraActionEye : public CInterfaceCameraAction {
public:
	CObjVec mLook;  // _40, x up and down, y around, z the fov offset
	CVec mOverride; // _70, a non zero part replaces the target the stick gives

	virtual void Update(u32 step); // _10
	virtual void Reset();          // _14
	virtual void Init();           // _1C
	virtual u32 GetCameraFlags();  // _54
	virtual ~CCameraActionEye() { }
};
STATIC_ASSERT(sizeof(CCameraActionEye) == 0x80);

#endif

#endif
