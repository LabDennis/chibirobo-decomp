#ifndef GAME_WII_XOBJLETICKER_H
#define GAME_WII_XOBJLETICKER_H

#include "game/wii/animsfx.h"
#include "game/wii/xobj.h"

// the rising lifts, name from the GameCube RTTI; it carries up to two other objects
// along with it and its camera is CCameraActionLeticker. The Wii class keeps the
// GameCube's fields shifted 0x28 behind the bigger XObj; the retail pool stride is
// 0xAC8 (offsets verified against ResetObjectState at 0x8011A334 and UpdateObject
// at 0x8011A3D0, which show the GameCube layout +0x28)
class XObjLeticker : public XObj {
public:
	typedef void (XObjLeticker::*Func)();

	XObjLeticker();
	virtual ~XObjLeticker();                                           // _08, defined in xobjctor.cpp
	virtual void ResetObjectState();                                   // _154
	virtual void UpdateObject(u32 frames);                             // _164
	virtual void DrawObject();                                         // _174
	virtual void DrawObjectEffects();                                  // _188
	virtual s32 HandleEventCommand(s32 count, s32* args);              // _1E8
	virtual void OnPushDisplacement(const Vec& displacement);          // _1F4
	virtual BOOL ReceivePlugPower(f32 amount);                         // _1F8
	virtual void OnHitFromPosition(const Vec& position, f32 strength); // _1FC
	virtual void PauseSounds(f32 fade);                                // _20C
	virtual void ResumeSounds(f32 fade);                               // _210
	virtual void OnUnplugged();                                        // _228 (slot unverified)
	virtual void DrawLate();                                           // _28C, CGame::DrawScene's last pass

	void StateRide();
	void StateCount();
	void StateLift();

	Func mFunc;              // _9A8, the state callback, __ptmf_null after reset
	u32 mStep;               // _9B4, the event counter StateRide counts down
	s32 mLinked[2];          // _9B8, object table indices of the pair moving in lock-step
	s32 mLinkedCount;        // _9C0, 1 or 2, set from the placement's links
	s32 mCurrentLevel;       // _9C4, the floor's level, 0 is bottom
	s32 mMaxLevel;           // _9C8, 1 or 2, set from the placement
	s32 mLevelBySide[4];     // _9CC, per side which level is reachable through it
	u32 mFuncTime;           // _9DC, frames the current state callback has run
	XObjFloor mBox[2];       // _9E0, the box floors, [0] the lift itself, [1] the lid
	f32 mLevelProgress;      // _A20, 0..1 between the current level and the next
	f32 mMaxLevelProgress;   // _A24, 1.0 or 2.0, set with mMaxLevel
	f32 mHeight;             // _A28, the lift's current height, in hundredths
	Vec mStackTopPosition;   // _A2C, the top of the stack StateLift passes on
	u8 mCameraBounded;       // _A38, StateRide's camera is boxed to the lift's bounds
	f32 mCameraMinX;         // _A3C
	f32 mCameraMaxX;         // _A40
	f32 mCameraMinZ;         // _A44
	f32 mCameraMaxZ;         // _A48
	u8 mReachedTargetHeight; // _A4C, the stack reached its target height once
	f32 mHeading;            // _A50, the direction the lid faces, in radians
	u8 mPtcl;                // _A54, the dust particle has been made
	u8 mPowered;             // _A55, ReceivePlugPower flipped the switch on
	u8 mWaiting;             // _A56, waiting for a linked leticker to finish
	u32 mPowerTimer;         // _A58, frames the power has been held
	f32 mSpeedScale;         // _A5C, the placement's speed multiplier
	s32 mState;              // _A60, the leticker's state (0 idle, 2 unplugged, 3 riding); retail compares it signed
	AnimSfx mSfx;            // _A64, the lid's animation sound effects
	u8 _AC0[8];              // _AC0, the pool stride runs to 0xAC8
};
STATIC_ASSERT(sizeof(XObjLeticker) == 0xAC8);
STATIC_ASSERT(offsetof(XObjLeticker, mLinkedCount) == 0x9C0);
STATIC_ASSERT(offsetof(XObjLeticker, mBox) == 0x9E0);
STATIC_ASSERT(offsetof(XObjLeticker, mState) == 0xA60);
STATIC_ASSERT(offsetof(XObjLeticker, mSfx) == 0xA64);

#endif
