// The player object's file, .text 0x800C9E84-0x800E9D30: its .rodata names it "xobj_chr.cpp"
// (wii_units.md Nr. 60 and 61), the GameCube's obja_chr.cpp. The functions come in the reverse of the
// GameCube's order.
// NONMATCHING: the small accessors and helpers have source, the player's update, draw and menu code is
// still to do. The state checks call the action accessors above them, which -inline auto expands.
#include "rvl_msl/PowerPC_EABI_Support/MSL_C/MSL_Common/math_api.h"
#include "game/wii/xobjflags.h" // its AnimSfx needs the real nw4r::math, ahead of xobj.h's shim
#include "game/attachmentvalues.h"
#include "game/ringmenu.h"
#include "game/wii/xobj.h"
#include "game/wii/xobjcamera.h"
#include "game/wii/cgame.h"
#include "game/xobjattachment.h"
#include "game/wii/xchrmenu.h"
#include "game/wii/csound.h"
#include "game/playeranimationdata.h"
#include "game/pointptcl.h"
#include "game/wii/xact.h"
#include "game/mapfloorhit.h"
#include "game/gmath.h"
#include "game/globals.h"
#include "arithmetic.h"
#include "JSystem/JMath/Inline.h"
#include <math.h>
#include <stdlib.h>

// the Wii build inlines PSVECScale as paired-single code like the other units
#define PSVECScale JMathInlineVEC::PSVECScale
// NONMATCHING: ScaleVectorInPlace's retail body interleaves the paired-single loads,
// multiplies and stores (its SDK inline's statement order); ours loads both pairs
// first, so the wrapper stays below a match

extern "C" void ReleasePushAction(XAction* action);
extern "C" BOOL IsPlayerStopped(const WiiPlayerState* chr);
extern "C" BOOL IsTumbleActionState(const XAction* act, s32 wanted);
extern "C" void fn_802AEDA0(WiiGsSceneObj** head, u32 frames); // steps the queued scene draws, as gameupdate.cpp declares it
extern CustomShadow gCustomShadow;

// the GameCube kept a weak copy of it
static inline BOOL IsObjectActive(XObj* object)
{
	return (object->mStatus.mFlags & 1ULL) != 0;
}

static inline bool TestStatus(const XObj* object, u64 mask)
{
	return (u8)((object->mStatus.mFlags & mask) != 0);
}

// out of battery: the battery is flat and the gauge on screen has run down to it
static inline bool IsStopped(const WiiPlayerState* chr)
{
	return chr->mParams.mBattery <= 0.0f && InfoScrn_IsBatteryGaugeEmpty(&gGame.mInfoScrn);
}

// out of battery and slumped over
static inline bool IsDown(const WiiPlayerState* chr)
{
	bool ret = false;
	if (IsPlayerStopped(chr)) {
		if (chr->mBatteryDrains != 0) {
			ret = true;
		}
	}
	return ret;
}

static inline bool IsEventHeld(const WiiPlayerState* chr)
{
	bool ret = false;
	if (gGame.GetStatusFlags(0x40000000) && IsDown(chr)) {
		ret = true;
	}
	return ret;
}

static inline bool IsTumbling(const WiiPlayerState* chr)
{
	return IsTumbleActionState(&chr->mTumble, 4) || IsTumbleActionState(&chr->mTumble, 5) || IsTumbleActionState(&chr->mTumble, 6);
}

static inline void SpinMarks(WiiPlayerState& chr)
{
	for (int i = 0; i < 2; i++) {
		chr.mOXMenu.mMark[i].mSpin  = DEG2RAD * (rand() % 360);
		chr.mOXMenu.mMark[i].mTilt  = DEG2RAD * (rand() % 360);
		chr.mOXMenu.mMark[i].mScale = 0.5f;
	}
}

static inline u8 IsRopeState(const WiiPlayerState* chr, s32 state)
{
	return chr->mRope.mState == state;
}

static inline Vec FirstEdgePoint(const XObj* object, s32 index, f32 height)
{
	Vec point;
	point.x = object->mMoveTranslate.x + object->mEdges[index].mFrom.x;
	point.y = height;
	point.z = object->mMoveTranslate.z + object->mEdges[index].mFrom.z;
	return point;
}

static inline Vec SecondEdgePoint(const XObj* object, s32 index, f32 height)
{
	Vec point;
	point.x = object->mMoveTranslate.x + object->mEdges[index].mTo.x;
	point.y = height;
	point.z = object->mMoveTranslate.z + object->mEdges[index].mTo.z;
	return point;
}

static inline void FocusEquipmentRing(XObjChrMenu* menu)
{
	if (menu->mSel == 0) {
		menu->mAtc.mFocus       = 0;
		menu->mDrs.mFocus       = 1;
		menu->mDrs.mCursorIndex = -1;
	} else {
		menu->mAtc.mFocus       = 1;
		menu->mAtc.mCursorIndex = -1;
		menu->mDrs.mFocus       = 0;
	}
	PlayWiiSoundEffect(&gGame.mSound, 0xF3, 1.0f, 0.0f);
}

static inline void OpenMenus(WiiPlayerState* chr)
{
	gGame.SetStatusFlags(0x400ULL);
	PlayWiiSoundEffect(&gGame.mSound, 0xF3, 1.0f, 0.0f);
	if (chr->mMenu.mSel == 0) {
		chr->mMenu.mAtc.Open(0);
		chr->mMenu.mDrs.Open(1);
	} else {
		chr->mMenu.mAtc.Open(1);
		chr->mMenu.mDrs.Open(0);
	}
}

// he stops where he is, and lets go of what he pushes
static inline void StopMoving(WiiPlayerState* chr)
{
	chr->mMotion.mValue.x  = 0.0f;
	chr->mMotion.mValue.y  = 0.0f;
	chr->mMotion.mValue.z  = 0.0f;
	chr->mMotion.mTarget.x = 0.0f;
	chr->mMotion.mTarget.y = 0.0f;
	chr->mMotion.mTarget.z = 0.0f;
	chr->mMotion.mSpeed.x  = 0.0f;
	chr->mMotion.mSpeed.y  = 0.0f;
	chr->mMotion.mSpeed.z  = 0.0f;
	chr->mMoveRotate.Set(chr->mMoveTarget1);
	if (chr->mPush.mState != 0) {
		ReleasePushAction(&chr->mPush);
		chr->mPush.mState = 0;
		chr->SetAnim(17, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
		chr->mIdleLoops = 3;
	}
}

// the y angle a direction faces
static inline f32 GetYAngle(const Vec& dir)
{
	return atan2(dir.x, dir.z);
}

// the ground type of a map triangle, 0 for none
static inline s32 GetMapTriangleFloorType(XObjMap* map, s32 tri)
{
	if (tri < 0)
		return 0;
	return map->mTrigon.mTri[tri].mFloorType;
}

// events wait while he lies there out of battery
static inline bool IsEventBlocked(const WiiPlayerState* chr)
{
	bool ret = false;
	if (IsEventHeld(chr) && IsTumbling(chr)) {
		ret = true;
	}
	return ret;
}

class EnvInfo;

extern "C" {
u32 CheckStageFlag(s32 stage, u32 mask);
BOOL IsCarryingMug(XAction* action);
BOOL IsCarryingSquirter(XAction* action);
BOOL IsCarryingSpoon(XAction* action);
BOOL IsCarryingToothbrush(XAction* action);
void ApplyPlayerItemTintToTarget(XAction* action);
BOOL fn_800C98E0(XObjMap* map, const Vec* pos, s32 start, XObjMapHit* hit);
void UpdateHeldObject(XAction* action, u32 frames);
void UpdatePushedObject(XAction* action, u32 frames);
void GetOutwardPerpendicularToEdge(Vec* a, Vec* b, Vec* c, Vec* out);
s32 GetClearLetickerApproachSide(XObj* object, const Vec* pos);
void StartPointParticles(PointPtcl* ptcl, s32 type, s32 amount);
BOOL IsLiftTargetPlug(XAction* action);
UnkStageObj* GetSpawnData(s32 index);
s32 IsStageDataLoaded(void);
EnvInfo* GetStageEnvironment(void);
UnkStageObj* GetStageObjectPlacementByIndex(s32 index);
void ShowIdlePlayerCopter(XObjCopter* copter);
void ShowIdlePlayerRadar(XObjRadar* radar);
extern s32 gSpecialSuitSpawnIds[];
BOOL LoadPlayerSuitItem(WiiPlayerState* player);
void ApplyPlayerSuit(WiiPlayerState* player);
void UpdatePlayerSuitChange(WiiPlayerState* player);
void fn_800C659C(XObjScnCamInitRec* self, XObj* obj);
void InitializePlayerPopperAction(XActPopper* act, WiiPlayerState* owner);
void InitRadarAction(XActRadar* act, WiiPlayerState* owner);
void ResetPlayerLiftAction(XActLift* act);
void ResetPlayerJumpAction(XActJump* act);
void ResetTumbleAction(XActTumble* act);
void ResetPlayerRadarAction(XActRadar* act);
void ResetPopperCharge(XActPopperCharge* charge);
void GetHudPointParticleOrigin(InfoScrn* scrn, f32* y, f32* x);
void fn_800AF12C(XObj* obj); // obja.cpp: rebinds the playing anim to the object's own file, then releases the archive
extern PointPtcl gPointParticlePool[10];
extern PointPtcl* gPointParticleSlots[10];
extern s32 gRefreshRate;

// the suit-change spiral's pool, the GameCube's gPlayerSuitChangeEffect (the .bss
// object at lbl_8045BBD0): the Wii's records are 0x18 bytes with the scale at +0
// and the angle at +8; what the rest of the 0x258 holds only the spiral code knows
struct SuitChangeTrail {
	s32 mActive; // _00, nonzero while the spiral runs
	u8 _04[0xC];
	struct {
		f32 scale; // _00
		u8 _04[4];
		f32 angle; // _08
		u8 _0C[0xC];
	} mParticles[20]; // _10
	u8 _1F0[0x258 - 0x1F0];
};
extern SuitChangeTrail lbl_8045BBD0;
extern "C" void fn_800F2CF4(SuitChangeTrail* trail); // starts the suit-change spiral

// the delay the suit change keeps the old suit's resources around, the GameCube's
// gPlayerSuitChangeTimer; the Wii resets it to 0.0f and waits on the file itself
static f32 gPlayerSuitChangeTimer;

static inline void ResetSuitChangeTrail(SuitChangeTrail& trail)
{
	trail.mActive = 0;
	for (s32 i = 0; i < 20; ++i) {
		trail.mParticles[i].scale = 0.0f;
		trail.mParticles[i].angle = 0.0f;
	}
}

static inline void ClearRingHighlights(OctaMenu& ring)
{
	ring.mOpen = 0;
	ring.mFade = 0.0f;
	for (s32 i = 0; i < 8; ++i)
		ring.mItems[i].mHighlight = 0.0f;
}

// the head of the stage's info record, as far as LoadFromPlacement reads it: the GameCube's
// UnkStageInfo (game/cgameutil.h, which the Wii cannot include). The loaded stage's EnvInfo
// starts with a pointer to it
struct WiiStageInfo {
	u8 _00[0x9];
	u8 _09; // _09, the ambient light level in percent, vf1A8's argument
};

static inline WiiStageInfo* GetStageInfo()
{
	return *(WiiStageInfo**)GetStageEnvironment();
}

// _154, the GameCube's XObjChr::ResetObjectState: the player state, his actions,
// the embedded objects, the menus and the pools all go back to their defaults
// NONMATCHING (~90 %): same instruction multiset, call set and store order; retail colours
// this into r30 and keeps gPointParticlePool's base live in r31 from the prologue, deriving
// gPointParticleSlots (+0x3D18) and lbl_8045BBD0 (+0x3D40) from it as contiguous .bss. Ours
// rematerializes each base inside the loop region — register-allocation wall, no logic diff.
void XObjChr::ResetObjectState()
{
	WiiPlayerState* chr = (WiiPlayerState*)this;
	XObj::ResetObjectState();
	chr->vf28C(0);
	chr->mMotion.Reset(0.0f, 0.0f, 0.0f);
	chr->mStick[2] = chr->mStick[3] = chr->mStick[0] = chr->mStick[1] = 0.0f;
	chr->m_0A1C = chr->m_0A18    = -1;
	chr->mFloorContactObjects[4] = chr->mFloorContactObjects[2] = chr->mFloorContactObjects[0] = chr->mFloorContactObjects[1] = -1;
	chr->mLastContactFloorIndex                                                                                               = -1;
	chr->mNearDistance                                                                                                        = 2.0f;
	chr->mLetickerIndex                                                                                                       = -1;
	chr->_0A70                                                                                                                = 0;
	chr->mLiftPlugFlags &= ~0x80;
	chr->mPressTimer = chr->mAPressed = 0;
	chr->mIdleLoops                   = 0;
	chr->mIdleTimer                   = 0.0f;
	chr->mFlashTimer = chr->mFlashCount = 0;
	chr->mHitStrength                   = 0.0f;
	chr->mMoveSpeedScale                = 1.0f;
	chr->mHang.Init(chr);
	chr->mHang.mStickAway = FALSE;
	chr->mJump.Init(chr);
	((XActJump*)&chr->mJump)->mStrength = ((XActJump*)&chr->mJump)->mTargetStrength = 0.0f;
	chr->mLadder.Init(chr);
	((XActLadder*)&chr->mLadder)->mBPressed = FALSE;
	chr->mRope.Init(chr);
	chr->mPush.Init(chr);
	chr->mTumble.Init(chr);
	chr->mCopter.Init(chr);
	chr->mCopter.mTime = gRefreshRate;
	InitializePlayerPopperAction((XActPopper*)&chr->mPopper, chr);
	InitRadarAction((XActRadar*)&chr->mRadar, chr);
	chr->_969C             = 0;
	chr->mFrameTime.mFrame = chr->mFrameTime.mLastFrame = -1.0f;
	chr->mBlinkWait                                     = 0;
	chr->mBlinkFrame                                    = 0;
	fn_800C659C((XObjScnCamInitRec*)chr->_9810, this);
	fn_800C659C((XObjScnCamInitRec*)chr->_987C, this);
	chr->mLook.mActive = FALSE;
	chr->mLook.mMode   = -1;
	chr->mLook.mObject = NULL;
	Vec zero           = { 0.0f, 0.0f, 0.0f };
	chr->mLook.mOffset = zero;
	chr->mLook.mPulse = chr->mLook.mPhase = chr->mLook.mFade = 0.0f;
	chr->mLook.mObject                                       = this;
	chr->mAttachmentRequest                                  = -1;
	chr->mNextClimbType = chr->mClimbType = chr->mScriptAttach = chr->mAttachmentStep = 0;
	chr->mFallDistance                                                                = 0.0f;
	chr->mCameraTarget[0] = chr->mCameraTarget[1] = chr->mCameraTarget[2] = NAN;
	chr->mCameraTargetMargin = chr->mCameraTargetMarginSetting = 1.0f;
	chr->mHeldMoveAngle = chr->mMoveAngle = chr->mHeldStickAngle = NAN;
	chr->mBatteryWarningFrames                                   = 0;
	chr->mBatteryWarningFade = chr->mBatteryWarningAngle = 0.0f;
	chr->mGaugeTarget                                    = NULL;
	chr->mGaugeTime                                      = 0.0f;
	chr->mPlug.ResetObjectState();
	((XObj*)chr->mCopterObj)->ResetObjectState();
	((XObj*)chr->mPopperObj)->ResetObjectState();
	((XObj*)chr->mRadarObj)->ResetObjectState();
	((XObj*)chr->mOXObj)->ResetObjectState();
	for (s32 i = 0; i < 9; ++i)
		chr->mSuits.mObjects[i].ResetObjectState();
	chr->mWalkHeading = NAN;
	ClearRingHighlights(chr->mMenu.mAtc);
	ClearRingHighlights(chr->mMenu.mDrs);
	chr->mMenu.mTargetY = chr->mMenu.mCursorY = chr->mMenu.mFade = 0.0f;
	chr->mOXMenu.mStarted                                        = FALSE;
	chr->mOXMenu._10                                             = -1;
	chr->mOXMenu.mFade                                           = 0.0f;
	chr->mOnGround                                               = 0;
	mStatus.mFlags &= ~0x800000ULL;
	InitParticlePool(&mDustPool.mPtcl, mDustPool.mPtcls, sizeof(mDustPool.mPtcls[0]), ARRAY_SIZE(mDustPool.mPtcls));
	for (s32 i = 0; i < 10; ++i) {
		PointPtcl& p = gPointParticlePool[i];
		GetHudPointParticleOrigin(&gGame.mInfoScrn, &p.mStartY, &p.mStartX);
		p.burst.mState = p.display.mState = 0;
		gPointParticleSlots[i]            = NULL;
	}
	ResetSuitChangeTrail(lbl_8045BBD0);
	chr->mGroundType = 0;
	chr->_1C9A8.x    = 0.0f;
	chr->_1C9A8.y    = 0.0f;
	chr->_1C9A8.z    = 0.0f;
	chr->_1C9B8[0] = chr->_1C9B8[1] = chr->_1C9B8[2] = chr->_1C9C4[0] = chr->_1C9C4[1] = 0.0f;
	chr->mEyeAim.Set(0.0f, 0.0f, 0.0f);
	chr->mEyeAimTarget.Set(0.0f, 0.0f, 0.0f);
	chr->mEyeAimStep.Set(0.0f, 0.0f, 0.0f);
	chr->mHeadOffset      = 5.0f;
	chr->mHeadOffsetReady = FALSE;
	mActor.Reset();
}

void SetVectorComponents(CVec* value, f32 x, f32 y, f32 z)
{
	value->Set(x, y, z);
}

void ClearObjectStatusFlags(XObj* object, u64 mask)
{
	object->mStatus.mFlags &= ~mask;
}

// the attachment goes away: its sounds stop, its object hides, and the actions and
// the carried object all reset; the GameCube's ResetPlayerAttachment (obja_chr.cpp).
// On the Wii the put-away is shared with the attachment switch, so the lift's early
// states keep the held item from jumping straight back into the inventory
void ResetPlayerAttachment(WiiPlayerState* chr, s32 nextAnimation)
{
	switch (chr->GetAttachment()) {
	case 1:
		if (IsWiiSfxPlaying(&gGame.mSound, 0x75)) {
			StopWiiSfx(&gGame.mSound, 0x75, 0.0f);
		}
		((XObjAttachment*)chr->mCopterObj)->Hide();
		break;
	case 2:
		if (IsWiiSfxPlaying(&gGame.mSound, 0x1F3)) {
			StopWiiSfx(&gGame.mSound, 0x1F3, 0.0f);
		}
		if (IsWiiSfxPlaying(&gGame.mSound, 0x1F8)) {
			StopWiiSfx(&gGame.mSound, 0x1F8, 0.0f);
		}
		((XObjAttachment*)chr->mPopperObj)->Hide();
		chr->vf290(FALSE);
		ResetPopperCharge(&((XActPopper*)&chr->mPopper)->mCharge);
		break;
	case 3:
		if (IsWiiSfxPlaying(&gGame.mSound, 0x1F9)) {
			StopWiiSfx(&gGame.mSound, 0x1F9, 0.0f);
		}
		if (IsWiiSfxPlaying(&gGame.mSound, 0x1FA)) {
			StopWiiSfx(&gGame.mSound, 0x1FA, 0.0f);
		}
		if (IsWiiSfxPlaying(&gGame.mSound, 0x1FB)) {
			StopWiiSfx(&gGame.mSound, 0x1FB, 0.0f);
		}
		((XObjAttachment*)chr->mRadarObj)->Hide();
		break;
	case 4:
	case 5:
	case 6:
	case 7:
	default:
		if (chr->mLift.mTarget != NULL) {
			if (chr->mLift.mTarget->GetPool() == 8) {
				BOOL inLift = (u8)(chr->mLift.mState == 1 || chr->mLift.mState == 2);
				inLift      = inLift || (u8)(chr->mLift.mState == 3);
				inLift      = inLift && ((XActLift*)&chr->mLift)->_AC;
				if (!inLift) {
					SetOrAddInventoryItemCount(&gGame.GetPlayer()->mParams.mInventory, chr->mLift.mTarget->GetItem(), 1, TRUE);
					((XObjItem*)chr->mLift.mTarget)->SetFlag();
				}
			}
			if (chr->mLift.mTarget->GetPool() != 24) {
				// the Wii schedules the release over two frames; the GameCube called Release here
				chr->mLift.mTarget->mStatus.mFlags &= ~0x10000ULL;
				chr->mLift.mTarget->mReleaseCountdown = 2;
			}
		}
		break;
	}
	ResetPlayerLiftAction((XActLift*)&chr->mLift);
	chr->mHang.Reset();
	ResetPlayerJumpAction((XActJump*)&chr->mJump);
	chr->mLadder.Reset();
	chr->mRope.Reset();
	ReleasePushAction(&chr->mPush);
	ResetTumbleAction(&chr->mTumble);
	chr->mCopter.Reset();
	chr->mPopper.Reset();
	ResetPlayerRadarAction((XActRadar*)&chr->mRadar);
	chr->mAttachment             = 0;
	chr->mAttachmentRequest      = -1;
	chr->mAttachmentStep         = 0;
	chr->mScriptAttach           = 0;
	chr->mClimbType              = 0;
	chr->mNextClimbType          = 0;
	chr->m_0A18                  = -1;
	chr->m_0A1C                  = -1;
	chr->mFloorContactObjects[2] = -1;
	if (nextAnimation >= 0) {
		chr->SetAnim(nextAnimation, TRUE, 0.0f, 1.0f, 1.0f, 10.0f);
	}
	chr->mFallDistance = 0.0f;
	InfoScrn_ApplyObjectGaugeCommand(&gGame.mInfoScrn, NULL, 0, 1, 0);
	InfoScrn_ApplyObjectGaugeCommand(&gGame.mInfoScrn, NULL, 0, 1, 1);
	InfoScrn_ApplyObjectGaugeCommand(&gGame.mInfoScrn, NULL, 0, 1, 2);
	InfoScrn_ApplyObjectGaugeCommand(&gGame.mInfoScrn, NULL, 0, 1, 3);
}

XSound* GetGameSound(CGame* game)
{
	return &game->mSound;
}

BOOL HasPlayerActionTarget(const XAction* action)
{
	return action->mTarget != NULL;
}

s16 GetObjectCategory(const XObj* object)
{
	return object->GetCategory();
}

XObj* GetPlayerActionTarget(const XAction* action)
{
	return action->mTarget;
}

BOOL IsLiftActionState(const XAction* act, s32 wanted)
{
	return (u8)(wanted == act->mState);
}

s32 GetObjectPlacementParam(const XObj* object, s32 index)
{
	const s32* entries = &object->mPlacementParam0;
	return entries[index];
}

// stops him where he stands
// NONMATCHING (99.6 %): retail loads the rotation target into f1-f3 the other way round
void StopPlayerMotion(WiiPlayerState* chr)
{
	StopMoving(chr);
}

CVec* GetObjectVectorTarget(CObjVec* value)
{
	return &value->mTarget;
}

BOOL IsPushActionState(const XAction* act, s32 wanted)
{
	return (u8)(wanted == act->mState);
}

void SetPushActionState(XAction* act, s32 state)
{
	act->mState = state;
}

// _158, the GameCube's XObjChr::LoadFromPlacement: the stage load runs one step a frame.
// The Wii model's head joints come out of the scene graph by name, the attachments mount on
// world matrices instead of joint paths, and his two status textures and the suit-change
// sound actor are made here; the GameCube's pose samples, O/X marks and idle joints are gone
// NONMATCHING (93 %): same instruction multiset, call set and control flow; retail colours six
// callee-saves (r26-r31, this in r30, the unit string base in r28, the new actor's manager and
// vtable halves in r26/r27/r28/r29), ours three - and materializes the case-0 'placed' bool at
// the merge instead of keeping it in r29. Pool referents (100.0f, the u8 magic, the jump table
// and the string base) still land in @labels; they resolve as the unit fills in
BOOL XObjChr::LoadFromPlacement(const CoordInfo& info)
{
	WiiPlayerState* chr = (WiiPlayerState*)this;
	// Each ready model falls through to the next load. A pending one resumes at the same step.
	switch (chr->mLoadStep) {
	case 0: {
		BOOL placed = XObj::LoadFromPlacement(info);
		if (placed && chr->mObject.m_pScnMdl != NULL) {
			chr->mHeadNodes[0] = GetResNode("second_null");
			chr->mHeadNodes[1] = GetResNode("head_chn");
			chr->mHeadNodes[2] = GetResNode("head_cap");
			chr->mHeadNodes[3] = GetResNode("l_mabuta_down");
			chr->mHeadNodes[4] = GetResNode("l_mabuta_up");
			chr->mHeadNodes[5] = GetResNode("r_mabuta_down");
			chr->mHeadNodes[6] = GetResNode("r_mabuta_up");
			chr->mObject.m_pScnMdl->EnableScnObjCallbackExecOp(nw4r::g3d::ScnObj::EXECOP_CALC_WORLD);
		}
		if (!placed)
			break;
		mStatus.mFlags |= 0x200000000000ULL;
		chr->mBlinkWait  = 0;
		chr->mBlinkFrame = 0;
		mObject.m_blend.m_pBlend->UseQuaternionBlend(true);
		mObject.m_blend.m_pBlend->UseAccurateScaleBlend(true);
		++chr->mLoadStep;
	}
	case 1: // the plug
		if (chr->mPlug.LoadFromPlacement(CoordInfo(GetSpawnData(15)))) {
			++chr->mLoadStep;
		} else {
			break;
		}
	case 2: // the copter
		if (((XObjCopter*)chr->mCopterObj)->LoadFromPlacement(CoordInfo(GetSpawnData(17)))) {
			((XObjCopter*)chr->mCopterObj)->SetParent(GetWorldMtx("head"), TRUE);
			++chr->mLoadStep;
		} else {
			break;
		}
	case 3: // the popper
		if (((XObjPopper*)chr->mPopperObj)->LoadFromPlacement(CoordInfo(GetSpawnData(18)))) {
			((XObjPopper*)chr->mPopperObj)->SetParent(GetWorldMtx("l_arm_chn"), TRUE);
			++chr->mLoadStep;
		} else {
			break;
		}
	case 4: // the radar
		if (((XObjRadar*)chr->mRadarObj)->LoadFromPlacement(CoordInfo(GetSpawnData(19)))) {
			((XObjRadar*)chr->mRadarObj)->SetParent(GetWorldMtx("head"), TRUE);
			++chr->mLoadStep;
		} else {
			break;
		}
	case 5: // the O/X mark attachment; a Japan-only game always loads entry 20
		if (((XObjOX*)chr->mOXObj)->LoadFromPlacement(CoordInfo(GetSpawnData(20)))) {
			((XObjOX*)chr->mOXObj)->SetParent(GetWorldMtx("head"), TRUE);
			++chr->mLoadStep;
		} else {
			break;
		}
	case 6:
		// the GameCube's other editions loaded the two O/X menu marks here, in their own steps
		++chr->mLoadStep;
	case 7: // the suit parts the stage itself provides
		if (chr->mSuitParts[3].LoadFromPlacement(CoordInfo(GetSpawnData(36)))) {
			++chr->mLoadStep;
		} else {
			break;
		}
	case 8:
		if (chr->mSuitParts[6].LoadFromPlacement(CoordInfo(GetSpawnData(37)))) {
			++chr->mLoadStep;
		} else {
			break;
		}
	case 9: {
		u32 flags[] = { mStatus.mFlags & 0x800000, mStatus.mFlags & 0x200000, mStatus.mFlags & 0x40, mStatus.mFlags & 0x20000,
			            mStatus.mFlags & 0x400000 };
		vf1A0(flags);
		if ((mStatus.mFlags & 0x01000000) && IsStageDataLoaded()) {
			vf1A8(GetStageInfo()->_09 / 100.0f);
		}
		++chr->mLoadStep;
	}
	case 10: // his two status textures, out of the global tpl's palette
		gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[123], 0);
		mTex[0] = gGame.mTpl.mTex;
		gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[124], 0);
		mTex[1] = gGame.mTpl.mTex;
		++chr->mLoadStep;
	case 11:
		++chr->mLoadStep;
	case 12:
		if (chr->GetAttachment() == 1) {
			ShowIdlePlayerCopter((XObjCopter*)chr->mCopterObj);
		} else if (chr->GetAttachment() == 2) {
			chr->mPopper.mState = 3;
		} else if (chr->GetAttachment() == 3) {
			ShowIdlePlayerRadar((XObjRadar*)chr->mRadarObj);
		}
		++chr->mLoadStep;
		break;
	case 13: // the suit change's sound actor
		mActor.Create();
		++chr->mLoadStep;
	}
	if (chr->mLoadStep > 0xD) {
		mStatus.mFlags |= 0x40000ULL;
		chr->mLoadStep = 0;
		return TRUE;
	}
	return FALSE;
}

void SetObjectStatusFlags(XObj* object, u64 mask)
{
	object->mStatus.mFlags |= mask;
}

u64 GetMaskedObjectStatus(const XObj* object, u64 mask)
{
	return mask & object->mStatus.mFlags;
}

BOOL IsPlayerAttachmentEquipped(const WiiPlayerState* chr, s32 wanted)
{
	return (u8)(wanted == chr->GetAttachment());
}

// the attachment's animation is part of the player's own files here, so there's nothing to wait for
BOOL LoadPlayerAttachmentAnimation(WiiPlayerState*)
{
	return TRUE;
}

// the GameCube's SuitFadeFinished: the fade SetColor queues is done once the second color
// step's speed ran out; the Wii keeps the pair inside the animation queue
static inline bool SuitFadeFinished(const WiiPlayerState* player)
{
	const Vec& speed = player->mAnmQueue.mColorStep[1].mSpeed;
	return speed.x == 0.0f && speed.y == 0.0f && speed.z == 0.0f;
}

// the queued suit change, one call a frame: flashes him white, swaps the worn suit for the
// requested one, mounts the attachments back on his joints and fades back to its tint.
// The GameCube's UpdatePlayerSuitChange; on the Wii its RemovePlayerSuit work is inlined and
// the suit's own animation file is a CJArchive instead of a CJAnim
// _0x500, the GameCube's UpdatePlayerSuitChange: fades to white, starts the spiral, frees
// the old suit and its animation file, loads the new one through LoadPlayerSuitItem, then
// ApplyPlayerSuit and fades back. Step 3 commits mCurrent=mRequested and drops through 4->5
// NONMATCHING (~96 %): same instruction multiset, call set and control flow. Retail dedicates
// r30 to the unit's shared string block (lbl_802DAA6C+0x95/"head", +0x9A/"l_arm_chn") and
// keeps `player` in r31; our pool still carries only this function's two strings, so MWCC
// keeps `player` in r30 and no register goes to the strings — every remaining diff is that
// rename, the same-value float labels (@-labels vs lbl_805A4Exx), the timer's .sbss address
// (lbl_805A1C50) and the scheduling it drags with it. Closes when the unit's rodata/.sbss
// fill in in retail order (fn_800CE054 also binds these strings)
void UpdatePlayerSuitChange(WiiPlayerState* player)
{
	switch (player->mSuits.mChangeStep) {
	case 1:
		player->SetColor(2, 255, 255, 255, 0.5f);
		fn_800F2CF4(&lbl_8045BBD0);
		PlayWiiSoundEffect(&gGame.mSound, 0xB2, 1.0f, 0.0f);
		++player->mSuits.mChangeStep;
		break;
	case 2:
		if (SuitFadeFinished(player)) {
			++player->mSuits.mChangeStep;
		}
		break;
	case 3:
		if (player->mSuits.mCurrent != 0) {
			s32 anim  = player->GetAnimId();
			f32 frame = player->GetAnimFrame();
			player->ClearAnim();
			((XObjAttachment*)player->mCopterObj)->SetParent(player->GetWorldMtx("head"), FALSE);
			((XObjAttachment*)player->mPopperObj)->SetParent(player->GetWorldMtx("l_arm_chn"), FALSE);
			((XObjAttachment*)player->mRadarObj)->SetParent(player->GetWorldMtx("head"), FALSE);
			((XObjAttachment*)player->mOXObj)->SetParent(player->GetWorldMtx("head"), FALSE);
			if (IsObjectActive(&player->mSuits.mObjects[player->mSuits.mCurrent])) {
				if (player->GetAttachment() == 2) {
					player->mSuits.mObjects[player->mSuits.mCurrent].vf290(FALSE);
				}
				player->mSuits.mObjects[player->mSuits.mCurrent].ClearAnim();
				player->mSuits.mObjects[player->mSuits.mCurrent].Release();
			}
			if (player->mSuitAnm.mAnm.mFile.IsLoaded()) {
				player->mSuitAnm.mAnm.Release(TRUE);
				player->mSuitAnm.mAnm.mFile.Close();
			}
			player->SetAnim(anim, TRUE, frame, 1.0f, 1.0f, 15.0f);
			player->SetColor(2, 255, 255, 255, 0.0f);
		}
		player->mSuits.mLoadStep = 1;
		player->mSuits.mCurrent  = player->mSuits.mRequested;
		++player->mSuits.mChangeStep;
		gPlayerSuitChangeTimer = 0.0f;
		// fall through
	case 4:
		gPlayerSuitChangeTimer -= 2.0f;
		if (gPlayerSuitChangeTimer <= 0.0f) {
			gPlayerSuitChangeTimer = 0.0f;
		}
		if (!LoadPlayerSuitItem(player) || gPlayerSuitChangeTimer != 0.0f) {
			break;
		}
		++player->mSuits.mChangeStep;
		// fall through
	case 5: {
		ApplyPlayerSuit(player);
		player->SetColor(2, 255, 255, 255, 0.0f);
		player->SetColor(2, 0, 0, 0, 0.5f);
		const Vec& tint = player->mAnmQueue.mColorStep[0].mValue;
		u32 color       = ((u32)tint.x << 24) | ((u32)tint.y << 16) | ((u32)tint.z << 8);
		player->SetColor(0, color >> 24, color >> 16, color >> 8, 0.0f);
		++player->mSuits.mChangeStep;
		break;
	}
	case 6:
		if (SuitFadeFinished(player)) {
			player->mSuits.mChangeStep = 0;
		}
		break;
	}
}

BOOL fn_800CDDAC(const XObj* object)
{
	return (object->mStatus.mFlags & 1) != 0;
}

// the player's suit step of the stage setup: puts on the suit the save asked for, TRUE once it is on
// NONMATCHING: pure register allocation — retail puts the +0x10000 base in r4, mCurrent in r0
// and mRequested in r3 (param reg reused); our build bases in r3 and loads mRequested into r4
// first. Identical instruction stream otherwise, every spelling tried converges to ours.
BOOL fn_800CDC08(WiiPlayerState* player)
{
	if (player->mSuits.mLoadStep == 0) {
		player->mSuits.mLoadStep = 1;
	}
	if (player->mSuits.mCurrent != player->mSuits.mRequested) {
		player->mSuits.mCurrent    = player->mSuits.mRequested;
		player->mSuits.mChangeStep = 1;
	}
	if (LoadPlayerSuitItem(player)) {
		player->mSuits.mChangeStep = 0;
		ApplyPlayerSuit(player);
		return TRUE;
	}
	return FALSE;
}

// loads the suit the change asked for, one call a frame: TRUE once its object is placed and
// reading the player's animations. The GameCube's LoadPlayerSuitItem; its second step (the
// suit's own animation file) is gone — the suit sources the player's NW_Obj instead
BOOL LoadPlayerSuitItem(WiiPlayerState* player)
{
	if (player->mSuits.mCurrent == 0) {
		player->mSuits.mLoadStep = 0;
	}
	if (player->mSuits.mLoadStep == 0) {
		return TRUE;
	}
	switch (player->mSuits.mLoadStep) {
	case 1: {
		WiiSuitObject* suit = &player->mSuits.mObjects[player->mSuits.mCurrent];
		s32 id              = player->mSuits.mCurrent == 7 ? gSpecialSuitSpawnIds[player->mSuits.mSpecial] : player->mSuits.mCurrent + 25;
		if (suit->LoadFromPlacement(CoordInfo(GetSpawnData(id)))) {
			suit->SetAnmSource(&player->mObject);
			player->mSuits.mLoadStep = 0;
			return TRUE;
		}
		break;
	}
	case 2:
		player->mSuits.mLoadStep = 0;
		return TRUE;
	}
	return FALSE;
}

// mounts the attachment objects on the player's joints once the new suit's model is there,
// then restarts the idle animation. The GameCube's ApplyPlayerSuit; the Wii version finds
// the mount joints by name on the nw4r model instead of grafting HSD joint trees
void ApplyPlayerSuit(WiiPlayerState* player)
{
	if ((player->mStatus.mFlags & 1) && player->mSuits.mObjects[player->mSuits.mCurrent].mObject.m_pScnMdl != NULL) {
		player->ClearAnim();
		if (player->GetAttachment() == 2) {
			player->vf290(FALSE);
		}
		((XObjAttachment*)player->mCopterObj)->SetParent(player->GetWorldMtx("head"), FALSE);
		((XObjAttachment*)player->mPopperObj)->SetParent(player->GetWorldMtx("l_arm_chn"), FALSE);
		((XObjAttachment*)player->mRadarObj)->SetParent(player->GetWorldMtx("head"), FALSE);
		((XObjAttachment*)player->mOXObj)->SetParent(player->GetWorldMtx("head"), FALSE);
		if (!gGame.GetStatusFlags(0x100)) {
			player->vf28C(FALSE);
		}
		if (player->GetAttachment() == 2) {
			player->vf290(TRUE);
		}
	}
	if (player->GetAttachment() == 1 || player->GetAttachment() == 3) {
		player->SetAnim(0x4A, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
	} else {
		player->SetAnim(17, FALSE, 0.0f, 1.0f, 1.0f, 15.0f);
		player->mIdleLoops = 3;
	}
}

// NONMATCHING (96 %): same instruction multiset, call set and control flow; retail keeps this in
// r31 and the unit string table lbl_802DAA6C in r30, this build swaps both down one register
// (this in r30, the two new strings at the top of the pool in r29) — closes with unit
// completeness, as in ApplyPlayerSuit and UpdatePlayerSuitChange

// _40, the GameCube's XObjChr::Release with RemovePlayerSuit folded in: the player flags and
// the pad reset, the base matrix, the carried object's saved state, both animation archives,
// the four attachments back on his own joints, the worn suit and the event script
void XObjChr::Release()
{
	WiiPlayerState* chr = (WiiPlayerState*)this;
	chr->mPadReset      = TRUE;
	chr->mPlayerFlags   = 0;
	chr->mPlayerFlags |= 0x20000000;
	chr->mPlayerFlags |= 7;
	if (HasModel()) {
		ClearBaseMtx();
	}
	if (chr->mLift.mState != 0) {
		XObj* target = chr->mLift.mTarget;
		if (target != NULL) {
			if ((s16)target->GetPool() == 8) {
				chr->mSavedTarget      = target->GetObjFlags();
				chr->mSavedTargetAnim  = chr->mLift.mTarget->GetAnimId();
				chr->mSavedTargetState = chr->mLift.mTargetState;
			}
		}
	} else {
		switch (chr->GetAttachment()) {
		case 4:
		case 5:
		case 6:
		case 7:
			chr->mAttachment = 0;
			break;
		}
	}
	fn_800AF12C(this);
	if (chr->mAttachmentAnm.mAnm.mFile.IsLoaded()) {
		chr->mAttachmentAnm.mAnm.Release(TRUE);
		chr->mAttachmentAnm.mAnm.mFile.Close();
	}
	if (chr->mSuitAnm.mAnm.mFile.IsLoaded()) {
		chr->mSuitAnm.mAnm.Release(TRUE);
		chr->mSuitAnm.mAnm.mFile.Close();
	}
	s32 anim  = GetAnimId();
	f32 frame = GetAnimFrame();
	ClearAnim();
	((XObjAttachment*)chr->mCopterObj)->SetParent(GetWorldMtx("head"), FALSE);
	((XObjAttachment*)chr->mPopperObj)->SetParent(GetWorldMtx("l_arm_chn"), FALSE);
	((XObjAttachment*)chr->mRadarObj)->SetParent(GetWorldMtx("head"), FALSE);
	((XObjAttachment*)chr->mOXObj)->SetParent(GetWorldMtx("head"), FALSE);
	if (IsObjectActive(&chr->mSuits.mObjects[chr->mSuits.mCurrent])) {
		if (chr->GetAttachment() == 2) {
			chr->mSuits.mObjects[chr->mSuits.mCurrent].vf290(FALSE);
		}
		chr->mSuits.mObjects[chr->mSuits.mCurrent].ClearAnim();
		chr->mSuits.mObjects[chr->mSuits.mCurrent].Release();
	}
	if (chr->mSuitAnm.mAnm.mFile.IsLoaded()) {
		chr->mSuitAnm.mAnm.Release(TRUE);
		chr->mSuitAnm.mAnm.mFile.Close();
	}
	SetAnim(anim, TRUE, frame, 1.0f, 1.0f, 15.0f);
	if (chr->mEvt.mData.mScript != NULL) {
		ResetEventScriptState(&chr->mEvt, FALSE, FALSE, FALSE, FALSE);
	}
}

// reads the player's pad once more
void fn_800CE48C(WiiPlayerState* player)
{
	player->mPad.Update(gWiiGameCubePads, 0);
}

// picks up again what he was holding before the reset, TRUE once it is back in his hands;
// the GameCube's RestorePlayerHeldItem. The Wii flags the respawned object as carried
// (0x10000, what ResetPlayerAttachment clears) and, while he holds a tool, mirrors his
// own hidden flag into the target's 0x20000000
extern "C" BOOL RestorePlayerHeldItem(WiiPlayerState* chr)
{
	if (chr->mLift.mState != 0) {
		XObj* target = chr->mLift.mTarget;
		if (target != NULL) {
			if ((s16)target->GetPool() == 24) {
				chr->mLift.mTarget      = &chr->mPlug;
				chr->mLift.mTargetFloor = chr->mLift.mTarget->mSelectedFloorIndex;
				chr->mPlug.mStatus.mFlags |= 0x0400000000000000ULL;
				chr->SetAnim(8, TRUE, 0.0f, 1.0f, 1.0f, 15.0f);
				return TRUE;
			}
		}
		ResetPlayerLiftAction((XActLift*)&chr->mLift);
	}

	s16 id = chr->mSavedTarget;
	if (id >= 0) {
		chr->mPendingTarget = SpawnObjectFromPlacement(&gGame.mXObjs, &CoordInfo(GetStageObjectPlacementByIndex(id)), 0, TRUE);
		chr->mSavedTarget   = -1;
	}
	if (chr->mPendingTarget != NULL) {
		if (!(chr->mPendingTarget->mStatus.mFlags & 0x0040000000000000ULL)) {
			chr->mPendingTarget->mStatus.mFlags |= 0x10000;
			ResetPlayerLiftAction((XActLift*)&chr->mLift);
			chr->mLift.mState = 4;
			chr->mLift.SetTarget(chr->mPendingTarget);
			chr->mLift.mTargetState = 4;
			chr->mLift.mTarget->SetAnim(chr->mSavedTargetAnim, TRUE, 0.0f, 1.0f, 1.0f, 15.0f);
			chr->mLift.mTarget->mStatus.mFlags |= 0x0400000000000000ULL;
			chr->SetAnim(8, TRUE, 0.0f, 1.0f, 1.0f, 15.0f);
			chr->mPendingTarget = NULL;
			if (IsCarryingSquirter(&chr->mLift)) {
				ApplyPlayerItemTintToTarget(&chr->mLift);
			}
			if (IsCarryingToothbrush(&chr->mLift) || IsCarryingSpoon(&chr->mLift) || IsCarryingMug(&chr->mLift)
			    || IsCarryingSquirter(&chr->mLift)) {
				if ((chr->mStatus.mFlags & 0x20000000) != 0) {
					if ((chr->mLift.mTarget->mStatus.mFlags & 0x20000000) == 0) {
						chr->mLift.mTarget->mStatus.mFlags |= 0x20000000;
					}
				} else if ((chr->mLift.mTarget->mStatus.mFlags & 0x20000000) != 0) {
					chr->mLift.mTarget->mStatus.mFlags &= ~0x20000000ULL;
				}
			}
			return TRUE;
		}
	} else {
		return TRUE;
	}
	return FALSE;
}

void SetPlayerActionTarget(XAction* action, XObj* target)
{
	action->mTarget      = target;
	action->mTargetFloor = action->mTarget->mSelectedFloorIndex;
}

s32 GetSelectedFloorIndex(const XObj* object)
{
	return object->mSelectedFloorIndex;
}

TVec* GetObjectMoveRotation(CObj* object)
{
	return &object->mMoveRotate;
}

void SetObjectYAndTarget(CObjVec* value, f32 y)
{
	value->mValue.y  = y;
	value->mTarget.y = y;
}

void CopyVecToCVec(Vec* dst, const Vec* src)
{
	dst->x = src->x;
	dst->y = src->y;
	dst->z = src->z;
}

BOOL IsPlayerRingMenuOpen(const XObjChrMenu* menu)
{
	return menu->IsOpen();
}

s32 IsGameMenuActive(const CGame* game)
{
	return game->mMenu.mActive;
}

BOOL IsTumbleActionState(const XAction* act, s32 wanted)
{
	return (u8)(wanted == act->mState);
}

BOOL IsEventScriptActive(const CGame* game)
{
	return game->mEvt.mData.mScript != NULL;
}

// how far the control stick is pushed
f32 GetControlStickMagnitude(const CPadOne* pad)
{
	f32 x = pad->mStick.x;
	f32 y = pad->mStick.y;
	return FSqrt(x * x + y * y);
}

// 0 the stage he is on, 1 the one before
s32 fn_800D168C(const CGame* game, s32 index)
{
	return (&game->mStage)[index];
}

u32 GetMaskedPlayerStatus(const WiiPlayerState* chr, u32 mask)
{
	return mask & chr->mPlayerFlags;
}

u32 GetMaskedPlayerStatusFlags(const WiiPlayerState* chr, u32 mask)
{
	return mask & chr->mPlayerFlags;
}

BOOL IsHangActionState(const XAction* act, s32 wanted)
{
	return (u8)(wanted == act->mState);
}

BOOL IsJumpActionState(const XAction* action, s32 state)
{
	return state == action->mState;
}

BOOL IsLadderActionState(const XAction* action, s32 state)
{
	return state == action->mState;
}

BOOL IsRopeActionState(const XAction* action, s32 state)
{
	return state == action->mState;
}

BOOL IsCopterActionState(const XAction* action, s32 state)
{
	return state == action->mState;
}

BOOL IsPopperActionState(const XAction* act, s32 wanted)
{
	return (u8)(wanted == act->mState);
}

BOOL IsRadarActionState(const XAction* act, s32 wanted)
{
	return (u8)(wanted == act->mState);
}

BOOL fn_800D1748(const WiiPlayerState* chr)
{
	return chr->_1C9B8[2] > 0.0f;
}

f32 fn_800D1764(const WiiPlayerState* chr, s32 index)
{
	return chr->_1C9B8[index];
}

f32 WrapPlayerActionAngle(f32 angle)
{
	angle = WrapAngle(angle);
	return angle;
}

f32 GetVectorDistance(const Vec* a, const Vec* b)
{
	return PSVECDistance((Vec*)a, (Vec*)b);
}

// moves what he carries along and queues its model for the scene
void fn_800D1A8C(WiiPlayerState* player, u32 frames)
{
	if (player->mLift.mTarget != NULL) {
		UpdateHeldObject(&player->mLift, frames);
		if (player->mLift.mTarget != NULL) {
			gGame.mGs.mScene.mHead->vf034(gGame.mGs.mScene.mHead->mPending, player->mLift.mTarget->mObject.m_pScnMdl);
		}
	}
}

// the same for what he pushes
void fn_800D1AF8(WiiPlayerState* player, u32 frames)
{
	if (player->mPush.mTarget != NULL) {
		UpdatePushedObject(&player->mPush, frames);
		if (player->mPush.mTarget != NULL) {
			gGame.mGs.mScene.mHead->vf034(gGame.mGs.mScene.mHead->mPending, player->mPush.mTarget->mObject.m_pScnMdl);
		}
	}
}

// the plug and its cord wire, queued for the scene with it
void fn_800D1B64(WiiPlayerState* player, u32 frames)
{
	player->mPlug.UpdateObject(frames);
	gGame.mGs.mScene.mHead->vf034(gGame.mGs.mScene.mHead->mPending, player->mPlug.mObject.m_pScnMdl);
	gGame.mGs.mScene.mHead->vf034(gGame.mGs.mScene.mHead->mPending, player->mCordWire.mObject.m_pScnMdl);
}

// the extra model of suits 3 and 6 while he wears one, queued for the scene
void fn_800D1BF0(WiiPlayerState* player, u32 frames)
{
	if (IsObjectActive(&player->mSuits.mObjects[player->mSuits.mCurrent])) {
		s32 suit = player->mSuits.mCurrent;
		if (suit == 3 || suit == 6) {
			player->mSuitParts[suit].UpdateObject(frames);
			gGame.mGs.mScene.mHead->vf034(gGame.mGs.mScene.mHead->mPending, player->mSuitParts[player->mSuits.mCurrent].mObject.m_pScnMdl);
		}
	}
}

void UpdatePlayerHeadOffset(WiiPlayerState* chr, u32 frames)
{
	if (chr->mHeadOffsetReady == 0 && gGame.mEvt.mData.mScript == NULL)
		chr->mHeadOffsetReady = 1;
	f32 current, target, rate;
	if (IsPushActionState(&chr->mPush, 1) || IsPushActionState(&chr->mPush, 2)) {
		if (chr->mPush.mMoveDirection == 1)
			target = 8.0f;
		else if (chr->mPush.mMoveDirection == -1)
			target = -5.0f;
		else
			target = 0.0f;
		rate = 0.4f;
	} else if (gGame.mEvt.mData.mScript != NULL && chr->mHeadOffsetReady != 0) {
		target = 0.0f;
		rate   = 0.2f;
	} else if (IsRopeState(chr, 1) || IsRopeState(chr, 2) || IsRopeState(chr, 3) || IsRopeState(chr, 4) || IsRopeState(chr, 5)
	           || IsRopeState(chr, 6) || IsRopeState(chr, 7) || IsRopeState(chr, 8)) {
		target = 0.0f;
		rate   = 0.2f;
	} else {
		target = 5.0f;
		rate   = 0.2f;
	}
	if (__fpclassifyf(target) != FP_NAN) {
		current = chr->mHeadOffset;
		if (current > target) {
			const f32 next   = current - rate * frames;
			chr->mHeadOffset = next;
			if (next < target)
				chr->mHeadOffset = target;
		} else if (current < target) {
			const f32 next   = current + rate * frames;
			chr->mHeadOffset = next;
			if (next > target)
				chr->mHeadOffset = target;
		}
	}
}

void SpawnPlayerPointParticles(WiiPlayerState*, s32 type, s32 value)
{
	for (s32 i = 0; i < 10; ++i) {
		if (gPointParticlePool[i].IsFree()) {
			for (s32 j = 0; j < 10; ++j) {
				if (gPointParticleSlots[j] == NULL) {
					gPointParticleSlots[j] = &gPointParticlePool[i];
					StartPointParticles(gPointParticleSlots[j], type, value);
					return;
				}
			}
			return;
		}
	}
}

// opens the rings unless an attachment or a suit is still changing
static inline void OpenPlayerEquipmentRings(WiiPlayerState* chr)
{
	if (chr->mAttachmentRequest != -1)
		return;
	if (chr->mSuits.mChangeStep != 0)
		return;
	if (!chr->mMenu.IsOpen()) {
		StopMoving(chr);
		OpenMenus(chr);
	} else if (chr->mMenu.IsClosing()) {
		OpenMenus(chr);
	}
}

// NONMATCHING (98.5 %): the mDrs.mOpen test is a branch here; retail materializes mOpen <= 0
// through cntlzw/rlwnm into the open flag (init TRUE). Tried &&/||/ternary/!/bool spellings,
// MWCC lowers each to branches or neg/andc/srwi. Also the rotation copy as in StopPlayerMotion.
void SelectPlayerEquipmentRing(WiiPlayerState* chr, s32 selection)
{
	if (selection >= 0) {
		chr->mMenu.mPrevSel = chr->mMenu.mSel;
		chr->mMenu.mSel     = selection;
	}
	if (chr->mMenu.IsOpening()) {
		FocusEquipmentRing(&chr->mMenu);
	} else {
		OpenPlayerEquipmentRings(chr);
	}
}

// closes both rings, 0 cancels, above 0 confirms, below 0 backs out
void ClosePlayerEquipmentMenu(WiiPlayerState* chr, s32 result)
{
	if (result == 0) {
		PlayWiiSoundEffect(&gGame.mSound, 0xF3, 1.0f, 0.0f);
		if (chr->mMenu.mAtc.mMenuType != 3)
			chr->mMenu.mSel = chr->mMenu.mPrevSel;
	} else if (result > 0) {
		PlayWiiSoundEffect(&gGame.mSound, 0xF5, 1.0f, 0.0f);
	} else if (result < 0) {
		PlayWiiSoundEffect(&gGame.mSound, 0xF3, 1.0f, 0.0f);
		if (chr->mMenu.mAtc.mMenuType != 3)
			chr->mMenu.mSel = chr->mMenu.mPrevSel;
	}
	chr->mMenu.mAtc.Select(result);
	chr->mMenu.mDrs.Select(result);
}

// opens the yes/no marks
void OpenPlayerYesNoMenu(WiiPlayerState* chr, s32 crossMask, s32 selectedIndex)
{
	if (!chr->mOXMenu.mStarted) {
		chr->mOXMenu.mStarted       = TRUE;
		chr->mOXMenu.mCrossMask     = crossMask;
		chr->mOXMenu.mStartRot      = chr->mOXMenu.mChr->mMoveRotate.y;
		chr->mOXMenu.mSelectedIndex = selectedIndex;
		SpinMarks(*chr);
		PlayWiiSoundEffect(&gGame.mSound, 0xF3, 1.0f, 0.0f);
		chr->mOXMenu.mChr->SetAnim(0x5D, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
		gGame.SetStatusFlags(0x800ULL);
	}
}

// starts an object's event script unless chibi robo lies there out of battery
// NONMATCHING: the event block check, as in IsPlayerBlockingEvents
void TryStartObjectEvent(CGame* game, const void* script, XObj* owner, s16 first, s16 second)
{
	if (!IsEventBlocked(game->GetPlayer())) {
		StartOrQueueEventScript(&game->mEvt, (u8*)script, owner, first, second);
	}
}

BOOL IsObjectLiftable(const XObj* object)
{
	return TestStatus(object, 0x0000000100000000ULL);
}

BOOL IsObjectClimbableLedge(const XObj* object)
{
	return TestStatus(object, 0x0000000200000000ULL);
}

BOOL IsObjectLadder(const XObj* object)
{
	return TestStatus(object, 0x0000000800000000ULL);
}

// is position behind the object, within 60 degrees either side
BOOL IsPositionBehindObject(void*, const Vec* pos, XObj* object)
{
	CVec* objectPos = object->GetPos();
	f32 angle       = atan2(objectPos->x - pos->x, objectPos->z - pos->z);
	angle           = WrapAngle(angle - object->mMoveRotate.y);
	return angle >= 2.0f * PI / 3.0f && angle <= 4.0f * PI / 3.0f;
}

BOOL IsObjectRope(const XObj* object)
{
	return TestStatus(object, 0x0000001000000000ULL);
}

// a fourth contact kind after ladder and rope, the GameCube's fn_800B1E04; it always says no
s32 fn_800D5680(void*)
{
	return 0;
}

BOOL IsObjectPushable(const XObj* object)
{
	return TestStatus(object, 0x0000008000000000ULL);
}

BOOL IsObjectPushableFromBack(const XObj*)
{
	return false;
}

// the heading out of the object across one floor's edge, from point
// NONMATCHING (86 %): retail reads point->y ahead of the object's position, the rest is the same
f32 GetObjectEdgeOutwardYaw(const XObj* object, Vec* point, s32 index)
{
	f32 height = point->y;
	Vec first  = FirstEdgePoint(object, index, height);
	Vec second = SecondEdgePoint(object, index, height);
	CVec direction;
	GetOutwardPerpendicularToEdge(point, &first, &second, &direction);
	f32 angle = atan2(direction.x, direction.z);
	angle     = WrapAngle(angle);
	return angle;
}

CVec* GetObjectPreviousMoveTranslation(XObj* object)
{
	return &object->mLastPos;
}

void SetObjectVectorTargetX(CObjVec* value, f32 x)
{
	value->mTarget.x = x;
}

void SetObjectVectorTargetZ(CObjVec* value, f32 z)
{
	value->mTarget.z = z;
}

f32 GetVectorMagnitude(const Vec* value)
{
	return PSVECMag((Vec*)value);
}

Vec* GetObjectVectorCurrent(CObjVec* value)
{
	return &value->mValue;
}

void SetObjectXAndTarget(CObjVec* value, f32 x)
{
	value->mValue.x  = x;
	value->mTarget.x = x;
}

void SetObjectZAndTarget(CObjVec* value, f32 z)
{
	value->mValue.z  = z;
	value->mTarget.z = z;
}

void* GetMapPoolEntry(const CGame* game, s32 index)
{
	s32 stride  = game->mXObjs.mPools[3].mStride;
	u8* entries = game->mXObjs.mPools[3].mObjects;
	return entries + index * stride;
}

BOOL CanFallDuringHangAction(const XActHang* act)
{
	return (act->mState == 0 || act->mState == 10);
}

BOOL CanFallDuringJumpAction(const XAction* act)
{
	return IsJumpActionState(act, 0) || IsJumpActionState(act, 2) || IsJumpActionState(act, 3) || IsJumpActionState(act, 4)
	    || IsJumpActionState(act, 17);
}

BOOL CanFallDuringCopterAction(const XAction* act)
{
	return IsCopterActionState(act, 0) || IsCopterActionState(act, 3) || IsCopterActionState(act, 4) || IsCopterActionState(act, 7)
	    || IsCopterActionState(act, 8) || IsCopterActionState(act, 5) || IsCopterActionState(act, 6) || IsCopterActionState(act, 9);
}

u32 CheckCurrentStageFlags(const CGame* game, u32 mask)
{
	return CheckStageFlag(game->mStage, mask);
}

// the object id of the placement, -1 without one, as XObj::GetKind reads it
s32 fn_800DBD44(const AttrInfo* attr)
{
	return attr->mPlacement != NULL ? attr->mPlacement->mObjectId : -1;
}

AttrInfo* fn_800DBD60(XObj* object)
{
	return &object->mAttr;
}

BOOL fn_800DBD68(const XObj* object)
{
	return (object->mStatus.mFlags & 0x10001) == 0x10001;
}

// the map floor under pos, the search starting where the last hit was
BOOL FindMapFloorBelow(XObjMap* map, const Vec* pos, const XObjMapHit* last, XObjMapHit* hit)
{
	return fn_800C98E0(map, pos, last->mTriangle, hit);
}

// how far apart two positions are, seen from above
f32 GetHorizontalDistance(const Vec* a, const Vec* b)
{
	f32 dx = a->x - b->x;
	f32 dz = a->z - b->z;
	return FSqrt(dx * dx + dz * dz);
}

f32 GetObjectFloorXZExtent(const XObj* object, s32 index)
{
	return object->mFloorXZExtent[index];
}

BOOL HasObjectCustomLedgeHeight(const XObj* object)
{
	return TestStatus(object, 0x0000002000000000ULL);
}

s32 GetStageObjectCount(const CGame* game)
{
	return game->mXObjs.mObjectCount;
}

f32 GetHeadingBetweenPositions(const Vec* from, const Vec* to)
{
	return atan2(to->x - from->x, to->z - from->z);
}

u32 GetPlayerActionMoveDirection(const XAction* action)
{
	return action->mMoveDirection;
}

void SetObjectVectorAndTargetXYZ(CObjVec* value, f32 x, f32 y, f32 z)
{
	value->mValue.x  = x;
	value->mValue.y  = y;
	value->mValue.z  = z;
	value->mTarget.x = x;
	value->mTarget.y = y;
	value->mTarget.z = z;
}

void SetObjectVelocity(CObjVec* value, f32 x, f32 y, f32 z)
{
	value->mSpeed.Set(x, y, z);
}

void ResetPlayerHeadOffset(WiiPlayerState* chr)
{
	chr->mHeadOffset      = 5.0f;
	chr->mHeadOffsetReady = 0;
}

BOOL IsPlayerActionMoveDirection(const XAction* action, s32 wanted)
{
	return action->mMoveDirection == wanted;
}

f32 GetObjectMaxFloorHeight(XObj* object)
{
	return object->mMaxFloorHeight;
}

void ReleasePlayerLiftTarget(WiiPlayerState* chr)
{
	XObj* target = chr->mLift.mTarget;
	if (target != NULL) {
		target->mStatus.mFlags &= ~0x0400000000000000ULL;
		chr->mLift.mTarget = NULL;
	}
	chr->mLift.mState = 0;
}

f32 GetSelectedFloorHeight(const XObj* object)
{
	return object->mSelectedFloorHeight;
}

void SetHangActionState(XAction* act, s32 state)
{
	act->mState = state;
}

void SetPushTargetState(XAction* act, s32 value)
{
	act->mTargetState = value;
}

void SetCopterActionState(XAction* action, s32 state)
{
	action->mState = state;
}

void SetObjectVectorTarget(CObjVec* value, const Vec& target)
{
	value->mTarget.Set(target);
}

f32 PlayerActionSine(f32 angle)
{
	return sin(angle);
}

f32 GetPlayerMoveSpeedScale(const WiiPlayerState* chr)
{
	return chr->mMoveSpeedScale;
}

f32 PlayerActionCosine(f32 angle)
{
	return cos(angle);
}

void SetVectorX(CVec* value, f32 x)
{
	value->x = x;
}

void SetVectorZ(CVec* value, f32 z)
{
	value->z = z;
}

CVec* GetObjectVectorSpeed(CObjVec* value)
{
	return &value->mSpeed;
}

void SetObjectVectorSpeedX(CObjVec* value, f32 x)
{
	value->mSpeed.x = x;
}

void SetObjectVectorSpeedZ(CObjVec* value, f32 z)
{
	value->mSpeed.z = z;
}

Vec* GetHangFloorCenter(XActHang* act)
{
	return &act->mFloorCenter;
}

f32 GetHangDistance(const XActHang* act)
{
	return act->mDistance;
}

void SetTumbleStateAndFade(WiiPlayerState* chr, s32 state, BOOL restart)
{
	chr->mTumble.mState = state;
	if (restart) {
		chr->mTumble.mFade.mActive = TRUE;
		chr->mTumble.mFade.mAmount = 0.0f;
	}
}

XObjFloor** GetObjectFloorTableAddress(XObj* object)
{
	return &object->mFloors;
}

void SetObjectVectorAndTargetFromVec(CObjVec* value, const Vec& v)
{
	f32 z = v.z, y = v.y, x = v.x;
	value->mValue.x  = x;
	value->mValue.y  = y;
	value->mValue.z  = z;
	value->mTarget.x = x;
	value->mTarget.y = y;
	value->mTarget.z = z;
}

void SetObjectVerticalSpeed(CObjVec* value, f32 y)
{
	value->mSpeed.y = y;
}

BOOL CanFallDuringLadderAction(const XAction* act)
{
	return IsLadderActionState(act, 0) || IsLadderActionState(act, 10) || IsLadderActionState(act, 11);
}

BOOL CanFallDuringRopeAction(const XAction* act)
{
	return IsRopeActionState(act, 0) || IsRopeActionState(act, 9) || IsRopeActionState(act, 10) || IsRopeActionState(act, 11);
}

BOOL CanFallDuringTumbleAction(const XAction* act)
{
	return !(act->mState == 8 || act->mState == 9);
}

BOOL CanFallDuringPopperAction(const XAction*)
{
	return true;
}

BOOL CanFallDuringRadarAction(const XAction*)
{
	return true;
}

CVec* NormalizeVectorInPlace(CVec* value)
{
	PSVECNormalize(value, value);
	return value;
}

BOOL CanLiftActionSlideOnSlope(const XAction*)
{
	return true;
}

BOOL IsHangActionIdle(const XAction* act)
{
	return (u8)(act->mState == 0);
}

BOOL CanJumpActionSlideOnSlope(const XAction* act)
{
	return act->mState == 0 || act->mState == 4;
}

BOOL CanLadderActionSlideOnSlope(const XAction* act)
{
	return act->mState == 0 || act->mState == 11;
}

BOOL CanRopeActionSlideOnSlope(const XAction* act)
{
	return act->mState == 0 || act->mState == 10;
}

BOOL CanPushActionSlideOnSlope(const XAction*)
{
	return true;
}

BOOL CanTumbleActionSlideOnSlope(const XAction*)
{
	return true;
}

BOOL CanCopterActionSlideOnSlope(const XAction* act)
{
	return IsCopterActionState(act, 0) || IsCopterActionState(act, 6) || IsCopterActionState(act, 9);
}

BOOL CanPopperActionSlideOnSlope(const XAction*)
{
	return true;
}

BOOL CanRadarActionSlideOnSlope(const XAction*)
{
	return true;
}

Vec GetFloorExtent(XObjFloor* const* floors, s32 index)
{
	Vec extent = { (*floors)[index].mExtent[0] / 100.0f, (*floors)[index].mExtent[1] / 100.0f, (*floors)[index].mExtent[2] / 100.0f };
	return extent;
}

Vec GetFloorCenterOffset(XObjFloor* const* floors, s32 index)
{
	Vec offset = { (*floors)[index].mCenterOffset[0] / 100.0f, (*floors)[index].mCenterOffset[1] / 100.0f,
		           (*floors)[index].mCenterOffset[2] / 100.0f };
	return offset;
}

XObj* GetDrawListObject(const CGame* game, s32 list, s32 index)
{
	return (XObj*)game->mDrawLists[list].mRefs[index].mObj;
}

s32 GetDrawListIndex(const CGame* game, s32 list, s32 index)
{
	return game->mDrawLists[list].mRefs[index].mIndex;
}

s32 GetDrawListCount(const CGame* game, s32 list)
{
	return game->mDrawLists[list].mCount;
}

// the direction square to bc pointing from a toward the line, so out of the triangle abc
void GetOutwardPerpendicularToEdge(Vec* a, Vec* b, Vec* c, Vec* out)
{
	Vec bc;
	Vec ba;
	Vec n;
	PSVECSubtract(b, c, &bc);
	PSVECSubtract(b, a, &ba);
	PSVECCrossProduct(&ba, &bc, &n);
	PSVECCrossProduct(&bc, &n, out);
}

f32 GetFloorYawRadians(XObjFloor* const* floors, s32 index)
{
	return DEG2RAD * (*floors)[index].mYawDegrees;
}

f32 GetObjectHitFloorHeight(const XObj* object)
{
	return object->mFloorHeights[object->mHitFloorIndex];
}

s32 GetActionTargetFloor(const XAction* action)
{
	return action->mTargetFloor;
}

BOOL CanFallDuringPushAction(const XAction*)
{
	return true;
}

void SetObjectTargetY(CObjVec* value, f32 y)
{
	value->mTarget.y = y;
}

f32 GetCopterDescentAcceleration()
{
	return (s32)GetCopterParameter(1) / 100.0f;
}

f32 GetCopterDescentSpeedLimit()
{
	return (s32)GetCopterParameter(2) / 100.0f;
}

s32 GetObjectPoolUsedCount(const CGame* game, s32 index)
{
	return game->mXObjs.mPools[index].mUsed;
}

BOOL IsPlayerLookMarkerVisible(const XObjChrLook* look)
{
	return look->mFade > 0.0f;
}

void DisablePlayerLook(XObjChrLook* look)
{
	look->mActive = FALSE;
}

void EnablePlayerLook(XObjChrLook* look)
{
	look->mActive = TRUE;
	look->_30     = TRUE;
}

void SetPlayerLookMode(XObjChrLook* look, s32 mode)
{
	look->mMode = mode;
}

void SetLookTargetPosition(XObjChrLook* look, f32 x, f32 y, f32 z)
{
	Vec target;
	target.x      = x;
	target.y      = y;
	target.z      = z;
	look->mTarget = target;
}

void fn_800DBF70(XObjChrLook* look)
{
	look->_30 = FALSE;
}

BOOL IsObjectClimbableFromHang(XObj* object)
{
	return (u8)((object->mStatus.mFlags & 0x0000000400000000ULL) != 0);
}

f32 GetAutoActionHoldTime()
{
	return (f32)(s32)GetInteractionParameter(0) / 100.0f;
}

f32 GetAutoActionChargeRate()
{
	return (f32)(s32)GetInteractionParameter(1) / 100.0f;
}

f32 GetAutoActionSoftTiltCap()
{
	return (f32)(s32)GetInteractionParameter(2) / 100.0f;
}

void SetPlayerActionState(XAction* action, s32 state)
{
	action->mState = state;
}

void SetPlayerHangState(WiiPlayerState* chr, s32 state, XObj* target)
{
	SetPlayerActionState(&chr->mHang, state);
	if (target != NULL) {
		SetPlayerActionTarget(&chr->mHang, target);
	}
}

void SetPlayerLadderState(WiiPlayerState* chr, s32 state, XObj* target)
{
	SetPlayerActionState(&chr->mLadder, state);
	if (target != NULL) {
		SetPlayerActionTarget(&chr->mLadder, target);
	}
}

void SetPlayerRopeState(WiiPlayerState* chr, s32 state, XObj* target)
{
	SetPlayerActionState(&chr->mRope, state);
	if (target != NULL) {
		SetPlayerActionTarget(&chr->mRope, target);
	}
}

XObj* GetGameObjectByIndex(const CGame* game, s32 index)
{
	return game->mXObjs.mObjects[index];
}

CVec* GetCameraAngles(CCamera* camera)
{
	return &camera->mView.mAngle;
}

CPadOne* GetPadForPort(CGame* game, u8 port)
{
	return &gWiiPads[port];
}

u64 GetHeldPadButtons(const CPadOne* pad, u64 mask)
{
	return pad->mButton & mask;
}

BOOL IsActionTargetFloorId(const XAction* action, s32 wanted)
{
	return action->mTarget->mFloors[action->mTargetFloor].mId == wanted;
}

void* GetObjectFromPool(const CGame* game, s32 pool, s32 index)
{
	return game->mXObjs.mPools[pool].Get(index);
}

UnkStageObj* GetIndexedCoordPlacement(const CoordInfo* info, s32 index)
{
	return info->mPlacement + index;
}

CoordInfo* GetObjectCoordInfo(XObj* object)
{
	return &object->mCoord;
}

// the eyelids: in the anims that allow it he blinks after a random wait, stepping the lids two frames at a time
void fn_800DDF2C(WiiPlayerState* chr, s32 frames)
{
	if (chr->mBlinkAnims[chr->GetAnimId() % 1000]) {
		if (chr->mBlinkWait > 0) {
			if ((chr->mBlinkWait -= frames) < 0)
				chr->mBlinkWait = 0;
		}
		if (chr->mBlinkWait == 0) {
			if (chr->mBlinkFrame < 0)
				chr->mBlinkFrame = 0;
			else
				chr->mBlinkFrame += 2;
			if (chr->mBlinkFrame > 10) {
				s32 wait         = 0.5f * gRefreshRate + rand() % gRefreshRate;
				chr->mBlinkFrame = -1;
				chr->mBlinkWait  = wait;
			}
		}
	} else {
		s32 wait         = 0.5f * gRefreshRate + rand() % gRefreshRate;
		chr->mBlinkFrame = -1;
		chr->mBlinkWait  = wait;
	}
}

// fires on the frame it crosses mStart
s32 FrameSoundOnStartFrame(XObj* object, FrameEventTime* time, const FrameEvent* event)
{
	if ((time->mLastFrame < event->mStart || time->mLastFrame > time->mFrame) && time->mFrame >= event->mStart)
		return 1;
	return 0;
}

// fires on the frame it crosses mStart or mEnd, a loop back counts as crossing
s32 FrameSoundOnEitherBoundary(XObj* object, FrameEventTime* time, const FrameEvent* event)
{
	if (((time->mLastFrame < event->mStart || time->mLastFrame > time->mFrame) && time->mFrame >= event->mStart)
	    || ((time->mLastFrame < event->mEnd || time->mLastFrame > time->mFrame) && time->mFrame >= event->mEnd))
		return 1;
	return 0;
}

// keep the sound playing until the anim ends
s32 FrameSoundUntilAnimEnd(XObj* object, FrameEventTime* time, const FrameEvent* event)
{
	if (object->IsAnimDone()) {
		if (IsWiiSfxPlaying(&gGame.mSound, event->mId))
			return -1;
	} else {
		if (!IsWiiSfxPlaying(&gGame.mSound, event->mId))
			return 1;
	}
	return 0;
}

// keep the sound playing inside the window only
s32 FrameSoundInFrameWindow(XObj* object, FrameEventTime* time, const FrameEvent* event)
{
	if (time->mFrame >= event->mStart && time->mFrame < event->mEnd) {
		if (!IsWiiSfxPlaying(&gGame.mSound, event->mId))
			return 1;
	} else {
		if (IsWiiSfxPlaying(&gGame.mSound, event->mId))
			return -1;
	}
	return 0;
}

// start while the sound isn't playing
s32 FrameSoundStartIfSilent(XObj* object, FrameEventTime* time, const FrameEvent* event)
{
	return IsWiiSfxPlaying(&gGame.mSound, event->mId) ? FALSE : TRUE;
}

// stop once the sound is playing
s32 FrameSoundStopIfPlaying(XObj* object, FrameEventTime* time, const FrameEvent* event)
{
	return IsWiiSfxPlaying(&gGame.mSound, event->mId) ? -1 : 0;
}

// the anim rate at the game's frame rate, 1.135 times faster in lift state 4 except for anim 0x13B
// NONMATCHING: retail reads mLift.mState ahead of the prologue's stores
f32 fn_800E32B8(WiiPlayerState* chr, s32 anim)
{
	f32 rate;
	if (chr->mLift.mState == 4 && IsLiftTargetPlug(&chr->mLift) && anim != 0x13B) {
		rate = 1.135f;
	} else {
		rate = 1.0f;
	}
	return rate * (60.0f / gRefreshRate);
}

// sets every anim of a model to frame
static inline void SetModelFrame(XObj* object, f32 frame)
{
	for (int i = 0; i < 6; i++) {
		nw4r::g3d::AnmObj* anm = object->mObject.m_pScnMdl->GetAnmObj((nw4r::g3d::ScnMdlSimple::AnmObjType)i);
		if (anm != NULL) {
			anm->SetFrame(frame);
		}
	}
}

// the anim rate scale of fn_800E32B8 shared with fn_800E3968
static inline f32 PlayerAnimRateScale(WiiPlayerState* chr, s32 anim)
{
	f32 rate;
	if (chr->mLift.mState == 4 && IsLiftTargetPlug(&chr->mLift) && anim != 0x13B) {
		rate = 1.135f;
	} else {
		rate = 1.0f;
	}
	return rate * (60.0f / gRefreshRate);
}

// scales his own anim rate and the worn suit's by frame
void fn_800E3968(WiiPlayerState* chr, f32 frame)
{
	chr->CObj::SetAnimRate(frame * PlayerAnimRateScale(chr, chr->GetAnimId()));
	if ((chr->mSuits.mObjects[chr->mSuits.mCurrent].mStatus.mFlags & 1) == TRUE) {
		chr->mSuits.mObjects[chr->mSuits.mCurrent].SetAnimRate(frame * PlayerAnimRateScale(chr, chr->GetAnimId()));
	}
}

// his anims and the worn suit's to frame
void fn_800E3AE4(WiiPlayerState* chr, f32 frame)
{
	SetModelFrame(chr, frame);
	if ((chr->mSuits.mObjects[chr->mSuits.mCurrent].mStatus.mFlags & 1) == TRUE) {
		SetModelFrame(&chr->mSuits.mObjects[chr->mSuits.mCurrent], frame);
	}
}

// the eye camera takes and frees his motion: the flag the draw checks, and the worn suit's vf28C
extern "C" void fn_800F3E9C(WiiPlayerState* chr, BOOL held);
void fn_800E3BCC(WiiPlayerState* chr, s32 held)
{
	chr->m_9C8 = held != 0;
	fn_800F3E9C(chr, held != 0);
	WiiSuitObject* suit = &chr->mSuits.mObjects[chr->mSuits.mCurrent];
	if ((suit->mStatus.mFlags & 1) == TRUE) {
		suit->vf28C(held);
	}
}

// the same for the popper mount while the popper is out
extern "C" void fn_800F403C(WiiPlayerState* chr, BOOL on);
void fn_800E3C50(WiiPlayerState* chr, s32 on)
{
	chr->m_9C9 = on != 0;
	fn_800F403C(chr, on != 0);
	WiiSuitObject* suit = &chr->mSuits.mObjects[chr->mSuits.mCurrent];
	if ((suit->mStatus.mFlags & 1) == TRUE) {
		suit->vf290(on);
	}
}

// the carry joint's matrix: the worn suit's while he carries at it
// NONMATCHING: only the name's place differs, retail pools it in the file's .rodata string table
MtxP fn_800E59DC(WiiPlayerState* chr)
{
	XObj* object;
	if (chr->_0AF8 != 0) {
		object = chr;
	} else if (IsObjectActive(&chr->mSuits.mObjects[chr->mSuits.mCurrent])) {
		object = &chr->mSuits.mObjects[chr->mSuits.mCurrent];
	} else {
		object = chr;
	}
	return object->GetWorldMtx("carry_obj_nll");
}

// where the carried object hangs, in the world
// NONMATCHING: only the name's place differs, retail pools it in the file's .rodata string table
CVec fn_800E5A2C(XObj* player)
{
	return player->GetWorldPos("carry_obj_nll");
}

// and relative to its joint
// NONMATCHING: as above, the name's place
CVec fn_800E5A48(XObj* player)
{
	return player->GetJointTranslate("carry_obj_nll");
}

// XObjChr::SetMoveDir, stops whatever push is running and keeps the heading he walks towards
// NONMATCHING: only the pool label names and the order of the two argument loads for atan2
void XObjChr::SetMoveDir(const Vec* direction, f32 speed)
{
	WiiPlayerState* chr = (WiiPlayerState*)this;
	StopMoving(chr);
	CObj::SetMoveDir(direction, speed);
	CVec horizontal = chr->mMoveSpeed0;
	horizontal.y    = 0.0f;
	if (PSVECMag((Vec*)&horizontal) > 0.0f) {
		chr->mWalkHeading = WrapAngle(GetYAngle(chr->mMoveTarget0 - chr->mMoveTranslate));
	}
}

// how high the carried object sits: half again his scale, and with a mug the carry joint's height on top
// except while the anims 10 and 11 lift and lower it
// NONMATCHING: the joint name's place, as for fn_800E5A2C
f32 fn_800E5BFC(WiiPlayerState* chr)
{
	f32 height = 1.5f * chr->mMoveScale.x;
	if (IsCarryingMug(&chr->mLift) && !(chr->GetAnimId() == 11 && chr->GetAnimFrame() < 45.0f)
	    && !(chr->GetAnimId() == 10 && chr->GetAnimFrame() > 22.0f)) {
		CVec scale;
		chr->GetJointScale(chr->FindResNode("carry_obj_nll"), &scale);
		height += 4.0f * scale.x;
	}
	return height;
}

// XObjChr::SetColor, the color goes to everything he wears and carries too. while a costume
// change is running, blend 2 turns into 1 and 1 is dropped; the Wii also tints the two
// extra parts suits 3 and 6 bring
void XObjChr::SetColor(s32 blend, u8 red, u8 green, u8 blue, f32 time)
{
	WiiPlayerState* chr = (WiiPlayerState*)this;
	if (chr->mSuits.mChangeStep != 0) {
		if (blend == 2) {
			blend = 1;
		} else if (blend == 1) {
			return;
		}
	}
	XObj::SetColor(blend, red, green, blue, time);
	chr->mPlug.SetColor(blend, red, green, blue, time);
	((XObj*)chr->mCopterObj)->SetColor(blend, red, green, blue, time);
	((XObj*)chr->mPopperObj)->SetColor(blend, red, green, blue, time);
	((XObj*)chr->mRadarObj)->SetColor(blend, red, green, blue, time);
	((XObj*)chr->mOXObj)->SetColor(blend, red, green, blue, time);
	for (s32 i = 0; i < 9; ++i) {
		chr->mSuits.mObjects[i].SetColor(blend, red, green, blue, time);
		if (i == 3 || i == 6) {
			chr->mSuitParts[i].SetColor(blend, red, green, blue, time);
		}
	}
	if (IsCarryingMug(&chr->mLift)) {
		chr->mLift.mTarget->SetColor(blend, red, green, blue, time);
	}
}

// between his eyes, a little behind them
// NONMATCHING: the joint names' place, as for fn_800E5A2C
Vec GetPlayerFirstPersonEyePosition(WiiPlayerState* chr)
{
	CVec first  = chr->GetJointPos("L_eye");
	CVec second = chr->GetJointPos("R_eye");
	Vec point   = first + (second - first) / 2.0f;
	point.x -= 1.5f * std::sinf(chr->mMoveRotate.y);
	point.z -= 1.5f * std::cosf(chr->mMoveRotate.y);
	return point;
}

// NONMATCHING (92 %): retail keeps the held flag apart from the down flag, this build folds the two
BOOL IsPlayerBlockingEvents(const WiiPlayerState* player)
{
	return IsEventBlocked(player);
}

// resets the plug, keeping its rotation while it's carried
void ResetPlayerPlug(WiiPlayerState* chr)
{
	if (chr->mPlug.mStatus.mFlags & 0x0400000000000000ULL) {
		Vec rotation = (Vec&)chr->mPlug.mMoveRotate;
		chr->mPlug.mStatus.mFlags &= ~0x0400000000000000ULL;
		chr->mPlug.ResetObjectState();
		chr->mPlug.UpdateObject(10);
		chr->mPlug.SetRot(rotation);
		chr->mPlug.mStatus.mFlags |= 0x0400000000000000ULL;
	} else {
		chr->mPlug.ResetObjectState();
		chr->mPlug.UpdateObject(10);
	}
}

// the event call 101: which side of a clear leticker he comes from, 1 for anything else
// NONMATCHING: retail loads the data record ahead of the prologue's stores
s32 GetCharacterObjectApproachResult(XObj* chr, XObj* object)
{
	return object->GetPool() == 9 ? GetClearLetickerApproachSide(object, chr->GetPos()) : 1;
}

// the highest floor of the object under a floor group that's still below chibi robo,
// stops early on the preferred one, -1 for none. The contact flag is read off the group's own entry
// NONMATCHING (94 %): retail recomputes owner + group inside the loop, this build hoists it
s32 FindPlayerSupportingFloor(WiiPlayerState* chr, s32 group, s32 preferred)
{
	s32 result = -1;
	if (chr->mFloorContactObjects[group] >= 0) {
		XObj* owner = gGame.mXObjs.mObjects[chr->mFloorContactObjects[group]];
		f32 best    = NAN;
		for (int i = 0; owner->mFloors[i].mId != -1; i++) {
			if ((owner->mFloors[group].mFlags & 1) && (owner->mFloorMasks[group] & (1 << i))) {
				if ((fpclassify(best) == FP_NAN || owner->mFloorHeights[i] >= best)
				    && owner->mFloorHeights[i] <= 1.0f + chr->mMoveTranslate.y) {
					best   = owner->mFloorHeights[i];
					result = i;
					if (i == preferred)
						break;
				}
			}
		}
	}
	return result;
}

// the ground type under chibi robo, from what he hangs or climbs on, the object he stands
// on, or else the map triangle under him. 13 and up count as none
// NONMATCHING (95 %): retail keeps the u8 conversion as a clrlwi. where this build tests the lbz result directly
s32 GetPlayerGroundType(WiiPlayerState* chr)
{
	s32 type = 0;
	if (chr->mHang.mState == 5 && chr->mHang.mTarget != NULL) {
		XObj* owner = chr->mHang.mTarget;
		type        = owner->mFloors->mMaterial;
	} else if (chr->mRope.mState == 5 && chr->mRope.mTarget != NULL) {
		XObj* owner = chr->mRope.mTarget;
		type        = owner->mFloors->mMaterial;
	} else if (chr->mLadder.mState == 5 && chr->mLadder.mTarget != NULL) {
		XObj* owner = chr->mLadder.mTarget;
		type        = owner->mFloors->mMaterial;
	} else if (chr->mFloorContactObjects[0] >= 0) {
		XObj* owner = gGame.mXObjs.mObjects[chr->mFloorContactObjects[0]];
		f32 best    = NAN;
		for (s32 i = 0; owner->mFloors[i].mId != -1; ++i) {
			if ((owner->mFloors[i].mFlags & 1) && (owner->mFloorMasks[0] & (1U << i))
			    && (__fpclassifyf(best) == FP_NAN || owner->mFloorHeights[i] > best)) {
				f32 height = owner->mFloorHeights[i];
				if (height <= 1.0f + chr->mMoveTranslate.y) {
					best = height;
					type = owner->mFloors[i].mMaterial;
				}
			}
		}
	}
	type = (u8)type;
	if (type == 0 && chr->mFloorHit.mTriangle >= 0 && chr->mFloorHit.mTrigon >= 0) {
		type = GetMapTriangleFloorType((XObjMap*)gGame.mXObjs.mPools[3].mObjects, chr->mFloorHit.mTriangle);
	}
	if (type >= 13)
		type = 0;
	return type;
}

void SetAttachmentCountById(AttachmentValues* values, s32 id, s32 value)
{
	values->SetCount(id, value);
}

// out of battery: the battery is flat and the gauge on screen has run down to it
BOOL IsPlayerStopped(const WiiPlayerState* chr)
{
	return IsStopped(chr);
}

u8 IsPlayerFlashingFromHit(WiiPlayerState* chr)
{
	return chr->mFlashTimer > 0;
}

// hit, flinches when he's free to and starts the flash timer. The Wii adds two carry states he may flinch
// from and flinches only while the player status allows it
void ApplyPlayerHitReaction(WiiPlayerState* chr, BOOL start, BOOL cancel, f32 value)
{
	if (!gGame.GetStatusFlags(0xC00ULL) && chr->mAttachmentRequest < 0 && gGame.mEvt.mData.mScript == NULL) {
		if (start && (chr->mLift.mState == 0 || chr->mLift.mState == 4 || chr->mLift.mState == 29 || chr->mLift.mState == 31)
		    && chr->mHang.mState == 0 && chr->mJump.mState == 0 && chr->mLadder.mState == 0 && chr->mRope.mState == 0
		    && chr->mPush.mState == 0 && chr->mTumble.mState == 0 && chr->mCopter.mState == 0
		    && (chr->mPopper.mState == 0 || chr->mPopper.mState == 4 || chr->mPopper.mState == 8) && chr->mRadar.mState == 0) {
			if (chr->mLift.mState == 0)
				chr->SetAnim(0xD4, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
			else
				chr->SetAnim(0x94, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
			if (chr->mPlayerFlags & 0x20000000)
				chr->mTumble.mState = 8;
		} else if (cancel) {
			if (chr->mHang.mState == 5)
				chr->mHang.mState = 8;
			else if (chr->mLadder.mState == 5)
				chr->mLadder.mState = 8;
			else if (chr->mRope.mState == 5)
				chr->mRope.mState = 7;
		}
	}
	if (0.0f != value) {
		chr->mFlashTimer  = gRefreshRate;
		chr->mFlashCount  = 0;
		chr->mHitStrength = value;
	}
}

// XObjChr::OnHitFromPosition, hit from a position. flashes what he carries instead when he lifts something
void XObjChr::OnHitFromPosition(const Vec&, f32 strength)
{
	WiiPlayerState* chr = (WiiPlayerState*)this;
	if (IsCarryingMug(&chr->mLift)) {
		chr->mLift.mTarget->SetColor(1, 150, 150, 0, 0.0f);
		chr->mLift.mTarget->SetColor(1, 0, 0, 0, 0.2f);
	} else if (chr->mFlashTimer <= 0) {
		ApplyPlayerHitReaction(chr, TRUE, TRUE, strength);
	}
}

f32 fn_800D1740(f32 x)
{
	return __fabsf(x);
}

// nonzero while the plug sits in a socket
s32 GetPlayerPlugConnection(const XObjPlug* plug)
{
	return plug->mConnection;
}

// the cord's pull on the player while the plug is connected
CVec* GetPlayerPlugCordPull(XObjPlug* plug)
{
	return &plug->mCord.mPull;
}

// unplug: only the socket flag; the GameCube's ClearPlugState also reset the joint,
// the Wii's joint handling lives in ExecCallbackB instead
void ResetPlayerPlugState(XObjPlug* plug, BOOL resetJoint)
{
	plug->mConnection = 0;
}

// the vertex position of the map triangle a floor hit rests on
void fn_800DB958(XObjMap* map, XObjMapHit* hit, Vec* out)
{
	TRIGON_INFO* tri = &map->mTrigon.mTri[hit->mTriangle];
	tri->mGetVtx(tri, out);
}

void ScaleVectorInPlace(Vec* value, f32 scale)
{
	PSVECScale(value, value, scale);
}

u32 GetObjectMarkerOverride(XObj* object)
{
	return object->mAnmQueue.mMarkerOverride;
}

u32 GetMaskedObjectMarkerOverride(XObj* object, u32 mask)
{
	return object->mAnmQueue.mMarkerOverride & mask;
}

} // extern "C"

// NONMATCHING: identical except our compiler appends a dead blr after the tail call;
// every guard spelling tried keeps it
void XObjAttachment::DrawWithView(Mtx view)
{
	if (mVisible) {
		XObj::DrawWithView(view);
	}
}

void XObjAttachment::Hide()
{
	mVisible = FALSE;
}

// _1BC, the guarded cull mode forward: himself, the plug, the visible attachments and the
// suit he wears, plus the extra part suits 3 and 6 bring
void XObjChr::vf1BC(s32 mode)
{
	XObj::vf1BC(mode);
	WiiPlayerState* chr = (WiiPlayerState*)this;
	if (chr->mPlug.HasModel()) {
		chr->mPlug.vf1BC(mode);
	}
	if (((XObjAttachment*)chr->mCopterObj)->mVisible) {
		((XObjAttachment*)chr->mCopterObj)->vf1BC(mode);
	}
	if (((XObjAttachment*)chr->mPopperObj)->mVisible) {
		((XObjAttachment*)chr->mPopperObj)->vf1BC(mode);
	}
	if (((XObjAttachment*)chr->mRadarObj)->mVisible) {
		((XObjAttachment*)chr->mRadarObj)->vf1BC(mode);
	}
	if (((XObjAttachment*)chr->mOXObj)->mVisible) {
		((XObjAttachment*)chr->mOXObj)->vf1BC(mode);
	}
	WiiSuitObject* suit = &chr->mSuits.mObjects[chr->mSuits.mCurrent];
	if (IsObjectActive(suit)) {
		suit->vf1BC(mode);
		if (chr->mSuits.mCurrent == 3 || chr->mSuits.mCurrent == 6) {
			chr->mSuitParts[chr->mSuits.mCurrent].vf1BC(mode);
		}
	}
}

// _1C0, the same forward at the tag-preserving cull mode slot
void XObjChr::SetCullMode(s32 mode)
{
	XObj::SetCullMode(mode);
	WiiPlayerState* chr = (WiiPlayerState*)this;
	if (chr->mPlug.HasModel()) {
		chr->mPlug.SetCullMode(mode);
	}
	if (((XObjAttachment*)chr->mCopterObj)->mVisible) {
		((XObjAttachment*)chr->mCopterObj)->SetCullMode(mode);
	}
	if (((XObjAttachment*)chr->mPopperObj)->mVisible) {
		((XObjAttachment*)chr->mPopperObj)->SetCullMode(mode);
	}
	if (((XObjAttachment*)chr->mRadarObj)->mVisible) {
		((XObjAttachment*)chr->mRadarObj)->SetCullMode(mode);
	}
	if (((XObjAttachment*)chr->mOXObj)->mVisible) {
		((XObjAttachment*)chr->mOXObj)->SetCullMode(mode);
	}
	WiiSuitObject* suit = &chr->mSuits.mObjects[chr->mSuits.mCurrent];
	if (IsObjectActive(suit)) {
		suit->SetCullMode(mode);
		if (chr->mSuits.mCurrent == 3 || chr->mSuits.mCurrent == 6) {
			chr->mSuitParts[chr->mSuits.mCurrent].SetCullMode(mode);
		}
	}
}

// _1C4, and back to the materials' own cull modes
void XObjChr::ResetCullMode()
{
	XObj::ResetCullMode();
	WiiPlayerState* chr = (WiiPlayerState*)this;
	if (chr->mPlug.HasModel()) {
		chr->mPlug.ResetCullMode();
	}
	if (((XObjAttachment*)chr->mCopterObj)->mVisible) {
		((XObjAttachment*)chr->mCopterObj)->ResetCullMode();
	}
	if (((XObjAttachment*)chr->mPopperObj)->mVisible) {
		((XObjAttachment*)chr->mPopperObj)->ResetCullMode();
	}
	if (((XObjAttachment*)chr->mRadarObj)->mVisible) {
		((XObjAttachment*)chr->mRadarObj)->ResetCullMode();
	}
	if (((XObjAttachment*)chr->mOXObj)->mVisible) {
		((XObjAttachment*)chr->mOXObj)->ResetCullMode();
	}
	WiiSuitObject* suit = &chr->mSuits.mObjects[chr->mSuits.mCurrent];
	if (IsObjectActive(suit)) {
		suit->ResetCullMode();
		if (chr->mSuits.mCurrent == 3 || chr->mSuits.mCurrent == 6) {
			chr->mSuitParts[chr->mSuits.mCurrent].ResetCullMode();
		}
	}
}

// _1A0, the dst-alpha flags go to every model he carries: the plug and attachments when
// they have one, all nine suits and their extra parts
void XObjChr::vf1A0(const u32* flags)
{
	XObj::vf1A0(flags);
	WiiPlayerState* chr = (WiiPlayerState*)this;
	if (chr->mPlug.HasModel()) {
		chr->mPlug.vf1A0(flags);
	}
	if (((XObj*)chr->mCopterObj)->HasModel()) {
		((XObj*)chr->mCopterObj)->vf1A0(flags);
	}
	if (((XObj*)chr->mPopperObj)->HasModel()) {
		((XObj*)chr->mPopperObj)->vf1A0(flags);
	}
	if (((XObj*)chr->mRadarObj)->HasModel()) {
		((XObj*)chr->mRadarObj)->vf1A0(flags);
	}
	if (((XObj*)chr->mOXObj)->HasModel()) {
		((XObj*)chr->mOXObj)->vf1A0(flags);
	}
	for (s32 i = 0; i < 9; i++) {
		if (chr->mSuits.mObjects[i].HasModel()) {
			chr->mSuits.mObjects[i].vf1A0(flags);
		}
		if (chr->mSuitParts[i].HasModel()) {
			chr->mSuitParts[i].vf1A0(flags);
		}
	}
}

// _1A4, the fade on himself and every model he carries, the GameCube's
// XObjChr::SetDestinationAlpha with the suit parts added
void XObjChr::SetDestinationAlpha(BOOL enabled, u8 value)
{
	XObj::SetDestinationAlpha(enabled, value);
	WiiPlayerState* chr = (WiiPlayerState*)this;
	if (chr->mPlug.HasModel()) {
		chr->mPlug.SetDestinationAlpha(enabled, value);
	}
	if (((XObj*)chr->mCopterObj)->HasModel()) {
		((XObj*)chr->mCopterObj)->SetDestinationAlpha(enabled, value);
	}
	if (((XObj*)chr->mPopperObj)->HasModel()) {
		((XObj*)chr->mPopperObj)->SetDestinationAlpha(enabled, value);
	}
	if (((XObj*)chr->mRadarObj)->HasModel()) {
		((XObj*)chr->mRadarObj)->SetDestinationAlpha(enabled, value);
	}
	if (((XObj*)chr->mOXObj)->HasModel()) {
		((XObj*)chr->mOXObj)->SetDestinationAlpha(enabled, value);
	}
	for (s32 i = 0; i < 9; i++) {
		if (chr->mSuits.mObjects[i].HasModel()) {
			chr->mSuits.mObjects[i].SetDestinationAlpha(enabled, value);
		}
		if (chr->mSuitParts[i].HasModel()) {
			chr->mSuitParts[i].SetDestinationAlpha(enabled, value);
		}
	}
}

// drops the stale object indexes first, then draws himself or the current suit and the
// attachments; the popper turns with the camera while the menu flag is up
// NONMATCHING (92 %): retail's register and schedule residue over the five index cleanups
void XObjChr::DrawWithView(Mtx view)
{
	WiiPlayerState* chr = (WiiPlayerState*)this;
	if (chr->m_0A18 >= 0 && !IsObjectActive(gGame.mXObjs.mObjects[chr->m_0A18])) {
		chr->m_0A18 = -1;
	}
	if (chr->mFloorContactObjects[0] >= 0 && !IsObjectActive(gGame.mXObjs.mObjects[chr->mFloorContactObjects[0]])) {
		chr->mFloorContactObjects[0] = chr->mFloorContactObjects[1] = -1;
	}
	if (chr->mFloorContactObjects[2] >= 0 && !IsObjectActive(gGame.mXObjs.mObjects[chr->mFloorContactObjects[2]])) {
		chr->mFloorContactObjects[2] = -1;
	}
	if (chr->m_0A1C >= 0 && !IsObjectActive(gGame.mXObjs.mObjects[chr->m_0A1C])) {
		chr->m_0A1C = -1;
	}
	if (chr->mFloorContactObjects[4] >= 0 && !IsObjectActive(gGame.mXObjs.mObjects[chr->mFloorContactObjects[4]])) {
		chr->mFloorContactObjects[4] = -1;
	}
	if (gGame.mMotionControlCamera.GetMapDistance() < 1.0f) {
		return;
	}
	WiiSuitObject* suit = &chr->mSuits.mObjects[chr->mSuits.mCurrent];
	if (!IsObjectActive(suit) || chr->mSuits.mChangeStep == 3) {
		XObj::DrawWithView(view);
	} else {
		suit->DrawWithView(view);
		if (chr->m_9C8 == 0 && (chr->mSuits.mChangeStep == 3 || chr->mSuits.mChangeStep == 6)) {
			chr->mSuitParts[chr->mSuits.mChangeStep].DrawWithView(view);
		}
	}
	if (!(gGame.GetStatusFlags(0x100) && chr->mLift.mState == 4 && IsLiftTargetPlug(&chr->mLift))) {
		// no plug in this lift state
		if (gGame.GetStatusFlags(0x0040000000000000ULL)) {
			chr->mPlug.DrawWithView(view);
		}
	}
	((XObj*)chr->mCopterObj)->DrawWithView(view);
	if (gGame.GetStatusFlags(0x100)) {
		Mtx rotation;
		Mtx combined;
		PSMTXRotRad(rotation, 'x', -gGame.mMotionControlCamera.mView.mAngle.x);
		if (view != NULL) {
			PSMTXConcat(rotation, view, combined);
		} else {
			PSMTXConcat(rotation, gGame.mMotionControlCamera.mViewMtx, combined);
		}
		((XObj*)chr->mPopperObj)->DrawWithView(combined);
	} else {
		((XObj*)chr->mPopperObj)->DrawWithView(view);
	}
	((XObj*)chr->mRadarObj)->DrawWithView(view);
	((XObj*)chr->mOXObj)->DrawWithView(view);
}

// the eye highlight would light the reflection, so it goes off for the draw and back on
void XObjChr::DrawReflection(Mtx view)
{
	WiiPlayerState* chr = (WiiPlayerState*)this;
	if (gGame.mMotionControlCamera.GetMapDistance() < 1.0f) {
		return;
	}
	if (gGame.GetStatusFlags(0x100)) {
		chr->vf28C(0);
	}
	DrawWithView(view);
	if (gGame.GetStatusFlags(0x100)) {
		chr->vf28C(1);
	}
	if (gGame.GetStatusFlags(0x0040000000000000ULL)) {
		chr->mPlug.DrawReflection(view);
	}
	((XObj*)chr->mCopterObj)->DrawReflection(view);
	((XObj*)chr->mPopperObj)->DrawReflection(view);
	((XObj*)chr->mRadarObj)->DrawReflection(view);
	((XObj*)chr->mOXObj)->DrawReflection(view);
}

// the carried object's shadow while he holds something up, his own otherwise
void XObjChr::DrawGroundShadow(f32 opacity)
{
	WiiPlayerState* chr = (WiiPlayerState*)this;
	if (gGame.mMotionControlCamera.GetMapDistance() < 1.0f) {
		return;
	}
	if (mStatus.mFlags & 4) {
		if (gGame.GetStatusFlags(0x0040000000000000ULL)) {
			chr->mPlug.DrawGroundShadow(opacity);
		}
	} else {
		XObj::DrawGroundShadow(opacity);
	}
}

// the mug casts a blob under itself scaled by its carry joint; on the koshi anims the shadow
// follows that joint instead
// NONMATCHING (99.7 %): a single residue left, body matches otherwise
void XObjChr::DrawGroundShadowAt(const Vec* origin, f32 opacity)
{
	WiiPlayerState* chr = (WiiPlayerState*)this;
	if (gGame.mMotionControlCamera.GetMapDistance() < 1.0f) {
		return;
	}
	if (mStatus.mFlags & 4) {
		if (gGame.GetStatusFlags(0x0040000000000000ULL)) {
			chr->mPlug.DrawGroundShadow(opacity);
		}
	} else {
		Vec position = GetJointWorldPos("second_null");
		if (chr->mLift.mState != 0 && IsCarryingMug(&chr->mLift)) {
			CVec jointScale;
			chr->GetJointScale(chr->FindResNode("carry_obj_nll"), &jointScale);
			u32 shadowSize = mAttr.mObjData != NULL ? mAttr.mObjData->mShadowSize : 0;
			Vec size       = mMoveScale * (f32)shadowSize + jointScale * 2.5f;
			gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
			gCustomShadow.Draw(position, size, (u32)(170.0f * opacity));
		} else {
			if ((u32)(GetAnimId() - 0x14D) <= 4) {
				position = GetJointWorldPos("koshi_model");
			}
			XObj::DrawGroundShadowAt(&position, opacity);
		}
	}
}

void XObjChr::vf1A8(f32)
{
}

// queues the models he wears on the scene root and steps the queued draws for the frame:
// himself, the suit he has on (suits 3 and 6 carry an extra part), and the popper mount
extern "C" void fn_800E7EA8(WiiPlayerState* chr)
{
	gGame.mGs.mScene.mHead->vf034(gGame.mGs.mScene.mHead->mPending, chr->mObject.m_pScnMdl);
	WiiSuitObject* suit = &chr->mSuits.mObjects[chr->mSuits.mCurrent];
	if (IsObjectActive(suit)) {
		gGame.mGs.mScene.mHead->vf034(gGame.mGs.mScene.mHead->mPending, suit->mObject.m_pScnMdl);
		if (chr->mSuits.mCurrent == 3 || chr->mSuits.mCurrent == 6) {
			gGame.mGs.mScene.mHead->vf034(gGame.mGs.mScene.mHead->mPending, chr->mSuitParts[chr->mSuits.mCurrent].mObject.m_pScnMdl);
		}
	}
	gGame.mGs.mScene.mHead->vf034(gGame.mGs.mScene.mHead->mPending, ((XObj*)chr->mPopperObj)->mObject.m_pScnMdl);
	fn_802AEDA0(&gGame.mGs.mScene.mHead, 0);
}

// the eye camera setup: resets the aim state to his facing, and while he holds attachment 2
// (the eye camera) it queues the same worn models as fn_800E7EA8, steps the scene draws once,
// then drains the queue again
extern "C" void SetPlayerEyeCameraJointsHidden(WiiPlayerState* player, BOOL hidden, f32 rotY, f32 unused)
{
	player->mEyeAim.Set(unused, rotY, 0.0f);
	player->mEyeAimTarget.Set(unused, rotY, 0.0f);
	player->mEyeAimStep.Set(0.0f, 0.0f, 0.0f);
	if (player->GetAttachment() == 2 && hidden) {
		gGame.mGs.mScene.mHead->vf034(gGame.mGs.mScene.mHead->mPending, player->mObject.m_pScnMdl);
		WiiSuitObject* suit = &player->mSuits.mObjects[player->mSuits.mCurrent];
		if (IsObjectActive(suit)) {
			gGame.mGs.mScene.mHead->vf034(gGame.mGs.mScene.mHead->mPending, suit->mObject.m_pScnMdl);
			if (player->mSuits.mCurrent == 3 || player->mSuits.mCurrent == 6) {
				gGame.mGs.mScene.mHead->vf034(gGame.mGs.mScene.mHead->mPending,
				                              player->mSuitParts[player->mSuits.mCurrent].mObject.m_pScnMdl);
			}
		}
		gGame.mGs.mScene.mHead->vf034(gGame.mGs.mScene.mHead->mPending, ((XObj*)player->mPopperObj)->mObject.m_pScnMdl);
		fn_802AEDA0(&gGame.mGs.mScene.mHead, 0);
		WiiGsSceneObj* head = gGame.mGs.mScene.mHead;
		s32 pending;
		while ((pending = head->mPending) != 0) {
			if (pending != 0) {
				head->vf038(pending - 1);
			}
		}
	}
}

// swings the eye camera's aim at a new target: 3 degrees a frame on each axis,
// both angles wrapped into [0, 2pi) and the target swung round when the other way is shorter
extern "C" void fn_800E8140(WiiPlayerState* player, f32 targetY, f32 targetX)
{
	player->mEyeAimTarget.y = targetY;
	player->mEyeAimStep.y   = DEG2RAD * 3;
	f32 curY                = WrapAngle(player->mEyeAim.y);
	f32 tgtY                = WrapAngle(player->mEyeAimTarget.y);
	player->mEyeAim.y       = curY;
	player->mEyeAimTarget.y = tgtY;
	if (tgtY != curY) {
		f32 diff = tgtY - curY;
		diff     = __fabsf(diff);
		if (diff >= TWO_PI) {
			diff -= TWO_PI;
		}
		if (diff > PI) {
			if (tgtY < curY) {
				player->mEyeAimTarget.y = TWO_PI + tgtY;
			} else {
				player->mEyeAimTarget.y = tgtY - TWO_PI;
			}
		}
	}
	player->mEyeAimTarget.x = targetX;
	player->mEyeAimStep.x   = DEG2RAD * 3;
	f32 curX                = WrapAngle(player->mEyeAim.x);
	f32 tgtX                = WrapAngle(player->mEyeAimTarget.x);
	player->mEyeAim.x       = curX;
	player->mEyeAimTarget.x = tgtX;
	if (tgtX != curX) {
		f32 diff = tgtX - curX;
		diff     = __fabsf(diff);
		if (diff >= TWO_PI) {
			diff -= TWO_PI;
		}
		if (diff > PI) {
			if (tgtX < curX) {
				player->mEyeAimTarget.x = TWO_PI + tgtX;
			} else {
				player->mEyeAimTarget.x = tgtX - TWO_PI;
			}
		}
	}
}

void XObjChr::SetBaseMtx(Mtx mtx)
{
	mBaseMtxPtr = mBaseMtx;
	PSMTXCopy(mtx, mBaseMtx);
	WiiPlayerState* chr = (WiiPlayerState*)this;
	for (int i = 0; i < 9; i++) {
		chr->mSuits.mObjects[i].SetBaseMtx(mtx);
	}
}

void XObjChr::ClearBaseMtx()
{
	mBaseMtxPtr         = NULL;
	WiiPlayerState* chr = (WiiPlayerState*)this;
	for (int i = 0; i < 9; i++) {
		chr->mSuits.mObjects[i].ClearBaseMtx();
	}
}

// one of his tools is out
static inline BOOL IsToolOut(WiiPlayerState* chr, s16 item)
{
	switch (item) {
	case 0x10:
		return IsCarryingToothbrush(&chr->mLift);
	case 0x16:
		return IsCarryingMug(&chr->mLift);
	case 0x15:
		return IsCarryingSpoon(&chr->mLift);
	default:
		return IsCarryingSquirter(&chr->mLift);
	}
}

// he can use the tool, he has it or it is out
static inline s32 CanUseTool(WiiPlayerState* chr, s16 item)
{
	s32 has = 1;
	if (FindOwnedInventoryItemIndex(chr->mParams.mInventory.mItems, item) < 0 && !IsToolOut(chr, item))
		has = 0;
	return has;
}

static inline void SetAttachmentOn(AttachmentValues* values, s32 id, s32 on)
{
	s32 index = values->Find(id);
	if (index >= 0)
		values->mEntries[index].mValue = on;
}

// which of his tools he can use, he has them or one of them is going; the
// GameCube's UpdateAttachmentAvailability, only ever inlined here on the Wii
static inline void UpdateAttachmentAvailability(AttachmentValues* values)
{
	WiiPlayerState* chr = gGame.GetPlayer();
	SetAttachmentOn(values, 4, CanUseTool(chr, 0x10));
	SetAttachmentOn(values, 6, CanUseTool(chr, 0x16));
	SetAttachmentOn(values, 5, CanUseTool(chr, 0x15));
	SetAttachmentOn(values, 7, CanUseTool(chr, 0x38));
	SetAttachmentOn(values, 8, FALSE);
}

// the count for an attachment id, -1 when it isn't in the list
// NONMATCHING: register allocation only (values in r29 vs retail's r30 and
// vice versa), the inlined Find/CanUseTool chains are instruction-identical
/**
 * @note Address: 0x800E84BC
 * @note Size: 0x5E4
 */
extern "C" s32 GetAttachmentCountById(AttachmentValues* values, s32 id)
{
	s32 index = values->Find(id);
	if (index >= 0) {
		UpdateAttachmentAvailability(values);
		return values->mEntries[index].mValue;
	}
	return -1;
}

// OctaMenu::Open, ahead of the two menus that inline it; the Wii's head aim is the
// scene-camera record at WiiPlayerState _987C (snapped to its neutral angle here,
// released in Select), not the GameCube's neck/head CJointRot pair
// NONMATCHING: register allocation in the head-aim block (mChr held in r7 vs
// retail's r6), the instruction stream is otherwise identical
/**
 * @note Address: 0x800E9304
 * @note Size: 0x2AC
 */
void OctaMenu::Open(s32 focus)
{
	if (mOpen == 0) {
		mOpeningHeading        = mChr->mMoveRotate.y;
		XObjScnCamInitRec* aim = (XObjScnCamInitRec*)((WiiPlayerState*)mChr)->_987C;
		aim->mSnap[0] = aim->mSnap[1] = 1;
		aim->m_0C                     = NULL;
		aim->mTarget.x                = NAN;
		aim->m_20                     = NULL;
		aim->m_24                     = 0.0f;
		aim->m_28                     = 0.0f;
	}
	mOpen  = 1;
	mFocus = focus;
	if (mFocus != 0) {
		mGlow = 1.0f;
	}
	Vec offset = { 0.0f, 8.8f, 0.0f };
	mChr->ProjectOffsetToScreen(&offset, &mOpenScreenX, &mOpenScreenY, NULL, NULL);
	for (int i = 0; i < 8; i++) {
		mItems[i].mPulsePhase = DEG2RAD * (rand() % 360);
		mItems[i]._0C         = DEG2RAD * (rand() % 360);
		mItems[i].mScale      = mFocus != 0 ? 0.3f : 0.5f;
	}
	mCursorIndex = -1;
	mLabelLength = 0;
	for (int i = 0; i < (int)ARRAY_SIZE(mLabelChars); i++) {
		mLabelChars[i].mAngle0 = DEG2RAD * (rand() % 360);
		mLabelChars[i].mAngle1 = DEG2RAD * (rand() % 360);
	}
	mChr->SetAnim(17, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
	mPointedItem = mPreviousPointedItem = -1;
}

// OctaMenu::Select, closes the ring and turns chibi robo back
// NONMATCHING: register/schedule noise only, the instruction stream is identical
/**
 * @note Address: 0x800E95B0
 * @note Size: 0x11C
 */
void OctaMenu::Select(s32 result)
{
	if (result > 0 && mFocus == 0)
		mOpen = -1;
	else
		mOpen = -2;
	mPendingFirstVisibleItem = -1;
	mFocus                   = 0;
	if (((WiiPlayerState*)mChr)->mTumble.mState == 0) {
		mChr->SetAnim(17, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
		mChr->SetRotateY(mOpeningHeading, DEG2RAD * 12.0f);
		XObjScnCamInitRec* aim = (XObjScnCamInitRec*)((WiiPlayerState*)mChr)->_987C;
		aim->mSnap[0] = aim->mSnap[1] = 2;
		aim->m_0C                     = NULL;
		aim->mTarget.x                = NAN;
		aim->m_20                     = NULL;
		aim->m_24                     = NAN;
		aim->m_28                     = NAN;
		aim->m_2C                     = 0.0f;
		aim->m_30                     = 0.0f;
		aim->m_4C                     = 0.0f;
		aim->m_50                     = 0.0f;
		aim->m_54                     = 0.0f;
	}
	mPointedItem = mPreviousPointedItem = -1;
}

// nothing to decide on the base menu
void OctaMenu::ConfirmSelection()
{
}
