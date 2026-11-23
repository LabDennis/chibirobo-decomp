#ifndef _GAME_XOBJACTORBASE_H
#define _GAME_XOBJACTORBASE_H

#include "game/xobjjointpart.h"
#include "game/csound.h"

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
	void ResetMovementStep();

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

#endif
