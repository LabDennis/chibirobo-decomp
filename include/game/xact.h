#ifndef _GAME_XACT_H
#define _GAME_XACT_H

#include "game/xobj.h"
#include "game/cptcl.h"
#include "game/cscrn.h"
#include "game/xaction.h"

// chibi robo's actions, XAction and its XAct* kinds from RTTI. every one of them sits in
// XObjChr and shares XAction's two virtuals. offsets are the same in every region

class XObjChr;
struct HSD_Generator;

class XAction {
public:
	XObjChr* mOwner; // _00
	s32 mState;      // _04, 0 while idle
	// _08 vtable

	virtual void Init(XObjChr* owner); // _08, 0x800BBD7C
	virtual void Reset();              // _0C, 0x800BB708

	s32 _0C;          // _0C
	CVec _10;         // _10, push keeps the offset to its target here
	XObj* mTarget;    // _20
	s32 _24;          // _24
	s32 mTargetFloor; // _28, the selected floor when the target was picked

	// todo: cleanup
	void SetTarget(XObj* target)
	{
		mTarget      = target;
		mTargetFloor = mTarget->mSelectedFloorIndex;
	}
	void SetState(s32 state, XObj* target)
	{
		mState = state;
		if (target != NULL) {
			SetTarget(target);
		}
	}
};
STATIC_ASSERT(sizeof(XAction) == 0x2C);

// picking things up and carrying them
class XActLift : public XAction {
public:
	f32 _2C;        // _2C
	CVec _30;       // _30
	f32 mYawOffset; // _40, held object yaw relative to the player
	u8 _44[4];      // _44
	s16 _48;        // _48, -1 when there's none
	u8 _4A[2];
	XObj* mPendingItem;            // _4C
	XObj* mPlugTarget;             // _50
	u32 _54;                       // _54, copied to XObjPlug's _D3C
	f32 mBatteryCost;              // _58
	s32 mContactEffectId;          // _5C
	HSD_Generator* mContactEffect; // _60
	BOOL mCheckContact;            // _64
	s32 mUseEffectId;              // _68
	HSD_Generator* mUseEffect;     // _6C
	s32 mToolEffectId;             // _70
	HSD_Generator* mToolEffect;    // _74
	BOOL mCheckToolHit;            // _78
};
STATIC_ASSERT(sizeof(XActLift) == 0x7C);

extern "C" void UpdateLiftActionAnimation(XActLift* act, s32 anim, HSD_AObj* aobj, u32 frames, f32 frame, f32 end);
extern "C" void UpdateHeldObject(XActLift* act, s32 frames);

class XActHang : public XAction {
public:
	BOOL _2C;             // _2C, set when the stick points away
	Vec mHangStartOffset; // _30, player position relative to the target
	Vec _3C;              // _3C
	f32 _48;              // _48
	u8 _4C[0x50 - 0x4C];  // _4C
};
STATIC_ASSERT(sizeof(XActHang) == 0x50);

class XActJump : public XAction {
public:
	Vec mArcPosition;    // _2C, horizontal position and starting height
	f32 mRise;           // _38
	f32 mArcAngle;       // _3C
	f32 mArcStep;        // _40
	f32 mForward;        // _44
	f32 mStrength;       // _48
	f32 mTargetStrength; // _4C
	f32 mTargetHeight;   // _50
};
STATIC_ASSERT(sizeof(XActJump) == 0x54);

class XActLadder : public XAction {
public:
	BOOL _2C;   // _2C, set when b is pressed on it
	Vec mPos;   // _30, where he hangs, in front of the ladder
	f32 mDist;  // _3C, how far he still has to go to get there
	f32 mAngle; // _40, from mPos to where he started
};
STATIC_ASSERT(sizeof(XActLadder) == 0x44);

class XActRope : public XAction {
public:
	Vec mPos;   // _2C, horizontal approach point
	f32 mDist;  // _38
	f32 mAngle; // _3C
};
STATIC_ASSERT(sizeof(XActRope) == 0x40);

class XActPush : public XAction {
public:
	u8 _2C[0x34 - 0x2C];       // _2C
	CVec _34;                  // _34
	CVec _44;                  // _44
	BOOL mPushPositionClamped; // _54
};
STATIC_ASSERT(sizeof(XActPush) == 0x58);

// the ring on the floor around him while he tumbles
struct XActTumbleFade {
	BOOL mActive; // _00
	Vec mOffset;  // _04, from his feet, turned with him
	f32 mAmount;  // _10
};
STATIC_ASSERT(sizeof(XActTumbleFade) == 0x14);

class XActTumble : public XAction {
public:
	XActTumbleFade mFade;         // _2C
	f32 mSavedYaw;                // _40
	HSD_Generator* mIdleParticle; // _44
};
STATIC_ASSERT(sizeof(XActTumble) == 0x48);

class XActCopter : public XAction {
public:
	f32 mTime;  // _2C, how long he can stay up
	f32 mPhase; // _30, of the bob
	f32 mBob;   // _34, how high the bob lifts him
};
STATIC_ASSERT(sizeof(XActCopter) == 0x38);

// one of the popper's rings, drawn as two glow sprites and a soft disc
struct XActPopperRing {
	Vec mPos;    // _00
	f32 mAmount; // _0C, 0 when it's gone
	f32 mRadius; // _10
	u32 mColor;  // _14
};
STATIC_ASSERT(sizeof(XActPopperRing) == 0x18);

struct PopperShotDust {
	CPtcl mPtcl;           // _00
	CPtclDust2 mPtcls[40]; // _14
};

// one popper shot, it keeps its last ten positions for the trail
struct XActPopperShot {
	BOOL mActive;              // _00
	CVec mPos;                 // _04
	f32 mYaw;                  // _14
	f32 mPitch;                // _18
	f32 mSpeed;                // _1C
	f32 mDistance;             // _20
	f32 mRange;                // _24
	f32 _28;                   // _28
	f32 mStrength;             // _2C
	s32 mKind;                 // _30, 1 for the small shot
	XObj* mHomingTarget;       // _34
	XObj* mHitTarget;          // _38, deferred hit during an event
	Vec mHistory[10];          // _3C
	XActPopperRing mRing[2];   // _B4
	PopperShotDust mDust;      // _E4
	HSD_Generator* mGen;       // _D78
	HSD_Generator* mChargeGen; // _D7C
	HSD_JObj* mJoint;          // _D80

	XActPopperShot();
	~XActPopperShot();
};
STATIC_ASSERT(sizeof(XActPopperShot) == 0xD84);

// the popper's charge effect
struct XActPopperCharge {
	u32 mState;                // _00
	f32 mTimer;                // _04
	Vec mPos;                  // _08
	HSD_Generator* mGen;       // _14
	HSD_Generator* mChargeGen; // _18
	HSD_JObj* mJoint;          // _1C
};
STATIC_ASSERT(sizeof(XActPopperCharge) == 0x20);

// the chibi-blaster
class XActPopper : public XAction {
public:
	f32 _2C;                  // _2C
	XActPopperShot mShot[10]; // _30
	XActPopperCharge mCharge; // _8758
};
STATIC_ASSERT(sizeof(XActPopper) == 0x8778);

struct RadarAimAxis {
	f32 angle, targetAngle, reached;
};

class XActRadar : public XAction {
public:
	XObj* mTarget;        // _2C
	RadarAimAxis mYaw;    // _30
	RadarAimAxis mPitch;  // _3C
	CScrnColor mColor[2]; // _48
	f32 mFade;            // _B0
	f32 mAngle[2];        // _B4
	f32 mTargetDistance;  // _BC, surface distance or NAN when there is no target
};
STATIC_ASSERT(sizeof(XActRadar) == 0xC0);

#endif
