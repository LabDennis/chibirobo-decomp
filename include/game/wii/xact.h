#ifndef GAME_WII_XACT_H
#define GAME_WII_XACT_H

#include "game/wii/cgame.h"

// chibi robo's actions on the Wii player, the GameCube's XAct* kinds (game/xact.h). XAction, XActHang and
// XActTumble live in game/wii/cgame.h with the player; WiiPlayerState still declares the others as XAction
// with padding behind them, these are their full shapes for the action code (objb_act.cpp)

// picking things up and carrying them; the Wii's is 0x34 bytes longer than the GameCube's
class XActLift : public XAction {
public:
	f32 mTimer;            // _2C, frames left in a timed lift state
	CVec mLiftStartOffset; // _30
	f32 mYawOffset;        // _40, held object yaw relative to the player
	u8 _44[4];             // _44
	s16 mPendingItemId;    // _48, -1 when there's none
	u8 _4A[2];             // _4A
	XObj* mPendingItem;    // _4C
	XObj* mPlugTarget;     // _50, WiiPlayerState's mLiftPlugTarget
	s32 mPlugConnection;   // _54, the GameCube's _54, copied to the plug's connection word with 0x10000 set
	s32 _58;               // _58, cleared on reset
	f32 mBatteryCost;      // _5C
	u32 mContactEffectId;  // _60
	BOOL mContactEffect;   // _64, the contact effect plays; the Wii stops it by id
	BOOL mCheckContact;    // _68
	u32 mUseEffectId;      // _6C
	BOOL mUseEffect;       // _70
	u32 mToolEffectId;     // _74
	BOOL mToolEffect;      // _78
	BOOL mCheckToolHit;    // _7C
	f32 _80;               // _80, cleared on reset
	f32 _84;               // _84, cleared on reset; the plug pull is done at a quarter turn (pi/2)
	f32 _88;               // _88, cleared on reset
	u8 _8C[0xAC - 0x8C];   // _8C
	u8 _AC;                // _AC, cleared on reset
	u8 _AD[0xB0 - 0xAD];
};
STATIC_ASSERT(sizeof(XActLift) == 0xB0);

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
	BOOL mBPressed; // _2C
	Vec mPos;       // _30, where he hangs, in front of the ladder
	f32 mDist;      // _3C
	f32 mAngle;     // _40
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

#endif
