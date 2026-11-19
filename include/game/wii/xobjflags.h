#ifndef GAME_WII_XOBJFLAGS_H
#define GAME_WII_XOBJFLAGS_H

#include "game/wii/animsfx.h"
#include "game/wii/xobj.h"
#include "game/stagedata.h"

// the Wii XObjFlags family: flag-backed pickups and the sound objects. Pulled out of
// xobj.h because AnimSfx needs the nw4r sound headers, which only the units that use
// these classes can afford to include (rvl_msl include path).

// the sound objects chibi robo's world places; each forwards to its embedded AnimSfx,
// the GameCube played its object sounds without one
class XObjSound : public XObj {
public:
	virtual void InitializeObject();       // _14C
	virtual void ResetObjectState();       // _154
	virtual void UpdateObject(u32 frames); // _164
	virtual void PauseSounds(f32 fade);    // _20C
	virtual void ResumeSounds(f32 fade);   // _210
	virtual void SoundUpdate(f32 frame);   // _28C

	AnimSfx mAnimSfx; // _9A8
	s32 _A04;         // _A04, cleared on reset; what they hold is not decoded yet
	s32 _A08;         // _A08
	u8 _A0C[4];
};
STATIC_ASSERT(sizeof(XObjSound) == 0xA10);

// objects that remember being collected in a CEvtFlags, a bit per placement id; the GameCube's
// XObjFlags has the pointer at _980, the Wii moved it behind the object
class XObjFlags : public XObj {
public:
	virtual void ResetForNewGame(); // _150, clears every flag
	virtual s32 GetFlagIndex();     // _28C, the bit in mFlags, 0 and below have none

	CEvtFlags* mFlags; // _9A8

	// the placement's collection flag index within the group, 0 and below have none
	s32 GetPlacementFlagIndex(s32 group) const
	{
		s32 id = mPlacementParam1;
		if (id > 0) {
			u32 base = GetStageCollectionFlagBase(group);
			return (s32)(base + (u32)id);
		}
		return 0;
	}

	// the collected bit, FALSE without one
	u32 IsFlagSet()
	{
		if (GetFlagIndex() > 0) {
			u32 index = GetFlagIndex();
			return mFlags->mFlags[index >> 5] & (1u << (index & 31));
		}
		return FALSE;
	}
	void SetFlag()
	{
		u32 index = GetFlagIndex();
		mFlags->mFlags[index / 32] |= 1 << (index & 31);
	}
};

STATIC_ASSERT(sizeof(XObjFlags) == 0x9B0); // the XObj alignment pads the tail
STATIC_ASSERT(offsetof(XObjFlags, mFlags) == 0x9A8);

// the money, scrap, battery and experience pickups; two sound actors of their own
class XObjCoin : public XObjFlags {
public:
	virtual void InitializeObject();                                                 // _14C
	virtual void ResetObjectState();                                                 // _154
	virtual BOOL LoadFromPlacement(const CoordInfo& info);                           // _158
	virtual void Release();                                                          // _40
	virtual void UpdateObject(u32 frames);                                           // _164
	virtual s32 OnPlayerContact(s32 unused, s32 value);                              // _1CC
	virtual s32 HandleEventCommand(s32 count, s32* args);                            // _1E8
	virtual void vf208(const Vec* pointer, const Vec* motion, BOOL over, f32 speed); // _208
	virtual s32 GetFlagIndex();                                                      // _28C

	// the item index its data record names, -1 without one (the GameCube XObj's inline)
	s32 GetItem() const { return mAttr.mObjData != NULL ? mAttr.mObjData->GetItem() : -1; }

	f32 mSpin;               // _9AC, current spin angle, NAN-free sign picks the turn direction
	f32 mPhase;              // _9B0, spin step, counts down to the kind's floor after the coin settles
	f32 mFade;               // _9B4, 1 when picked up, falls 0.02 a frame; the scale is multiplied by 1.12 times it
	AnimSfxActor mActors[2]; // _9B8
	s32 mHandleSlot;         // _9D0, the coin's slot in the game's handle table, -1 for none
	s32 mState;              // _9D4, 1 while falling
	s32 mParam0;             // _9D8, the event command's args
	s32 mParam1;             // _9DC
	s32 mFallFrames;         // _9E0
	u32 mParams[8];          // _9E4, the rest of the event command's args, raw words
	u8 _A04[0x20];           // _A04
	f32 mHeading;            // _A24
	f32 mFallSpeed;          // _A28
	u8 mBouncing;            // _A2C, the pointer bounce sets it
	u8 _A2D[3];
};
STATIC_ASSERT(sizeof(XObjCoin) == 0xA30);

// the item pickups; the Wii keeps one word of its own
class XObjItem : public XObjFlags {
public:
	virtual void ResetObjectState();                       // _154
	virtual BOOL LoadFromPlacement(const CoordInfo& info); // _158
	virtual void UpdateObject(u32 frames);                 // _164
	virtual s32 GetFlagIndex();                            // _28C

	void SetUseAnim(); // picks the kind's in-use animation and stashes the one to return to

	s32 mAnimId2; // _9AC, the anim id to go back to once the use anim ends, -1 when it keeps its own
};
STATIC_ASSERT(sizeof(XObjItem) == 0x9B0);

// the dust piles, swept up by the vacuum or blown off by chibi robo; the GameCube class
// (xobjflags.h) behind the Wii XObjFlags, its fields line up the same from _9B0 on
class XObjDust : public XObjFlags {
public:
	XObjDust();
	virtual ~XObjDust();

	virtual void Release();                     // _40
	virtual void SetAnim(s32 id, BOOL force, f32 frame, f32 rate, f32 blend,
	                     f32 blendRate);        // _108, forwards to XObj's
	virtual void ResetObjectState();            // _154
	virtual BOOL LoadFromPlacement(const CoordInfo& info); // _158
	virtual void UpdateObject(u32 frames);                 // _164
	virtual void DrawWithView(Mtx view);                   // _178
	virtual void DrawObjectEffects();                      // _188
	virtual s32 OnPlayerContact(s32 unused, s32 value);    // _1CC
	virtual void OnHitFromPosition(const Vec& position, f32 strength); // _1FC
	virtual BOOL OnItemUsed(const Vec* position, s32 value);           // _204
	virtual s32 GetFlagIndex();                                        // _28C

	void* mMatAnim;    // _9AC, the GameCube's HSD mat-anim slot; the Wii only ever clears it
	s32 mAnimId2;      // _9B0, the anim it was placed with
	s32 mAnimCount;    // _9B4
	f32 mBaseY;        // _9B8
	f32 mPhase;        // _9BC, bobbing
	f32 mPhaseStep;    // _9C0
	f32 mBouncePhase;  // _9C4
	f32 mBounceHeight; // _9C8
	f32 mHeading;      // _9CC, NAN until something pushes it
	f32 mSpeed;        // _9D0
	f32 mFade;         // _9D4
	Vec mStartScale;   // _9D8
	Vec mFrom;         // _9E4
	Vec mTo;           // _9F0
	s16 mEventId;      // _9FC
	s16 mEventArg;     // _9FE
};
STATIC_ASSERT(sizeof(XObjDust) == 0xA00);

extern "C" {
void UpdateFallingCoin(XObjCoin* coin, u32 frames);
}

#endif
