#ifndef _GAME_XACTCLASSES_H
#define _GAME_XACTCLASSES_H

#include "Version.h"
#include "game/xactdata.h"

class CPadOne;
#if defined(VERSION_R24J01)
struct EffectInstance;
#else
struct HSD_Generator;
#endif

// hanging from a ledge
class XActHang : public XAction {
public:
#if defined(VERSION_R24J01)
	XActHang();
	~XActHang();
#else
	void HandleInput(const CPadOne& pad, f32& moveWeight);
#endif

	BOOL mStickAway;      // _2C, set when the stick points away
	Vec mHangStartOffset; // _30, player position relative to the target
	Vec mFloorCenter;     // _3C
	f32 mDistance;        // _48
	u8 _4C[0x50 - 0x4C];  // _4C
};
STATIC_ASSERT(sizeof(XActHang) == 0x50);

class XActJump : public XAction {
public:
#if defined(VERSION_R24J01)
	XActJump();
	~XActJump();
#else
	void HandleInput(const CPadOne& pad, f32& moveWeight);
#endif

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
#if defined(VERSION_R24J01)
	XActLadder();
	~XActLadder();
#else
	void HandleInput(const CPadOne& pad, f32& moveWeight);
#endif

	BOOL mBPressed; // _2C, set when b is pressed on it
	Vec mPos;       // _30, where he hangs, in front of the ladder
	f32 mDist;      // _3C, how far he still has to go to get there
	f32 mAngle;     // _40, from mPos to where he started
};
STATIC_ASSERT(sizeof(XActLadder) == 0x44);

class XActRope : public XAction {
public:
#if defined(VERSION_R24J01)
	XActRope();
	~XActRope();
#else
	void HandleInput(const CPadOne& pad, f32& moveWeight);
#endif

	Vec mPos;   // _2C, horizontal approach point
	f32 mDist;  // _38
	f32 mAngle; // _3C
};
STATIC_ASSERT(sizeof(XActRope) == 0x40);

class XActPush : public XAction {
public:
#if defined(VERSION_R24J01)
	XActPush();
	~XActPush();
#else
	void HandleInput(const CPadOne& pad, f32& moveWeight);
#endif

	u8 _2C[0x34 - 0x2C];       // _2C
	CVec _34;                  // _34, no access in any edition
	CVec _44;                  // _44
	BOOL mPushPositionClamped; // _54
};
STATIC_ASSERT(sizeof(XActPush) == 0x58);

class XActTumble : public XAction {
public:
#if defined(VERSION_R24J01)
	XActTumble();
	~XActTumble();
#else
	void HandleInput(const CPadOne& pad, f32& moveWeight);
#endif

	XActTumbleFade mFade; // _2C
	f32 mSavedYaw;        // _40
#if defined(VERSION_R24J01)
	EffectInstance* mIdleParticle; // _44, the sleep effect while he lies idle
#else
	HSD_Generator* mIdleParticle; // _44
#endif
};
STATIC_ASSERT(sizeof(XActTumble) == 0x48);

class XActCopter : public XAction {
public:
#if defined(VERSION_R24J01)
	XActCopter();
	~XActCopter();
#else
	void HandleInput(const CPadOne& pad, f32& moveWeight);
#endif

	f32 mTime;  // _2C, how long he can stay up; on the Wii a copter parameter count of hundredths of a second in frames
	f32 mPhase; // _30, of the bob, where the propeller bob is in its sine
	f32 mBob;   // _34, how high the bob lifts him, the height the copter joint rides at
};
STATIC_ASSERT(sizeof(XActCopter) == 0x38);

// the radar action. the fields the Wii reaches are the GameCube layout's own,
// WiiPlayerState reserves _95D4 to _96A0 for it
class XActRadar : public XAction {
public:
#if defined(VERSION_R24J01)
	XActRadar();
	~XActRadar();
#else
	void HandleInput(const CPadOne& pad, f32& moveWeight);
#endif

	XObj* mTarget;         // _2C
	RadarAimAxis mYaw;     // _30
	RadarAimAxis mPitch;   // _3C
	RadarBeamColors mBeam; // _48
	f32 mFade;             // _B0
	f32 mAngle[2];         // _B4
	f32 mTargetDistance;   // _BC, surface distance or NAN when there is no target
};
STATIC_ASSERT(sizeof(XActRadar) == 0xC0);

#endif
