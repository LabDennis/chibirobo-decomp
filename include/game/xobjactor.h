#ifndef _GAME_XOBJACTOR_H
#define _GAME_XOBJACTOR_H

#include "game/xobjjointpart.h"
#include "game/ctpl.h"
#include "game/csound.h"
#include "game/cptcl.h"
#include "game/cgame.h"
#include "game/evt.h"
#include "game/stagedata.h"
#include "game/xobjs.h"

// the people and toys, class names from RTTI and the vtables at 0x8022BF90 (XObjActor) and on.
// offsets are usa, everything past XObj sits 0x10 lower in japan

// the base of the people and toys
class XObjActor : public XObj {
public:
	// _00-_980 = XObj
	f32 mHeading;   // _980, the way it walks; UpdateGroundMovement steps mMoveSpeed along sin/cos of it
	f32 mMoveSpeed; // _984, move speed
	f32 mFacing; // _988, the yaw the model is turned to, UpdateGroundMovement applies it with SetRotateY; toy 0x26D reuses it as its joint
	             // spin angle
	f32 _98C;    // _98C, set to PI in ResetForNewGame and ResetObjectState, never read
	s32 mKindStep;        // _990, the kind's own step
	u32 mKindCounter;     // _994, the kind's counter, cleared with mKindStep: a hopping toy counts ticks in the air, Mama counts it while
	                      // holding prop 0x196, toys blink joints on bit 1
	CStatus mActorStatus; // _998, 2 a sound is due, 0x2000 busy
	u32 _9A0;             // _9A0, set to 0 in ResetForNewGame and ResetObjectState, never read
	s32 _9A4; // _9A4, set to 5 * frames when a toy's hit is processed (30 * frames for kinds 0x110, 0x26C, 0x485), 0 on reset, never
	          // counted down; toys 0x26C/0x485 start their joint offset while it is above 0
	f32 _9A8; // _9A8, set to 1.0 on reset and never changed; toys 0x159 and 0x1FB scale two joints by it, 0x26C/0x485 lower a joint by it
	          // and stop when it is 1
	f32 _9AC; // _9AC, set per kind when a toy's hit is processed (20, 3, 2, 3, 2.5, 4), 1.0 on reset, never read
	Vec mHitPos;       // _9B0
	Vec mHomePos;      // _9BC, where a knocked toy stood before it hopped away; it walks back here and is snapped onto it
	Vec mVelocity;     // _9C8, velocity
	f32 mHitStrength;  // _9D4
	f32 mFallSpeed;    // _9D8, vertical speed added to y each tick: 6.8 or 4.8 for a toy's hop, falls by 0.36 per tick down to -2.3, 0 on
	                   // landing
	f32 mGroundHeight; // _9DC, floor height under the actor plus the kind's offset (6 for kind 15); a fall stops there
	s32 mCmd[20];      // _9E0, the event command HandleEventCommand got and its args, the rest is the kind's scratch
	u8 _A30[4];
	s32 mMovePhase;  // _A34, index into the movement speed cycle
	f32 _A38;        // _A38, set to 0 in ResetForNewGame and ResetObjectState, never read
	f32 _A3C;        // _A3C, set to 0 in ResetForNewGame and ResetObjectState, never read
	Vec _A40;        // _A40, set from the fourth Vec of the reset/init table in ResetObjectState and ResetForNewGame, never read
	s16 mAttachedId; // _A4C, id of the attached object, -1 for none
	u8 _A4E[0xA50 - 0xA4E];
	XObj* mAttached;         // _A50, the attached object
	Mtx mAttachedMtx;        // _A54, where the attached object is held
	XObj* mHeldObjects[2];   // _A84, Mama can carry a pair
	AnimSfx mAnimSfx;        // _A8C, its events are the kind's table
	s32 mLookMode;           // _A9C, 0 ahead, 1 at mLookTarget, 3 at mLookAngle
	f32 mHeadYaw;            // _AA0
	f32 mLookAngle;          // _AA4
	f32 mLookRate;           // _AA8
	const Vec* mLookTarget;  // _AAC
	f32 mHeadPitch;          // _AB0
	XObjJointPart mParts[8]; // _AB4
	u8 _C74[0xC78 - 0xC74];

	XObjActor();

	virtual ~XObjActor() { }
	virtual void InitializeObject(u32 id);
	virtual void ResetForNewGame();
	virtual void ResetObjectState();
	virtual BOOL LoadFromPlacement(const CoordInfo& info);
	virtual void UpdateObject(u32 frames);
	virtual void DrawObject();
	virtual void DrawScreenOverlay();
	virtual void DrawDebugStatus();
	virtual Vec GetCameraTarget();
	virtual s32 OnPlayerContact(s32 unused, s32 value);
	virtual s32 HandleEventCommand(s32 count, s32* args);
	virtual void OnPushDisplacement(const Vec& delta);
	virtual void OnHitFromPosition(const Vec& position, f32 strength);
	virtual void UpdateAfterAnim(u32 frames);          // _148, after the animation step: head look, held props, attached parts
	virtual void RunStateHandler(u32 frames) = 0;      // _14C, runs the subclass's mState
	virtual void ClearStateHandler()         = 0;      // _150, clears mState
	virtual void UpdateGroundMovement(u32 frames);     // _154, walks along the heading and follows the floor
	virtual void ProcessPendingEvents(u32 frames) = 0; // _158, works off the queued flags
	virtual void ResetActorState();                    // _15C, resets the actor
	virtual bool IsHitPending();                       // _160, a hit waits for the kind's ProcessPendingEvents
	virtual void UpdateAnimationSounds(f32 frame);     // _164
	virtual HSD_JObj* GetHeadJoint();                  // _168, the head joint
	virtual CVec GetHeadWorldPosition();               // _16C, the head joint's position
	virtual f32 GetMaxHeadYaw();                       // _170, turn limits
	virtual f32 GetMinHeadYaw();                       // _174
	virtual f32 GetMaxHeadPitch();                     // _178
	virtual f32 GetMinHeadPitch();                     // _17C
};
#if defined(VERSION_JP)
STATIC_ASSERT(sizeof(XObjActor) == 0xC68);
#else
STATIC_ASSERT(sizeof(XObjActor) == 0xC78);
#endif

// an object a toy reads in by id and holds on to
struct XObjActorToyPiece {
	s16 mId;    // _00, -1 for none
	s32 mState; // _04
	XObj* mObj; // _08
	Mtx mMount; // _0C, world matrix of the attachment joint
};
STATIC_ASSERT(sizeof(XObjActorToyPiece) == 0x3C);

// toys made of two more objects, read after the toy itself
class XObjActorToy : public XObjActor {
public:
	typedef void (XObjActorToy::*StateFunc)(u32 frames);

	// _00-_C78 = XObjActor
	StateFunc mState;             // _C78, run every frame by RunStateHandler
	CStatus mHideKinds;           // _C84, kinds queued to hide
	s32 mLoad;                    // _C8C, which part LoadFromPlacement is reading
	XObj mFirst;                  // _C90
	XObj mSecond;                 // _1610, _1600 in japan
	s32 mShownPart;               // _1F90, _1F70 in japan
	CVec mPendingMove;            // _1F94, movement kind 0xF adds to mMoveTranslate x/z (times 0.1 per frame) and then clears; cleared in
	                              // ResetActorState, no code sets it non-zero
	s32 _1FA4;                    // _1FA4, set to 0 in ResetActorState, never read
	XObjActorToyPiece mPieces[4]; // _1FA8, objects read in by id
	CStatus mKindFlags;           // _2098, _2068 in japan

	XObjActorToy();

	virtual ~XObjActorToy();                               // _08
	virtual void Release();                                // _54
	virtual void InitializeObject(u32 id);                 // _90
	virtual void ResetForNewGame();                        // _94
	virtual void ResetObjectState();                       // _98
	virtual BOOL LoadFromPlacement(const CoordInfo& info); // _9C
	virtual void DrawObject();                             // _AC
	virtual void DrawScreenOverlay();                      // _C8
	virtual s32 HandleEventCommand(s32 count, s32* args);  // _124
	virtual void OnPushDisplacement(const Vec& delta);     // _130
	virtual void UpdateAfterAnim(u32 frames);              // _148
	virtual void RunStateHandler(u32 frames);              // _14C
	virtual void ClearStateHandler();                      // _150
	virtual void ProcessPendingEvents(u32 frames);         // _158
	virtual void ResetActorState();                        // _15C
	virtual void UpdateAnimationSounds(f32 frame);         // _164

	bool IsFree() const;
	void StateCarry(u32 frames);
	void StateHitRecovery(u32 frames);
	void StatePickUp(u32 frames);
};
#if defined(VERSION_JP)
STATIC_ASSERT(sizeof(XObjActorToy) == 0x2070);
#else
STATIC_ASSERT(sizeof(XObjActorToy) == 0x20A0);
#endif

// jenny
class XObjActorJenny : public XObjActor {
public:
	typedef void (XObjActorJenny::*StateFunc)(u32 frames);

	// _00-_C78 = XObjActor
	StateFunc mState;    // _C78
	f32 _C84;            // _C84, set from _988 (zeroed just before) in ResetActorState, never read
	f32 _C88;            // _C88, zeroed in ResetActorState, never read
	u32 _C8C;            // _C8C, zeroed in ResetActorState, never read
	CStatus mJointFlags; // _C90, 4 head joint, 8 joint shown
	CStatus mHideKinds;  // _C98, kinds queued to hide
	u8 mJointHidden;     // _CA0, hides joint 0-3-0-0-4 below the head while drawn; command 8 arg 0 sets it, arg 1 clears it
	u8 _CA1[0xCA8 - 0xCA1];

	XObjActorJenny();

	virtual ~XObjActorJenny();                            // _08
	virtual void UpdateAnim(u32 frames);                  // _88
	virtual void UpdateObject(u32 frames);                // _A4
	virtual s32 HandleEventCommand(s32 count, s32* args); // _124
	virtual void UpdateAfterAnim(u32 frames);             // _148
	virtual void RunStateHandler(u32 frames);             // _14C
	virtual void ClearStateHandler();                     // _150
	virtual void ProcessPendingEvents(u32 frames);        // _158
	virtual void ResetActorState();                       // _15C
	virtual HSD_JObj* GetHeadJoint();                     // _168
	virtual CVec GetHeadWorldPosition();                  // _16C
	virtual f32 GetMaxHeadPitch();                        // _178
	virtual f32 GetMinHeadPitch();                        // _17C

	void StateNone(u32 frames);
	void Drop();
};
#if defined(VERSION_JP)
STATIC_ASSERT(sizeof(XObjActorJenny) == 0xC98);
#else
STATIC_ASSERT(sizeof(XObjActorJenny) == 0xCA8);
#endif

// mama
class XObjActorMama : public XObjActor {
public:
	typedef void (XObjActorMama::*StateFunc)(u32 frames);

	// _00-_C78 = XObjActor
	StateFunc mState;   // _C78
	f32 _C84;           // _C84, set from _988 (zeroed just before) in ResetActorState, never read
	u32 _C88;           // _C88, zeroed in ResetActorState, never read
	CStatus mHideKinds; // _C8C, kinds queued to hide
	u8 _C94[0xC98 - 0xC94];

	XObjActorMama();

	virtual ~XObjActorMama();                             // _08
	virtual void UpdateObject(u32 frames);                // _A4
	virtual s32 HandleEventCommand(s32 count, s32* args); // _124
	virtual void UpdateAfterAnim(u32 frames);             // _148
	virtual void RunStateHandler(u32 frames);             // _14C
	virtual void ClearStateHandler();                     // _150
	virtual void ProcessPendingEvents(u32 frames);        // _158
	virtual void ResetActorState();                       // _15C
	virtual HSD_JObj* GetHeadJoint();                     // _168
	virtual CVec GetHeadWorldPosition();                  // _16C
	virtual f32 GetMaxHeadYaw();                          // _170
	virtual f32 GetMinHeadYaw();                          // _174
	virtual f32 GetMaxHeadPitch();                        // _178
	virtual f32 GetMinHeadPitch();                        // _17C

	void StateNone(u32 frames);
	void Drop();
};
#if defined(VERSION_JP)
STATIC_ASSERT(sizeof(XObjActorMama) == 0xC88);
#else
STATIC_ASSERT(sizeof(XObjActorMama) == 0xC98);
#endif

// papa
class XObjActorPapa : public XObjActor {
public:
	typedef void (XObjActorPapa::*StateFunc)(u32 frames);

	// _00-_C78 = XObjActor
	StateFunc mState;   // _C78
	f32 _C84;           // _C84, set from _988 (zeroed just before) in ResetActorState, never read
	u32 _C88;           // _C88, zeroed in ResetActorState, never read
	CStatus mHideKinds; // _C8C, kinds queued to hide
	u8 mJointShown; // _C94, shows joint 0-2-0-0-1 below the head, hidden while 0; command 8 arg 1 sets it, any other arg clears it; 1 after
	                // reset
	u8 _C95[0xC98 - 0xC95];

	XObjActorPapa();

	virtual ~XObjActorPapa();                             // _08
	virtual s32 HandleEventCommand(s32 count, s32* args); // _124
	virtual void UpdateAfterAnim(u32 frames);             // _148
	virtual void RunStateHandler(u32 frames);             // _14C
	virtual void ClearStateHandler();                     // _150
	virtual void ProcessPendingEvents(u32 frames);        // _158
	virtual void ResetActorState();                       // _15C
	virtual HSD_JObj* GetHeadJoint();                     // _168
	virtual CVec GetHeadWorldPosition();                  // _16C
	virtual f32 GetMaxHeadPitch();                        // _178
	virtual f32 GetMinHeadPitch();                        // _17C

	bool IsFree();
	void StatePickUp(u32 frames);
	void StateNone(u32 frames);
	void Drop();
};
#if defined(VERSION_JP)
STATIC_ASSERT(sizeof(XObjActorPapa) == 0xC88);
#else
STATIC_ASSERT(sizeof(XObjActorPapa) == 0xC98);
#endif

// tao, the dog
class XObjActorTao : public XObjActor {
public:
	typedef void (XObjActorTao::*StateFunc)(u32 frames);

	// _00-_C78 = XObjActor
	StateFunc mState;   // _C78
	f32 _C84;           // _C84, set from _988 (zeroed just before) in ResetActorState, never read
	u32 _C88;           // _C88, zeroed in ResetActorState, never read
	CStatus mHideKinds; // _C8C, kinds queued to hide
	Mtx mCarryMtx;      // _C94, where it holds what it carries
	Mtx mMountMtx;      // _CC4, mount for the object loaded by command 5
	s16 mMountObjId; // _CF4, id of the object held on mMountMtx, read with ReadObj; 0x82 when on (command 5, ResetActorState), -1 for none
	u8 _CF6[0xCF8 - 0xCF6];
	s32 mMountState; // _CF8, loading step for the mounted object: 0 none, 1 read it, 2 wait until it has loaded, 3 hold it on mMountMtx
	XObj* mMountObj; // _CFC, the object read in by mMountObjId and held on mMountMtx, NULL for none

	XObjActorTao();

	virtual ~XObjActorTao();                              // _08
	virtual s32 HandleEventCommand(s32 count, s32* args); // _124
	virtual void UpdateAfterAnim(u32 frames);             // _148
	virtual void RunStateHandler(u32 frames);             // _14C
	virtual void ClearStateHandler();                     // _150
	virtual void ProcessPendingEvents(u32 frames);        // _158
	virtual void ResetActorState();                       // _15C
	virtual HSD_JObj* GetHeadJoint();                     // _168
	virtual CVec GetHeadWorldPosition();                  // _16C

	bool IsFree();
	void StatePickUp(u32 frames);
	void StateNone(u32 frames);
	void Drop();
};
#if defined(VERSION_JP)
STATIC_ASSERT(sizeof(XObjActorTao) == 0xCF0);
#else
STATIC_ASSERT(sizeof(XObjActorTao) == 0xD00);
#endif

// deca
class XObjActorDeca : public XObjActor {
public:
	typedef void (XObjActorDeca::*StateFunc)(u32 frames);

	// _00-_C78 = XObjActor
	StateFunc mState; // _C78
	f32 _C84; // _C84, set to _988 (just zeroed) in ResetActorState, never read; Jenny, Mama, Papa and Deca do the same with their _C84
	f32 _C88; // _C88, set to 0 in ResetActorState, never read; the sister classes clear their _C88 the same way
	u32 _C8C; // _C8C, zeroed in ResetActorState, never read
	CStatus mJointFlags; // _C90, 4 head joint
	CStatus _C98;  // _C98, zeroed in ResetActorState, never read; Jenny keeps mHideKinds at the same offset and zeroes it at the same point
	f32 mJointYaw; // _CA0, Y rotation of joint 0-1-0-0-0, eases toward mJointYawTarget in steps of 0.1; PI after reset
	f32 mJointYawTarget; // _CA4, where mJointYaw eases to: command 3 sets PI (arg 0) or 0 (arg 1); PI after reset

	XObjActorDeca();

	virtual ~XObjActorDeca();                             // _08
	virtual void UpdateAnim(u32 frames);                  // _88
	virtual void UpdateObject(u32 frames);                // _A4
	virtual Vec GetCameraTarget();                        // _EC
	virtual s32 HandleEventCommand(s32 count, s32* args); // _124
	virtual void UpdateAfterAnim(u32 frames);             // _148
	virtual void RunStateHandler(u32 frames);             // _14C
	virtual void ClearStateHandler();                     // _150
	virtual void ProcessPendingEvents(u32 frames);        // _158
	virtual void ResetActorState();                       // _15C
	virtual HSD_JObj* GetHeadJoint();                     // _168
	virtual CVec GetHeadWorldPosition();                  // _16C

	void StatePickUp(u32 frames);
	void StateHold(u32 frames);
};
#if defined(VERSION_JP)
STATIC_ASSERT(sizeof(XObjActorDeca) == 0xC98);
#else
STATIC_ASSERT(sizeof(XObjActorDeca) == 0xCA8);
#endif

// XObjActor's own virtuals, the ones the subclasses call through to
extern "C" {
void UpdateAfterAnim__9XObjActorFUl(XObjActor* object, u32 frames);                        // UpdateAfterAnim
BOOL LoadFromPlacement__9XObjActorFRC9CoordInfo(XObjActor* object, const CoordInfo& info); // LoadFromPlacement
void ResetObjectState__9XObjActorFv(XObjActor* object);                                    // ResetObjectState
void InitializeObject__9XObjActorFUl(XObjActor* object, u32 id);                           // InitializeObject

// the counts the toys hand to the script, game/xobjactorse.cpp
s32 CountEmergedSmallSpiders();
s32 CountSmallSpidersGrabbingPlayer();
}

// the joint at the end of the path, one child step then path[i] sibling steps per level
inline HSD_JObj* GetJointFromRoot(HSD_JObj* jobj, s32 depth, s32* path)
{
	if (jobj == NULL) {
		return NULL;
	}
	for (s32 i = 0; i < depth; i++) {
		jobj = HSD_JObjGetChild(jobj);
		for (s32 j = 0; j < *path; j++) {
			if ((jobj = HSD_JObjGetNext(jobj)) == NULL) {
				return NULL;
			}
		}
		path++;
	}
	return jobj;
}

// one of the toys in pool 7
inline XObjActorToy* GetToy(s32 i)
{
	return (XObjActorToy*)gGame.mXObjs.mPools[10].Get(i);
}

// starts reading in the object with the given id
inline XObj* ReadObj(s16 id)
{
	return SpawnObjectFromPlacement(&gGame.mXObjs, &CoordInfo(GetStageObjectPlacementByIndex(id)), 0, TRUE);
}

// an object an event tied to one of the actor's joints
struct XObjActor3Part {
	HSD_JObj* mJoint;   // _00
	XObj* mObj;         // _04
	u8 _08[0x38 - 0x8]; // _08
	Mtx mMtx;           // _38

	void Clear()
	{
		mObj   = NULL;
		mJoint = NULL;
	}
};
STATIC_ASSERT(sizeof(XObjActor3Part) == 0x68);

// the stage table entry an XObjActor3 is placed from, the positions in hundredths, the angles
// in degrees
struct XObjActor3Place {
	s32 mKind;   // _00
	s32 mPos[3]; // _04
	s32 mRot[3]; // _10
};
STATIC_ASSERT(sizeof(XObjActor3Place) == 0x1C);

// pool 5's objects, 48 of them
class XObjActor3 : public XObj {
public:
	// _00-_980 = XObj
	CTpl mPanelTpl; // _980, tpl/stage02.tpl, read by kinds 0x149 and 0x42B and closed in Release; the stage02 timer and spider counter
	                // panels draw from it
	f32 _B70; // _B70, kind 10's vertical speed: starts at 4, drops 0.36 a frame to -2.3 and is added to y; kind 9 keeps the heading it
	          // turns away from here
	f32 mMoveSpeed;      // _B74, speed along mHeading, kind 13's projectiles: distance to the robot / 100 clamped 1.2-24, or 2, or 2.4
	f32 mKindTimer;      // _B78, timer
	f32 mHeading;        // _B7C, heading
	s32 mKindStep;       // _B80, the kind's step
	u32 _B84;            // _B84, never accessed
	s32 mFrameCount;     // _B88, frame count
	CStatus mActorFlags; // _B8C, 1 hit, 2 carries the mParts objects, 4 idle until started (no move pushed to the model), 0x10 update and
	                     // draw skipped, 0x20 kind 9's lunge window, 0x80 kind 9 grabbed the robot, 0x100 holds the 0x40B object, 0x200
	                     // kind 9's stun effect is due
	s32 mEventCommand;   // _B94, command from the event
	s32 mEventArg1;      // _B98, its arg
	s32 mEventArg2;      // _B9C, its second arg
	s32 mEventArg3;      // _BA0, its third arg
	u32 mExtraEventArgs[7]; // _BA4, argv[4] to argv[9] from HandleEventCommand, never read; [6] is zeroed there and kind 9 counts its
	                        // 10-second lunge window down in it
	f32 mWindowLeft;        // _BC0, window left
	f32 mWindowRight;       // _BC4, window right
	f32 mOrbitAngle;        // _BC8, angle around the owner
	f32 mOrbitX; // _BCC, x of the offset around the owner (mOrbitDistance * sin mOrbitAngle), with mOrbitHeight as y; kind 13 aims at it as
	             // its target x
	f32 mOrbitHeight; // _BD0, height above the owner
	f32 mOrbitZ; // _BD4, z of the offset around the owner (mOrbitDistance * cos mOrbitAngle), with mOrbitHeight as y; kind 13 aims at it as
	             // its target z
	f32 mOrbitDistance;   // _BD8, distance from the owner
	CVec _BDC;            // _BDC, zeroed in ResetActor3Fields, never read
	s32 mTimerFadeMode;   // _BEC, drives mTimerPanelAlpha: 0 becomes 4, 1 shown, 2 hidden, 3 fades in, 4 fades out; set by the spider
	                      // counter's command 1; kind 13 keeps its volley slot here (0 is the shot at the centre, which makes the landing
	                      // effect)
	f32 mTimerPanelAlpha; // _BF0
	f32 mCountdownFrames; // _BF4
	u8 mCountdownRunning; // _BF8
	u8 _BF9[0xBFC - 0xBF9];
	s32 mCounterFadeMode;   // _BFC
	f32 mCounterPanelAlpha; // _C00
	s32 mSpiderCount;       // _C04, count of shown pool 17 objects
	u8 _C08;                // _C08, zeroed in ResetActor3Fields, never read
	u8 _C09[0xC0C - 0xC09];
	DustParticlePool mDust; // _C0C
	CTexObj mTex[2];        // _1BC0
	s32 mGens[6];           // _1DC0, particle generators kept as s32; only [0] is used, the effect the kind is playing; [1]-[5] only zeroed
	s32 mRumble;            // _1DD8
	s32 mRumbleTimer;       // _1DDC
	XObj* mHeld;            // _1DE0, the 0x40B object kind 8 holds
	s32 _1DE4;              // _1DE4, zeroed in ResetActor3Fields, never read
	s32 mSe;                // _1DE8, the looping sfx haisui_nezumi plays
	AnimSfx mAnimSfx;       // _1DEC
	XObjActor3Part mParts[8]; // _1DFC
	u8 _213C[0x2140 - 0x213C];

	XObjActor3();

	virtual ~XObjActor3(); // _08
	virtual BOOL LoadFromPlacement(const CoordInfo& info);

	virtual void Release();
	virtual void SetAnim(s32 id, BOOL force, void* jointAnim, void* matAnim, void* shapeAnim, f32 frame, f32 rate, f32 blend,
	                     f32 blendRate);
	virtual void InitializeObject(u32 id);
	virtual void ResetForNewGame();
	virtual void ResetObjectState();
	virtual void UpdateObject(u32 frames);
	virtual void DrawObject();
	virtual void DrawObjectEffects();
	virtual void DrawScreenOverlay();
	virtual void DrawDebugStatus();
	virtual s32 HandleEventCommand(s32 count, s32* args);
	virtual void OnHitFromPosition(const Vec& position, f32 strength);
	virtual void UpdateAnimationSounds(); // _148

	// the stage table entry it was placed from
	const XObjActor3Place* GetPlace() const { return (const XObjActor3Place*)mCoord.mPlacement; }

	// what the actor does each frame by its kind, through gActor3UpdateHandlers
	void Kind0(u32 frames);                    // 0x8011529C
	void UpdateObjectGauge(u32 frames);        // 0x801151E0
	void Kind2(u32 frames);                    // 0x80114CC8
	void Kind3(u32 frames);                    // 0x801147C0
	void Kind4(u32 frames);                    // 0x801146F8
	void Kind5(u32 frames);                    // 0x801146F4
	void Kind6(u32 frames);                    // 0x8011463C
	void Kind7(u32 frames);                    // 0x80114540
	void Kind8(u32 frames);                    // 0x80114060
	void Kind9(u32 frames);                    // 0x80111A78
	void Kind10(u32 frames);                   // 0x801115FC
	void Kind11(u32 frames);                   // 0x8010FF94
	void Kind12(u32 frames);                   // 0x8010F318
	void Kind13(u32 frames);                   // 0x8010D65C
	void UpdateSpiderCounterPanel(u32 frames); // 0x8010D114
	void Kind15(u32 frames);                   // 0x8010CE80
	void Kind16(u32 frames);                   // 0x8010C304
	void Kind17(u32 frames);                   // 0x8010BF7C
	void Kind18(u32 frames);                   // 0x8010BC0C
	void Kind19(u32 frames);                   // 0x8010BC08
};

typedef void (XObjActor3::*XObjActor3Func)(u32 frames);

extern "C" XObjActor3Func gActor3UpdateHandlers[20];
#if defined(VERSION_JP)
STATIC_ASSERT(sizeof(XObjActor3) == 0x2130);
#else
STATIC_ASSERT(sizeof(XObjActor3) == 0x2140);
#endif

#endif
