#ifndef _GAME_XACT_H
#define _GAME_XACT_H

#include "game/xobj.h"
#include "game/cptcl.h"
#include "game/cscrn.h"
#include "game/xaction.h"
#include "game/xactdata.h"

// chibi robo's actions, XAction and its XAct* kinds from RTTI. every one of them sits in
// XObjChr and shares XAction's two virtuals. offsets are the same in every region

class XObjChr;
struct HSD_Generator;

#include "game/xactionbase.h"

// picking things up and carrying them
class XActLift : public XAction {
public:
	f32 mTimer;            // _2C, frames left in a timed lift state: pickup hold (state 2), plug delay (state 12)
	CVec mLiftStartOffset; // _30, held object's position relative to the carry joint when the lift starts; blended away while he lifts it
	f32 mYawOffset;        // _40, held object yaw relative to the player
	u8 _44[4];             // _44, no access in any edition
	s16 mPendingItemId;    // _48, -1 when there's none
	u8 _4A[2];
	XObj* mPendingItem;            // _4C
	XObj* mPlugTarget;             // _50
	s32 _54;                       // _54, copied to XObjPlug's _D3C
	f32 mBatteryCost;              // _58
	s32 mContactEffectId;          // _5C
	HSD_Generator* mContactEffect; // _60
	BOOL mCheckContact;            // _64
	s32 mUseEffectId;              // _68
	HSD_Generator* mUseEffect;     // _6C
	s32 mToolEffectId;             // _70
	HSD_Generator* mToolEffect;    // _74
	BOOL mCheckToolHit;            // _78

	void HandleInput(const CPadOne& pad, f32& moveWeight);
};
STATIC_ASSERT(sizeof(XActLift) == 0x7C);

extern "C" void UpdateLiftActionAnimation(XActLift* act, s32 anim, HSD_AObj* aobj, u32 frames, f32 frame, f32 end);
extern "C" void UpdateHeldObject(XActLift* act, s32 frames);

#include "game/xactclasses.h"

extern "C" void CancelPlayerPushAction(XActPush* act);

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
	f32 _28;                   // _28, StartPopperShot writes its 'power' argument (3 for a charged shot, 1 otherwise), never read
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
	s32 mState;                // _00
	f32 mTimer;                // _04
	Vec mPos;                  // _08
	HSD_Generator* mGen;       // _14
	HSD_Generator* mChargeGen; // _18
	HSD_JObj* mJoint;          // _1C
};
STATIC_ASSERT(sizeof(XActPopperCharge) == 0x20);

// the chibi-blaster
// the popper's ten shots. retail's destructor tests this object's address before it destroys them, so they sit in
// an object of their own
struct PopperShots {
	XActPopperShot mShot[10]; // _00
};
STATIC_ASSERT(sizeof(PopperShots) == 0x8728);

class XActPopper : public XAction {
public:
	f32 mShotStrength; // _2C, strength of the shot fired this frame (action parameter 30, 31 when charged, /100), 0 when none; also drained
	                   // from the battery while XObjChr _158A4 is set
	PopperShots mShots;       // _30
	XActPopperCharge mCharge; // _8758

	void HandleInput(const CPadOne& pad, f32& moveWeight);
};
STATIC_ASSERT(sizeof(XActPopper) == 0x8778);

#endif
