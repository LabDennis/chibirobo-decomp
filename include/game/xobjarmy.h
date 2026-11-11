#ifndef _GAME_XOBJARMY_H
#define _GAME_XOBJARMY_H

#include "game/xobj.h"
#include "game/csound.h"

// one of the army's shots, it flies until mLife runs out or it hits chibi robo
struct XObjArmyShot {
	s32 mActive; // _00
	u32 mLife;   // _04
	CVec mPos;   // _08
	CVec mVel;   // _18
	f32 mSpeed;  // _28
	s32 mPhase;  // _2C, 0 flying, 1 bouncing off, 2 lying on the floor

	void Move()
	{
		mPos.x += mSpeed * mVel.x;
		mPos.y += mSpeed * mVel.y;
		mPos.z += mSpeed * mVel.z;
	}
};
STATIC_ASSERT(sizeof(XObjArmyShot) == 0x30);

// the toy soldiers, they march about or stand guard and shoot at chibi robo
class XObjArmy : public XObj {
public:
	typedef void (XObjArmy::*Func)();

	// _00-_980 = XObj
	Func mFunc;             // _980
	s32 mStep;              // _98C, the step of mFunc
	s32 mTimer;             // _990
	f32 _994;               // _994
	u8 _998[0x9A0 - 0x998]; // _998
	CStatus mFlags;         // _9A0, 1 ignores hits, 2 lies down, 0x80 shoots quietly, 0x100 plays its sounds at full volume
	CVec mGuardPos;         // _9A8, where it aims while on guard
	s32 mMode;              // _9B8, what it goes back to after a wait, 1 marches, 2 stands guard
	XObjArmyShot mShots[5]; // _9BC
	AnimSfx mSfx;           // _AAC
	u8 _ABC[0xAC0 - 0xABC]; // _ABC

	XObjArmy();

	virtual void ResetObjectState();                                 // _98
	virtual void UpdateObject(u32 frames);                           // _A4
	virtual void DrawObjectEffects();                                // _C0
	virtual s32 HandleEventCommand(s32 count, s32* args);            // _124
	virtual void OnHitFromPosition(const Vec& position, f32 amount); // _138
	virtual void UpdateAnimationSounds();                            // _148

	virtual ~XObjArmy();

	void StateGuard();
	void StateFall();
	void StateKnock();
	void StateWait();
	void StateMarch();
};
#if defined(VERSION_JP)
STATIC_ASSERT(sizeof(XObjArmy) == 0xAB0);
#else
STATIC_ASSERT(sizeof(XObjArmy) == 0xAC0);
#endif

#endif
