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
	_988.Reset(0.0f, 0.0f, 0.0f);
	_9BC[1] = _9BC[0] = _9BC[3] = _9BC[2] = 0.0f;
	mHitObjectIndex = _9D0        = -1;
	mFloorContacts[1].objectIndex = mFloorContacts[0].objectIndex = mFloorContacts[2].objectIndex = mFloorContacts[4].objectIndex = -1;
	mLastContactFloorIndex                                                                                                        = -1;
	mNearDistance                                                                                                                 = 2.0f;
	_A10                                                                                                                          = -1;
	_A14                                                                                                                          = 0;
	mLiftPlugFlags &= ~0x80;
	_A1C = _A20 = 0;
	_A2C        = 0;
	_A30        = 0.0f;
	_A38 = _A34 = 0;
	_A3C        = 0.0f;
	_A40        = 1.0f;
	mHang.Init(this);
	mHang._2C = FALSE;
	mJump.Init(this);
	mJump.mTargetStrength = mJump.mStrength = 0.0f;
	mLadder.Init(this);
	mLadder._2C = FALSE;
	mRope.Init(this);
	mPush.Init(this);
	mTumble.Init(this);
	mCopter.Init(this);
	mCopter.mTime = FRAME_RATE;
	InitializePlayerPopperAction(&mPopper, this);
	InitRadarAction(&mRadar, this);
	mFrameTime.mLastFrame = mFrameTime.mFrame = -1.0f;
	for (s32 i = 0; i < 4; ++i) {
		_9500[i].mJObj      = NULL;
		_9500[i].mAnimJoint = NULL;
		_9500[i]._08        = NULL;
	}
	_9530                = 0;
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
	mLook.mFade = mLook.mPhase = mLook.mPulse = 0.0f;
	mLook.mObject                             = this;
	mAttachmentRequest                        = -1;
	_95D8 = _95DC = _95E4 = _95E0 = 0;
	_9A90                         = 0.0f;
	mCameraTarget[2] = mCameraTarget[1] = mCameraTarget[0] = NAN;
	mCameraTargetMarginSetting = mCameraTargetMargin = 1.0f;
	_9AB0 = _9AAC = _9AA8 = NAN;
	mBatteryWarningFrames = 0;
	mBatteryWarningFade = mBatteryWarningAngle = 0.0f;
	_9AC0                                      = 0;
	_9AC4                                      = 0.0f;
	mPlug.ResetObjectState();
	mCopterObj.ResetObjectState();
	mPopperObj.ResetObjectState();
	mRadarObj.ResetObjectState();
	mOXObj.ResetObjectState();
	for (s32 i = 0; i < 9; ++i)
		mSuits.mObjects[i].ResetObjectState();
	_12DF8 = NAN;
	ClearRingHighlights(mMenu.mAtc);
	ClearRingHighlights(mMenu.mDrs);
	mMenu.mFade = mMenu.mCursorY = mMenu.mTargetY = 0.0f;
	mOXMenu.mStarted                              = FALSE;
	mOXMenu.mFade                                 = 0.0f;
	_146D8                                        = TRUE;
	InitParticlePool(&mDust.mPtcl, mDust.mPtcls, sizeof(mDust.mPtcls[0]), ARRAY_SIZE(mDust.mPtcls));
	for (s32 i = 0; i < 10; ++i) {
		PointPtcl& p = gPointParticlePool[i];
		GetHudPointParticleOrigin(&gGame.mInfoScrn, &p.mStartY, &p.mStartX);
		p.display.mState = p.burst.mState = 0;
		gPointParticleSlots[i]            = NULL;
	}
	gPlayerSuitChangeEffect.active = FALSE;
	for (s32 i = 0; i < 2; ++i)
		for (s32 j = 0; j < 10; ++j) {
			gPlayerSuitChangeEffect.rings[i][j].scale = 0.0f;
			gPlayerSuitChangeEffect.rings[i][j].angle = 0.0f;
		}
	_158B8 = _158BC = _158C0 = 0;
	_158C4.Set(0.0f, 0.0f, 0.0f);
	mHeadOffset      = 5.0f;
	mHeadOffsetReady = FALSE;
}
