#ifndef _GAME_XOBJSPIDER_H
#define _GAME_XOBJSPIDER_H

#include "game/xobj.h"
#include "game/csound.h"

// one of the coins a spider knocks loose, it hops up and drops to mTarget
struct CoinBounce {
	XObj* mCoin; // _00
	Vec mPos;    // _04
	s32 mPhase;  // _10
	s32 mCount;  // _14
	Vec mTarget; // _18
};
STATIC_ASSERT(sizeof(CoinBounce) == 0x24);

// a carried model, drawn with the object 0x13C model the spider claimed a slot for
struct CarryInfo {
	CVec mPos;      // _00
	CVec _10;       // _10
	f32 mAngle;     // _20
	f32 _24;        // _24
	s32 mState;     // _28, its anim, 1 opens, 2 follows mObj's joint, 3 closes
	u32 _2C;        // _2C
	CStatus mFlags; // _30, 1 drawn, 2 the robot was hit with it
	XObj* mObj;     // _38, the object whose joint it follows
};
STATIC_ASSERT(sizeof(CarryInfo) == 0x3C);

// an object an event tied to one of the spider's joints
struct SpiderTie {
	HSD_JObj* mJoint; // _00
	XObj* mObj;       // _04
	Mtx mMtx;         // _08
};
STATIC_ASSERT(sizeof(SpiderTie) == 0x38);

// the spiders, offsets are usa, pal has 4 more bytes after _9B8 and 4 at the end
class XObjSpider : public XObj {
public:
	typedef void (XObjSpider::*Func)(u32 frames);

	// _00-_980 = XObj
	f32 mDir;         // _980, the way it walks
	f32 mSpeed;       // _984
	f32 mFacing;      // _988, eased toward mDir while it turns
	f32 _98C;         // _98C
	f32 mTurnSpeed;   // _990
	f32 mSavedDir;    // _994
	f32 mThreadAlpha; // _998, 0 to 1
	f32 mRadius;      // _99C, shadow size, also kept off the area edges
	f32 mFallSpeed;   // _9A0
	s32 mState;       // _9A4
	s32 mPhase;       // _9A8
	s32 mCounter;     // _9AC
	CStatus mFlags;   // _9B0
	s32 mPrevState;   // _9B8
#if defined(VERSION_PAL)
	f32 mAnimStartFrame; // _9BC
#endif
	s32 mWait;               // _9BC
	f32 _9C0;                // _9C0
	f32 _9C4;                // _9C4
	Vec mHitPos;             // _9C8, where it was hit from
	Vec mHome;               // _9D4, y is the floor
	f32 mHitStrength;        // _9E0
	s32 mWall;               // _9E4, 2 to 5 while it walks on a wall, on 3 it carries sideways
	s32 mHits;               // _9E8, hits left
	s32 mMaxHits;            // _9EC
	Vec mWallPos;            // _9F0
	Vec mTarget;             // _9FC, where it walks to
	Vec mShotOrigin;         // _A08
	Vec mShotPosition;       // _A14
	u8 _A20[0xA2C - 0xA20];  // _A20
	Vec mShotTo;             // _A2C, where the carried model is shot at
	f32 mShotRange;          // _A38
	u8 _A3C[0xA40 - 0xA3C];  // _A3C
	f32 mShotProgress;       // _A40
	s32 mShotState;          // _A44
	s32 _A48;                // _A48
	Vec mAreaPos;            // _A4C, corner of the area it stays in
	f32 mAreaWidth;          // _A58
	f32 mAreaDepth;          // _A5C
	Vec mStartPos;           // _A60
	f32 _A6C;                // _A6C
	s32 mCommand;            // _A70, from the event scripts, 0 once handled
	s32 mArgs[5];            // _A74
	u32 _A88;                // _A88
	CarryInfo mCarry;        // _A8C
	CarryInfo mSlotCarry;    // _AC8, mObj is the slot's model
	u8 mSlot;                // _B04, 255 for none
	u8 _B05[0xB10 - 0xB05];  // _B05
	s16 _B10;                // _B10
	s32 _B14[4];             // _B14
	s32 _B24[3];             // _B24
	u8 _B30[0xB50 - 0xB30];  // _B30
	s16 _B50;                // _B50
	s32 _B54;                // _B54
	s16 mEffectId;           // _B58, a stage object it holds on its joint
	u16 _B5A;                // _B5A
	XObj* mEffect;           // _B5C
	s32 mEffectOn;           // _B60
	Mtx mEffectMtx;          // _B64
	s32 mHitState;           // _B94
	s32 mHitTimer;           // _B98
	s32 mDroppedSpiderIndex; // _B9C, FindNearbyDroppedSpider's pool index
	s32 mRumble;             // _BA0
	s32 mRumbleTimer;        // _BA4
	AnimSfx mSfx;            // _BA8
	SpiderTie mTies[8];      // _BB8
#if defined(VERSION_PAL)
	u8 _D78_PAL[0x4]; // _D78
#endif

#if defined(VERSION_PAL)
	void SetAnim(s32 id, BOOL force, void* jointAnim, void* matAnim, void* shapeAnim, f32 frame, f32 rate, f32 blend, f32 blendRate);
#endif
	virtual ~XObjSpider();
	virtual void InitializeObject(u32 id);                           // _90
	virtual void vf94();                                             // _94
	virtual void ResetObjectState();                                 // _98
	virtual BOOL LoadFromPlacement(const CoordInfo& info);           // _9C
	virtual void UpdateObject(u32 frames);                           // _A4
	virtual void vfAC();                                             // _AC
	virtual void DrawGroundShadow();                                 // _B8
	virtual void DrawGroundShadowAt(const Vec* origin, f32 opacity); // _BC
	virtual void DrawObjectEffects();                                // _C0
	virtual void vfC8();                                             // _C8
	virtual void DrawDebugStatus();                                  // _CC
	virtual s32 vf100(s32 unused, s32 value);                        // _100
	virtual s32 HandleEventCommand(s32 count, s32* args);            // _124
	virtual void OnHitFromPosition(const Vec& from, f32 strength);   // _138
	virtual void UpdateAnimationSounds();                            // _148, plays the sounds along the anim

	// the states, Update calls them by number
	void State0(u32 frames);
	void State1(u32 frames);
	void State2(u32 frames);
	void State3(u32 frames);
	void State4(u32 frames);
	void State5(u32 frames);
	void State6(u32 frames);
	void State7(u32 frames);
	void State8(u32 frames);
	void State9(u32 frames);

	void SetState(s32 state)
	{
		mPhase     = 0;
		mCounter   = 0;
		mPrevState = mState;
		mState     = state;
	}

	XObjSpider();
};
#if defined(VERSION_JP)
STATIC_ASSERT(sizeof(XObjSpider) == 0xD68);
#elif defined(VERSION_PAL)
STATIC_ASSERT(sizeof(XObjSpider) == 0xD80);
#else
STATIC_ASSERT(sizeof(XObjSpider) == 0xD78);
#endif

extern XObjSpider::Func gSpiderStateHandlers[10];

extern "C" {
u8 HasSpiderFinishedDeathAnimation(const XObjSpider* spider);
// draws a CarryInfo with the claimed slot's model
void DrawSpiderCarryModel(CarryInfo* carry, u8 slot, bool sideways);
}

#endif
