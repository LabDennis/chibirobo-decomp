#ifndef _GAME_XOBJLETICKER_H
#define _GAME_XOBJLETICKER_H

#include "game/xobj.h"
#include "game/csound.h"

// carries up to two other objects along with it, its camera is CCameraActionLeticker
class XObjLeticker : public XObj {
public:
	typedef void (XObjLeticker::*Func)();

	// _00-_980 = XObj
	Func mFunc;                                             // _980, null while idle
	s32 mStep;                                              // _98C, the step of mFunc, 10 holds chibi robo on joint 9
	s32 mLinked[2];                                         // _990, object table indices that move with it
	s32 mLinkedCount;                                       // _998
	s32 mCurrentLevel;                                      // _99C
	s32 mMaxLevel;                                          // _9A0
	s32 mLevelBySide[4];                                    // _9A4, picked by which way it faces
	s32 mFuncTime;                                          // _9B4, frames since mFunc was set
	XObjFloor mBox[2];                                      // _9B8, the box the first linked object stands on
	f32 mLevelProgress;                                     // _9F8, ten units per level
	f32 mMaxLevelProgress;                                  // _9FC, ten units per level
	f32 mHeight;                                            // _A00, the stacked joint is drawn every 24 under it
	Vec mStackTopPosition;                                  // _A04, the top of the stack
	u8 mCameraBounded;                                      // _A10
	f32 mCameraMinX, mCameraMaxX, mCameraMinZ, mCameraMaxZ; // _A14
	u8 mReachedTargetHeight;                                // _A24, set once mHeight is reached
	u8 _A25[0xA28 - 0xA25];                                 // _A25
	f32 mHeading;                                           // _A28
	u8 mPtcl;                                               // _A2C, particle 165 is running
	u8 mPowered;                                            // _A2D, set on the first vf134 feed from the plug, cleared by vf148
	u8 mWaiting;                                            // _A2E
	s32 mPowerTimer;        // _A30, frames it has been fed; under 40 counts as starting, 30 and counting down while waiting
	f32 mSpeedScale;        // _A34, movement and sound pitch scale
	s32 mState;             // _A38, 1 draws the stack, 2 and 3 skip the camera pop
	AnimSfx mSfx;           // _A3C
	u8 _A4C[0xA50 - 0xA4C]; // _A4C

	XObjLeticker();

	virtual void ResetObjectState();                                 // _98
	virtual void UpdateObject(u32 frames);                           // _A4
	virtual void vfAC();                                             // _AC
	virtual void DrawObjectEffects();                                // _C0
	virtual s32 HandleEventCommand(s32 count, s32* args);            // _124
	virtual void OnPushDisplacement(const Vec& delta);               // _130
	virtual BOOL vf134(f32 amount);                                  // _134
	virtual void OnHitFromPosition(const Vec& position, f32 amount); // _138
	virtual void vf148();                                            // _148

	void StateRide();
	void StateCount();
	void StateLift();
};
#if defined(VERSION_JP)
STATIC_ASSERT(sizeof(XObjLeticker) == 0xA40);
#else
STATIC_ASSERT(sizeof(XObjLeticker) == 0xA50);
#endif

#endif
