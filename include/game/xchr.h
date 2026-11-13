#ifndef _GAME_XCHR_H
#define _GAME_XCHR_H

#include "game/xobj.h"
#include "game/ctpl.h"
#include "game/cptcl.h"
#include "game/xact.h"
#include "game/xpad.h"
#include "game/octamenu.h"
#include "game/xobjattachment.h"
#include "game/xobjchr.h"
#include "game/inventory.h"

// chibi robo's own object, XObjChr. the layout comes from the ctor at 0x80134694. offsets
// are usa, japan is 0x10 lower up to mPlug, see the notes after that

// chibi robo's numbers, watts, happy points and the items he holds
class Params {
public:
	s32 mExperience;      // _00
	s32 mLevel;           // _04
	s32 mRank;            // _08
	f32 mBattery;         // _0C
	f32 mMaxBattery;      // _10
	s32 mMoney;           // _14
	s32 mScrap;           // _18
	u32 _1C[8];           // _1C
	Inventory mInventory; // _3C
	// _440 vtable

	virtual ~Params() { } // _08 (weak)

	void SetBattery(f32 value)
	{
		mBattery = value > 0.0f ? value : 0.0f;
		if (mBattery < 0.0f)
			mBattery = 0.0f;
		if (mBattery > mMaxBattery)
			mBattery = mMaxBattery;
	}

	// money and scrap stay between 0 and 99999
	void SetMoney(s32 value)
	{
		mMoney = value > 0 ? value : 0;
		if (mMoney < 0)
			mMoney = 0;
		if (mMoney > 99999)
			mMoney = 99999;
	}
	void SetScrap(s32 value)
	{
		mScrap = value > 0 ? value : 0;
		if (mScrap < 0)
			mScrap = 0;
		if (mScrap > 99999)
			mScrap = 99999;
	}
};
STATIC_ASSERT(sizeof(Params) == 0x444);

extern "C" {
s32 GetOwnedInventoryItemCount(Params* stats, s16 id);
void AddPlayerScrap(Params* stats, s32 delta);
void AdjustPlayerMoney(Params* stats, s32 delta);
void AddPlayerRank(Params* stats, s32 delta);
void AdjustPlayerLevel(Params* stats, s32 delta);
void AdjustPlayerExperience(Params* stats, s32 delta);
void ResetPlayerStats(Params* stats);
}

// where chibi robo looks, beside the three CJointRots that turn his head
struct XObjChrLook {
	BOOL mActive;     // _00
	s32 mMode;        // _04
	XObj* mObject;    // _08
	Vec mOffset;      // _0C
	Vec mTarget;      // _18
	f32 mFade;        // _24, the marker fades in over 0.3 seconds
	f32 mPhase;       // _28, degrees
	f32 mPulse;       // _2C, 0 to 2
	u32 mMarkerCount; // _30, visible look markers the update found
};
STATIC_ASSERT(sizeof(XObjChrLook) == 0x34);

// a joint that loops its own idle anim
struct XObjChrIdleJoint {
	HSD_JObj* mJObj;           // _00
	HSD_AnimJoint* mAnimJoint; // _04
	u8* mLastAdHead;           // _08, the fobj's ad_head last frame
};
STATIC_ASSERT(sizeof(XObjChrIdleJoint) == 0xC);

// a count per attachment, id then value for each of the 9
struct AttachmentValue {
	s32 mId;    // _00
	s32 mValue; // _04
};

struct AttachmentValues {
	AttachmentValue mEntries[9]; // _00
};
STATIC_ASSERT(sizeof(AttachmentValues) == 0x48);

struct PlayerSuits {
	s32 mLoadStep;
	s32 mChangeStep;
	s32 mCurrent; // the DrsMenu pick
	s32 mRequested;
	s32 mSpecial;
	u32 _14;
	XObj mObjects[9];
	HSD_JObj* mJoints[9];
	u32 _55BC; // _552C in japan
};
#if defined(VERSION_JP)
STATIC_ASSERT(sizeof(PlayerSuits) == 0x5530);
#else
STATIC_ASSERT(sizeof(PlayerSuits) == 0x55C0);
#endif

// the two ring menus side by side
struct XObjChrMenu {
	AtcMenu mAtc; // _00
	DrsMenu mDrs; // _298, _280 in japan
	f32 mFade;    // _530
	f32 mCursorY; // _534
	f32 mTargetY; // _538
	f32 mCursorX; // _53C
	f32 mTargetX; // _540
	s32 mSel;     // _544, XObjChr _13388
	s32 mPrevSel; // _548, put back into mSel when the rings close

	// todo: cleanup
	u8 IsOpen() const { return mAtc.mOpen != 0 || mDrs.mOpen != 0; }
	bool IsOpening() const { return mAtc.mOpen > 0 || mDrs.mOpen > 0; }
	bool IsClosing() const
	{
		bool ret = false;
		if (mAtc.mOpen < 0 || mDrs.mOpen < 0) {
			ret = true;
		}
		return ret;
	}
};

struct FrameEventTime {
	f32 mFrame;
	f32 mLastFrame;
};

// the two joints that turn his head. retail's destructor tests this object's address before it destroys the two,
// so they sit in an object of their own
struct PlayerHeadTurn {
	CJointRot mNeck; // _00
	CJointRot mHead; // _20
};
STATIC_ASSERT(sizeof(PlayerHeadTurn) == 0x40);

class XObjChr : public XObj {
public:
	// _00-_980 = XObj
	CStatus mPlayerStatus;  // _980
	CObjVec mMotion;        // _988, cleared whenever he stops
	f32 mMoveSpeed;         // _9B8
	f32 mStick[4];          // _9BC, stick magnitude and angle, current then previous
	s32 mHitObjectIndex;    // _9CC
	s32 mTaggedObjectIndex; // _9D0, highest object with the tag mask under him
	union {
		s32 objectIndex;
		XObj* object;
	} mFloorContacts[5];        // Slot 3 holds the supporting object. The other slots hold object indices.
	CVec mContactPos;           // _9E8, where the contact-4 object stood
	s32 mLastContactFloorIndex; // _9F8
	CVec mEdgePos;              // _9FC, the climb edge's world offset
	f32 mNearDistance;          // _A0C
	s32 mOutletIndex;           // _A10, the plug target in front of him
	s32 _A14;
	u32 mLiftPlugFlags;
	s32 mHoldTimer; // _A1C, frames the button has been held on a plug
	s32 mAHeld;     // _A20, pad trigger bit 8
	s32 mAttachmentActionType;
	s32 mToolActionType;                 // _A28
	s32 mStandTimer;                     // _A2C, 3 after his anim goes back to standing
	f32 mIdleTimer;                      // _A30
	s32 mFlashTimer;                     // _A34
	s32 mFlashCount;                     // _A38, frames the flash has run
	f32 mBatteryCost;                    // _A3C, charge the tumble takes when it ends
	f32 mMoveSpeedScale;                 // _A40
	XActLift mLift;                      // _A44
	XActHang mHang;                      // _AC0
	XActJump mJump;                      // _B10
	XActLadder mLadder;                  // _B64
	XActRope mRope;                      // _BA8
	XActPush mPush;                      // _BE8
	XActTumble mTumble;                  // _C40
	XActCopter mCopter;                  // _C88
	XActPopper mPopper;                  // _CC0
	XActRadar mRadar;                    // _9438
	FrameEventTime mFrameTime;           // _94F8
	XObjChrIdleJoint mIdleJoints[4];     // _9500
	s32 mIdleJointCooldown;              // _9530
	HSD_JObj* mAttachmentJoint;          // _9534
	HSD_AnimJoint* mAttachmentJointAnim; // _9538
	CJointRot mJointRot;                 // _953C
	PlayerHeadTurn mHeadTurn;            // _955C
	XObjChrLook mLook;                   // _959C
	s32 mAttachment;                     // _95D0, the AtcMenu pick
	s32 mAttachmentRequest;              // _95D4, -1 when there's none
	s32 mAttachmentStep;                 // _95D8, how far the attachment change is
	s32 mScriptAttach;                   // _95DC, the request came from an event command
	s32 mClimbType;                      // _95E0, climb queued after the swap, 1 ladder, 2 hang
	s32 mNextClimbType;                  // _95E4
	u8 _95E8[0x960C - 0x95E8];           // _95E8
	CObjVec mSlopeMotion;                // _960C, slide speed down a floor slope
	CVec mFloorSlope;                    // _963C
	Params mParams;                      // _964C
	f32 mFallDistance;                   // _9A90, zeroed when he hits an XObjHBox
	f32 mCameraTarget[3];                // _9A94, last camera target on each axis
	f32 mCameraTargetMargin;             // _9AA0
	f32 mCameraTargetMarginSetting;      // _9AA4
	f32 mCamDirection;                   // _9AA8, the cameras put mMoveDirection back here
	f32 mMoveDirection;                  // _9AAC, the direction he walks this frame, NAN idle
	f32 mCamStickAngle;                  // _9AB0, the stick angle mCamDirection was taken at
	s32 mBatteryWarningFrames;           // _9AB4
	f32 mBatteryWarningFade;             // _9AB8
	f32 mBatteryWarningAngle;            // _9ABC
	s32 _9AC0;                           // _9AC0
	f32 _9AC4;                           // _9AC4, time shown on the player gauge
	s32 mLoadStep;
	u8 _9ACC[4];                   // _9AC8
	XObjPlug mPlug;                // _9AD0
	XObjCopter mCopterObj;         // _B1B0, _B180 in japan
	XObjPopper mPopperObj;         // _BB38, _BAF8 in japan
	XObjRadar mRadarObj;           // _C4C0, _C470 in japan
	XObjOX mOXObj;                 // _CE58, _CDF8 in japan
	PlayerSuits mSuits;            // _D7E0, _D770 in japan
	XPad mPad;                     // _12DA0, _12CA0 in japan
	f32 _12DF8;                    // _12DF8, heading he walks towards
	AttachmentValues mAttachments; // _12DFC, goes in the save as is
	XObjChrMenu mMenu;             // _12E44, _12D44 in japan
	OXMenu mOXMenu;                // _13390, _13260 in japan
	BOOL _146D8;                   // _146D8, set while he's off the floor
	DustParticlePool mDust;        // _146DC, _1329C in japan
	CTexObj mTex[2];               // _15690
	u32 _15890;                    // _15890
	s16 mSavedTarget;              // _15894, _14454 in japan, the lifted object's id while he's reset
	u16 _15896;                    // _15896
	s32 mSavedTargetAnim;          // _15898
	s32 mSavedTargetState;         // _1589C
	XObj* mPendingTarget;          // _158A0
	BOOL _158A4;                   // _158A4
	s32 _158A8;                    // _158A8, saved
	union {
		GXColor mItemTint; // _158AC, _1446C in japan, tints the held item and its particles
		s32 mItemTintWord; // _158AC, the save copies it as a word
	};
	f32 mHeadOffset;        // _158B0, _14470 in japan
	BOOL mHeadOffsetReady;  // _158B4
	s32 _158B8;             // _158B8, idle mode, 1 looks at the camera, 2 fidgets
	s32 _158BC;             // _158BC, idle timer
	s32 _158C0;             // _158C0, the ground type under the player
	CVec _158C4;            // _158C4
	CJAnim mAttachmentAnim; // _158D4, _14494 in japan, the attachment's anims
	CJAnim mSuitAnim;       // _15958, _14510 in japan, the suit's anims

	// out of line as ResetPlayerCameraValue
	void ResetCameraValue()
	{
		if (mHang.mState != 5 && mRope.mState != 5 && mLadder.mState != 5 && mPush.mState != 2) {
			mCamDirection = mMoveDirection;
		}
	}

	// how far above the floor he is
	f32 GetHeight() const { return _146D8 ? mMoveTranslate.y - mFloorHit.mHeight : 0.0f; }

	// todo: cleanup
	s32 GetAttachment() const { return mAttachmentRequest >= 0 ? mAttachmentRequest : mAttachment; }

	XObjChr();
	virtual ~XObjChr();
	virtual void SetAnimRate(f32 rate, u32 mask);
	virtual void Release();
	virtual void SetMoveDir(const Vec* dir, f32 speed);
	virtual void UpdateAnim(u32 speed);
	virtual void InitializeObject(u32 id);
	virtual void vf94();
	virtual void ResetObjectState();
	virtual BOOL LoadFromPlacement(const CoordInfo& info);
	virtual void UpdateObject(u32 frames);
	virtual void DrawWithView(Mtx view);
	virtual void DrawReflection(Mtx view);
	virtual void DrawGroundShadow();
	virtual void DrawGroundShadowAt(const Vec* origin, f32 opacity);
	virtual void DrawObjectEffects();
	virtual void vfC4();
	virtual void vfC8();
	virtual void DrawDebugStatus();
	virtual void RegisterShadowObjects(CShadow* shadow);
	virtual void vfD8(u32* flags);
	virtual void vfDC(BOOL enabled, u8 value);
	virtual void vfE0(f32 factor);
	virtual Vec GetCameraTarget();
	virtual HSD_AnimJoint* GetJointAnimation(s32 id);
	virtual HSD_MatAnimJoint* GetMaterialAnimation(s32 id);
	virtual HSD_ShapeAnimJoint* GetShapeAnimation(s32 id);
	virtual f32 GetCollisionHeight();
	virtual void vf120(s32 mode);
	virtual s32 HandleEventCommand(s32 count, s32* args);
	virtual void vf12C(s32 blend, u8 r, u8 g, u8 b, f32 time);
	virtual void OnHitFromPosition(const Vec& position, f32 strength);
	virtual void vf144(s32 unused, s32 volume);
	void SetAnim(s32 id, BOOL force, void* jointAnim, void* matAnim, void* shapeAnim, f32 frame, f32 rate, f32 blend, f32 blendRate);
};

extern "C" Vec GetPlayerFirstPersonEyePosition(XObjChr* player);
extern "C" void StopPlayerMotion(XObjChr* chr);

extern "C" void ApplyPlayerSuit(XObjChr* player);
extern "C" void RemovePlayerSuit(XObjChr* player);

#endif
