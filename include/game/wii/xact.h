#ifndef GAME_WII_XACT_H
#define GAME_WII_XACT_H

#include "game/wii/cgame.h"
#include "game/wii/animsfx.h"
#include "game/cptcl.h"
#include "Dolphin/mtx.h"
#include "game/wii/ceffect.h"
#include "game/xactdata.h"

// chibi robo's actions on the Wii player, the GameCube's XAct* kinds (game/xact.h). XAction, XActHang and
// XActTumble live in game/wii/cgame.h with the player; WiiPlayerState still declares the others as XAction
// with padding behind them, these are their full shapes for the action code (objb_act.cpp)

// picking things up and carrying them; the Wii's is 0x34 bytes longer than the GameCube's
class XActLift : public XAction {
public:
	XActLift();
	~XActLift();

	f32 mTimer;                     // _2C, frames left in a timed lift state
	CVec mLiftStartOffset;          // _30
	f32 mYawOffset;                 // _40, held object yaw relative to the player
	u8 _44[4];                      // _44
	s16 mPendingItemId;             // _48, -1 when there's none
	u8 _4A[2];                      // _4A
	XObj* mPendingItem;             // _4C
	XObj* mPlugTarget;              // _50, WiiPlayerState's mLiftPlugTarget
	s32 mPlugConnection;            // _54, the GameCube field of the same name, copied to the plug's connection word with 0x10000 set
	s32 _58;                        // _58, cleared on reset
	f32 mBatteryCost;               // _5C
	u32 mContactEffectId;           // _60
	EffectInstance* mContactEffect; // _64, the contact effect plays; the Wii stops it by id
	BOOL mCheckContact;             // _68
	u32 mUseEffectId;               // _6C
	EffectInstance* mUseEffect;     // _70
	u32 mToolEffectId;              // _74
	EffectInstance* mToolEffect;    // _78
	BOOL mCheckToolHit;             // _7C
	f32 _80;                        // _80, cleared on reset
	f32 _84;                        // _84, cleared on reset; the plug pull is done at a quarter turn (pi/2)
	f32 _88;                        // _88, cleared on reset
	u8 _8C[0xAC - 0x8C];            // _8C
	u8 _AC;                         // _AC, cleared on reset
	u8 _AD[0xB0 - 0xAD];
};
STATIC_ASSERT(sizeof(XActLift) == 0xB0);

// the shot's dust pool: like XObjChr::CPtclSet it wraps the pool and its array in a
// class of its own whose vtable gains a first virtual at _08 (retail RTTI names the
// kind XBullet::CPtclSet); its vptr sits at _C94, the word behind the last particle
struct PopperShotDust {
	CPtcl mPtcl;           // _00
	CPtclDust2 mPtcls[40]; // _14
	// _C94 vtable
	virtual void vf08(CGs* gs); // _08
};
STATIC_ASSERT(sizeof(PopperShotDust) == 0xC98);

// one popper shot, it keeps its last ten positions for the trail. The Wii shape is the
// GameCube's (game/xact.h) with the two generators as event-effect instances and two
// sound actors appended; the word at _D78 is the dust pool's vtable pointer
struct XActPopperShot {
	BOOL mActive;               // _00
	CVec mPos;                  // _04
	f32 mYaw;                   // _14
	f32 mPitch;                 // _18
	f32 mSpeed;                 // _1C
	f32 mDistance;              // _20
	f32 mRange;                 // _24
	f32 _28;                    // _28, StartPopperShot writes its 'power' argument (3 charged, 1 else), never read
	f32 mStrength;              // _2C
	s32 mKind;                  // _30, 1 for the small shot
	XObj* mHomingTarget;        // _34
	XObj* mHitTarget;           // _38, deferred hit during an event
	Vec mHistory[10];           // _3C
	XActPopperRing mRing[2];    // _B4
	PopperShotDust mDust;       // _E4
	EffectInstance* mGen;       // _D7C, the flight effect
	EffectInstance* mChargeGen; // _D80, the charge effect
	AnimSfxActor mSfxActors[2]; // _D84, launch and impact sounds
};
STATIC_ASSERT(sizeof(XActPopperShot) == 0xD9C);

// the charge's glow at the popper's muzzle, the GameCube's XActPopperCharge (game/xact.h)
// grown on the Wii: the joint is an external matrix pointer now and two sound actors
// (wind-up and charged loop) sit behind the generators. The word at _38 is only ever
// cleared by the action's init
struct XActPopperCharge {
	s32 mState;                 // _00
	f32 mTimer;                 // _04, 1.0 is a full charge
	Vec mPos;                   // _08, where the generators are anchored
	MtxP mMtx;                  // _14, the muzzle joint's world matrix while charging, NULL otherwise
	EffectInstance* mGen;       // _18, the wind-up effect
	EffectInstance* mChargeGen; // _1C, the charged glow, made when mTimer reaches 1.0
	AnimSfxActor mSfxActors[2]; // _20, the charge sounds
	u32 _38;                    // _38
};
STATIC_ASSERT(sizeof(XActPopperCharge) == 0x3C);

// the popper's ten shots, an object of their own like the GameCube's PopperShots
struct PopperShots {
	XActPopperShot mShot[10]; // _00
};
STATIC_ASSERT(sizeof(PopperShots) == 0x8818);

// the chibi-blaster, the GameCube's XActPopper: it fills WiiPlayerState's mPopper slot
// at _0D50 out to mRadar at _95D4
class XActPopper : public XAction {
public:
	XActPopper();
	~XActPopper();

	f32 mShotStrength;        // _2C
	PopperShots mShots;       // _30
	XActPopperCharge mCharge; // _8848
};
STATIC_ASSERT(sizeof(XActPopper) == 0x8884);

#endif
