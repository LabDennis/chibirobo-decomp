#ifndef _GAME_CMOTIONCONTROLCAMERA_H
#define _GAME_CMOTIONCONTROLCAMERA_H

#include "game/ccameraaction.h"
#include "game/ccameraactioneye.h"

// the game's camera and the actions that drive it. class names are RTTI, layouts are from
// CGame's ctor at 0x80019480 and the action ctor at 0x80019E5C

// where the actions sit in CCameraActions's table
enum {
	CAMACTION_DEFAULT, // 0
	CAMACTION_AREA,    // 1
	CAMACTION_CHASE,   // 2
	CAMACTION_EVT,     // 3
	CAMACTION_SCROLL2, // 4
	CAMACTION_EYE,     // 5
	CAMACTION_ANIM,    // 6
	CAMACTION_MAP,     // 7
	CAMACTION_LADR,    // 8
	CAMACTION_RESET,   // 9
	CAMACTION_10,      // 10, always null
	CAMACTION_SCROLL,  // 11
	CAMACTION_NUM,
};

// what the camera's flags say an action drives, from its GetCameraFlags
enum {
	CAMFLAG_EYE      = 0x1,
	CAMFLAG_INTEREST = 0x2,
	CAMFLAG_DISTANCE = 0x4,
	CAMFLAG_FOV      = 0x8,
	CAMFLAG_ANGLE    = 0x10,
};

// every camera action in one block, CGame keeps it at +0xE5FAC
class CCameraActions {
public:
	CCameraActionDefault mDefault;                 // _00
	CCameraActionArea mArea;                       // _40
	CCameraActionChase mChase;                     // _2D4
	CCameraActionEvt mEvt;                         // _344
	CCameraActionScroll2 mScroll2;                 // _828
	CCameraActionEye mEye;                         // _8D4
	CCameraActionAnim mAnim;                       // _954
	CCameraActionMap mMap;                         // _99C
	CCameraActionLadr mLadr;                       // _1B88
	CCameraActionReset mReset;                     // _1BCC
	CCameraActionScroll mScroll;                   // _1DD4
	CInterfaceCameraAction* mTable[CAMACTION_NUM]; // _1E40
	// _1E70 vtable

	CCameraActions();
	virtual ~CCameraActions() { } // _08

	void Init();
};
STATIC_ASSERT(sizeof(CCameraActions) == 0x1E74);

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

// the game's camera, a CCamera with whichever action drives it. CGame keeps it at +0xE5D38
class CMotionControlCamera {
public:
	CCamera mCamera;                       // _00
	CVec mEye;                             // _194, what the action wants this frame
	CVec mInterest;                        // _1A4
	CVec mAngle;                           // _1B4
	f32 mDistance;                         // _1C4
	f32 mFov;                              // _1C8
	s32 mMode;                             // _1CC, the action's GetMode, 1 eases eye and interest in
	CInterfaceCameraAction* mAction;       // _1D0
	CInterfaceCameraAction* mPrevAction;   // _1D4
	CInterfaceCameraAction* mPushedAction; // _1D8
	s32 _1DC;                              // _1DC
	s32 _1E0;                              // _1E0
	CStatus mFlags;                        // _1E4, the action's GetCameraFlags, see CAMFLAG
	s32 mInterestMode;                     // _1EC, the action's GetInterestMode, 2 and 3 follow the interest in
	s32 mEyeMode;                          // _1F0, 2 and 3 follow the eye in
	// _1F4 vtable

	virtual ~CMotionControlCamera() { } // _08

	CVec mFollowTarget;      // _1F8, Follow steps mFollowValue towards it
	CVec mFollowValue;       // _208
	CVec mFollowStep;        // _218
	BOOL mFollowDone;        // _228
	f32 mFollowDivide;       // _22C, how many steps it takes, the action's GetFollowDivisor
	CCameraMemento mMemento; // _230
	u8 _270[0x274 - 0x270];  // _270

	void Init();
	void InitCamera();
	void Update(u32 step);
	void Interpolate(u32 step);
	void Draw();
	void ChangeAction(s32 type);
	void ChangeAction(CInterfaceCameraAction* action);
	void PushAction(CInterfaceCameraAction* action);
	void PopAction();
	void RestoreAction();
	void ApplyAction();
	bool IsAction(s32 type);
	s32 GetActionType();
	void UpdateTarget(XObj* obj);
	void StartEye();
	void EndEye();
	CCameraMemento* CreateMemento(CCameraMemento* memento);
	BOOL SetMemento(const CCameraMemento* memento);
	f32 GetMapDistance();

	// what the action wants, the camera eases toward it in Interpolate
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

	// steps mFollowValue a fixed part of the way to the target
	void Follow(const Vec& target, const Vec& value)
	{
		mFollowTarget.x = target.x;
		mFollowTarget.y = target.y;
		mFollowTarget.z = target.z;
		mFollowValue.x  = value.x;
		mFollowValue.y  = value.y;
		mFollowValue.z  = value.z;
		mFollowStep     = (mFollowTarget - mFollowValue) / mFollowDivide;

		f32 step = PSVECMag(&mFollowStep);
		f32 dist = PSVECDistance(&mFollowTarget, &mFollowValue);
		if (dist <= step || dist < 0.05f) {
			mFollowValue.x = mFollowTarget.x;
			mFollowValue.y = mFollowTarget.y;
			mFollowValue.z = mFollowTarget.z;
			mFollowDone    = TRUE;
		} else {
			Vec step = mFollowStep;
			PSVECAdd(&mFollowValue, &step, &mFollowValue);
		}
	}

	// takes over what the new action drives
	void ReadAction()
	{
		mEyeMode = 0;
		if (mAction != NULL) {
			if (mAction == GetAction(CAMACTION_ANIM)) {
				mMode         = 0;
				mFlags.mFlags = 0;
				mFlags.mFlags |= CAMFLAG_EYE;
				mFlags.mFlags |= CAMFLAG_INTEREST;
				mFlags.mFlags |= CAMFLAG_FOV;
				mFlags.mFlags |= CAMFLAG_ANGLE;
				mFlags.mFlags |= CAMFLAG_DISTANCE;
				mFlags.mFlags |= 0x40000;
			} else {
				mFlags.mFlags = 0;
				mMode         = mAction->GetMode();
				mFlags.mFlags |= mAction->GetCameraFlags();
				mFollowDivide = mAction->GetFollowDivisor();
				mInterestMode = mAction->GetInterestMode();
			}
		}
	}

	// some actions start from the camera as it is, the rest from scratch
	void StartAction()
	{
		if (mAction != NULL) {
			if (mAction == GetAction(CAMACTION_CHASE) || mAction == GetAction(CAMACTION_10)) {
				mAction->Reset();
			} else if (mAction == GetAction(CAMACTION_EVT) || mAction == GetAction(CAMACTION_AREA) || mAction == GetAction(CAMACTION_MAP)
			           || mAction == GetAction(CAMACTION_RESET) || mAction == GetAction(CAMACTION_ANIM)) {
				mAction->SetCamera(&mCamera);
			} else {
				mAction->Reset();
			}
		}
	}

	// inline copy of ApplyAction
	void CopyAction()
	{
		if (mAction != NULL) {
			if (mFlags.mFlags & CAMFLAG_EYE) {
				mCamera.SetEye(mAction->mEye);
			}
			if (mFlags.mFlags & CAMFLAG_INTEREST) {
				mCamera.SetInterest(mAction->mInterest);
			}
			if (mFlags.mFlags & CAMFLAG_DISTANCE) {
				mCamera.SetDistance(mAction->mDistance);
			}
			if (mFlags.mFlags & CAMFLAG_FOV) {
				mCamera.SetFov(mAction->mFov);
			}
			if (mFlags.mFlags & CAMFLAG_ANGLE) {
				mCamera.SetAngle(mAction->mAngle);
			}
		}
	}

	static CInterfaceCameraAction* GetAction(s32 type);
	BOOL IsCurrent(s32 type) const { return mAction == GetAction(type) ? TRUE : FALSE; }
};
STATIC_ASSERT(sizeof(CMotionControlCamera) == 0x274);

#endif
