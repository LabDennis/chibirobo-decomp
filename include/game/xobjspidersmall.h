#ifndef _GAME_XOBJSPIDERSMALL_H
#define _GAME_XOBJSPIDERSMALL_H

#include "game/xobj.h"
#include "game/csound.h"
#include <stdlib.h>

// the small spiders, npc_spider_2, pool 15. class name from RTTI and the vtable at 0x8022B308,
// the units from 0x800F49F0 to 0x80100264. offsets are usa, everything past XObj sits 0x10 lower
// in japan

// the fx_spider_01 the spider carries around, drawn from gSmallSpiderEffectObjects
struct XObjSpiderSmallFx {
	CVec mPos;       // _00
	CVec _10;        // _10
	f32 mRotY;       // _20
	f32 _24;         // _24
	u32 mState;      // _28, its anim, 1 and 3 play one, 10 puts it away
	s32 _2C;         // _2C
	CStatus mStatus; // _30, 1 shown

	// shows it at pos with its anim from state
	void Show(const CVec& pos, u32 state)
	{
		mStatus.mFlags |= 1;
		mPos   = pos;
		mState = state;
	}
};
STATIC_ASSERT(sizeof(XObjSpiderSmallFx) == 0x38);

class XObjSpiderSmall : public XObj {
public:
	// _00-_980 = XObj
	f32 mWalkHeading;       // _980, the way it walks
	f32 mWalkSpeed;         // _984, walk speed
	f32 _988;               // _988
	f32 mTurnSpeed;         // _98C, turn speed
	u8 _990[0x994 - 0x990]; // _990
	f32 _994;               // _994
	f32 mRadius;            // _998
	f32 mFallSpeed;         // _99C, fall speed
	f32 mScale;             // _9A0, its scale
	f32 mTargetScale;       // _9A4, the scale it grows to
	s32 mState;             // _9A8
	s32 mStep;              // _9AC
	s32 mTimer;             // _9B0
	CStatus mSpiderStatus;  // _9B4, 1 hit, 2 knocked over, 0x40 shadow, 0x800000 grabbed
	s32 mPrevState;         // _9BC
	u32 _9C0;               // _9C0
	f32 _9C4;               // _9C4
	f32 _9C8;               // _9C8
	Vec mHitPos;            // _9CC
	Vec mHome;              // _9D8
	f32 mHitStrength;       // _9E4
	s32 mHitsLeft;          // _9E8, hits left
	s32 _9EC;               // _9EC
	Vec _9F0;               // _9F0
	Vec mWalkTarget;        // _9FC, where it walks to
	Vec mAreaPos;           // _A08, corner of the area it walks in
	f32 mAreaWidth;         // _A14
	f32 mAreaDepth;         // _A18
	Vec mTargetPos;         // _A1C
	f32 mTargetRadius;      // _A28
	s32 mStage;             // _A2C
	s32 mEvtArgs[6];        // _A30, from HandleEventCommand
	u32 _A48;               // _A48
	XObjSpiderSmallFx mFx;  // _A4C
	XObj* mFxObj;           // _A84
	u8 mFxIndex;            // _A88, into gSmallSpiderEffectObjects, 255 for none
	u8 _A89[0xA8C - 0xA89]; // _A89
	f32 mClingAngle;        // _A8C, the angle it sits at around the robot
	f32 _A90;               // _A90
	s32 mDrainTimer;        // _A94, frames to the next battery drain
	f32 mHoldTime;          // _A98, how long it holds on
	Vec _A9C;               // _A9C
	u8 _AA8;                // _AA8
	u8 _AA9[0xAAC - 0xAA9]; // _AA9
	s32 mPrevBgm;           // _AAC, the bgm it took over from
	u8 mPrevBgmVolume;      // _AB0, and its volume
	u8 _AB1[0xAB4 - 0xAB1]; // _AB1
	s32 mLoadPoolIndex;     // _AB4, pool index it loaded with
	s32 mTimeMode;          // _AB8
	s32 mAreaGraceTimer;    // _ABC
	s32 _AC0;               // _AC0
	s32 mBattleBgmState;    // _AC4, the battle bgm state
	AnimSfx mAnimSfx;       // _AC8

	XObjSpiderSmall();
	virtual ~XObjSpiderSmall();

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

	virtual void Release();                                            // _54
	virtual void InitializeObject(u32 id);                             // _90
	virtual void ResetForNewGame();                                    // _94
	virtual void ResetObjectState();                                   // _98
	virtual BOOL LoadFromPlacement(const CoordInfo& info);             // _9C
	virtual void UpdateObject(u32 frames);                             // _A4
	virtual void DrawObject();                                         // _AC
	virtual void DrawGroundShadow();                                   // _B8
	virtual void DrawGroundShadowAt(const Vec* origin, f32 opacity);   // _BC
	virtual void DrawObjectEffects();                                  // _C0
	virtual void DrawScreenOverlay();                                  // _C8
	virtual void DrawDebugStatus();                                    // _CC
	virtual s32 OnPlayerContact(s32 unused, s32 value);                // _100
	virtual s32 HandleEventCommand(s32 count, s32* args);              // _124
	virtual void OnHitFromPosition(const Vec& position, f32 strength); // _138
	virtual void UpdateAnimationSounds();                              // _148, anim sounds

	void Hide() { mSpiderStatus.mFlags |= 0x80000; }
	u32 IsDead() const { return (mSpiderStatus.mFlags >> 19) & 1; }

	// pos is inside the spider's area, mRadius in from its sides
	bool InArea(const Vec& pos)
	{
		f32 x     = pos.x;
		f32 z     = pos.z;
		f32 left  = mAreaPos.x + mRadius;
		f32 right = mAreaPos.x + mAreaWidth - mRadius;
		f32 near  = mAreaPos.z + mRadius;
		f32 far   = mAreaPos.z + mAreaDepth - mRadius;
		if (left > x || right < x || near > z || far < z) {
			return false;
		}
		return true;
	}

	// walks to a random spot around home
	void Wander(s32 step)
	{
		mWalkTarget.x = 12.0f + mHome.x - rand() % 2400 / 100.0f;
		mWalkTarget.y = mHome.y;
		mWalkTarget.z = 12.0f + mHome.z - rand() % 2400 / 100.0f;
		mStep         = step;
		mWalkSpeed    = 0.3f;
		SetAnim(2, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
	}

	// keeps the walk speed between the slowest and the fastest
	void ClampSpeed()
	{
		if (mWalkSpeed < 0.08f) {
			mWalkSpeed = 0.08f;
		} else if (mWalkSpeed > 1.1f) {
			mWalkSpeed = 1.1f;
		}
	}

	void SetState(s32 state)
	{
		mStep      = 0;
		mTimer     = 0;
		mPrevState = mState;
		mState     = state;
	}
};
#if defined(VERSION_JP)
STATIC_ASSERT(sizeof(XObjSpiderSmall) == 0xAC8);
#else
STATIC_ASSERT(sizeof(XObjSpiderSmall) == 0xAD8);
#endif

struct SmallSpiderStageStatus {
	CStatus mSpiders[16];
};
STATIC_ASSERT(sizeof(SmallSpiderStageStatus) == 0x80);

extern "C" {
void StartSmallSpiderWebEffect(XObjSpiderSmallFx* fx, const CVec& pos);
u8 HasSmallSpiderEmerged(XObjSpiderSmall* spider);
u8 IsSmallSpiderGrabbingPlayer(XObjSpiderSmall* spider);
void ApplySmallSpiderPlacement(XObjSpiderSmall* spider);
void ResetSmallSpiderState(XObjSpiderSmall* spider);

// gSmallSpiderStatusByStage's construct and destroy helpers
void DestroySmallSpiderStatusTable();
CStatus* __dt__22SmallSpiderStageStatusFv(CStatus* row, s32 flags);
CStatus* DestroySmallSpiderStatusEntry(CStatus* status, s32 flags);
CStatus* __ct__22SmallSpiderStageStatusFv(CStatus* row);
CStatus* __ct__7CStatusFv(CStatus* status);

extern void (XObjSpiderSmall::*gSmallSpiderStateHandlers[10])(u32);

// the spider file's bss
extern XObj* gSmallSpiderEffectObjects[12];                  // the stage's fx_spider_01 objects
extern s32 gSmallSpiderEffectSlotOwners[12];                 // which spider has each, -1 for none
extern u8 gSmallSpiderStatusDestructorChain[12];             // gSmallSpiderStatusByStage's DestructorChain
extern SmallSpiderStageStatus gSmallSpiderStatusByStage[39]; // a row per stage, a spider per pool index
extern AnimSfxEvent gSmallSpiderAnimSfxEvents[];             // the anim sound events
}

#endif
