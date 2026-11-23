#ifndef _GAME_CCAMERAMEMENTO_H
#define _GAME_CCAMERAMEMENTO_H

#ifdef _MTXTYPES_h_
#include <dolphin/gx/gxtypes.h>
#else
#include "Dolphin/mtx.h"
#endif
#include "game/ut.h"

class CInterfaceCameraAction;

// what CreateMemento saves of the camera, the class name is RTTI
class CCameraMemento {
public:
	CCameraMemento() { }
	// _00 vtable
	virtual ~CCameraMemento() { } // _08

	CVec mEye;                       // _04
	CVec mInterest;                  // _14
	CVec mAngle;                     // _24
	f32 mFov;                        // _34
	f32 mDistance;                   // _38
	CInterfaceCameraAction* mAction; // _3C

	void SetEye(f32 x, f32 y, f32 z)
	{
		mEye.x = x;
		mEye.y = y;
		mEye.z = z;
	}
	void SetInterest(f32 x, f32 y, f32 z)
	{
		mInterest.x = x;
		mInterest.y = y;
		mInterest.z = z;
	}
	void SetAngle(f32 x, f32 y, f32 z)
	{
		mAngle.x = x;
		mAngle.y = y;
		mAngle.z = z;
	}
	void SetEye(const Vec& eye) { SetEye(eye.x, eye.y, eye.z); }
	void SetInterest(const Vec& interest) { SetInterest(interest.x, interest.y, interest.z); }
	void SetAngle(const Vec& angle) { SetAngle(angle.x, angle.y, angle.z); }
};
STATIC_ASSERT(sizeof(CCameraMemento) == 0x40);

#endif
