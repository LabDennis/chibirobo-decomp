#ifndef STAGE02_CAMERAACTION_H
#define STAGE02_CAMERAACTION_H

#include "game/ut.h"

class CCamera;

class CInterfaceCameraAction {
public:
	CVec _00; // _00
	CVec _10; // _10
	CVec _20; // _20
	f32 _30;  // _30
	f32 _34;  // _34
	s32 _38;  // _38

	virtual ~CInterfaceCameraAction() { }       // _08
	virtual void PreviousUpdate() { }           // _0C
	virtual void Update() = 0;                  // _10
	virtual void vf14() { }                     // _14
	virtual void SetCamera(CCamera* camera) { } // _18
	virtual void Init() { }                     // _1C
	virtual void vf20() { }                     // _20
	virtual void vf24() { }                     // _24
	virtual void vf28() { }                     // _28
	virtual void SetEye(f32 x, f32 y, f32 z)    // _2C
	{
		_00.x = x;
		_00.y = y;
		_00.z = z;
	}
	virtual void SetInterest(f32 x, f32 y, f32 z) // _30
	{
		_10.x = x;
		_10.y = y;
		_10.z = z;
	}
	virtual void SetAngle(f32 x, f32 y, f32 z) // _34
	{
		_20.x = x;
		_20.y = y;
		_20.z = z;
	}
	virtual void SetEye(const Vec& v) { SetEye(v.x, v.y, v.z); }           // _38
	virtual void SetInterest(const Vec& v) { SetInterest(v.x, v.y, v.z); } // _3C
	virtual void SetAngle(const Vec& v) { SetAngle(v.x, v.y, v.z); }       // _40
	virtual void SetDistance(f32 v) { _30 = v; }                           // _44
	virtual void SetFov(f32 v) { _34 = v; }                                // _48
	virtual void SetAngleZ(f32 v) { _20.z = v; }                           // _4C
	virtual s32 GetMode() { return 0; }                                    // _50
	virtual s32 GetFlags() { return _38; }                                 // _54
	virtual f32 GetEaseDivisor() { return 12.0f; }                         // _58
	virtual s32 vf5C() { return 0; }                                       // _5C
	virtual s32 GetTarget() { return 0; }                                  // _60
};

#endif
