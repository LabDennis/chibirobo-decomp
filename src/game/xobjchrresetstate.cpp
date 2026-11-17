#include "game/xobjchrstate.h"
#include "game/video.h"
#include <math.h>

extern "C" {
void SetPlayerEyeCameraJointsHidden(XObj*, BOOL);
void InitializePlayerPopperAction(XActPopper*, XObjChr*);
void InitRadarAction(XActRadar*, XObjChr*);
}

static inline void ClearRingHighlights(OctaMenu& ring)
{
	ring.mOpen = 0;
	ring.mFade = 0.0f;
	for (s32 i = 0; i < 8; ++i)
		ring.mItems[i].mHighlight = 0.0f;
}

/**
 * @note Address: 0x800BB77C
 * @note Size: 0x600
 */
void XObjChr::ResetObjectState()
{
	ResetObjectState__4XObjFv(this);
	SetPlayerEyeCameraJointsHidden(this, FALSE);
	mMotion.Reset(0.0f, 0.0f, 0.0f);
	mStick[2] = mStick[3] = mStick[0] = mStick[1] = 0.0f;
	mActionObjectIndex = mHitObjectIndex = -1;
	mFloorContacts[4].objectIndex = mFloorContacts[2].objectIndex = mFloorContacts[0].objectIndex = mFloorContacts[1].objectIndex = -1;
	mLastContactFloorIndex                                                                                                        = -1;
	mNearDistance                                                                                                                 = 2.0f;
	mLetickerIndex                                                                                                                = -1;
	_A14                                                                                                                          = 0;
	mLiftPlugFlags &= ~0x80;
	mAPressed = mPressTimer = 0;
	mIdleLoops              = 0;
	mIdleTimer              = 0.0f;
	mFlashTimer = mFlashCount = 0;
	mHitStrength              = 0.0f;
	mMoveSpeedScale           = 1.0f;
	mHang.Init(this);
	mHang.mStickAway = FALSE;
	mJump.Init(this);
	mJump.mStrength = mJump.mTargetStrength = 0.0f;
	mLadder.Init(this);
	mLadder.mBPressed = FALSE;
	mRope.Init(this);
	mPush.Init(this);
	mTumble.Init(this);
	mCopter.Init(this);
	mCopter.mTime = FRAME_RATE;
	InitializePlayerPopperAction(&mPopper, this);
	InitRadarAction(&mRadar, this);
	mFrameTime.mFrame = mFrameTime.mLastFrame = -1.0f;
	for (s32 i = 0; i < 4; ++i) {
		mIdleJoints[i].mJObj       = NULL;
		mIdleJoints[i].mAnimJoint  = NULL;
		mIdleJoints[i].mLastAdHead = NULL;
	}
	mIdleJointCooldown   = 0;
	mAttachmentJoint     = NULL;
	mAttachmentJointAnim = NULL;
	s32 joint[]          = { 0, 1, 0, 1, 2 };
	mJointRot.Init(this, 5, joint, 'y');
	s32 neck[] = { 0, 1, 0, 1, 2 };
	mHeadTurn.mNeck.Init(this, 5, neck, 'y');
	s32 head[] = { 0, 1, 0, 1, 2 };
	mHeadTurn.mHead.Init(this, 5, head, 'z');
	mLook.mActive = FALSE;
	mLook.mMode   = -1;
	mLook.mObject = NULL;
	Vec zero      = { 0.0f, 0.0f, 0.0f };
	mLook.mOffset = zero;
	mLook.mPulse = mLook.mPhase = mLook.mFade = 0.0f;
	mLook.mObject                             = this;
	mAttachmentRequest                        = -1;
	mClimbType = mNextClimbType = mScriptAttach = mAttachmentStep = 0;
	mFallDistance                                                 = 0.0f;
	mCameraTarget[0] = mCameraTarget[1] = mCameraTarget[2] = NAN;
	mCameraTargetMargin = mCameraTargetMarginSetting = 1.0f;
	mHeldMoveAngle = mMoveAngle = mHeldStickAngle = NAN;
	mBatteryWarningFrames                         = 0;
	mBatteryWarningAngle = mBatteryWarningFade = 0.0f;
	mGaugeTarget                               = NULL;
	mGaugeTime                                 = 0.0f;
	mPlug.ResetObjectState();
	mCopterObj.ResetObjectState();
	mPopperObj.ResetObjectState();
	mRadarObj.ResetObjectState();
	mOXObj.ResetObjectState();
	for (s32 i = 0; i < 9; ++i)
		mSuits.mObjects[i].ResetObjectState();
	mWalkHeading = NAN;
	ClearRingHighlights(mMenu.mAtc);
	ClearRingHighlights(mMenu.mDrs);
	mMenu.mTargetY = mMenu.mCursorY = mMenu.mFade = 0.0f;
	mOXMenu.mStarted                              = FALSE;
	mOXMenu.mFade                                 = 0.0f;
	mInAir                                        = TRUE;
	InitParticlePool(&mDust.mPtcl, mDust.mPtcls, sizeof(mDust.mPtcls[0]), ARRAY_SIZE(mDust.mPtcls));
	for (s32 i = 0; i < 10; ++i) {
		PointPtcl& p = gPointParticlePool[i];
		GetHudPointParticleOrigin(&gGame.mInfoScrn, &p.mStartY, &p.mStartX);
		p.burst.mState = p.display.mState = 0;
		gPointParticleSlots[i]            = NULL;
	}
	gPlayerSuitChangeEffect.Reset();
	mGroundType = mIdleModeTimer = mIdleMode = 0;
	_158C4.Set(0.0f, 0.0f, 0.0f);
	mHeadOffset      = 5.0f;
	mHeadOffsetReady = FALSE;
}
