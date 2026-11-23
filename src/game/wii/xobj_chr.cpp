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
#include "game/wii/cpadact.h"
#include "game/wii/csound.h"
#include "game/playeranimationdata.h"
#include "game/pointptcl.h"
#include "game/wii/xact.h"
#include "game/mapfloorhit.h"
#include "game/gmath.h"
#include "game/globals.h"
#include "game/cmdline.h"
#include "arithmetic.h"
#include "JSystem/JMath/Inline.h"
#include <math.h>
#include <stdlib.h>

// the Wii build inlines PSVECScale as paired-single code like the other units
#define PSVECScale JMathInlineVEC::PSVECScale
#define NW4R_ASSERT_VALID_PTR_LINE(line, exp)                                                                                              \
	((void)((((u32)(exp) & 0xFF000000) == 0x80000000 || ((u32)(exp) & 0xFF800000) == 0x81000000 || ((u32)(exp) & 0xF8000000) == 0x90000000 \
	         || ((u32)(exp) & 0xFF000000) == 0xC0000000 || ((u32)(exp) & 0xFF800000) == 0xC1000000                                         \
	         || ((u32)(exp) & 0xF8000000) == 0xD0000000 || ((u32)(exp) & 0xFFFFC000) == 0xE0000000)                                        \
	        || (nw4r::db::Panic(__FILE__, line, "NW4R:Pointer Error\n" #exp "(=%p) is not valid pointer.", (exp)), 0)))
// NONMATCHING: ScaleVectorInPlace's retail body interleaves the paired-single loads,
// multiplies and stores (its SDK inline's statement order); ours loads both pairs
// first, so the wrapper stays below a match

extern "C" void ReleasePushAction(XAction* action);
extern "C" BOOL IsPlayerStopped(const WiiPlayerState* chr);
extern "C" BOOL IsTumbleActionState(const XAction* act, s32 wanted);
extern "C" void fn_802AEDA0(WiiGsSceneObj** head, u32 frames); // steps the queued scene draws, as gameupdate.cpp declares it
extern CustomShadow gCustomShadow;
extern MEMAllocator gWiiPadAllocator;

static s32 sBlockedObstacleCount;

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
	if (tri >= 0)
		return map->mTrigon.mTri[tri].mFloorType;
	return 0;
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
struct SuitChangeTrail;

struct PlayerFrameParticle {
	s32 mCode;
	Vec mTranslate;
	Vec mScale;
	Vec mRotate;
};
STATIC_ASSERT(sizeof(PlayerFrameParticle) == 0x28);

extern "C" {
f32 GetControlStickMagnitude(const CPadOne* pad);
s32 FindNearbyPlayerInteraction(WiiPlayerState* player, f32 extraRadius);
f32 UpdatePlayerActions(WiiPlayerState* player, u32 frames);
void UpdatePlayerActionAnimations(WiiPlayerState* player, u32 frames);
void UpdatePlayerMovementAndContacts(WiiPlayerState* player, u32 frames, f32 speed, f32 direction);
void UpdatePlayerBattery(WiiPlayerState* player, u32 frames);
void UpdatePlayerHeadOffset(WiiPlayerState* chr, u32 frames);
s32 GetPlayerGroundType(WiiPlayerState* chr);
PlayerFrameParticle ProcessPlayerFrameEvents(XObj* object, FrameEventTime* time, const FrameEvent* table);
void UpdateTumbleFade(XActTumbleFade* fade, BOOL increase, u32 frames);
void UpdateRadarBeamAnimation(XActRadar* act, u32 frames);
void UpdatePointParticleDisplay(PointPtcl* points, u32 frames);
void UpdateSpiralTrail(SuitChangeTrail* trail, u32 frames);
EffectInstance* fn_800A11A4(CEvtPtcl* particles, u32 code, s32 group);
void UpdateLiftActionAnimation(XActLift* act, s32 anim, CObj* model, u32 frames, f32 frame, f32 end);
void UpdatePlayerHangAction(XActHang* act, s32 anim, CObj* model, u32 frames, f32 frame, f32 end);
void UpdatePlayerJumpAction(XActJump* act, s32 anim, CObj* model, u32 frames, f32 frame, f32 end);
void UpdateLadderActionAnimation(XActLadder* act, s32 anim, CObj* model, u32 frames, f32 frame, f32 end);
void UpdatePlayerRopeAction(XActRope* act, s32 anim, CObj* model, u32 frames, f32 frame, f32 end);
void UpdatePushActionAnimation(XActPush* act, s32 anim, CObj* model, u32 frames, f32 frame, f32 end);
void UpdatePlayerTumbleAnimation(XActTumble* act, s32 anim, CObj* model, u32 frames, f32 frame, f32 end);
void UpdatePlayerCopterFlight(XActCopter* act, s32 anim, CObj* model, u32 frames, f32 frame, f32 end);
void UpdatePlayerPopperAction(XActPopper* act, s32 anim, CObj* model, u32 frames, f32 frame, f32 end);
void UpdateRadarTargeting(XActRadar* act, s32 anim, CObj* model, u32 frames, f32 frame, f32 end);
void UpdatePlayerLocomotionAnimation(WiiPlayerState* player, s32 anim, CObj* model, u32 frames, f32 frame, f32 end);
void UpdatePlayerLookAtObjects(WiiPlayerState* player, u32 frames);
void fn_800C6CF0(XObjScnCamInitRec* record, u32 frames);
u32 CheckStageFlag(s32 stage, u32 mask);
BOOL IsGlobalDataLoaded(void);
void ResetPlayerStats(Params* stats);
void InitializePlayerLiftAction(XAction* action, WiiPlayerState* owner);
BOOL IsCarryingMug(XAction* action);
BOOL IsCarryingSquirter(XAction* action);
BOOL IsCarryingSpoon(XAction* action);
BOOL IsCarryingToothbrush(XAction* action);
void SelectHighestActiveFloor(XObj* object);
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
void SpawnPlayerDust(WiiPlayerState* chr, CVec* position);
BOOL IsPlayerCarryingPlug(WiiPlayerState* chr);
void HSD_MtxGetScale(Mtx matrix, Vec* scale);
void PSMTXMultVecArray(const Mtx matrix, const Vec* src, Vec* dst, u32 count);
void ClosePlayerEquipmentRings(XObjChrMenu* menu, s32 result);
void TryLeaveHang(XActHang* action);
UnkStageObj* GetStagePlacementByIndex(s32 index);
f32 fn_800E32B8(WiiPlayerState* chr, s32 anim);
f32 fn_800D1740(f32 value);
f32 fn_800D1764(const WiiPlayerState* chr, s32 index);
s32 GetPlayerPlugConnection(const XObjPlug* plug);
CVec* GetPlayerPlugCordPull(XObjPlug* plug);
void ScaleVectorInPlace(Vec* value, f32 scale);
s32 GetCharacterObjectApproachResult(XObj* chr, XObj* object);
u32 GetObjectMarkerOverride(XObj* object);
u32 GetMaskedObjectMarkerOverride(XObj* object, u32 mask);
Vec GetFloorExtent(XObjFloor* const* floors, s32 index);
Vec GetFloorCenterOffset(XObjFloor* const* floors, s32 index);
void fn_800DB958(XObjMap* map, XObjMapHit* hit, Vec* out);
void TryStartObjectEvent(CGame* game, const void* script, XObj* owner, s16 first, s16 second);
void PlayCopterFlightAnimation(XObjCopter* copter);
void StartPlayerRadarScan(XObjRadar* radar);
BOOL IsPopperFireAnimationSelected(XObjPopper* popper);
u8* GetStageZButtonScript();
void HandleLiftActionInput(XActLift* act, const CPadOne* pad, f32* weight);
void HandleHangActionInput(XActHang* act, const CPadOne* pad, f32* weight);
void SuppressMovementDuringJump(XActJump* act, const CPadOne* pad, f32* weight);
void HandleLadderActionInput(XActLadder* act, const CPadOne* pad, f32* weight);
void HandleRopeActionInput(XActRope* act, const CPadOne* pad, f32* weight);
void HandlePushActionInput(XActPush* act, const CPadOne* pad, f32* weight);
void HandleTumbleActionInput(XActTumble* act, const CPadOne* pad, f32* weight);
void HandleCopterActionInput(XAction* act, const CPadOne* pad, f32* weight);
void ApplyPopperMovementWeight(XActPopper* act, const CPadOne* pad, f32* weight);
void ApplyRadarMovementWeight(XActRadar* act, const CPadOne* pad, f32* weight);
void UpdatePlayerAttachmentRequest(WiiPlayerState* chr, f32* weight);
void UpdatePlayerEquipmentRings(XObjChrMenu* menu, u32 frames);
void UpdateOXMenuTransition(WiiOXMenu* menu, u32 frames);
XObj* ResolveEventObjectId(const CEvtData* data, s16 id);
void SetLiftPlugConnection(XActLift* act, u32 value, BOOL apply);
void fn_801105C4(XObjPlug* plug, s32 mode);
void fn_801107AC(XObjPlug* plug);
void ResetPopperShot(XActPopperShot* shot);
void DrawTumbleRing(XActTumbleFade* fade, const Vec& pos, f32 angle);
void DrawRadarTargetBeam(XActRadar* act);
void DrawPopperShotParticles(XActPopper* act);
void DrawPopperShotRings(XActPopper* act);
void DrawPlayerRadarEffects(XActRadar* act);
void DrawPlayerLookMarker(XObjChrLook* look);
void DrawPointParticleDisplay(PointPtcl* points, f32 row);
f32 GetAreaCameraFovRatio(CMotionControlCamera* camera);
u32 GetPlayerActionParameter(s32 index);
u8* GetStageBatteryDepletedScript();
void StartScreenFade(CScrnColor* color, u8 r0, u8 g0, u8 b0, u8 a0, u8 r1, u8 g1, u8 b1, u8 a1, f32 time);
void SetAreaCameraInterestTarget(CCameraActionArea* area, s32 mode, XObj* target, f32 time);
void SelectActiveAreaCameraEntryByViewDirection(CCameraActionArea* area, s32 mode, f32 time);
void fn_800A12F8(EffectInstance* effects, u32 id, BOOL retireParticles);
void SetGsChannelEnabled(CGs* gs, s32 channel, u8 enable);
void ClearActiveObjectFloors(XObj* object);
XObj* GetGameObjectByIndex(const CGame* game, s32 index);
BOOL IsObjectLiftable(const XObj* object);
BOOL IsObjectClimbableLedge(const XObj* object);
BOOL IsObjectLadder(const XObj* object);
BOOL IsObjectRope(const XObj* object);
BOOL IsObjectPushable(const XObj* object);
void ReleasePlayerLiftTarget(WiiPlayerState* chr);
void SetHangActionState(XAction* act, s32 state);
void SetPlayerActionTarget(XAction* action, XObj* target);
void SetPlayerActionState(XAction* action, s32 state);
void SetPushActionState(XAction* act, s32 state);
void SetPushTargetState(XAction* act, s32 value);
void SetCopterActionState(XAction* action, s32 state);
BOOL IsPositionBehindObject(void*, const Vec* pos, XObj* object);
f32 GetHeadingBetweenPositions(const Vec* from, const Vec* to);
f32 WrapPlayerActionAngle(f32 angle);
void SetObjectVectorTarget(CObjVec* value, const Vec& target);
BOOL IsLiftActionState(const XAction* act, s32 wanted);
BOOL IsHangActionState(const XAction* act, s32 wanted);
BOOL IsJumpActionState(const XAction* action, s32 state);
BOOL IsLadderActionState(const XAction* action, s32 state);
BOOL IsRopeActionState(const XAction* action, s32 state);
BOOL IsPushActionState(const XAction* act, s32 wanted);
BOOL IsTumbleActionState(const XAction* act, s32 wanted);
BOOL IsCopterActionState(const XAction* action, s32 state);
BOOL IsPopperActionState(const XAction* act, s32 wanted);
BOOL IsRadarActionState(const XAction* act, s32 wanted);
BOOL fn_800CDDAC(const XObj* object);
s16 GetObjectCategory(const XObj* object);

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

static inline void UpdateAttachmentCounts(AttachmentValues* values)
{
	if (IsGlobalDataLoaded()) {
		SetAttachmentCountById(
		    values, 1, gCmdLineArgs.mCopterLevelOverride != 0 ? gCmdLineArgs.mCopterLevelOverride - 1 : GetStartingAttachmentLevel(0));
		SetAttachmentCountById(
		    values, 2, gCmdLineArgs.mPopperLevelOverride != 0 ? gCmdLineArgs.mPopperLevelOverride - 1 : GetStartingAttachmentLevel(1));
		SetAttachmentCountById(
		    values, 3, gCmdLineArgs.mRadarLevelOverride != 0 ? gCmdLineArgs.mRadarLevelOverride - 1 : GetStartingAttachmentLevel(2));
	} else {
		SetAttachmentCountById(values, 1, gCmdLineArgs.mCopterLevelOverride != 0 ? gCmdLineArgs.mCopterLevelOverride - 1 : 0);
		SetAttachmentCountById(values, 2, gCmdLineArgs.mPopperLevelOverride != 0 ? gCmdLineArgs.mPopperLevelOverride - 1 : 0);
		SetAttachmentCountById(values, 3, gCmdLineArgs.mRadarLevelOverride != 0 ? gCmdLineArgs.mRadarLevelOverride - 1 : 0);
	}
}

static inline void RefreshAttachmentMenuEntries(AttachmentValues* values)
{
	UpdateAttachmentCounts(values);
	UpdateAttachmentAvailability(values);
}

static inline void InitializeAttachmentMenuEntries(AttachmentValues* values)
{
	values->mEntries[0].mId = 1;
	values->mEntries[1].mId = 2;
	values->mEntries[2].mId = 3;
	values->mEntries[3].mId = 4;
	values->mEntries[4].mId = 5;
	values->mEntries[5].mId = 6;
	values->mEntries[6].mId = 7;
	values->mEntries[7].mId = 8;
	RefreshAttachmentMenuEntries(values);
}

static inline void ResetRingMode(OctaMenu& ring)
{
	ring.mOpen        = 0;
	ring.mCursorIndex = 0;
	u32 mode          = gCmdLineArgs.mAttachmentMenuType;
	if (mode <= 3) {
		ring.mMenuType = mode;
	} else {
		ring.mMenuType = 0;
	}
	ring.mFocus                 = 0;
	ring.mKeepPointedItemOnOpen = 0;
	ring.mHeadTurn              = NAN;
}

static inline void SetupPlayerRing(OctaMenu& ring, WiiPlayerState* owner, f32 direction)
{
	ring.mChr = (XObjChr*)owner;
	ResetRingMode(ring);
	ring.mRowOffset               = 0.0f;
	ring.mFirstVisibleItem        = 0;
	ring.mPendingFirstVisibleItem = -1;
	ring.mLastPointedItem         = 3;
	ring._10                      = direction;
}

static inline void SetRingItems(OctaMenu& ring, s32 a, s32 b, s32 c, s32 d, s32 e, s32 f, s32 g, s32 h)
{
	ring.mItems[0].mType = a;
	ring.mItems[1].mType = b;
	ring.mItems[2].mType = c;
	ring.mItems[3].mType = d;
	ring.mItems[4].mType = e;
	ring.mItems[5].mType = f;
	ring.mItems[6].mType = g;
	ring.mItems[7].mType = h;
}

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
void DrawSpiralTrail(SuitChangeTrail* trail, CVec* position);

// the delay the suit change keeps the old suit's resources around, the GameCube's
// gPlayerSuitChangeTimer; the Wii resets it to 0.0f and waits on the file itself
static f32 gPlayerSuitChangeTimer;

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

/**
 * @note Address: 0x800C9E84
 * @note Size: 0x1358
 */
// NONMATCHING: same call set; look/menu temporary store order and saved-GPR allocation differ
void XObjChr::InitializeObject()
{
	WiiPlayerState* player = (WiiPlayerState*)this;
	XObj::InitializeObject();
	player->mLook.mActive = FALSE;
	player->mLook.mMode   = -1;
	player->mLook.mObject = NULL;
	Vec zero              = { 0.0f, 0.0f, 0.0f };
	player->mLook.mOffset = zero;
	player->mLook.mPulse = player->mLook.mPhase = player->mLook.mFade = 0.0f;
	player->mLoadStep                                                 = 0;
	player->mPlug.InitializeObject();
	((XObj*)player->mCopterObj)->InitializeObject();
	((XObj*)player->mPopperObj)->InitializeObject();
	((XObj*)player->mRadarObj)->InitializeObject();
	((XObj*)player->mOXObj)->InitializeObject();
	InitializeAttachmentMenuEntries(&player->mAttachments);
	SetupPlayerRing(player->mMenu.mAtc, player, -1.0f);
	SetRingItems(player->mMenu.mAtc, 1, 2, 3, 4, 6, 5, 7, 8);
	player->mMenu.mAtc.mRowOffset        = -1.5f;
	player->mMenu.mAtc.mFirstVisibleItem = 4;
	player->mMenu.mDrs.Init(this);
	player->mMenu.mPrevSel         = 0;
	player->mMenu.mSel             = 0;
	player->mOXMenu.mChr           = player;
	player->mOXMenu.mLoadStep      = 0;
	player->mOXMenu.mSelectedIndex = 0;
	player->mOXMenu._10            = -1;
	for (s32 i = 0; i < 9; ++i) {
		player->mSuits.mObjects[i].InitializeObject();
		player->mSuits.mObjects[i].mOwner = player;
		player->mSuitParts[i].InitializeObject();
		player->mSuitParts[i].mOwner = &player->mSuits.mObjects[i];
	}
	player->mPlayer = player;
	// he may blink in every anim but these
	for (s32 i = 0; i < 0x167; ++i) {
		player->mBlinkAnims[i] = 1;
	}
	player->mBlinkAnims[356] = 0;
	player->mBlinkAnims[353] = 0;
	player->mBlinkAnims[352] = 0;
	player->mBlinkAnims[345] = 0;
	player->mBlinkAnims[337] = 0;
	player->mBlinkAnims[331] = 0;
	player->mBlinkAnims[328] = 0;
	player->mBlinkAnims[324] = 0;
	player->mBlinkAnims[323] = 0;
	player->mBlinkAnims[320] = 0;
	player->mBlinkAnims[319] = 0;
	player->mBlinkAnims[316] = 0;
	player->mBlinkAnims[311] = 0;
	player->mBlinkAnims[308] = 0;
	player->mBlinkAnims[307] = 0;
	player->mBlinkAnims[306] = 0;
	player->mBlinkAnims[305] = 0;
	player->mBlinkAnims[300] = 0;
	player->mBlinkAnims[299] = 0;
	player->mBlinkAnims[293] = 0;
	player->mBlinkAnims[270] = 0;
	player->mBlinkAnims[269] = 0;
	player->mBlinkAnims[266] = 0;
	player->mBlinkAnims[259] = 0;
	player->mBlinkAnims[258] = 0;
	player->mBlinkAnims[246] = 0;
	player->mBlinkAnims[245] = 0;
	player->mBlinkAnims[244] = 0;
	player->mBlinkAnims[243] = 0;
	player->mBlinkAnims[242] = 0;
	player->mBlinkAnims[241] = 0;
	player->mBlinkAnims[238] = 0;
	player->mBlinkAnims[237] = 0;
	player->mBlinkAnims[236] = 0;
	player->mBlinkAnims[234] = 0;
	player->mBlinkAnims[232] = 0;
	player->mBlinkAnims[231] = 0;
	player->mBlinkAnims[230] = 0;
	player->mBlinkAnims[229] = 0;
	player->mBlinkAnims[228] = 0;
	player->mBlinkAnims[223] = 0;
	player->mBlinkAnims[222] = 0;
	player->mBlinkAnims[212] = 0;
	player->mBlinkAnims[211] = 0;
	player->mBlinkAnims[210] = 0;
	player->mBlinkAnims[192] = 0;
	player->mBlinkAnims[187] = 0;
	player->mBlinkAnims[186] = 0;
	player->mBlinkAnims[175] = 0;
	player->mBlinkAnims[174] = 0;
	player->mBlinkAnims[173] = 0;
	player->mBlinkAnims[171] = 0;
	player->mBlinkAnims[170] = 0;
	player->mBlinkAnims[169] = 0;
	player->mBlinkAnims[167] = 0;
	player->mBlinkAnims[161] = 0;
	player->mBlinkAnims[156] = 0;
	player->mBlinkAnims[153] = 0;
	player->mBlinkAnims[151] = 0;
	player->mBlinkAnims[143] = 0;
	player->mBlinkAnims[83]  = 0;
	player->mBlinkAnims[76]  = 0;
	player->mBlinkAnims[71]  = 0;
	player->mBlinkAnims[68]  = 0;
	player->mBlinkAnims[67]  = 0;
	player->mBlinkAnims[65]  = 0;
	player->mBlinkAnims[54]  = 0;
	player->mBlinkAnims[53]  = 0;
	player->mBlinkAnims[14]  = 0;
	player->mBlinkAnims[7]   = 0;
	player->mBlinkAnims[6]   = 0;
	player->mBlinkAnims[5]   = 0;
	player->mBlinkAnims[2]   = 0;
	player->mBlinkAnims[1]   = 0;
	player->ResetForNewGame();
	m_pRoot = nw4r::g3d::ScnRoot::Construct(&gWiiPadAllocator, NULL, 8, 8, 8, 8);
	NW4R_ASSERT_VALID_PTR_LINE(0x8B, m_pRoot);
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
	chr->mActionObjectIndex = chr->mHitObjectIndex = -1;
	chr->mPreviousContactObject = chr->mFloorContactObjects[2] = chr->mFloorContactObjects[0] = chr->mFloorContactObjects[1] = -1;
	chr->mLastContactFloorIndex                                                                                              = -1;
	chr->mNearDistance                                                                                                       = 2.0f;
	chr->mLetickerIndex                                                                                                      = -1;
	chr->_0A70                                                                                                               = 0;
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
	fn_800C659C(&chr->mEyeAimRecord, this);
	fn_800C659C(&chr->mHeadAimRecord, this);
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
	lbl_8045BBD0.mActive = 0;
	for (s32 i = 0; i < 20; ++i) {
		lbl_8045BBD0.mParticles[i].scale = 0.0f;
		lbl_8045BBD0.mParticles[i].angle = 0.0f;
	}
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

/**
 * @note Address: 0x800CB1DC
 * @note Size: 0x10B8
 */
// NONMATCHING: same call set; NAN/menu store schedule differs
void XObjChr::ResetForNewGame()
{
	WiiPlayerState* player = (WiiPlayerState*)this;
	ResetPlayerStats(&player->mParams);
	player->mAttachment           = 0;
	player->mLiftPlugFlags        = gCmdLineArgs.mLiftPlugFlags;
	player->mAttachmentActionType = gCmdLineArgs.mAttachmentActionType;
	player->mToolActionType       = gCmdLineArgs.mToolActionType;
	player->vf290(FALSE);
	InitializePlayerLiftAction(&player->mLift, player);
	player->mPlug.ResetForNewGame();
	((XObj*)player->mCopterObj)->ResetForNewGame();
	((XObj*)player->mPopperObj)->ResetForNewGame();
	((XObj*)player->mRadarObj)->ResetForNewGame();
	((XObj*)player->mOXObj)->ResetForNewGame();
	player->mSuits.mLoadStep   = 0;
	player->mSuits.mChangeStep = 0;
	player->mSuits.mRequested  = 0;
	player->mSuits.mCurrent    = 0;
	player->mSuits.mSpecial    = 0;
	for (s32 i = 0; i < 9; ++i) {
		player->mSuits.mObjects[i].ResetForNewGame();
	}
	ApplyPlayerSuit(player);
	player->mPadReset = TRUE;
	RefreshAttachmentMenuEntries(&player->mAttachments);
	ResetRingMode(player->mMenu.mAtc);
	ResetRingMode(player->mMenu.mDrs);
	player->mMenu.mPrevSel         = 0;
	player->mMenu.mSel             = 0;
	player->mOXMenu.mLoadStep      = 0;
	player->mOXMenu.mSelectedIndex = 0;
	player->mOXMenu._10            = -1;
	player->_1C97C                 = 0;
	player->mSavedTarget           = -1;
	player->mPendingTarget         = NULL;
	player->mBatteryDrains         = TRUE;
	player->mSquirterContent       = 0;
	player->mItemTintWord          = -1;
	for (s32 i = 0; i < 10; ++i) {
		PointPtcl& p = gPointParticlePool[i];
		GetHudPointParticleOrigin(&gGame.mInfoScrn, &p.mStartY, &p.mStartX);
		p.display.mState = 0;
		p.burst.mState   = 0;
	}
	lbl_8045BBD0.mActive = 0;
	for (s32 i = 0; i < 20; ++i) {
		lbl_8045BBD0.mParticles[i].scale = 0.0f;
		lbl_8045BBD0.mParticles[i].angle = 0.0f;
	}
	player->mPlayerFlags = 0;
	player->mPlayerFlags |= 0x20000000;
	player->mPlayerFlags |= 7;
	player->mPadReset = TRUE;
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
// NONMATCHING (94 %): retail evaluates the lift-state test through nested bool inlines (each
// level clamped to a byte) that are not recovered; this plain test folds them
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
				if (!((chr->mLift.mState == 1 || chr->mLift.mState == 2 || chr->mLift.mState == 3) && ((XActLift*)&chr->mLift)->_AC)) {
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
	chr->mHitObjectIndex         = -1;
	chr->mActionObjectIndex      = -1;
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
		ApplyMaterialPresets(flags);
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

// NONMATCHING: the regular frame, turning, auto-approach, attachments, events and
// effects are present; the one-time pose initialization data and several suit/scene
// steps are unresolved, and aggregate/register allocation still differs
struct PlayerTurnState {
	f32 previousHeading;
	f32 heading;
	f32 previousChange;
	f32 change;
	f32 total;
};
PlayerTurnState gPlayerTurnState;

void XObjChr::UpdateObject(u32 frames)
{
	WiiPlayerState* chr = (WiiPlayerState*)this;
	CPadOne* pad        = &chr->mPad;
	if (chr->mPadReset && !chr->mMenu.IsOpen() && !chr->mOXMenu.mStarted && !gGame.mMenu.mActive && !chr->mSuits.mChangeStep)
		pad->Update(gWiiGameCubePads, 0);
	else
		pad->Clear();
	if (chr->mMenu.IsOpening() && chr->mTumble.mState)
		ClosePlayerEquipmentRings(&chr->mMenu, 0);
	chr->mStick[1] = chr->mStick[0];
	chr->mStick[3] = chr->mStick[2];
	chr->mStick[0] = gGame.IsRunning() && !gGame.mEvt.mData.mScript ? GetControlStickMagnitude(pad) : 0.0f;
	chr->mStick[2] = chr->mStick[0] > 0 ? -(f32)atan2(pad->mStick.x, pad->mStick.y) : -1.f;
	if (gGame.IsRunning() && !gGame.GetStatusFlags(0x400) && !gGame.GetStatusFlags(0x800)
	    && (!gGame.mEvt.mData.mScript || (gGame.mStage == 0x16 && (chr->mPlayerFlags & 0x80000000)))
	    && (chr->mLift.mState == 0 || chr->mLift.mState == 4) && chr->mHang.mState == 0 && chr->mJump.mState == 0
	    && chr->mLadder.mState == 0 && chr->mRope.mState == 0 && chr->mPush.mState == 0 && chr->mTumble.mState == 0
	    && chr->mCopter.mState == 0 && chr->mPopper.mState == 0 && chr->mRadar.mState == 0 && chr->GetAnimId() != 0x79
	    && chr->GetAnimId() != 0x89 && chr->GetAnimId() != 0xE0) {
		f32 remote  = gWiiPointer.mRemoteAcceleration[1].z - gWiiPointer.mRemoteAcceleration[2].z;
		f32 nunchuk = gWiiPointer.mNunchukAcceleration[1].z - gWiiPointer.mNunchukAcceleration[2].z;
		if (fn_800D1740(remote) > 2.0f || fn_800D1740(nunchuk) > 2.0f) {
			chr->_1C9B8[0] = 0.5f * gRefreshRate;
		} else {
			chr->_1C9B8[0] -= frames;
			if (chr->_1C9B8[0] <= 0.0f) {
				chr->_1C9B8[0] = 0.0f;
				chr->_1C9B8[1] = 0.0f;
			}
		}
		if (chr->_1C9B8[0] > 0.0f) {
			if (fn_800D1740(remote) > 2.0f)
				chr->_1C9B8[1] += frames;
			if (fn_800D1740(nunchuk) > 2.0f)
				chr->_1C9B8[1] += frames;
		}
		if (chr->_1C9B8[1] < 0.2f * gRefreshRate) {
			chr->_1C9B8[2] -= 0.1f * frames;
			if (chr->_1C9B8[2] < 0.0f)
				chr->_1C9B8[2] = 0.0f;
		} else {
			chr->_1C9B8[2] += 0.15f * frames;
			if (chr->_1C9B8[2] > 1.0f)
				chr->_1C9B8[2] = 1.0f;
		}
		if (chr->_1C9B8[2] <= 0.0f) {
			f32 angle      = WrapAngle(chr->mMoveRotate.y - gGame.mMotionControlCamera.mView.mAngle.y);
			chr->_1C9C4[0] = angle / TWO_PI;
		} else {
			chr->mStick[0] = 0.5f * chr->_1C9B8[2];
			chr->mStick[2] = -HALF_PI * fmodf(0.1f * chr->_1C9B8[2] + chr->_1C9C4[0], TWO_PI);
		}
		if (chr->_1C9B8[2] < 1.0f) {
			chr->_1C9C4[0] = 0.0f;
		} else {
			chr->_1C9C4[0] += frames;
			if (chr->_1C9C4[0] > 3.0f * gRefreshRate && (chr->mPlayerFlags & 0x20000000)) {
				chr->mTumble.mState = 10;
			}
			if (!IsWiiSfxPlaying(&gGame.mSound, 0x76))
				PlayWiiSoundEffect(&gGame.mSound, 0x76, 1.0f, 0.0f);
		}
	} else {
		chr->_1C9B8[0] = 0.0f;
		chr->_1C9B8[1] = 0.0f;
		chr->_1C9B8[2] = 0.0f;
		chr->_1C9C4[0] = 0.0f;
	}
	if (gGame.IsRunning() && !gGame.mEvt.mData.mScript && chr->GetAttachment() != 1 && chr->GetAttachment() != 2
	    && chr->GetAttachment() != 3
	    && (chr->mLift.mState == 0
	        || (chr->mLift.mState == 4 && !IsCarryingToothbrush(&chr->mLift) && !IsCarryingSpoon(&chr->mLift) && !IsCarryingMug(&chr->mLift)
	            && !IsCarryingSquirter(&chr->mLift)))
	    && !chr->mHang.mState && !chr->mJump.mState && !chr->mLadder.mState && !chr->mRope.mState && !chr->mPush.mState
	    && !chr->mTumble.mState && !chr->mCopter.mState && !chr->mPopper.mState && !chr->mRadar.mState) {
		if (chr->mPreviousContactObject < 0 && !chr->_0A70 && (pad->mTrigger & 0x0001000000000010ULL) && !(gGame.GetStatusFlags(0x100))
		    && (chr->mHitObjectIndex < 0 || chr->mHitObjectIndex == chr->mFloorContactObjects[0]
		        || (!IsLiftTargetPlug(&chr->mLift) && chr->mHitObjectIndex >= 0
		            && (GetGameObjectByIndex(&gGame, chr->mHitObjectIndex)->mStatus.mFlags & 0x20)))) {
			chr->mNearDistance = 2;
			do {
				if ((chr->mPreviousContactObject = FindNearbyPlayerInteraction(chr, chr->mNearDistance)) >= 0) {
					XObj* object                = GetGameObjectByIndex(&gGame, chr->mPreviousContactObject);
					chr->mContactPos.x          = object->mMoveTranslate.x;
					chr->mContactPos.y          = object->mMoveTranslate.y;
					chr->mContactPos.z          = object->mMoveTranslate.z;
					chr->mLastContactFloorIndex = object->mSelectedFloorIndex;
					chr->mEdgePos               = object->mEdges[object->mSelectedFloorIndex].mContactOffset;
					break;
				}
			} while ((chr->mNearDistance += 1.0f) <= 10);
		}
		if (chr->mPreviousContactObject >= 0) {
			chr->mStick[2] = WrapAngle((f32)atan2(chr->mEdgePos.x - chr->mMoveTranslate.x, chr->mEdgePos.z - chr->mMoveTranslate.z))
			               - gGame.mMotionControlCamera.mView.mAngle.y;
			chr->mStick[0] = 0.8f;
		}
	} else {
		chr->mPreviousContactObject = -1;
		chr->mNearDistance          = 2;
	}
	chr->mLiftPlugFlags &= ~0x80;

	f32 direction;
	f32 speed = 1.2f * chr->mStick[0];

	s32 hangState = chr->mHang.mState;
	if (!hangState && !chr->mJump.mState && !chr->mLadder.mState && !chr->mRope.mState && !chr->mPush.mState && !chr->mTumble.mState
	    && !chr->mCopter.mState && (chr->mPopper.mState == 0 || chr->mPopper.mState == 8 || chr->mPopper.mState == 4)
	    && !chr->mRadar.mState)
		speed *= 1.2f;
	if (speed > 0 && speed < 0.5f)
		speed = 0.5f;
	if (speed > 1.0f)
		speed *= speed;

	// the angle wraps are written out here: retail wraps direction (and the push yaw) in its own register, where the
	// WrapAngle inline leaves a copy (fmr) behind each one
	if (speed > 0) {

		if (gGame.GetStatusFlags(0x100)) {
			if (gGame.GetStatusFlags(0x200)) {
				if ((chr->mStick[2] >= 0 && chr->mStick[2] <= HALF_PI) || (chr->mStick[2] <= 0 && chr->mStick[2] >= -HALF_PI)) {
					direction = chr->mMoveTarget1.y + (2 * DEG2RAD * frames) * (chr->mStick[2] / HALF_PI);
					while (direction < 0.0f)
						direction += TWO_PI;
					while (direction >= TWO_PI)
						direction -= TWO_PI;
				} else if (chr->mStick[2] >= HALF_PI && chr->mStick[2] <= 130 * DEG2RAD) {
					direction = chr->mMoveTarget1.y + (2 * DEG2RAD * frames) * ((130 * DEG2RAD - chr->mStick[2]) / (40 * DEG2RAD));
					while (direction < 0.0f)
						direction += TWO_PI;
					while (direction >= TWO_PI)
						direction -= TWO_PI;
				} else if (chr->mStick[2] <= -HALF_PI && chr->mStick[2] >= -130 * DEG2RAD) {
					direction = chr->mMoveTarget1.y + (2 * DEG2RAD * frames) * ((-130 * DEG2RAD - chr->mStick[2]) / HALF_PI);
					while (direction < 0.0f)
						direction += TWO_PI;
					while (direction >= TWO_PI)
						direction -= TWO_PI;
				} else
					direction = -WrapAngle(chr->mStick[2] + chr->mMoveTarget1.y);
			} else {
				direction = NAN;
				speed     = 0;
			}
		} else if (hangState == 5) {
			f32 yaw   = chr->mMoveRotate.y;
			f32 angle = WrapAngle((chr->mStick[2] + gGame.mMotionControlCamera.mView.mAngle.y) - yaw);
			if (angle >= 30 * DEG2RAD && angle < 150 * DEG2RAD) {
				direction                 = TWO_PI + WrapAngle(HALF_PI + yaw);
				chr->mHang.mMoveDirection = -1;
				speed                     = 0.5f;
			} else if (angle >= 210 * DEG2RAD && angle < 330 * DEG2RAD) {
				direction                 = TWO_PI + WrapAngle(yaw - HALF_PI);
				chr->mHang.mMoveDirection = 1;
				speed                     = 0.5f;
			} else {
				direction                 = NAN;
				speed                     = 0;
				chr->mHang.mMoveDirection = 0;
			}
		} else if (chr->mCopter.mState == 3) {
			chr->mCopter.mMoveDirection = 1;
			direction                   = chr->mStick[2] + gGame.mMotionControlCamera.mView.mAngle.y;
			while (direction < 0.0f)
				direction += TWO_PI;
			while (direction >= TWO_PI)
				direction -= TWO_PI;
		} else if (chr->mCopter.mState == 4 || chr->mCopter.mState == 7 || chr->mCopter.mState == 8) {
			if ((gGame.GetStatusFlags(0x40000000)) && IsStopped(chr)) {
				direction = NAN;
				speed     = 0;
			} else {
				chr->mCopter.mMoveDirection = 1;
				direction                   = chr->mStick[2] + gGame.mMotionControlCamera.mView.mAngle.y;
				while (direction < 0.0f)
					direction += TWO_PI;
				while (direction >= TWO_PI)
					direction -= TWO_PI;
				speed *= 0.75f;
			}
		} else if (chr->mPopper.mState == 3 || chr->mPopper.mState == 4 || chr->mPopper.mState == 8) {
			direction = chr->mStick[2] + gGame.mMotionControlCamera.mView.mAngle.y;
			while (direction < 0.0f)
				direction += TWO_PI;
			while (direction >= TWO_PI)
				direction -= TWO_PI;
		} else if (chr->mRope.mState == 5) {
			if (pad->mStick.y > 0.1f) {
				chr->mRope.mMoveDirection = 1;
				f32 anim                  = GetAnimId();
				if (anim == 139.0f || anim == 138.0f)
					speed = 0.32f;
				else if (anim == 141.0f || anim == 140.0f)
					speed = 0.12f;
				else
					speed = 0;
			} else if (pad->mStick.y < -0.1f) {
				chr->mRope.mMoveDirection = -1;
				speed                     = 0.4f;
			} else {
				chr->mRope.mMoveDirection = 0;
			}
			direction = NAN;
		} else if (chr->mLadder.mState == 5) {
			if (pad->mStick.y > 0.1f) {
				chr->mLadder.mMoveDirection = 1;
				speed                       = 0;
			} else if (pad->mStick.y < -0.1f) {
				chr->mLadder.mMoveDirection = -1;
				speed                       = 0;
			} else {
				chr->mLadder.mMoveDirection = 0;
			}
			direction = NAN;
		} else if (chr->mPush.mState == 2) {
			f32 yaw      = chr->mMoveRotate.y;
			f32 relative = WrapAngle(HALF_PI + ((chr->mStick[2] + gGame.mMotionControlCamera.mView.mAngle.y) - yaw));
			if (relative >= 0 && relative < PI) {
				chr->mPush.mMoveDirection = 1;
			} else {
				chr->mPush.mMoveDirection = -1;
				yaw                       = PI + yaw;
			}
			while (yaw < 0.0f)
				yaw += TWO_PI;
			while (yaw >= TWO_PI)
				yaw -= TWO_PI;
			direction = TWO_PI + yaw;
			if (!chr->mPush.mTarget || ((XActPush*)&chr->mPush)->mPushPositionClamped)
				speed = 0;
			else
				speed = 1.5f;
		} else {
			direction = chr->mStick[2] + gGame.mMotionControlCamera.mView.mAngle.y;
			while (direction < 0.0f)
				direction += TWO_PI;
			while (direction >= TWO_PI)
				direction -= TWO_PI;
		}
		if (__fpclassifyf(chr->mHeldMoveAngle) != FP_NAN)
			direction = chr->mHeldMoveAngle + (chr->mStick[2] - chr->mHeldStickAngle);
		chr->mMoveAngle = direction;
	} else {
		direction = NAN;
		if (hangState == 5 && chr->mHang.mMoveDirection)
			chr->mHang.mMoveDirection = 0;
		else if (chr->mCopter.mState == 3 && chr->mCopter.mMoveDirection)
			chr->mCopter.mMoveDirection = 0;
		else if (chr->mRope.mState == 5) {
			if (GetAnimId() == 139) {
				speed                     = 0.32f;
				chr->mRope.mMoveDirection = 2;
			} else if (GetAnimId() == 141) {
				speed                     = 0.12f;
				chr->mRope.mMoveDirection = 2;
			} else if (chr->mRope.mMoveDirection != 0) {
				chr->mRope.mMoveDirection = 0;
			}
		} else if (chr->mLadder.mState == 5 && chr->mLadder.mMoveDirection)
			chr->mLadder.mMoveDirection = 0;
		else if (chr->mPush.mState == 2 && chr->mPush.mMoveDirection)
			chr->mPush.mMoveDirection = 0;
		chr->mHeldMoveAngle = chr->mMoveAngle = NAN;
	}
	if (__fpclassifyf(chr->mHeldMoveAngle) == FP_NAN)
		chr->mHeldStickAngle = chr->mStick[2];
	else if (Abs(chr->mStick[2] - chr->mHeldStickAngle) >= 3 * DEG2RAD) {
		chr->mHeldMoveAngle = chr->mMoveAngle = NAN;
		chr->mHeldStickAngle                  = chr->mStick[2];
	}
	if (__fpclassifyf(chr->mWalkHeading) != FP_NAN)
		direction = chr->mWalkHeading;
	f32 scale = UpdatePlayerActions(chr, frames);
	if (scale == 0.0f) {
		speed     = 0.0f;
		direction = NAN;
	} else {
		speed *= scale;
	}
	UpdatePlayerSuitChange(chr);
	UpdatePlayerMovementAndContacts(chr, frames, speed, direction);
	UpdatePlayerActionAnimations(chr, frames);
	XObj::UpdateObject(frames);
	((XObj*)chr->mCopterObj)->UpdateObject(frames);
	((XObj*)chr->mPopperObj)->UpdateObject(frames);
	((XObj*)chr->mRadarObj)->UpdateObject(frames);
	((XObj*)chr->mOXObj)->UpdateObject(frames);
	if (IsObjectActive(&chr->mSuits.mObjects[chr->mSuits.mCurrent]) && chr->mSuits.mChangeStep != 3) {
		WiiSuitObject* suit = &chr->mSuits.mObjects[chr->mSuits.mCurrent];
		suit->SetPos(*GetPos());
		suit->SetRot(chr->mMoveRotate);
		suit->SetScale(*GetScale());
		suit->UpdateObject(frames);
	}
	if (IsCarryingToothbrush(&chr->mLift) || IsCarryingSpoon(&chr->mLift) || IsCarryingSquirter(&chr->mLift)) {
		for (s32 i = 0; i < 2; ++i) {
			const Vec& rgb = chr->mAnmQueue.mColorStep[i].mValue;
			CColor* color  = i == 0 ? &chr->mLift.mTarget->mColor1 : &chr->mLift.mTarget->mColor0;
			color->r       = (u32)rgb.x;
			color->g       = (u32)rgb.y;
			color->b       = (u32)rgb.z;
			color->a       = 0;
		}
	}
	if (__fpclassifyf(chr->mWalkHeading) != FP_NAN) {
		CVec& target = chr->mMoveTarget0;
		if (chr->mMoveTranslate == target) {
			chr->mMoveSpeed0.Set(0.0f, 0.0f, 0.0f);
			chr->mWalkHeading = NAN;
		} else if (PSVECDistance(&chr->mMoveTranslate, &target) <= 0.000001f) {
			SetPos(target);
			chr->mMoveSpeed0.Set(0.0f, 0.0f, 0.0f);
			chr->mWalkHeading = NAN;
		}
	}
	if (chr->mJump.mState == 2 || chr->mJump.mState == 3 || chr->mHang.mState == 10 || chr->mRope.mState == 9 || chr->mLadder.mState == 10
	    || chr->mCopter.mState == 5)
		chr->mFallDistance += chr->mLastPos.y - chr->mMoveTranslate.y;
	else
		chr->mFallDistance = 0.0f;
	UpdatePlayerBattery(chr, frames);
	chr->mGroundType             = GetPlayerGroundType(chr);
	chr->mFrameTime.mLastFrame   = chr->mFrameTime.mFrame;
	chr->mFrameTime.mFrame       = GetAnimFrame();
	PlayerFrameParticle particle = ProcessPlayerFrameEvents(chr, &chr->mFrameTime, gPlayerAnimationFrameEvents);
	if (particle.mCode >= 0 && !gGame.GetStatusFlags(0x100)) {
		EffectInstance* effect = fn_800A11A4(&gGame.mEvtPtcl, particle.mCode | 0x10000, 1);
		if (effect != NULL) {
			effect->pos   = particle.mTranslate;
			effect->rot   = particle.mRotate;
			effect->scale = particle.mScale;
			UpdateEffectMtx(effect);
		}
	}
	if (((XObjAttachment*)chr->mCopterObj)->mVisible) {
		ProcessPlayerFrameEvents((XObj*)chr->mCopterObj, NULL, gCopterAnimationFrameEvents);
	}
	CVec neck          = GetJointWorldPos("neck");
	chr->mLook.mOffset = (Vec) { 0.0f, neck.y - chr->mMoveTranslate.y + 3.0f * chr->mMoveScale.y, 0.0f };
	if (chr->mLook.mActive == 0) {
		if (chr->mLook.mFade < chr->mLook.mPulse) {
			chr->mLook.mFade -= 1.0f / (0.3f * gRefreshRate) * frames;
			if (chr->mLook.mFade <= chr->mLook.mPulse)
				chr->mLook.mFade = chr->mLook.mPulse;
		}
	} else if (chr->mLook.mFade < 1.0f) {
		chr->mLook.mFade += 1.0f / (0.3f * gRefreshRate) * frames;
		chr->mLook.mPulse = chr->mLook.mFade;
		if (chr->mLook.mFade >= 1.0f) {
			chr->mLook.mPulse = 2.0f;
			chr->mLook.mFade  = 1.0f;
		}
	}
	if (chr->mLook.mFade < chr->mLook.mPulse) {
		chr->mLook.mPhase = fmodf(6.0f * frames + chr->mLook.mPhase, TWO_PI);
		if (chr->mLook.mPulse >= 1.0f) {
			chr->mLook.mPulse += 1.0f / (0.3f * gRefreshRate) * frames;
			if (chr->mLook.mPulse >= 2.0f)
				chr->mLook.mPulse = fmodf(chr->mLook.mPulse, 2.0f);
		}
	}
	UpdateParticlePool(&chr->mDust.mPtcl, frames);
	UpdateTumbleFade(&chr->mTumble.mFade, chr->mTumble.mState == 2 || chr->mTumble.mState == 16, frames);
	UpdateRadarBeamAnimation((XActRadar*)&chr->mRadar, frames);
	for (s32 i = 0; i < 10; ++i) {
		if (gPointParticleSlots[i]) {
			UpdatePointParticleDisplay(gPointParticleSlots[i], frames);
			if (gPointParticleSlots[i]->IsFree())
				gPointParticleSlots[i] = NULL;
		}
	}
	UpdateSpiralTrail(&lbl_8045BBD0, frames);
	if (chr->mPadReset) {
		gPlayerTurnState.previousHeading = gPlayerTurnState.heading;
		gPlayerTurnState.heading         = WrapAngle(chr->mMoveRotate.y);
		gPlayerTurnState.previousChange  = gPlayerTurnState.change;
		gPlayerTurnState.change          = gPlayerTurnState.heading - gPlayerTurnState.previousHeading;
		if (gPlayerTurnState.change > PI)
			gPlayerTurnState.change -= TWO_PI;
		else if (gPlayerTurnState.change < -PI)
			gPlayerTurnState.change += TWO_PI;
		if (chr->mLift.mState || chr->mCopter.mState || chr->mPopper.mState == 4) {
			gPlayerTurnState.total = 0.0f;
		} else {
			gPlayerTurnState.total += gPlayerTurnState.change;
			if (gPlayerTurnState.change * gPlayerTurnState.previousChange <= 0.0f)
				gPlayerTurnState.total = 0.0f;
			else if (fn_800D1740(gPlayerTurnState.change) < 10.0f * DEG2RAD)
				gPlayerTurnState.total = 0.0f;
		}
		s32 turns = (s32)(gPlayerTurnState.total / TWO_PI);
		if ((turns < 0 ? -turns : turns) >= 5) {
			chr->mTumble.mState    = 10;
			gPlayerTurnState.total = 0.0f;
		}
	}
	if (chr->mCameraTargetMargin < chr->mCameraTargetMarginSetting) {
		if ((chr->mCameraTargetMargin += 0.01f * frames) > chr->mCameraTargetMarginSetting)
			chr->mCameraTargetMargin = chr->mCameraTargetMarginSetting;
	} else if (chr->mCameraTargetMargin > chr->mCameraTargetMarginSetting) {
		if ((chr->mCameraTargetMargin -= 0.01f * frames) < chr->mCameraTargetMarginSetting)
			chr->mCameraTargetMargin = chr->mCameraTargetMarginSetting;
	}
	((CObjVec*)&chr->mEyeAim)->Step(frames);
	UpdatePlayerHeadOffset(chr, frames);
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

// NONMATCHING: the action state flow and Wii pad masks are present; the pointer-
// driven ring-selection branches, automatic approach timing and aggregate/register
// allocation still differ from retail
f32 UpdatePlayerActions(WiiPlayerState* player, u32 frames)
{
	if (GetMaskedPlayerStatus(player, 0x80000000))
		return 0;
	f32 speed          = -1;
	const CPadOne* pad = &player->mPad;
	if (gGame.IsRunning() && !IsEventScriptActive(&gGame) && (player->mPlayerFlags & 0x20000000)
	    && (IsLiftActionState(&player->mLift, 0) || IsLiftActionState(&player->mLift, 4))
	    && (IsHangActionState(&player->mHang, 0) && IsJumpActionState(&player->mJump, 0) && IsLadderActionState(&player->mLadder, 0)
	        && IsRopeActionState(&player->mRope, 0) && IsPushActionState(&player->mPush, 0) && IsTumbleActionState(&player->mTumble, 0)
	        && IsCopterActionState(&player->mCopter, 0) && IsPopperActionState(&player->mPopper, 0)
	        && IsRadarActionState(&player->mRadar, 0))) {
		if (!gGame.mMenu.mActive) {
			if ((gWiiPointer.mTrigger & 0x100) && !(pad->mButton & 0x0000000000800000ULL) && (gGame.GetStatusFlags(0x800000000000ULL))
			    && player->mAttachmentRequest < 0) {
				if (!(gGame.GetStatusFlags(0x100)))
					OpenPlayerEquipmentRings(player);
				else
					PlayWiiSoundEffect(&gGame.mSound, 0xF2, 1.0f, 0.0f);
			} else if (!IsPlayerAttachmentEquipped(player, 0) && (pad->mTrigger & 0x0000002000020000ULL)
			           && !(pad->mButton & 0x0000000000800000ULL) && (!(gGame.GetStatusFlags(0x100)) && player->mAttachmentRequest < 0)) {
				if (player->mAttachment != 0) {
					player->mAttachmentRequest = 0;
					player->mAttachmentStep    = 0;
				}
				return 0;
			}
		}
		if (!(gGame.GetStatusFlags(0x400))
		    && (IsPlayerAttachmentEquipped(player, 0) || IsPlayerAttachmentEquipped(player, 1) || IsPlayerAttachmentEquipped(player, 2)
		        || IsPlayerAttachmentEquipped(player, 3) || IsPlayerAttachmentEquipped(player, 4) || IsPlayerAttachmentEquipped(player, 5)
		        || IsPlayerAttachmentEquipped(player, 6) || IsPlayerAttachmentEquipped(player, 7)
		        || IsPlayerAttachmentEquipped(player, 8))) {
			BOOL press = (pad->mTrigger & 0x0000001000010000ULL) && !(gGame.GetStatusFlags(0x500)) && player->mAttachmentRequest < 0;
			s32 mode;
			if (IsPlayerAttachmentEquipped(player, 1) || IsPlayerAttachmentEquipped(player, 2) || IsPlayerAttachmentEquipped(player, 3)) {
				switch (player->mAttachmentActionType) {
				case 1:
					mode = 1;
					break;
				case 2:
					mode = 2;
					break;
				default:
					mode = 0;
				}
			} else if (IsPlayerAttachmentEquipped(player, 6))
				mode = 0;
			else if (IsPlayerAttachmentEquipped(player, 4) || IsPlayerAttachmentEquipped(player, 5) || IsPlayerAttachmentEquipped(player, 7)
			         || IsPlayerAttachmentEquipped(player, 8))
				switch (player->mToolActionType) {
				case 1:
					mode = 1;
					break;
				default:
					mode = 0;
				}
			else
				mode = 2;
			{
				// what A may do this frame: the equipment's mode (0 nothing, 1 put it away first, 2 act) and whether the hands are free
				struct {
					s32 mode;
					BOOL emptyHands;
				} use          = { 0, 0 };
				use.mode       = mode;
				use.emptyHands = IsPlayerAttachmentEquipped(player, 0);
				if (press && player->mLetickerIndex >= 0) {
					XObj* object = GetGameObjectByIndex(&gGame, player->mLetickerIndex);
					if (object->mStatus.mFlags & 0x10000000000ULL)
						TryStartObjectEvent(&gGame, object->GetEventScript(), object, -1, -1);
				} else if (press || (player->mPreviousContactObject >= 0 && player->mPreviousContactObject == player->mHitObjectIndex)
				           || player->mClimbType) {
					BOOL handled       = FALSE;
					player->mClimbType = 0;
					if (use.emptyHands && player->mFloorContactObjects[2] >= 0
					    && player->mFloorContactObjects[2] != player->mFloorContactObjects[0]) {
						XObj* object = GetGameObjectByIndex(&gGame, player->mFloorContactObjects[2]);
						if (IsObjectLiftable(object)) {
							if ((IsLiftActionState(&player->mLift, 0) || IsLiftActionState(&player->mLift, 4))
							    && object->mMaxFloorHeight > 1.f + player->mMoveTranslate.y) {
								if (IsLiftTargetPlug(&player->mLift))
									ReleasePlayerLiftTarget(player);
								player->mLift.mState = 1;
								SetPlayerActionTarget(&player->mLift, object);
								player->mLift.mTargetState = 0;
								handled                    = true;
							}
						} else if (object->mStatus.mFlags & 0x10000000000ULL) {
							TryStartObjectEvent(&gGame, object->GetEventScript(), object, -1, -1);
							handled = true;
						}
					} else if (use.emptyHands && player->mFloorContactObjects[2] >= 0
					           && player->mFloorContactObjects[2] == player->mFloorContactObjects[0]
					           && player->mHitObjectIndex == player->mFloorContactObjects[2]) {
						XObj* object = GetGameObjectByIndex(&gGame, player->mFloorContactObjects[2]);
						if ((object->mStatus.mFlags & 0x10000000000ULL) && !(object->mStatus.mFlags & 0x100000000000ULL)
						    && (s16)GetObjectCategory(object) != 8) {
							TryStartObjectEvent(&gGame, object->GetEventScript(), object, -1, -1);
							handled = true;
						}
					}
					if (!handled && (IsLiftActionState(&player->mLift, 0) || IsLiftActionState(&player->mLift, 4))
					    && player->mHitObjectIndex >= 0) {
						XObj* object = GetGameObjectByIndex(&gGame, player->mHitObjectIndex);
						if (use.emptyHands && IsObjectLiftable(object) && player->mHitObjectIndex != player->mFloorContactObjects[0]
						    && object->mMaxFloorHeight > 1.f + player->mMoveTranslate.y) {
							if (IsLiftTargetPlug(&player->mLift))
								ReleasePlayerLiftTarget(player);
							player->mLift.mState = 1;
							SetPlayerActionTarget(&player->mLift, GetGameObjectByIndex(&gGame, player->mHitObjectIndex));
							player->mLift.mTargetState = 0;
						} else if (use.emptyHands && IsObjectClimbableLedge(object)) {
							// Only the Japanese release starts a hang or a jump from this height check.
							// NONMATCHING: the other releases still take the height and compare it with zero (0x878 to 0x8A0 in
							// GGTE01, both ways of the compare lead to the same place); here the compiler drops the unused value.
							// It also drops it for an empty if/else, an empty inline in either arm and an else-if on the height;
							// only a body with code keeps retail's beq/b pair (US 0x800B3E68)
							f32 height = fn_800D1740(object->mSelectedFloorHeight - player->mMoveTranslate.y);
							if (height != 0) {
								if (height >= 12 && height <= 12.5f) {
									if (gGame.GetStatusFlags(0x80000000000ULL)) {
										if (IsLiftTargetPlug(&player->mLift))
											ReleasePlayerLiftTarget(player);
										SetHangActionState(&player->mHang, 1);
										SetPlayerActionTarget(&player->mHang, object);
									}
								} else if (gGame.GetStatusFlags(0x80000000000ULL)) {
									if (height <= 4) {
										if (IsLiftTargetPlug(&player->mLift))
											ReleasePlayerLiftTarget(player);
										SetPlayerActionState(&player->mJump, 8);
										SetPlayerActionTarget(&player->mJump, object);
									} else if (height <= 8) {
										if (IsLiftTargetPlug(&player->mLift))
											ReleasePlayerLiftTarget(player);
										SetPlayerActionState(&player->mJump, 11);
										SetPlayerActionTarget(&player->mJump, object);
									} else if (height <= 12.5f) {
										if (IsLiftTargetPlug(&player->mLift))
											ReleasePlayerLiftTarget(player);
										SetPlayerActionState(&player->mJump, 14);
										SetPlayerActionTarget(&player->mJump, object);
									}
								}
							}
						} else if (IsObjectLadder(object)) {
							s32 dir = 0;
							if (object->mSelectedFloorIndex == 0) {
								if (player->mHitObjectIndex == player->mFloorContactObjects[0]) {
									f32 angle = WrapPlayerActionAngle(player->mMoveRotate.y - object->mMoveRotate.y);
									if (angle <= PI * .5f || angle >= PI * 1.5f)
										dir = -1;
								} else if (object->mSelectedFloorHeight > player->mMaxFloorHeight
								           && IsPositionBehindObject(&player->mLadder, player->GetPos(), object))
									dir = 1;
							}
							if (dir != 0) {
								if (use.mode == 1) {
									player->mClimbType     = 0;
									player->mNextClimbType = 1;
									if (player->mAttachment != 0) {
										player->mAttachmentRequest = 0;
										player->mAttachmentStep    = 0;
									}
								} else if (use.mode == 2) {
									if (IsLiftTargetPlug(&player->mLift))
										ReleasePlayerLiftTarget(player);
									if (dir > 0)
										player->mLadder.mState = 1;
									else if (dir < 0)
										player->mLadder.mState = 2;
									SetPlayerActionTarget(&player->mLadder, object);
								}
								StopPlayerMotion(player);
							}
						} else if (IsObjectRope(object)) {
							s32 dir = 0;
							if (object->mSelectedFloorIndex == 0) {
								if (player->mHitObjectIndex == player->mFloorContactObjects[0]) {
									f32 angle = WrapPlayerActionAngle(player->mMoveRotate.y - object->mMoveRotate.y);
									if (angle <= PI * .5f || angle >= PI * 1.5f)
										dir = -1;
								} else if (object->mSelectedFloorHeight > player->mMaxFloorHeight)
									dir = 1;
							}
							if (dir != 0) {
								if (use.mode == 1) {
									player->mClimbType     = 0;
									player->mNextClimbType = 2;
									if (player->mAttachment != 0) {
										player->mAttachmentRequest = 0;
										player->mAttachmentStep    = 0;
									}
								} else if (use.mode == 2) {
									if (IsLiftTargetPlug(&player->mLift))
										ReleasePlayerLiftTarget(player);
									if (dir > 0)
										player->mRope.mState = 1;
									else if (dir < 0)
										player->mRope.mState = 2;
									SetPlayerActionTarget(&player->mRope, object);
								}
								StopPlayerMotion(player);
							}
						}
						if (!(gGame.GetStatusFlags(0x80000000000ULL)))
							if ((IsLiftActionState(&player->mLift, 0)
							     || (IsLiftActionState(&player->mLift, 4) && IsLiftTargetPlug(&player->mLift)))
							    && (IsHangActionState(&player->mHang, 0) && IsJumpActionState(&player->mJump, 0)
							        && IsLadderActionState(&player->mLadder, 0) && IsRopeActionState(&player->mRope, 0)
							        && IsPushActionState(&player->mPush, 0) && IsTumbleActionState(&player->mTumble, 0)
							        && IsCopterActionState(&player->mCopter, 0) && IsPopperActionState(&player->mPopper, 0)
							        && IsRadarActionState(&player->mRadar, 0))
							    && IsObjectPushable(object) && player->mHitObjectIndex != player->mFloorContactObjects[0]) {
								if (IsLiftTargetPlug(&player->mLift))
									ReleasePlayerLiftTarget(player);
								StopPlayerMotion(player);
								SetPushActionState(&player->mPush, 1);
								SetPlayerActionTarget(&player->mPush, object);
								SetPushTargetState(&player->mPush, 0);
							}
					}
				}
			}
		}
		if (!IsEventScriptActive(&gGame) && !(gGame.GetStatusFlags(0x400)) && player->mAttachmentRequest < 0
		    && (IsLiftActionState(&player->mLift, 0) || IsLiftActionState(&player->mLift, 4))
		    && (IsHangActionState(&player->mHang, 0) && IsJumpActionState(&player->mJump, 0) && IsLadderActionState(&player->mLadder, 0)
		        && IsRopeActionState(&player->mRope, 0) && IsPushActionState(&player->mPush, 0) && IsTumbleActionState(&player->mTumble, 0)
		        && IsCopterActionState(&player->mCopter, 0) && IsPopperActionState(&player->mPopper, 0)
		        && IsRadarActionState(&player->mRadar, 0))) {
			if (IsPlayerAttachmentEquipped(player, 0)) {
				if (!(gGame.GetStatusFlags(0x100))) {
					if (player->mPlug.mConnection && ((pad->mTrigger & 0x0000001000010000ULL) || (pad->mTrigger & 0x0000002000020000ULL))) {
						if (player->mLiftPlugTarget) {
							TryStartObjectEvent(&gGame, player->mLiftPlugTarget->GetEventScript(), player->mLiftPlugTarget, -1, -1);
							StopPlayerMotion(player);
							speed = 0;
						}
					} else if (IsLiftActionState(&player->mLift, 0) && IsPushActionState(&player->mPush, 0) && !player->mPlug.mConnection
					           && player->mPreviousContactObject < 0 && (pad->mButton & 0x0001000000000010ULL))
						player->mLiftPlugFlags |= 0x80;
				}
			} else if (IsPlayerAttachmentEquipped(player, 1)) {
				if ((pad->mTrigger & 0x0000001000010000ULL) && !(gGame.GetStatusFlags(0x100))) {
					SetCopterActionState(&player->mCopter, 3);
					player->SetAnim(39, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
					PlayCopterFlightAnimation((XObjCopter*)player->mCopterObj);
					StopPlayerMotion(player);
				}
			} else if (IsPlayerAttachmentEquipped(player, 2)) {
				s32 state = 0;
				if (pad->mTrigger & 0x0000001000010000ULL)
					state = 7;
				if (state != 0) {
					player->mPopper.mState = state;
					SetObjectVectorTarget((CObjVec*)&player->mMoveRotate, player->mMoveRotate);
				}
			} else if (IsPlayerAttachmentEquipped(player, 3)) {
				if ((pad->mTrigger & 0x0000001000010000ULL) && !(gGame.GetStatusFlags(0x100))) {
					player->mRadar.mState = 3;
					StartPlayerRadarScan((XObjRadar*)player->mRadarObj);
				}
			} else if (IsPlayerAttachmentEquipped(player, 4) && !IsLiftActionState(&player->mLift, 0)
			           && IsCarryingToothbrush(&player->mLift)) {
				if ((pad->mTrigger & 0x0000001000010000ULL) && !(gGame.GetStatusFlags(0x100)))
					player->mLift.mState = 28;
			} else if (IsPlayerAttachmentEquipped(player, 5) && !IsLiftActionState(&player->mLift, 0) && IsCarryingSpoon(&player->mLift)) {
				if ((pad->mTrigger & 0x0000001000010000ULL) && !(gGame.GetStatusFlags(0x100)) && player->mAnimId != 0x8ae
				    && player->GetAnimId() != 0x155 && player->GetAnimId() != 0x156 && player->GetAnimId() != 0x157)
					player->mLift.mState = 32;
			} else if (IsPlayerAttachmentEquipped(player, 6) && !IsLiftActionState(&player->mLift, 0) && IsCarryingMug(&player->mLift)) {
				if ((pad->mTrigger & 0x0000001000010000ULL) && !(gGame.GetStatusFlags(0x100)))
					player->mLift.mState = 34;
			} else if (IsPlayerAttachmentEquipped(player, 7) && !IsLiftActionState(&player->mLift, 0)
			           && IsCarryingSquirter(&player->mLift)) {
				if ((pad->mTrigger & 0x0000001000010000ULL) && !(gGame.GetStatusFlags(0x100)))
					player->mLift.mState = 38;
			} else
				IsPlayerAttachmentEquipped(player, 8);
		}
	}
	if (player->mLiftPlugFlags & 0x80) {
		BOOL lift = FALSE;
		if (player->mAPressed)
			player->mPressTimer += frames;
		else {
			player->mPressTimer = 0;
			player->mAPressed   = (pad->mTrigger & 0x0000001000010000ULL) != 0;
		}
		if ((player->mLiftPlugFlags & 2) && player->mPressTimer >= gRefreshRate * ((s32)GetInteractionParameter(3) / 100.f)) {
			if (player->mAPressed)
				lift = TRUE;
		} else if (player->mLiftPlugFlags & 1) {
			CVec joint = player->mPlug.GetWorldPos("plug_model");
			f32 angle  = WrapPlayerActionAngle(player->mMoveRotate.y - GetHeadingBetweenPositions(&player->mMoveTranslate, &joint));
			if ((angle < PI * .5f || angle > PI * 1.5f) && player->mAPressed)
				lift = TRUE;
		} else if (!(player->mLiftPlugFlags & 1) && !(player->mLiftPlugFlags & 2))
			lift = TRUE;
		if (lift) {
			player->mLift.mState = 1;
			SetPlayerActionTarget(&player->mLift, &player->mPlug);
			player->mLift.mTargetState = 0;
			player->mPressTimer        = 0;
		}
	} else
		player->mAPressed = player->mPressTimer = 0;
	if (IsPlayerAttachmentEquipped(player, 2) && !IsEventScriptActive(&gGame)
	    && (IsPopperActionState(&player->mPopper, 7) || IsPopperActionState(&player->mPopper, 8))
	    && IsPopperFireAnimationSelected((XObjPopper*)player->mPopperObj) && (pad->mTrigger & 0x0000001000010000ULL))
		player->mPopper.mState = 5;
	if ((gWiiPointer.mTrigger & 0x40) && !IsEventScriptActive(&gGame) && !(gGame.GetStatusFlags(0x400)) && player->mAttachmentRequest < 0
	    && !(gGame.GetStatusFlags(0x20)) && !(gGame.GetStatusFlags(0x100))
	    && (IsLiftActionState(&player->mLift, 0) || IsLiftActionState(&player->mLift, 4))
	    && (IsHangActionState(&player->mHang, 0) || IsHangActionState(&player->mHang, 5)) && IsJumpActionState(&player->mJump, 0)
	    && (IsLadderActionState(&player->mLadder, 0) || IsLadderActionState(&player->mLadder, 5))
	    && (IsRopeActionState(&player->mRope, 0) || IsRopeActionState(&player->mRope, 5))
	    && (IsPushActionState(&player->mPush, 0) || IsPushActionState(&player->mPush, 2)) && IsTumbleActionState(&player->mTumble, 0)
	    && IsCopterActionState(&player->mCopter, 0) && IsPopperActionState(&player->mPopper, 0) && IsRadarActionState(&player->mRadar, 0))
		TryStartObjectEvent(&gGame, GetStageZButtonScript(), NULL, -1, -1);
	if (player->mPreviousContactObject >= 0) {
		BOOL clear = FALSE;
		if (IsEventScriptActive(&gGame) || (gGame.GetStatusFlags(0x400))
		    || !(IsLiftActionState(&player->mLift, 0) || IsLiftActionState(&player->mLift, 4))
		    || !(IsHangActionState(&player->mHang, 0) && IsJumpActionState(&player->mJump, 0) && IsLadderActionState(&player->mLadder, 0)
		         && IsRopeActionState(&player->mRope, 0) && IsPushActionState(&player->mPush, 0) && IsTumbleActionState(&player->mTumble, 0)
		         && IsCopterActionState(&player->mCopter, 0) && IsPopperActionState(&player->mPopper, 0)
		         && IsRadarActionState(&player->mRadar, 0)))
			clear = TRUE;
		else if (player->mHitObjectIndex >= 0 && player->mHitObjectIndex != player->mFloorContactObjects[0]) {
			// he touches something he does not stand on: both arms clear it
			if (player->mPreviousContactObject != player->mHitObjectIndex
			    || player->mLastContactFloorIndex != GetGameObjectByIndex(&gGame, player->mHitObjectIndex)->mSelectedFloorIndex)
				clear = TRUE;
			else if (player->mPreviousContactObject == player->mHitObjectIndex
			         && player->mLastContactFloorIndex == GetGameObjectByIndex(&gGame, player->mHitObjectIndex)->mSelectedFloorIndex)
				clear = TRUE;
		} else if (player->mHitObjectIndex >= 0 && player->mHitObjectIndex == player->mFloorContactObjects[0]
		           && player->mPreviousContactObject == player->mHitObjectIndex
		           && player->mLastContactFloorIndex == GetGameObjectByIndex(&gGame, player->mHitObjectIndex)->mSelectedFloorIndex)
			clear = TRUE;
		if (!clear && PSVECDistance(&player->mMoveTranslate, &player->mAutoActionStart) <= 0.09f && ++player->mAutoActionFrames > 1) {
			clear = TRUE;
		}
		player->mAutoActionStart = player->mMoveTranslate;
		XObj* object             = GetGameObjectByIndex(&gGame, player->mPreviousContactObject);
		CVec* position           = object->GetPos();
		if (!fn_800CDDAC(object) || !(object->mStatus.mFlags & 0x10000) || player->mContactPos.x != position->x
		    || player->mContactPos.y != position->y || player->mContactPos.z != position->z)
			clear = TRUE;
		if (clear) {
			player->mPreviousContactObject = -1;
			player->mNearDistance          = 2;
		}
	}
	HandleLiftActionInput((XActLift*)&player->mLift, pad, &speed);
	HandleHangActionInput(&player->mHang, pad, &speed);
	SuppressMovementDuringJump((XActJump*)&player->mJump, pad, &speed);
	HandleLadderActionInput((XActLadder*)&player->mLadder, pad, &speed);
	HandleRopeActionInput((XActRope*)&player->mRope, pad, &speed);
	HandlePushActionInput((XActPush*)&player->mPush, pad, &speed);
	HandleTumbleActionInput((XActTumble*)&player->mTumble, pad, &speed);
	HandleCopterActionInput((XAction*)&player->mCopter, pad, &speed);
	ApplyPopperMovementWeight((XActPopper*)&player->mPopper, pad, &speed);
	ApplyRadarMovementWeight((XActRadar*)&player->mRadar, pad, &speed);
	UpdatePlayerAttachmentRequest(player, &speed);
	UpdatePlayerEquipmentRings(&player->mMenu, frames);
	UpdateOXMenuTransition(&player->mOXMenu, frames);
	if (-1.f == speed)
		speed = 1.f;
	return speed;
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

// NONMATCHING: the complete search and eligibility checks are present; aggregate return
// homes and float register/stack scheduling differ through the floor transform and probe loop
s32 FindNearbyPlayerInteraction(WiiPlayerState* player, f32 extraRadius)
{
	f32 radius = player->mMoveScale.x * GetFloorExtent(&player->mFloors, 0).x;
	f32 height = player->mMoveScale.x * (GetFloorCenterOffset(&player->mFloors, 0).y + GetFloorExtent(&player->mFloors, 0).y);
	Mtx rotation;
	PSMTXRotRad(rotation, 'y', player->mMoveRotate.y);
	Vec offset = player->mMoveScale * GetFloorCenterOffset(&player->mFloors, 0);
	PSMTXMultVec(rotation, &offset, &offset);
	const f32 x    = player->mMoveTranslate.x;
	const f32 y    = player->mMoveTranslate.y;
	const f32 z    = player->mMoveTranslate.z;
	f32 minHeight  = 1.0f + y;
	f32 bestHeight = NAN;
	s32 best       = -1;
	CVec push, contact;
	for (s32 i = 0; i < gGame.mXObjs.mObjectCount; i++) {
		XObj* object = gGame.mXObjs.mObjects[i];
		u64 flags    = object->mStatus.mFlags;
		if (!(flags & 1) || !(flags & 0x10000) || (flags & 0x10000000) || object == player || object->GetKind() >= 10000
		    || object->GetPool() == 7) {
			continue;
		}
		if (player->mFloorContactObjects[0] >= 0 && gGame.mXObjs.mObjects[player->mFloorContactObjects[0]] == object) {
			continue;
		}
		if ((flags & 0x0000020000000000ULL) || (flags & 0x100000) || (flags & 0x0002000000000000ULL)) {
			continue;
		}
		const Vec* objectPos = object->GetPos();
		f32 dx               = objectPos->x - x;
		f32 dz               = objectPos->z - z;
		if (FSqrt(dz * dz + dx * dx) > 100.0f + (0.9f * radius + object->mFloorXZExtent[0])) {
			continue;
		}
		u32 hit = object->TestFloorCollision(&CVec(x, y, z), &CVec(x, y, z), radius + extraRadius, height, &push, &contact, 0, 0);
		if ((hit & 3) != 3) {
			continue;
		}
		SelectHighestActiveFloor(object);
		if (best < 0 || __fpclassifyf(bestHeight) == FP_NAN || object->mMaxFloorHeight > bestHeight) {
			BOOL eligible = FALSE;
			CVec target   = object->mMoveTranslate + GetFloorCenterOffset(&object->mFloors, object->mSelectedFloorIndex);
			f32 angle     = atan2(target.x - player->GetPos()->x, target.z - player->GetPos()->z);
			angle         = WrapAngle(angle - player->mMoveRotate.y);
			if (angle <= HALF_PI || angle >= 3.0f * HALF_PI) {
				if (object->GetEventScript() != NULL) {
					eligible = TRUE;
				} else {
					if ((flags & 0x0000000100000000ULL) && object->mMaxFloorHeight > minHeight) {
						eligible = TRUE;
					}
					if (flags & 0x0000000800000000ULL) {
						if (object->mSelectedFloorIndex == 0 && object->mSelectedFloorHeight > player->mMaxFloorHeight
						    && IsPositionBehindObject(&player->mLadder, player->GetPos(), object)) {
							eligible = TRUE;
						}
					} else if ((flags & 0x0000001000000000ULL) && object->mSelectedFloorIndex == 0
					           && object->mSelectedFloorHeight > player->mMaxFloorHeight) {
						eligible = TRUE;
					}
				}
			}
			if (eligible) {
				bestHeight = object->mSelectedFloorHeight;
				best       = i;
			}
		}
	}
	return best;
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

// one action marker over the object: shown when the player can do that action there, or when the object's placement
// sets its own markers and includes it; allowed limits the markers to the ones this kind of contact can show
#define ADD_OBJECT_MARKER(allowed, bit)                                                                              \
	do {                                                                                                             \
		if (((allowed) & (bit))                                                                                      \
		    && ((GetObjectMarkerOverride(object) == 0x80000000 && (available & (bit)))                               \
		        || (GetObjectMarkerOverride(object) != 0x80000000 && GetMaskedObjectMarkerOverride(object, (bit))))) \
			marker |= (bit);                                                                                         \
	} while (0)
#define ADD_OBJECT_MARKERS(allowed)   \
	ADD_OBJECT_MARKER(allowed, 1);    \
	ADD_OBJECT_MARKER(allowed, 2);    \
	ADD_OBJECT_MARKER(allowed, 4);    \
	ADD_OBJECT_MARKER(allowed, 8);    \
	ADD_OBJECT_MARKER(allowed, 0x10); \
	ADD_OBJECT_MARKER(allowed, 0x20); \
	ADD_OBJECT_MARKER(allowed, 0x40); \
	ADD_OBJECT_MARKER(allowed, 0x80)

// NONMATCHING: the complete movement, collision and interaction flow is present,
// including the Wii-only stage repairs and lift states; aggregate temporary homes,
// duplicated branch tails and register/schedule allocation still differ
void UpdatePlayerMovementAndContacts(WiiPlayerState* player, u32 frames, f32 speed, f32 direction)
{
	if (GetMaskedPlayerStatus(player, 0x80000000))
		return;
	CVec floor(0, 0, 0);
	{
		if (!IsCopterActionState(&player->mCopter, 3) && !IsCopterActionState(&player->mCopter, 4)
		    && !IsCopterActionState(&player->mCopter, 7) && !IsCopterActionState(&player->mCopter, 8)
		    && !IsCopterActionState(&player->mCopter, 5) && !(IsPushActionState(&player->mPush, 2) && HasPlayerActionTarget(&player->mPush))
		    && player->mFloorContactObjects[0] >= 0 && player->mFloorContactObjects[0] == player->mFloorContactObjects[1]) {
			{
				XObj* support     = GetGameObjectByIndex(&gGame, player->mFloorContactObjects[0]);
				const CVec& delta = *support->GetPos() - *GetObjectPreviousMoveTranslation(support);
				CopyVecToCVec(&floor, &delta);
			}
		}
		player->mFloorContactObjects[1] = player->mFloorContactObjects[0];
		player->mFloorContactObjects[0] = -1;
	}
	CVec hang(0, 0, 0);
	{
		if (!IsHangActionState(&player->mHang, 0) && HasPlayerActionTarget(&player->mHang)) {
			XObj* support     = GetPlayerActionTarget(&player->mHang);
			const CVec& delta = *support->GetPos() - *GetObjectPreviousMoveTranslation(support);
			CopyVecToCVec(&hang, &delta);
		}
	}
	CVec ladder(0, 0, 0);
	{
		if (!IsLadderActionState(&player->mLadder, 0) && !IsLadderActionState(&player->mLadder, 2)
		    && !IsLadderActionState(&player->mLadder, 3) && HasPlayerActionTarget(&player->mLadder)) {
			XObj* target = GetPlayerActionTarget(&player->mLadder);
			if (IsLadderActionState(&player->mLadder, 1)) {
				f32 previous = GetObjectPreviousMoveTranslation(target)->y;
				ladder.y     = target->GetPos()->y - previous;
			} else {
				const CVec& delta = *target->GetPos() - *GetObjectPreviousMoveTranslation(target);
				CopyVecToCVec(&ladder, &delta);
			}
		}
	}
	CVec rope(0, 0, 0);
	{
		if (!IsRopeActionState(&player->mRope, 0) && !IsRopeActionState(&player->mRope, 2) && HasPlayerActionTarget(&player->mRope)) {
			XObj* target = GetPlayerActionTarget(&player->mRope);
			if (IsRopeActionState(&player->mRope, 1) || IsRopeActionState(&player->mRope, 3)) {
				f32 previous = GetObjectPreviousMoveTranslation(target)->y;
				rope.y       = target->GetPos()->y - previous;
			} else {
				const CVec& delta = *target->GetPos() - *GetObjectPreviousMoveTranslation(target);
				CopyVecToCVec(&rope, &delta);
			}
		}
	}
	u32 movementFrames = frames;
	if (movementFrames > 2)
		movementFrames = 2;
	f32 targetX, targetZ;
	{
		if (__fpclassifyf(direction) != FP_NAN) {
			f32 angle = direction >= 0.0f ? direction : -direction;
			if (angle > TWO_PI)
				angle -= TWO_PI;
			if (sBlockedObstacleCount >= 1) {
				f32 scale = 1.0f - 0.15f * sBlockedObstacleCount;
				if (scale < 0.7f)
					scale = 0.7f;
				targetX = movementFrames * (scale * ((speed * PlayerActionSine(angle)) * GetPlayerMoveSpeedScale(player)));
				targetZ = movementFrames * (scale * ((speed * PlayerActionCosine(angle)) * GetPlayerMoveSpeedScale(player)));
			} else {
				targetX
				    = movementFrames * (fn_800E32B8(player, -1) * ((speed * PlayerActionSine(angle)) * GetPlayerMoveSpeedScale(player)));
				targetZ
				    = movementFrames * (fn_800E32B8(player, -1) * ((speed * PlayerActionCosine(angle)) * GetPlayerMoveSpeedScale(player)));
			}
		} else {
			targetX = targetZ = 0.0f;
		}
		if (fn_800D1748(player))
			targetX = targetZ = 0.0f;
		SetObjectVectorTargetX(&player->mMotion, targetX);
		SetObjectVectorTargetZ(&player->mMotion, targetZ);
		player->mMoveSpeed = GetVectorMagnitude(GetObjectVectorTarget(&player->mMotion)) / movementFrames;
		if (__fpclassifyf(player->mWalkHeading) != FP_NAN)
			player->mMoveSpeed = 2.8f * GetVectorMagnitude(GetObjectVectorSpeed((CObjVec*)&player->mMoveTranslate));
	}
	Vec savedRotation = player->mMoveRotate;
	{
		if (!IsHangActionState(&player->mHang, 0)) {
			if (IsHangActionState(&player->mHang, 5)) {
				SetObjectVectorSpeedX(&player->mMotion, 0);
				SetObjectVectorSpeedZ(&player->mMotion, 0);
				if (HasPlayerActionTarget(&player->mHang) && IsActionTargetFloorId(&player->mHang, 0)) {
					// on a round ledge he shuffles along the circle around its centre
					Vec center = *GetHangFloorCenter(&player->mHang);
					f32 radius = GetHangDistance(&player->mHang);
					f32 period = (TWO_PI * radius) / (0.3f * speed);
					f32 angle
					    = movementFrames * ((TWO_PI / period) * (s32)GetPlayerActionMoveDirection(&player->mHang)) + player->mMoveRotate.y;
					player->SetRotationY(WrapPlayerActionAngle(angle));
					f32 facing = PI + angle;
					SetVectorX(&player->mMotion.mValue, (radius * PlayerActionSine(facing) + center.x) - player->mMoveTranslate.x);
					SetVectorZ(&player->mMotion.mValue, (radius * PlayerActionCosine(facing) + center.z) - player->mMoveTranslate.z);
					PlayerActionSine(facing);
					PlayerActionCosine(facing);
				} else {
					SetVectorX(&player->mMotion.mValue, targetX);
					SetVectorZ(&player->mMotion.mValue, targetZ);
				}
			} else {
				SetObjectVectorAndTargetXYZ(&player->mMotion, 0, 0, 0);
			}
		} else if (IsPushActionState(&player->mPush, 2)) {
			SetObjectVectorSpeedX(&player->mMotion, 0);
			SetObjectVectorSpeedZ(&player->mMotion, 0);
			s32 animation = player->GetAnimId();
			if (animation == 64 || animation == 65) {
				SetVectorX(&player->mMotion.mValue, targetX);
				SetVectorZ(&player->mMotion.mValue, targetZ);
			} else {
				SetVectorX(&player->mMotion.mValue, 0);
				SetVectorZ(&player->mMotion.mValue, 0);
			}
		} else if (!IsRopeActionState(&player->mRope, 0)) {
			if (IsRopeActionState(&player->mRope, 5)) {
				SetObjectVectorSpeedX(&player->mMotion, 0);
				SetObjectVectorSpeedZ(&player->mMotion, 0);
				SetVectorX(&player->mMotion.mValue, 0);
				SetVectorZ(&player->mMotion.mValue, 0);
				f32 sign = (s32)GetPlayerActionMoveDirection(&player->mRope);
				if (sign > 0)
					sign = 1;
				if (sign < 0)
					sign = -1;
				SetObjectYAndTarget((CObjVec*)&player->mMoveTranslate, player->mMoveTranslate.y + sign * (speed * movementFrames));
			} else {
				SetObjectVectorAndTargetXYZ(&player->mMotion, 0, 0, 0);
			}
		} else if (!IsLadderActionState(&player->mLadder, 0)) {
			if (IsLadderActionState(&player->mLadder, 5)) {
				SetObjectVectorSpeedX(&player->mMotion, 0);
				SetObjectVectorSpeedZ(&player->mMotion, 0);
				SetVectorX(&player->mMotion.mValue, 0);
				SetVectorZ(&player->mMotion.mValue, 0);
				f32 climbDir = (s32)GetPlayerActionMoveDirection(&player->mLadder);
				SetObjectYAndTarget((CObjVec*)&player->mMoveTranslate, player->mMoveTranslate.y + speed * climbDir);
			} else {
				SetObjectVectorAndTargetXYZ(&player->mMotion, 0, 0, 0);
			}
		} else {
			f32 step = (1.3f * 1.2f) * (0.05f * movementFrames);
			if (!(gGame.GetStatusFlags(0x100))) {
				Vec* current = GetObjectVectorCurrent(&player->mMotion);
				Vec* target  = GetObjectVectorTarget(&player->mMotion);
				f32 rate     = fn_800E32B8(player, -1);
				f32 distance = GetVectorDistance(current, target) / rate;
				if (distance >= 3.0f) {
					f32 difference = fn_800D1740(distance - 8.0f);
					step *= difference / 2.5f;
				}
			}
			SetObjectVectorSpeedX(&player->mMotion, step);
			SetObjectVectorSpeedZ(&player->mMotion, step);
		}
	}
	player->mMotion.Step(frames);
	{
		if (IsHangActionState(&player->mHang, 0) && IsLadderActionState(&player->mLadder, 0) && IsRopeActionState(&player->mRope, 0)
		    && IsPushActionState(&player->mPush, 0) && IsTumbleActionState(&player->mTumble, 0)) {
			if (IsEventScriptActive(&gGame)) {
				if (__fpclassifyf(direction) != FP_NAN && direction >= 0.0f)
					player->SetRotateY(direction, 0.20943952f);
			} else {
				f32 rate;
				{
					f32 current = WrapPlayerActionAngle(player->mMoveRotate.y);
					f32 desired;
					if (__fpclassifyf(direction) != FP_NAN && direction >= 0.0f)
						desired = WrapPlayerActionAngle(direction);
					else
						desired = WrapPlayerActionAngle(GetObjectVectorTarget((CObjVec*)&player->mMoveRotate)->y);
					f32 difference = fn_800D1740(desired - current);
					if (difference > 0.008726646f) {
						if (difference >= TWO_PI)
							difference -= TWO_PI;
						if (difference > PI) {
							if (desired < current)
								desired += TWO_PI;
							else
								desired -= TWO_PI;
						}
						f32 turn = fn_800D1740(desired - current);
						if (gGame.GetStatusFlags(0x400))
							rate = turn * 0.15f;
						else if (gGame.GetStatusFlags(0x800))
							rate = turn * 0.2f;
						else if (IsPlayerAttachmentEquipped(player, 2) || fn_800D1748(player))
							rate = turn * 0.1f;
						else
							rate = turn * 0.2f;
					} else {
						rate = DEG2RAD;
					}
				}
				if ((player->GetAnimId() == 0x895 || player->GetAnimId() == 0x152 || player->GetAnimId() == 0x154)
				    && (IsLiftActionState(&player->mLift, 28) || IsLiftActionState(&player->mLift, 29)
				        || IsLiftActionState(&player->mLift, 30) || IsLiftActionState(&player->mLift, 31)
				        || IsLiftActionState(&player->mLift, 4))) {
					SetObjectXAndTarget(&player->mMotion, 0);
					SetObjectZAndTarget(&player->mMotion, 0);
					if (player->GetAnimId() == 0x895 && __fpclassifyf(direction) != FP_NAN && direction >= 0.0f)
						player->SetRotateY(direction, 0.25f * rate);
				} else if (GetHeldPadButtons(GetPadForPort(&gGame, 0), 0x0001000000000010ULL) && !(gGame.GetStatusFlags(0x400))
				           && IsJumpActionState(&player->mJump, 0)
				           && (IsLiftActionState(&player->mLift, 4) || IsLiftActionState(&player->mLift, 29))
				           && (IsCarryingToothbrush(&player->mLift) || IsCarryingSpoon(&player->mLift)
				               || IsCarryingSquirter(&player->mLift))
				           && player->GetAnimId() != 0x8ae && player->GetAnimId() != 0x8af && player->GetAnimId() != 0x155
				           && player->GetAnimId() != 0x156 && player->GetAnimId() != 0x157) {
					SetObjectXAndTarget(&player->mMotion, 0.7f * player->mMotion.mValue.x);
					SetObjectZAndTarget(&player->mMotion, 0.7f * player->mMotion.mValue.z);
					player->mMoveSpeed = fn_800E32B8(player, -1);
					if (__fpclassifyf(direction) != FP_NAN && direction >= 0.0f) {
						f32 diff = WrapPlayerActionAngle(player->mMoveRotate.y - WrapPlayerActionAngle(direction));
						// he turns only toward a stick ahead of him; with the stick behind him he backs up (anim 0x12a) facing the same way
						if (diff < HALF_PI || diff >= 1.5f * PI)
							player->SetRotateY(direction, 0.25f * rate);
						if (player->mMoveSpeed > 0) {
							if (IsLiftActionState(&player->mLift, 29))
								player->SetAnim(0x153, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
							else if (diff > HALF_PI && diff < 1.5f * PI)
								player->SetAnim(0x12a, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
							else
								player->SetAnim(3, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
						} else if (IsLiftActionState(&player->mLift, 29)) {
							player->SetAnim(0x153, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
						} else {
							player->SetAnim(17, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
						}
					} else {
						SetObjectVectorAndTargetFromVec((CObjVec*)&player->mMoveRotate, player->mMoveRotate);
						if (IsLiftActionState(&player->mLift, 29))
							player->SetAnim(0x153, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
						else
							player->SetAnim(17, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
					}
				} else {
					if (__fpclassifyf(direction) != FP_NAN && direction >= 0.0f)
						player->SetRotateY(direction, rate);
					else
						SetObjectVerticalSpeed((CObjVec*)&player->mMoveRotate, rate);
					if (player->GetAnimId() == 0x12a)
						player->SetAnim(17, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
				}
			}
		}
	}

	player->mMoveTranslate.y += rope.y + (ladder.y + (floor.y + hang.y));
	player->mSlopeMotion.Step(frames);
	void* map    = GetMapPoolEntry(&gGame, 0);
	Vec position = player->mMoveTranslate;
	{
		f32 dx = floor.x + 0.3f * player->mMotion.mValue.x;
		position.x += player->mSlopeMotion.mValue.x + (rope.x + (ladder.x + (hang.x + dx)));
		f32 dz = floor.z + 0.3f * player->mMotion.mValue.z;
		position.z += player->mSlopeMotion.mValue.z + (rope.z + (ladder.z + (hang.z + dz)));
	}
	{
		if (GetPlayerPlugConnection((XObjPlug*)&player->mPlug)) {
			if (!IsEventScriptActive(&gGame) && !IsGameMenuActive(&gGame) && CanFallDuringHangAction(&player->mHang)
			    && CanFallDuringJumpAction(&player->mJump) && CanFallDuringLadderAction(&player->mLadder)
			    && CanFallDuringRopeAction(&player->mRope) && IsPushActionState(&player->mPush, 0)
			    && CanFallDuringTumbleAction(&player->mTumble) && CanFallDuringCopterAction(&player->mCopter)
			    && CanFallDuringPopperAction(&player->mPopper) && CanFallDuringRadarAction(&player->mRadar)) {
				CVec pull((const Vec&)*GetPlayerPlugCordPull((XObjPlug*)&player->mPlug));
				CVec combined = player->mMotion.mValue + pull;
				combined.y    = 0;
				f32 strength  = GetVectorMagnitude(&pull);
				if (strength > 0.2f) {
					position.x += 0.2f * (pull.x * frames);
					position.z += 0.2f * (pull.z * frames);
					if (strength > 1 && GetVectorMagnitude(&combined) > 1
					    && (IsLiftActionState(&player->mLift, 0) || IsLiftActionState(&player->mLift, 4))
					    && IsHangActionState(&player->mHang, 0) && IsJumpActionState(&player->mJump, 0)
					    && IsLadderActionState(&player->mLadder, 0) && IsRopeActionState(&player->mRope, 0)
					    && IsPushActionState(&player->mPush, 0) && IsTumbleActionState(&player->mTumble, 0)
					    && IsCopterActionState(&player->mCopter, 0)
					    && (IsPopperActionState(&player->mPopper, 0) || IsPopperActionState(&player->mPopper, 8)
					        || IsPopperActionState(&player->mPopper, 4))
					    && IsRadarActionState(&player->mRadar, 0))
						SpawnPlayerDust(player, player->GetPos());
				}
			}
			if (CheckCurrentStageFlags(&gGame, 1)) {
				CVec joint;
				PSMTXMultVec(player->mPlug.GetWorldMtx("second_null"), player->GetPos(), &joint);
				f32 distance = GetVectorDistance(player->GetPos(), &joint);
				if (distance >= 34 && !IsEventScriptActive(&gGame)) {
					PlayWiiSoundEffect(GetGameSound(&gGame), 0xA4, 1.0f, 0.0f);
					ResetPlayerPlugState((XObjPlug*)&player->mPlug, TRUE);
					if (IsLiftActionState(&player->mLift, 0) && IsHangActionState(&player->mHang, 0) && IsJumpActionState(&player->mJump, 0)
					    && IsLadderActionState(&player->mLadder, 0) && IsRopeActionState(&player->mRope, 0)
					    && IsPushActionState(&player->mPush, 0) && !IsPlayerAttachmentEquipped(player, 1)
					    && !IsPlayerAttachmentEquipped(player, 2) && !IsPlayerAttachmentEquipped(player, 3))
						SetTumbleStateAndFade(player, 7, FALSE);
				}
			}
		}
	}
	{
		if (GetVectorMagnitude(&player->_1C9A8) > 0 && !IsEventScriptActive(&gGame) && !IsGameMenuActive(&gGame)
		    && CanFallDuringHangAction(&player->mHang) && CanFallDuringJumpAction(&player->mJump)
		    && CanFallDuringLadderAction(&player->mLadder) && CanFallDuringRopeAction(&player->mRope)
		    && IsPushActionState(&player->mPush, 0) && CanFallDuringTumbleAction(&player->mTumble)
		    && CanFallDuringCopterAction(&player->mCopter) && CanFallDuringPopperAction(&player->mPopper)
		    && CanFallDuringRadarAction(&player->mRadar)) {
			CVec extPush  = player->_1C9A8;
			extPush.y     = 0;
			CVec combined = player->mMotion.mValue + extPush;
			combined.y    = 0;
			position.x += 0.1f * (extPush.x * frames);
			position.z += 0.1f * (extPush.z * frames);
			if (GetVectorMagnitude(&extPush) > 1 && GetVectorMagnitude(&combined) > 1 && IsLiftActionState(&player->mLift, 0)
			    && IsHangActionState(&player->mHang, 0) && IsJumpActionState(&player->mJump, 0) && IsLadderActionState(&player->mLadder, 0)
			    && IsRopeActionState(&player->mRope, 0) && IsPushActionState(&player->mPush, 0) && IsTumbleActionState(&player->mTumble, 0)
			    && IsCopterActionState(&player->mCopter, 0) && IsPopperActionState(&player->mPopper, 0)
			    && IsRadarActionState(&player->mRadar, 0))
				SpawnPlayerDust(player, player->GetPos());
		}
	}
	{
		BOOL hit = FindMapFloorBelow(static_cast<XObjMap*>(map), &position, &player->mFloorHit, &player->mFloorHit);
		if (hit) {
			fn_800DB958((XObjMap*)map, &player->mFloorHit, player->mFloorTriangle);
			SetVectorX(&player->mMoveTranslate, position.x);
			SetVectorZ(&player->mMoveTranslate, position.z);
			CVec normal;
			CalcTriangleNormal(&player->mFloorTriangle[0], &player->mFloorTriangle[1], &player->mFloorTriangle[2], &normal);
			NormalizeVectorInPlace(&normal);
			Vec down   = { 0, -1, 0 };
			Vec origin = { 0, 0, 0 };
			GetOutwardPerpendicularToEdge(&down, &origin, &normal, &player->mFloorSlope);
			{
				if (CanLiftActionSlideOnSlope(&player->mLift) && IsHangActionIdle(&player->mHang)
				    && CanJumpActionSlideOnSlope(&player->mJump) && CanLadderActionSlideOnSlope(&player->mLadder)
				    && CanRopeActionSlideOnSlope(&player->mRope) && CanPushActionSlideOnSlope(&player->mPush)
				    && CanTumbleActionSlideOnSlope(&player->mTumble) && CanCopterActionSlideOnSlope(&player->mCopter)
				    && CanPopperActionSlideOnSlope(&player->mPopper) && CanRadarActionSlideOnSlope(&player->mRadar)) {
					CVec slopeDir = player->mFloorSlope;
					if (GetVectorMagnitude(&slopeDir) > 0.3f) {
						f32 vertical = fn_800D1740(slopeDir.y);
						slopeDir.y   = 0;
						NormalizeVectorInPlace(&slopeDir);
						ScaleVectorInPlace(&slopeDir, -vertical);
						f32 slopeSpeed = 0.02f * GetVectorMagnitude(&slopeDir);
						if (slopeSpeed < 0.01f)
							slopeSpeed = 0.01f;
						SetObjectVelocity(&player->mSlopeMotion, slopeSpeed, slopeSpeed, slopeSpeed);
					} else {
						SetVectorComponents(&slopeDir, 0, 0, 0);
					}
					SetObjectVectorTarget(&player->mSlopeMotion, slopeDir);
				} else {
					SetObjectVectorAndTargetXYZ(&player->mSlopeMotion, 0, 0, 0);
				}
			}
			if (player->mFloorHit.mHeight > 1.0f + player->mMoveTranslate.y) {
				player->mFloorHit.mTriangle = -1;
				player->mFloorHit.mTrigon   = -1;
				player->mFloorHit.mHeight   = -1;
				SetVectorX(&player->mMoveTranslate, position.x);
				SetVectorZ(&player->mMoveTranslate, position.z);
				SetObjectVectorAndTargetXYZ(&player->mSlopeMotion, 0, 0, 0);
				SetVectorComponents(&player->mFloorSlope, 0, 0, 0);
			}
		} else {
			{
				player->mFloorHit.mTriangle = -1;
				player->mFloorHit.mTrigon   = -1;
				player->mFloorHit.mHeight   = -1;
				SetVectorX(&player->mMoveTranslate, position.x);
				SetVectorZ(&player->mMoveTranslate, position.z);
				SetObjectVectorAndTargetXYZ(&player->mSlopeMotion, 0, 0, 0);
				SetVectorComponents(&player->mFloorSlope, 0, 0, 0);
			}
		}
	}

	f32 radius, height;
	s32 liftState22;
	Mtx rotation;
	{
		radius      = player->mMoveScale.x * GetFloorExtent(&player->mFloors, 0).x;
		height      = player->mMoveScale.y * (GetFloorCenterOffset(&player->mFloors, 0).y + GetFloorExtent(&player->mFloors, 0).y);
		liftState22 = 0;
		if (IsCarryingMug(&player->mLift)) {
			Vec scale;
			HSD_MtxGetScale(player->GetWorldMtx(player->FindResNode("carry_obj_nll")), &scale);
			radius += 2.5f * scale.x;
			if (IsLiftActionState(&player->mLift, 23)) // lift state 23: he is taking the mug out
				liftState22 = 1;
		}
		PSMTXRotRad(rotation, 'y', player->mMoveRotate.y);
	}
	Vec offset = player->mMoveScale * GetFloorCenterOffset(&player->mFloors, 0);
	PSMTXMultVec(rotation, &offset, &offset);
	position         = player->mMoveTranslate;
	u32 contactFlags = 0;
	u32 queryFlags   = IsEventScriptActive(&gGame) ? 0 : 2;
	f32 floorHeight  = player->mFloorHit.mHeight;
	s32 floorIndex   = -1;
	BOOL canRise     = FALSE;
	CVec push, contact;
	sBlockedObstacleCount = 0;
	if (!player->mOnGround) {
		if (CanFallDuringHangAction(&player->mHang) || IsHangActionState(&player->mHang, 9)) {
			for (s32 i = 0; i < GetDrawListCount(&gGame, 0); ++i) {
				XObj* object = GetDrawListObject(&gGame, 0, i);
				if (!fn_800CDDAC(object) || !(GetMaskedObjectStatus(object, 0x10000))
				    || (GetMaskedObjectStatus(object, 0x0004000000100000ULL)) || player == object)
					continue;
				// what he pushes only counts while he stands on it
				if ((GetMaskedObjectStatus(object, 0x08000000)) && IsPushActionState(&player->mPush, 2)
				    && IsPlayerActionMoveDirection(&player->mPush, 1)) {
					s32 floorIdx = player->mFloorContactObjects[1];
					if (floorIdx < 0 || GetGameObjectByIndex(&gGame, floorIdx) != object)
						continue;
				}
				f32 range = 100.0f + (radius + GetObjectFloorXZExtent(object, 0));
				if (GetHorizontalDistance(object->GetPos(), &position) > range)
					continue;
				BOOL touched = false;
				// one floor per call: the cursor resumes after the floor that was hit and is -1 once all are tested
				for (s32 cursor = 0; cursor != -1;) {
					position.x = player->mMoveTranslate.x + offset.x;
					position.z = player->mMoveTranslate.z + offset.z;
					u32 hit;
					{
						CVec from(position), to(position);
						hit = object->TestFloorCollision(&from, &to, radius, height, &push, &contact, &cursor, queryFlags | 1);
					}
					if (hit & 1) {
						touched = true;
						f32 top = GetObjectHitFloorHeight(object);
						BOOL blocked;
						if (__fpclassifyf(top) == FP_NAN) {
							if (GetMaskedObjectStatus(object, 0x0002000000000000ULL)) {
								blocked = false;
							} else {
								blocked = true;
								if (liftState22 && GetMaskedObjectStatus(object, 0x4000) && !GetMaskedObjectStatus(object, 0x10))
									SetObjectStatusFlags(object, 0x10);
							}
						} else if (player->mMoveTranslate.y + 1.0f >= top) {
							blocked = false;
							if (top >= floorHeight) {
								floorHeight = top;
								floorIndex  = GetDrawListIndex(&gGame, 0, i);
							}
						} else {
							if (GetMaskedObjectStatus(object, 0x0002000000000000ULL)) {
								blocked = false;
							} else {
								blocked = true;
								if (liftState22 && GetMaskedObjectStatus(object, 0x4000) && !GetMaskedObjectStatus(object, 0x10))
									SetObjectStatusFlags(object, 0x10);
							}
						}
						if (blocked && GetMaskedPlayerStatusFlags(player, 1) && CanFallDuringLadderAction(&player->mLadder)
						    && CanFallDuringRopeAction(&player->mRope)) {
							if (IsPushActionState(&player->mPush, 2) && HasPlayerActionTarget(&player->mPush)) {
								SetObjectVectorAndTargetFromVec((CObjVec*)&player->mMoveTranslate, player->mLastPos);
							} else {
								SetVectorX(&player->mMoveTranslate, contact.x - offset.x);
								SetVectorZ(&player->mMoveTranslate, contact.z - offset.z);
							}
						}
						if (blocked)
							++sBlockedObstacleCount;
					}
					contactFlags |= hit;
				}
				if (!touched && GetMaskedObjectStatus(object, 0x0002000000000000ULL))
					ClearObjectStatusFlags(object, 0x0002000000000000ULL);
			}
			// taking the mug out widens him: an object marked 0x10 above that he ends up more than 1.5 inside stops blocking
			// him (0x0002000000000000) until he is out of it
			if (liftState22) {
				for (s32 i = 0; i < GetDrawListCount(&gGame, 0); ++i) {
					XObj* object = GetDrawListObject(&gGame, 0, i);
					if (!(GetMaskedObjectStatus(object, 0x10)))
						continue;
					ClearObjectStatusFlags(object, 0x10ULL);
					for (s32 cursor = 0; cursor != -1;) {
						u32 hit = object->TestFloorCollision(&player->mMoveTranslate, &player->mMoveTranslate, radius, height, &push,
						                                     &contact, &cursor, 0);
						if ((hit & 3) != 3)
							continue;
						f32 top = GetObjectHitFloorHeight(object);
						if (__fpclassifyf(top) != FP_NAN && player->mMoveTranslate.y + 1.0f >= top)
							continue;
						u32 separated;
						{
							const CVec& difference = player->mMoveTranslate - contact;
							separated              = GetVectorMagnitude(&difference) > 1.5f;
						}
						if (separated)
							SetObjectStatusFlags(object, 0x0002000000000000ULL);
					}
				}
			}
		} else {
			if (IsHangActionState(&player->mHang, 5) && HasPlayerActionTarget(&player->mHang)) {
				f32 side = player->mMoveRotate.y + HALF_PI * (s32)(0u - GetPlayerActionMoveDirection(&player->mHang));
				Vec joint;
				PSMTXMultVec(player->GetWorldMtx("second_null"), player->GetPos(), &joint);
				Vec body;
				body.x = joint.x + 3.0f * PlayerActionSine(side) - 2.5f * PlayerActionSine(player->mMoveRotate.y);
				body.z = joint.z + 3.0f * PlayerActionCosine(side) - 2.5f * PlayerActionCosine(player->mMoveRotate.y);
				body.y = player->mMoveTranslate.y + 1.0f;
				Vec lower, upper;
				lower.x       = joint.x + 3.0f * PlayerActionSine(side);
				lower.z       = joint.z + 3.0f * PlayerActionCosine(side);
				lower.y       = joint.y - 1.0f;
				upper.x       = joint.x + 3.0f * PlayerActionSine(side);
				upper.z       = joint.z + 3.0f * PlayerActionCosine(side);
				upper.y       = joint.y + 0.5f;
				u32 supported = false;
				for (s32 i = 0; i < GetDrawListCount(&gGame, 0); ++i) {
					XObj* object = GetDrawListObject(&gGame, 0, i);

					if (!fn_800CDDAC(object) || !(GetMaskedObjectStatus(object, 0x10000))
					    || (GetMaskedObjectStatus(object, 0x0004000000100000ULL)))
						continue;
					if ((s16)GetObjectCategory(object) == 7 || (s16)GetObjectCategory(object) == 20 || player == object)
						continue;
					f32 range = 100.0f + (radius + GetObjectFloorXZExtent(object, 0));
					if (GetHorizontalDistance(object->GetPos(), &body) <= range) {
						u32 bodyHit;
						{
							CVec from(body), to(body);
							bodyHit = object->TestFloorCollision(&from, &to, 0.5f, height, &push, &contact, 0, 0);
						}
						if ((bodyHit & 3) == 3) {
							SetObjectVectorAndTargetFromVec((CObjVec*)&player->mMoveTranslate, player->mLastPos + hang);
							SetObjectVectorAndTargetFromVec((CObjVec*)&player->mMoveRotate, savedRotation);
							supported = true; // already put back
							break;
						}
						u32 upperHit;
						{
							CVec from(upper), to(upper);
							upperHit = object->TestFloorCollision(&from, &to, 0.1f, 0.1f, &push, &contact, 0, 0);
						}
						if ((upperHit & 3) == 3) {
							SetObjectVectorAndTargetFromVec((CObjVec*)&player->mMoveTranslate, player->mLastPos + hang);
							SetObjectVectorAndTargetFromVec((CObjVec*)&player->mMoveRotate, savedRotation);
							supported = true; // already put back
							break;
						}
					}
					range = 100.0f + (radius + GetObjectFloorXZExtent(object, 0));
					if (GetHorizontalDistance(object->GetPos(), &lower) <= range) {
						u32 lowerHit;
						{
							CVec from(lower), to(lower);
							lowerHit = object->TestFloorCollision(&from, &to, 0.5f, 0.5f, &push, &contact, 0, 0);
						}
						if ((lowerHit & 3) == 3)
							supported |= lowerHit;
					}
				}
				if (!supported) {
					SetObjectVectorAndTargetFromVec((CObjVec*)&player->mMoveTranslate, player->mLastPos + hang);
					SetObjectVectorAndTargetFromVec((CObjVec*)&player->mMoveRotate, savedRotation);
				}
			}
		}
		if (IsPushActionState(&player->mPush, 2) && !IsPlayerActionMoveDirection(&player->mPush, 0)
		    && HasPlayerActionTarget(&player->mPush)) {
			XObj* target  = GetPlayerActionTarget(&player->mPush);
			Vec targetPos = *target->GetPos();
			f32 angle     = 0;
			if (IsPlayerActionMoveDirection(&player->mPush, 1))
				angle = GetObjectMoveRotation(player)->y;
			else if (IsPlayerActionMoveDirection(&player->mPush, -1))
				angle = PI + GetObjectMoveRotation(player)->y;
			targetPos.x += 2.5f * (0.4f * PlayerActionSine(angle));
			targetPos.z += 2.5f * (0.4f * PlayerActionCosine(angle));
			s32 index        = GetActionTargetFloor(&player->mPush);
			Vec scaledOffset = *target->GetScale() * GetFloorCenterOffset(GetObjectFloorTableAddress(target), index);
			index            = GetActionTargetFloor(&player->mPush);
			Vec extent       = *target->GetScale() * GetFloorExtent(GetObjectFloorTableAddress(target), index);
			index            = GetActionTargetFloor(&player->mPush);
			f32 yaw          = GetObjectMoveRotation(target)->y + GetFloorYawRadians(GetObjectFloorTableAddress(target), index);
			scaledOffset.y   = 0;
			if (IsActionTargetFloorId(&player->mPush, 2)) {
				extent.y *= (2.0f / nw4r::math::FSqrt(3.0f)) / 2.0f;
				extent.x *= (2.0f / nw4r::math::FSqrt(3.0f)) / 2.0f;
				extent.z *= (2.0f / nw4r::math::FSqrt(3.0f)) / 2.0f;
			}
			f32 boxHeight = 2.0f * extent.y;
			f32 boxRadius = extent.x;
			f32 boxDepth  = extent.z;
			Mtx translation, center, scale, rotation, transform;
			PSMTXTrans(translation, targetPos.x, targetPos.y, targetPos.z);
			PSMTXTrans(center, scaledOffset.x, scaledOffset.y, scaledOffset.z);
			PSMTXScale(scale, extent.x, extent.y, extent.z);
			PSMTXRotRad(rotation, 'y', yaw);
			PSMTXConcat(translation, rotation, transform);
			PSMTXConcat(transform, center, transform);
			PSMTXConcat(transform, scale, transform);
			Vec boxCenter = { 0, 0, 0 };
			PSMTXMultVec(transform, &boxCenter, &boxCenter);
			f32 ratio   = extent.x / extent.z;
			Vec boxRear = { 0, 0, -1.0f + ratio };
			PSMTXMultVec(transform, &boxRear, &boxRear);
			Vec boxFront = { 0, 0, 1.0f - ratio };
			PSMTXMultVec(transform, &boxFront, &boxFront);
			for (s32 i = 0; i < GetDrawListCount(&gGame, 0); ++i) {
				XObj* object = GetDrawListObject(&gGame, 0, i);

				if (!fn_800CDDAC(object) || !(GetMaskedObjectStatus(object, 0x10000)) || (GetMaskedObjectStatus(object, 0x08000000)))
					continue;
				if ((GetMaskedObjectStatus(object, 0x0004000000100000ULL)) && !(GetMaskedObjectStatus(object, 0x40000000)))
					continue;
				if ((s16)GetObjectCategory(object) == 7 || (s16)GetObjectCategory(object) == 20 || object == player)
					continue;
				if (GetPlayerActionTarget(&player->mPush) == object)
					continue;
				f32 range = 100.0f + (boxRadius + GetObjectFloorXZExtent(object, 0));
				if (GetHorizontalDistance(object->GetPos(), &boxCenter) > range)
					continue;
				{
					u32 hit;
					{
						CVec from(boxCenter), to(boxCenter);
						hit = object->TestFloorCollision(&from, &to, boxRadius, boxHeight, &push, &contact, 0, 0);
					}
					if ((hit & 3) == 3) {
						f32 top   = GetObjectHitFloorHeight(object);
						f32 delta = fn_800D1740(boxCenter.y - top);
						if (delta <= 1 && !GetMaskedObjectStatus(object, 0x40000000)) {
							GetPlayerActionTarget(&player->mPush)->SetTranslateY(top);
						} else if (fn_800DBD44(fn_800DBD60(object)) == 0x87 && boxCenter.y > top) {
							GetPlayerActionTarget(&player->mPush)->SetTranslateY(top);
						} else {
							SetObjectVectorAndTargetFromVec((CObjVec*)&player->mMoveTranslate, player->mLastPos);
							SetObjectVectorAndTargetFromVec((CObjVec*)&player->mMoveRotate, savedRotation);
							break;
						}
					}
				}
				if (boxDepth / boxRadius > 8.0f) {
					{
						u32 hit;
						{
							CVec from(boxRear), to(boxRear);
							hit = object->TestFloorCollision(&from, &to, boxRadius, boxHeight, &push, &contact, 0, 0);
						}
						if ((hit & 3) == 3) {
							f32 top   = GetObjectHitFloorHeight(object);
							f32 delta = fn_800D1740(boxRear.y - top);
							if (delta <= 1 && !GetMaskedObjectStatus(object, 0x40000000)) {
								GetPlayerActionTarget(&player->mPush)->SetTranslateY(top);
							} else {
								SetObjectVectorAndTargetFromVec((CObjVec*)&player->mMoveTranslate, player->mLastPos);
								SetObjectVectorAndTargetFromVec((CObjVec*)&player->mMoveRotate, savedRotation);
								break;
							}
						}
					}
					{
						u32 hit;
						{
							CVec from(boxFront), to(boxFront);
							hit = object->TestFloorCollision(&from, &to, boxRadius, boxHeight, &push, &contact, 0, 0);
						}
						if ((hit & 3) == 3) {
							f32 top   = GetObjectHitFloorHeight(object);
							f32 delta = fn_800D1740(boxFront.y - top);
							if (delta <= 1 && !GetMaskedObjectStatus(object, 0x40000000)) {
								GetPlayerActionTarget(&player->mPush)->SetTranslateY(top);
							} else if (fn_800DBD44(fn_800DBD60(object)) == 0x87 && boxFront.y > top) {
								GetPlayerActionTarget(&player->mPush)->SetTranslateY(top);
							} else {
								SetObjectVectorAndTargetFromVec((CObjVec*)&player->mMoveTranslate, player->mLastPos);
								SetObjectVectorAndTargetFromVec((CObjVec*)&player->mMoveRotate, savedRotation);
								break;
							}
						}
					}
				}
			}
		} else {
			if (IsRopeActionState(&player->mRope, 5) && IsPlayerActionMoveDirection(&player->mRope, -1)
			    && HasPlayerActionTarget(&player->mRope)) {
				CVec joint;
				PSMTXMultVec(player->GetWorldMtx("second_null"), player->GetPos(), &joint);
				Vec point;
				point.x = joint.x;
				point.z = joint.z;
				point.y = 9.0f + player->GetPos()->y;
				u32 hit;
				{
					CVec from(point), to(point);
					hit = GetPlayerActionTarget(&player->mRope)->TestFloorCollision(&from, &to, 0.5f, 0.5f, &push, &contact, 0, 0);
				}
				if ((hit & 3) != 3) {
					SetObjectVectorAndTargetFromVec((CObjVec*)&player->mMoveTranslate, player->mLastPos + hang);
					SetObjectVectorAndTargetFromVec((CObjVec*)&player->mMoveRotate, savedRotation);
				}
			}
		}
	} else {
		SetObjectVectorAndTargetXYZ(&player->mSlopeMotion, 0, 0, 0);
		SetVectorComponents(&player->mFloorSlope, 0, 0, 0);
	}
	if (1.0f + player->mMoveTranslate.y >= floorHeight)
		canRise = TRUE;
	if (!contactFlags)
		canRise = TRUE;
	if (floorIndex >= 0)
		player->mFloorSupport = GetGameObjectByIndex(&gGame, floorIndex);
	else
		player->mFloorSupport = static_cast<XObj*>(map);
	f32 distance = fn_800D1740(player->mMoveTranslate.y - floorHeight);
	if (distance <= 1 && floorIndex >= 0)
		player->mFloorContactObjects[0] = floorIndex;
	if (player->mFloorContactObjects[0] == player->mFloorContactObjects[1]) {
		if (IsCopterActionState(&player->mCopter, 4) || IsCopterActionState(&player->mCopter, 7)
		    || IsCopterActionState(&player->mCopter, 8)) {
			fn_800C98E0((XObjMap*)map, &position, -1, &player->mFloorHit);
		}
	} else {
		fn_800C98E0((XObjMap*)map, &position, -1, &player->mFloorHit);
		if (fn_800D168C(&gGame, 0) == 9 && player->mFloorContactObjects[0] >= 0) {
			XObj* support = GetGameObjectByIndex(&gGame, player->mFloorContactObjects[0]);
			if (support != NULL && fn_800DBD68(support) && fn_800DBD44(fn_800DBD60(support)) == 0x376) {
				TryStartObjectEvent(&gGame, support->GetEventScript(), support, -1, -1);
			}
		}
	}
	if (fn_800D168C(&gGame, 0) == 1) {
		if (floorIndex == 0xAC || floorIndex == 9) {
			fn_800C98E0((XObjMap*)map, &position, -1, &player->mFloorHit);
		}
	} else if (fn_800D168C(&gGame, 0) == 4) {
		if (player->mFloorSupport != NULL && fn_800DBD44(fn_800DBD60(player->mFloorSupport)) == 0x4A) {
			if (fn_800C98E0((XObjMap*)map, &position, -1, &player->mFloorHit) && position.y < player->mFloorHit.mHeight
			    && player->mFloorHit.mHeight >= 3.66519141f) {
				player->SetTranslateY(player->mFloorHit.mHeight);
			}
		} else if (floorIndex == 0x16B) {
			fn_800C98E0((XObjMap*)map, &position, -1, &player->mFloorHit);
		}
	} else if (fn_800D168C(&gGame, 0) == 6) {
		if (player->mFloorSupport != NULL
		    && (fn_800DBD44(fn_800DBD60(player->mFloorSupport)) == 0x62 || fn_800DBD44(fn_800DBD60(player->mFloorSupport)) == 0x253)) {
			if (fn_800C98E0((XObjMap*)map, &position, -1, &player->mFloorHit) && position.y < player->mFloorHit.mHeight
			    && player->mFloorHit.mHeight >= 2.61799383f) {
				player->SetTranslateY(player->mFloorHit.mHeight);
			}
		} else if (floorIndex == 0x2A || floorIndex == 0xB0 || floorIndex == 0xB1) {
			fn_800C98E0((XObjMap*)map, &position, -1, &player->mFloorHit);
		}
	} else if (fn_800D168C(&gGame, 0) == 18) {
		if (player->mFloorSupport != NULL
		    && (fn_800DBD44(fn_800DBD60(player->mFloorSupport)) == 0x62 || fn_800DBD44(fn_800DBD60(player->mFloorSupport)) == 0x253)) {
			if (fn_800C98E0((XObjMap*)map, &position, -1, &player->mFloorHit) && position.y < player->mFloorHit.mHeight
			    && player->mFloorHit.mHeight >= 2.61799383f) {
				player->SetTranslateY(player->mFloorHit.mHeight);
			}
		} else if (floorIndex > 0x3C && floorIndex < 0x3F) {
			fn_800C98E0((XObjMap*)map, &position, -1, &player->mFloorHit);
		}
	}

	if (floorHeight > player->mFloorHit.mHeight) {
		player->mFloorHit.mHeight = floorHeight;
		SetObjectVectorAndTargetXYZ(&player->mSlopeMotion, 0, 0, 0);
		SetVectorComponents(&player->mFloorSlope, 0, 0, 0);
	}
	CPadOne* pad = &player->mPad;
	if (!player->mOnGround) {
		if (player->mMoveTranslate.y == player->mFloorHit.mHeight) {
			SetObjectYAndTarget((CObjVec*)&player->mMoveTranslate, player->mFloorHit.mHeight);
			SetObjectVerticalSpeed((CObjVec*)&player->mMoveTranslate, 0);
		} else if (player->mMoveTranslate.y > player->mFloorHit.mHeight && CanFallDuringHangAction(&player->mHang)
		           && CanFallDuringJumpAction(&player->mJump) && CanFallDuringLadderAction(&player->mLadder)
		           && CanFallDuringRopeAction(&player->mRope) && CanFallDuringPushAction(&player->mPush)
		           && CanFallDuringTumbleAction(&player->mTumble) && CanFallDuringCopterAction(&player->mCopter)
		           && CanFallDuringPopperAction(&player->mPopper) && CanFallDuringRadarAction(&player->mRadar)
		           && GetMaskedPlayerStatusFlags(player, 2)) {
			SetObjectTargetY((CObjVec*)&player->mMoveTranslate, player->mFloorHit.mHeight);
			f32 speed;
			if (IsCopterActionState(&player->mCopter, 4) || IsCopterActionState(&player->mCopter, 7)
			    || IsCopterActionState(&player->mCopter, 8)) {
				f32 acceleration = GetCopterDescentAcceleration();
				f32 limit        = GetCopterDescentSpeedLimit();
				speed            = acceleration + GetObjectVectorSpeed((CObjVec*)&player->mMoveTranslate)->y;
				if (speed > limit)
					speed = limit;
			} else {
				speed = 0.15f + GetObjectVectorSpeed((CObjVec*)&player->mMoveTranslate)->y;
				if (speed > 10.0f)
					speed = 10.0f;
			}
			SetObjectVerticalSpeed((CObjVec*)&player->mMoveTranslate, speed);
			f32 drop = player->mMoveTranslate.y - player->mFloorHit.mHeight;
			if (speed > 0.1f && drop > 1) {
				if (!(player->mFloorContactObjects[0] >= 0
				      || (!IsLiftActionState(&player->mLift, 0) && !IsLiftActionState(&player->mLift, 4)
				          && !IsLiftActionState(&player->mLift, 29))
				      || !IsHangActionState(&player->mHang, 0) || !IsJumpActionState(&player->mJump, 0)
				      || !IsLadderActionState(&player->mLadder, 0) || !IsRopeActionState(&player->mRope, 0)
				      || !IsTumbleActionState(&player->mTumble, 0)
				      || (!IsCopterActionState(&player->mCopter, 0) && !IsCopterActionState(&player->mCopter, 3))
				      || (!IsPopperActionState(&player->mPopper, 0) && !IsPopperActionState(&player->mPopper, 8)
				          && !IsPopperActionState(&player->mPopper, 4))
				      || !IsRadarActionState(&player->mRadar, 0) || player->mAttachmentRequest >= 0)) {
					if (IsCopterActionState(&player->mCopter, 3)) {
						SetObjectVerticalSpeed((CObjVec*)&player->mMoveTranslate, 0);
					} else {
						if (IsGameMenuActive(&gGame) || IsPlayerRingMenuOpen(&player->mMenu))
							ClosePlayerEquipmentRings(&player->mMenu, -1);
						if (!IsPushActionState(&player->mPush, 0))
							ReleasePushAction(&player->mPush);
						if (IsPlayerAttachmentEquipped(player, 1)) {
							SetCopterActionState(&player->mCopter, 4);
							SetObjectVerticalSpeed((CObjVec*)&player->mMoveTranslate, 0);
						} else {
							XObj* support = NULL;
							if (player->mFloorContactObjects[1] >= 0) {
								support         = GetGameObjectByIndex(&gGame, player->mFloorContactObjects[1]);
								s32 ledgeHeight = HasObjectCustomLedgeHeight(support) ? GetObjectPlacementParam(support, 0) : 12;
								if (!(drop > 0.5f + ledgeHeight))
									support = NULL;
							}
							if (support != NULL && IsObjectClimbableLedge(support)) {
								if (__fpclassifyf(player->mWalkHeading) == FP_NAN) {
									if (IsEventScriptActive(&gGame) || player->mSuits.mChangeStep) {
										SetObjectVectorAndTargetFromVec((CObjVec*)&player->mMoveTranslate, player->mLastPos);
										SetObjectVerticalSpeed((CObjVec*)&player->mMoveTranslate, 0);
										player->mFloorContactObjects[0] = player->mFloorContactObjects[1];
									} else if (player->GetAnimId() == 4 || player->GetAnimId() == 13 || player->GetAnimId() == 0x898
									           || player->GetAnimId() == 0x819) {
										SetPlayerHangState(player, 4, support);
									} else if (IsPlayerAttachmentEquipped(player, 2) || IsPlayerAttachmentEquipped(player, 3)
									           || !IsLiftActionState(&player->mLift, 0)) {
										SetPlayerHangState(player, 4, support);
									} else {
										if (!IsLiftActionState(&player->mLift, 0))
											ReleasePlayerLiftTarget(player);
										if (IsObjectLadder(support))
											SetPlayerLadderState(player, 3, support);
										else if (IsObjectRope(support))
											SetPlayerRopeState(player, 3, support);
										else
											SetPlayerHangState(player, 3, support);
									}
								}
							} else {
								SetPlayerActionState(&player->mJump, 2);
								SetObjectVerticalSpeed((CObjVec*)&player->mMoveTranslate, 0);
							}
						}
					}
				}
			}
		} else if (player->mMoveTranslate.y < player->mFloorHit.mHeight && canRise) {
			if (IsRopeActionState(&player->mRope, 5)) {
				SetObjectYAndTarget((CObjVec*)&player->mMoveTranslate, player->mFloorHit.mHeight);
				SetObjectVerticalSpeed((CObjVec*)&player->mMoveTranslate, 0);
			} else if (IsLadderActionState(&player->mLadder, 5)) {
				SetObjectYAndTarget((CObjVec*)&player->mMoveTranslate, player->mFloorHit.mHeight);
				SetObjectVerticalSpeed((CObjVec*)&player->mMoveTranslate, 0);
			} else {
				SetObjectTargetY((CObjVec*)&player->mMoveTranslate, player->mFloorHit.mHeight);
				f32 speed = 0.2f + GetObjectVectorSpeed((CObjVec*)&player->mMoveTranslate)->y;
				if (speed > 5)
					speed = 5;
				SetObjectVerticalSpeed((CObjVec*)&player->mMoveTranslate, speed);
			}
		} else {
			SetObjectVerticalSpeed((CObjVec*)&player->mMoveTranslate, 0);
		}
	}
	{
		if (!IsGameMenuActive(&gGame) && !(gGame.GetStatusFlags(0x100))
		    && (IsLiftActionState(&player->mLift, 0) || IsLiftActionState(&player->mLift, 4)) && IsHangActionState(&player->mHang, 0)
		    && IsJumpActionState(&player->mJump, 0) && IsLadderActionState(&player->mLadder, 0) && IsRopeActionState(&player->mRope, 0)
		    && IsPushActionState(&player->mPush, 0) && IsTumbleActionState(&player->mTumble, 0) && IsCopterActionState(&player->mCopter, 0)
		    && (IsPopperActionState(&player->mPopper, 0) || IsPopperActionState(&player->mPopper, 8)
		        || IsPopperActionState(&player->mPopper, 4))
		    && IsRadarActionState(&player->mRadar, 0)) {
			const Vec* current    = GetObjectVectorCurrent(&player->mMotion);
			const Vec* dustTarget = GetObjectVectorTarget(&player->mMotion);
			f32 rate              = fn_800E32B8(player, -1);
			f32 dustDistance      = GetVectorDistance(current, dustTarget) / rate;
			if (dustDistance >= 3.0f) {
				SpawnPlayerDust(player, player->GetPos());
				if (dustDistance >= 5.0f && !IsWiiSfxPlaying(GetGameSound(&gGame), 0x88))
					PlayWiiSoundEffect(GetGameSound(&gGame), 0x88, 255.0f, 0.0f);
			}
		}
	}
	if (fn_800D1764(player, 2) >= 1.0f) {
		SpawnPlayerDust(player, player->GetPos());
	}
	{
		Vec probePoint;
		probePoint.x    = (player->mMoveTranslate.x + offset.x) + (0.5f * radius) * PlayerActionSine(player->mMoveRotate.y);
		probePoint.z    = (player->mMoveTranslate.z + offset.z) + (0.5f * radius) * PlayerActionCosine(player->mMoveRotate.y);
		probePoint.y    = player->mMoveTranslate.y;
		f32 probeRadius = 0.9f * radius;
		player->mFloorContactObjects[2] = player->mActionObjectIndex = player->mHitObjectIndex = -1;
		f32 best = NAN, bestTagged = NAN;
		for (s32 i = 0; i < GetStageObjectCount(&gGame); ++i) {
			XObj* object = GetGameObjectByIndex(&gGame, i);

			if (!fn_800CDDAC(object) || !(GetMaskedObjectStatus(object, 0x10000)) || GetMaskedObjectStatus(object, 0x0004000000000000ULL)
			    || GetMaskedObjectStatus(object, 0x0002000000000000ULL) || player == object)
				continue;
			if (GetHorizontalDistance(object->GetPos(), &probePoint) > 100.0f + (probeRadius + GetObjectFloorXZExtent(object, 0)))
				continue;
			u32 hit;
			{
				CVec from(probePoint), to(probePoint);
				hit = object->TestFloorCollision(&from, &to, probeRadius, height, &push, &contact, 0, queryFlags);
			}
			if ((hit & 3) == 3) {
				SelectHighestActiveFloor(object);
				if (player->mHitObjectIndex < 0 || __fpclassifyf(best) == FP_NAN || GetSelectedFloorHeight(object) > best) {
					player->mHitObjectIndex = i;
					best                    = GetSelectedFloorHeight(object);
					if (object->GetEventScript()
					    && (!GetMaskedObjectStatus(object, 0x20)
					        || (GetMaskedObjectStatus(object, 0x20) && IsLiftTargetPlug(&player->mLift)
					            && player->mHitObjectIndex != player->mFloorContactObjects[0])))
						player->mFloorContactObjects[2] = i;
				} else if (player->mFloorContactObjects[2] < 0 && object->GetEventScript()
				           && (!GetMaskedObjectStatus(object, 0x20)
				               || (GetMaskedObjectStatus(object, 0x20) && IsLiftTargetPlug(&player->mLift)
				                   && player->mFloorContactObjects[2] != player->mFloorContactObjects[0]))) {
					player->mFloorContactObjects[2] = i;
				}
				if ((player->mActionObjectIndex < 0 || __fpclassifyf(bestTagged) == FP_NAN || GetSelectedFloorHeight(object) > bestTagged)
				    && GetMaskedObjectStatus(object, 0x000000ff00000000ULL)) {
					player->mActionObjectIndex = i;
					bestTagged                 = GetSelectedFloorHeight(object);
				}
			} else {
				ClearObjectStatusFlags(object, 0x8000000000000000ULL);
			}
		}
		if (player->mFloorContactObjects[0] >= 0 && player->mHitObjectIndex < 0) {
			player->mHitObjectIndex = player->mFloorContactObjects[0];
			if (GetMaskedObjectStatus(GetGameObjectByIndex(&gGame, player->mHitObjectIndex), 0x000000ff00000000ULL))
				player->mActionObjectIndex = player->mHitObjectIndex;
		}
	}
	{
		player->mLetickerIndex = -1;
		if (!((player->mHitObjectIndex >= 0 && player->mHitObjectIndex != player->mFloorContactObjects[0])
		      || (player->mFloorContactObjects[2] >= 0 && player->mFloorContactObjects[2] != player->mFloorContactObjects[0])
		      || !IsLiftActionState(&player->mLift, 4) || !IsLiftTargetPlug(&player->mLift))) {
			for (s32 i = 0; i < GetObjectPoolUsedCount(&gGame, 9); ++i) {
				XObj* object = (XObj*)GetObjectFromPool(&gGame, 9, i);
				if (!fn_800CDDAC(object) || !GetMaskedObjectStatus(object, 0x10000) || !GetMaskedObjectStatus(object, 0x20)
				    || !object->GetEventScript())
					continue;
				if (GetVectorDistance(object->GetPos(), player->GetPos()) >= 18.0f)
					continue;
				f32 yaw   = GetObjectMoveRotation(object)->y;
				f32 angle = WrapPlayerActionAngle(GetHeadingBetweenPositions(object->GetPos(), player->GetPos()) - yaw);
				// he stands within 30 degrees of square to one of its four sides
				if (angle < 0.52359879f || angle > 5.7595863f || (angle > 1.04719758f && angle < 2.09439516f)
				    || (angle > 2.61799383f && angle < 3.66519141f) || (angle > 4.18879032f && angle < 5.23598766f)) {
					player->mLetickerIndex = GetIndexedCoordPlacement(GetObjectCoordInfo(object), 0) - GetStagePlacementByIndex(0);
					break;
				}
			}
		}
	}
	// available: actions the objects around him offer, 1 event, 2 push, 4 lift, 8 climb (japan), 0x10 ladder, 0x20 rope,
	// 0x40 never (fn_800D5680), 0x80 plug in; marker: the ones whose marker shows. actionMode: what a full gauge on target
	// starts, 1-3 jump up 4, 8 or 12.5, 4 grab a ledge, 5 climb up from a hang, 6 let go of it, 9 push (japan)
	s32 count                  = 0;
	player->mLook.mMarkerCount = 0;
	if (IsPlayerLookMarkerVisible(&player->mLook))
		++player->mLook.mMarkerCount;
	DisablePlayerLook(&player->mLook);
	u32 available       = 0;
	XObj* target        = NULL;
	s32 actionMode      = -1;
	u32 marker          = 0;
	BOOL markerOverride = FALSE;
	if (!IsEventScriptActive(&gGame) && !(gGame.GetStatusFlags(0x500)) && player->mAttachmentRequest < 0
	    && (IsLiftActionState(&player->mLift, 0) || (IsLiftActionState(&player->mLift, 4) && !IsCarryingMug(&player->mLift)))
	    && IsHangActionState(&player->mHang, 0) && IsJumpActionState(&player->mJump, 0) && IsLadderActionState(&player->mLadder, 0)
	    && IsRopeActionState(&player->mRope, 0) && IsPushActionState(&player->mPush, 0) && IsTumbleActionState(&player->mTumble, 0)
	    && IsCopterActionState(&player->mCopter, 0) && IsPopperActionState(&player->mPopper, 0) && IsRadarActionState(&player->mRadar, 0)) {
		{
			s32 index = player->mFloorContactObjects[2];
			if (index >= 0 && index != player->mFloorContactObjects[0]) {
				XObj* object = GetGameObjectByIndex(&gGame, index);
				if (!player->_0A70 || !GetMaskedObjectStatus(object, 0x100000)) {
					if (IsObjectLiftable(object))
						available |= 4;
					else if (GetMaskedObjectStatus(object, 0x0000010000000000ULL))
						available |= 1;
				}
				if (GetMaskedObjectStatus(object, 0x20) && IsLiftTargetPlug(&player->mLift)
				    && GetCharacterObjectApproachResult(player, object))
					available |= 0x80;
			} else if (index >= 0 && player->mFloorContactObjects[2] == player->mFloorContactObjects[0]
			           && player->mHitObjectIndex == player->mFloorContactObjects[2] && !player->_0A70) {
				XObj* object = GetGameObjectByIndex(&gGame, player->mFloorContactObjects[2]);
				if (GetMaskedObjectStatus(object, 0x0000010000000000ULL) && !GetMaskedObjectStatus(object, 0x0000100000000000ULL))
					available |= 1;
			}
		}
		{
			s32 index = player->mActionObjectIndex;
			if (index >= 0) {
				XObj* object = GetGameObjectByIndex(&gGame, index);
				if (IsObjectLiftable(object) && index != player->mFloorContactObjects[0])
					available |= 4;
				if (IsObjectPushable(object) && index != player->mFloorContactObjects[0]) {
					if (IsLiftActionState(&player->mLift, 0) || IsPlayerCarryingPlug(player)) {
						available |= 2;
						if (gGame.GetStatusFlags(0x80000000000ULL) != 0) {
							target     = object;
							actionMode = 9;
						}
					}
				} else if (IsObjectPushableFromBack(object) && index != player->mFloorContactObjects[0]) {
					f32 yaw   = GetObjectMoveRotation(object)->y;
					s32 edge  = GetSelectedFloorIndex(object);
					f32 angle = GetObjectEdgeOutwardYaw(object, player->GetPos(), edge) - yaw;
					// the edge he is at faces straight back, within 0.1 degrees
					if (angle > 3.13984752f && angle < 3.14333797f) {
						available |= 2;
						if (gGame.GetStatusFlags(0x80000000000ULL) != 0) {
							target     = object;
							actionMode = 9;
						}
					}
				}
				if (IsObjectClimbableLedge(object)) {
					f32 climbHeight = fn_800D1740(GetSelectedFloorHeight(object) - player->mMoveTranslate.y);
					if (climbHeight != 0) {
						if (climbHeight >= 12 && climbHeight <= 12.5f) {
							if (IsLiftActionState(&player->mLift, 0)
							    || (IsLiftActionState(&player->mLift, 4) && !IsCarryingToothbrush(&player->mLift)
							        && !IsCarryingSpoon(&player->mLift) && !IsCarryingMug(&player->mLift)
							        && !IsCarryingSquirter(&player->mLift))) {
								if (gGame.GetStatusFlags(0x80000000000ULL) != 0)
									available |= 8;
								if (!gGame.GetStatusFlags(0x80000000000ULL)) {
									target     = object;
									actionMode = 4;
								}
							}
						} else {
							if (gGame.GetStatusFlags(0x80000000000ULL) != 0)
								available |= 8;
							if (climbHeight <= 1) {
								// too low to need climbing
							} else if (climbHeight <= 4) {
								if (!gGame.GetStatusFlags(0x80000000000ULL)) {
									target     = object;
									actionMode = 1;
								}
							} else if (climbHeight <= 8) {
								if (!gGame.GetStatusFlags(0x80000000000ULL)) {
									target     = object;
									actionMode = 2;
								}
							} else if (climbHeight <= 12.5f) {
								if (!gGame.GetStatusFlags(0x80000000000ULL)) {
									target     = object;
									actionMode = 3;
								}
							}
						}
					}
				} else if (IsObjectLadder(object)) {
					if (GetSelectedFloorIndex(object) == 0) {
						if (index == player->mFloorContactObjects[0]) {
							f32 facing = GetObjectMoveRotation(object)->y;
							f32 angle  = WrapPlayerActionAngle(GetObjectMoveRotation(player)->y - facing);
							if (angle <= HALF_PI || angle >= 1.5f * PI)
								available |= 0x10;
						} else {
							f32 playerHeight = GetObjectMaxFloorHeight(player);
							if (GetSelectedFloorHeight(object) > playerHeight
							    && IsPositionBehindObject(&player->mLadder, player->GetPos(), object))
								available |= 0x10;
						}
					}
				} else if (IsObjectRope(object)) {
					if (GetSelectedFloorIndex(object) == 0) {
						if (index == player->mFloorContactObjects[0]) {
							f32 facing = GetObjectMoveRotation(object)->y;
							f32 angle  = WrapPlayerActionAngle(GetObjectMoveRotation(player)->y - facing);
							if (angle <= HALF_PI || angle >= 1.5f * PI)
								available |= 0x20;
						} else {
							f32 playerHeight = GetObjectMaxFloorHeight(player);
							// a rope can be grabbed from any side
							if (GetSelectedFloorHeight(object) > playerHeight)
								available |= 0x20;
						}
					}
				} else if (fn_800D5680(object)) {
					available |= 0x40;
				}
				if ((GetMaskedObjectStatus(object, 0x20)) && IsLiftTargetPlug(&player->mLift) && index != player->mFloorContactObjects[0]
				    && GetCharacterObjectApproachResult(player, object))
					available |= 0x80;
				if (player->mGaugeTarget)
					player->mHitObjectIndex = player->mActionObjectIndex;
			}
		}
		{
			if (player->mLetickerIndex >= 0)
				available |= 0x80;
			if (!available && player->mHitObjectIndex >= 0
			    && !(player->mPreviousContactObject == player->mHitObjectIndex
			         && player->mLastContactFloorIndex == GetSelectedFloorIndex(GetGameObjectByIndex(&gGame, player->mHitObjectIndex)))
			    && player->mFloorContactObjects[2] >= 0
			    && !(player->mPreviousContactObject == player->mFloorContactObjects[2]
			         && player->mLastContactFloorIndex
			                == GetSelectedFloorIndex(GetGameObjectByIndex(&gGame, player->mFloorContactObjects[2])))) {
				player->mPreviousContactObject = -1;
				player->mNearDistance          = 2;
			}
			if (player->mFloorContactObjects[2] >= 0) {
				XObj* object = GetGameObjectByIndex(&gGame, player->mFloorContactObjects[2]);
				ADD_OBJECT_MARKERS(0xff);
				if (GetMaskedObjectStatus(object, 0x0000040000000000ULL))
					markerOverride = TRUE;
			}
			if (player->mHitObjectIndex >= 0) {
				XObj* object = GetGameObjectByIndex(&gGame, player->mHitObjectIndex);
				ADD_OBJECT_MARKERS(0xfe);
				if (GetMaskedObjectStatus(object, 0x0000040000000000ULL))
					markerOverride = TRUE;
			}
			if (player->mLetickerIndex >= 0) {
				XObj* object = GetGameObjectByIndex(&gGame, player->mLetickerIndex);
				ADD_OBJECT_MARKERS(0x80);
				if (GetMaskedObjectStatus(object, 0x0000040000000000ULL))
					markerOverride = TRUE;
			}
		}
	}
	{
		if (marker & 0x1)
			++count;
		if (marker & 0x2)
			++count;
		if (marker & 0x4)
			++count;
		if (marker & 0x10)
			++count;
		if (marker & 0x20)
			++count;
		if (marker & 0x40)
			++count;
		if (marker & 0x80)
			++count;
		if (marker & 0x8)
			++count;
		if (count) {
			EnablePlayerLook(&player->mLook);
			if (marker & 0x80) {
				SetPlayerLookMode(&player->mLook, 0x72);
				SetLookTargetPosition(&player->mLook, 0, 0, 0);
			} else {
				SetPlayerLookMode(&player->mLook, 0x71);
				SetLookTargetPosition(&player->mLook, 0, 0, -0.52359879f);
			}
			s32 kind = -1;
			if (player->mHitObjectIndex >= 0) {
				XObj* object = GetGameObjectByIndex(&gGame, player->mHitObjectIndex);
				kind         = fn_800DBD44(fn_800DBD60(object));
			}
			if (IsPlayerAttachmentEquipped(player, 0)) {
				if (!markerOverride && !(available & 0x72)) {
					if (kind == 0x2D1 || kind == 0x249)
						DisablePlayerLook(&player->mLook);
					else
						fn_800DBF70(&player->mLook);
				}
			} else if (kind == 0x2D1 || kind == 0x249) {
				if (markerOverride)
					fn_800DBF70(&player->mLook);
				else
					DisablePlayerLook(&player->mLook);
			}
		} else {
			DisablePlayerLook(&player->mLook);
		}
	}
	if (count) {
		if (count > player->mLook.mMarkerCount)
			player->mLook.mMarkerCount = count;
	} else { // no marker: the stick and the buttons start an action
		if (!IsHangActionState(&player->mHang, 0)) {
			if (IsHangActionState(&player->mHang, 5) && (player->mStick[0] > 0) && HasPlayerActionTarget(&player->mHang)) {
				f32 angle = WrapPlayerActionAngle((player->mStick[2] + GetCameraAngles(gGame.GetCamera())->y) - player->mMoveRotate.y);
				// stick within 30 degrees of ahead climbs up, within 30 degrees of behind lets go
				if ((angle < 0.52359879f || angle > 5.7595863f) && IsObjectClimbableFromHang(GetPlayerActionTarget(&player->mHang))) {
					target     = GetPlayerActionTarget(&player->mHang);
					actionMode = 5;
				}
				if (angle > 2.61799383f && angle < 3.66519141f) {
					if (!gGame.GetStatusFlags(0x80000000000ULL)) {
						target     = GetPlayerActionTarget(&player->mHang);
						actionMode = 6;
					}
				}
			}
		} else {
			// retail makes these checks and drops every result
			if (!IsCopterActionState(&player->mCopter, 0))
				IsCopterActionState(&player->mCopter, 3);
			else if (!IsRopeActionState(&player->mRope, 0))
				IsRopeActionState(&player->mRope, 5);
			else if (!IsLadderActionState(&player->mLadder, 0))
				IsLadderActionState(&player->mLadder, 5);
			else if (!IsPushActionState(&player->mPush, 0))
				IsPushActionState(&player->mPush, 2);
			else if (IsPlayerAttachmentEquipped(player, 1))
				IsCopterActionState(&player->mCopter, 0);
			else if (IsPlayerAttachmentEquipped(player, 2))
				IsPopperActionState(&player->mPopper, 0);
			else
				IsPlayerAttachmentEquipped(player, 3);
		}
	}
	{
		if (!player->mOnGround && !gGame.GetStatusFlags(0x20)
		    && (IsPlayerAttachmentEquipped(player, 0) || IsPlayerAttachmentEquipped(player, 1) || IsPlayerAttachmentEquipped(player, 2)
		        || IsPlayerAttachmentEquipped(player, 3) || IsPlayerAttachmentEquipped(player, 4) || IsPlayerAttachmentEquipped(player, 5)
		        || IsPlayerAttachmentEquipped(player, 6) || IsPlayerAttachmentEquipped(player, 7)
		        || IsPlayerAttachmentEquipped(player, 8))) {
			f32 magnitude = GetControlStickMagnitude(pad);
			if (magnitude == 0 && player->mPreviousContactObject >= 0)
				magnitude = 0.8f;
			if (target && !GetMaskedObjectStatus(target, 0x0002000000000000ULL) && !GetHeldPadButtons(pad, 0x0002000000000020ULL)
			    && magnitude > 0) {
				f32 threshold = gRefreshRate * GetAutoActionHoldTime();
				if (target == player->mGaugeTarget) {
					if (GetAutoActionChargeRate() > 0)
						player->mGaugeTime += frames * GetAutoActionChargeRate();
					else
						player->mGaugeTime += magnitude * frames;
					f32 cap = GetAutoActionSoftTiltCap();
					if (cap > 0 && magnitude <= 0.7f && player->mGaugeTime > threshold * cap)
						player->mGaugeTime = threshold * cap;
				} else {
					player->mGaugeTarget = target;
					player->mGaugeTime   = 0;
				}
				if (player->mGaugeTime >= threshold) {
					XObj* hit = GetGameObjectByIndex(&gGame, player->mHitObjectIndex);
					if (actionMode == 1) {
						SetPlayerActionState(&player->mJump, 8);
						SetPlayerActionTarget(&player->mJump, hit);
					} else if (actionMode == 2) {
						SetPlayerActionState(&player->mJump, 11);
						SetPlayerActionTarget(&player->mJump, hit);
					} else if (actionMode == 3) {
						SetPlayerActionState(&player->mJump, 14);
						SetPlayerActionTarget(&player->mJump, hit);
					} else if (actionMode == 4) {
						if (IsPlayerCarryingPlug(player))
							ReleasePlayerLiftTarget(player);
						SetHangActionState(&player->mHang, 1);
						SetPlayerActionTarget(&player->mHang, hit);
					} else if (actionMode == 5) {
						if (IsPlayerCarryingPlug(player))
							ReleasePlayerLiftTarget(player);
						TryLeaveHang(&player->mHang);
						StopPlayerMotion(player);
					} else if (actionMode == 6) {
						if (IsPlayerCarryingPlug(player))
							ReleasePlayerLiftTarget(player);
						SetHangActionState(&player->mHang, 9);
						StopPlayerMotion(player);
					} else if (actionMode == 9) {
						if (IsPlayerCarryingPlug(player))
							ReleasePlayerLiftTarget(player);
						StopPlayerMotion(player);
						SetPushActionState(&player->mPush, 1);
						SetPlayerActionTarget(&player->mPush, hit);
						SetPushTargetState(&player->mPush, 0);
					}
					player->mGaugeTarget = NULL;
					player->mGaugeTime   = 0;
				}
			} else {
				player->mGaugeTarget = NULL;
				player->mGaugeTime   = 0;
			}
		} else {
			player->mGaugeTarget = NULL;
			player->mGaugeTime   = 0;
		}
	}
}

s32 gPlayerSuitIdleAnimations[9] = { 1, 0xCF7, 0xCF9, 0xCF8, 0xCFD, 1, 0xCFA, 0xCFB, 0xCFC };

static inline f32 GetStickMotion(CPadOne* pad)
{
	u8 blocked = (gGame.GetStatusFlags(0x20)) != 0;
	if (!blocked) {
		blocked = gGame.mEvt.mData.mScript != NULL;
	}
	if (blocked) {
		return 0.0f;
	}
	f32 x = pad->mStick.x;
	f32 y = pad->mStick.y;
	return FSqrt(x * x + y * y);
}

static inline f32 PlayerGaitRate(f32 speed, bool slow)
{
	if (slow) {
		f32 rate = speed > 1.0f ? 1.0f : speed;
		if (rate < 0.5f)
			rate = 0.5f;
		return rate;
	} else {
		f32 rate = 0.5f * speed;
		if (rate > 1.0f)
			rate = 1.0f;
		if (rate < 0.7f)
			rate = 0.7f;
		return rate;
	}
}

static inline void ChangePlayerGait(WiiPlayerState* p, s32 id, f32 rate)
{
	p->mFrameTime.mFrame = 4.0f + p->GetAnimFrame();
	if (p->mFrameTime.mFrame >= p->GetAnimEndFrame())
		p->mFrameTime.mFrame -= p->GetAnimEndFrame();
	p->SetAnim(id, FALSE, p->mFrameTime.mFrame, rate, 0.0f, 10.0f);
}

static inline void SetPlayerIdleAnim(WiiPlayerState* p, s32 id)
{
	p->SetAnim(id, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
}

/**
 * @note Address: 0x800DCD40
 * @note Size: 0x11EC
 */
// NONMATCHING: gait, idle, fidget and action-specific animation selection are present;
// the ordered call set matches; switch layout, rate scheduling and register allocation still differ
void UpdatePlayerLocomotionAnimation(WiiPlayerState* p, s32, CObj* model, u32, f32, f32)
{
	CPadOne* pad = &p->mPad;
	f32 motion   = GetStickMotion(pad);
	if (motion == 0.0f && p->mPreviousContactObject >= 0)
		motion = 0.8f;
	if (motion > 0.0f && (gGame.GetStatusFlags(0x100)) && !(gGame.GetStatusFlags(0x200)))
		motion = 0.0f;
	if (__fpclassifyf(p->mWalkHeading) != FP_NAN)
		motion = PSVECMag(&p->mMoveSpeed0);

	// the gait goes by the speed he tries to go: divide out what the walk code scaled him by (obstacles or the rate multiplier)
	f32 speed;
	if (sBlockedObstacleCount >= 1) {
		f32 scale = 1.0f - 0.15f * (f32)sBlockedObstacleCount;
		if (scale < 0.7f)
			scale = 0.7f;
		speed = p->mMoveSpeed / scale;
	} else {
		speed = p->mMoveSpeed / (((IsLiftActionState(&p->mLift, 4) && IsLiftTargetPlug(&p->mLift)) ? 0.4f : 1.0f) * (60.0f / gRefreshRate));
	}
	if ((p->mLift.mState == 0 || p->mLift.mState == 4) && !p->mHang.mState && !p->mJump.mState && !p->mLadder.mState && !p->mRope.mState
	    && !p->mPush.mState && !p->mTumble.mState && !p->mCopter.mState
	    && (p->mPopper.mState == 0 || p->mPopper.mState == 8 || p->mPopper.mState == 4) && !p->mRadar.mState) {
		f32 distance = PSVECDistance(&p->mMotion.mValue, &p->mMotion.mTarget);
		if (distance / (((IsLiftActionState(&p->mLift, 4) && IsLiftTargetPlug(&p->mLift)) ? 0.4f : 1.0f) * (60.0f / gRefreshRate))
		    >= 3.0f) {
			p->SetAnim(0x79, FALSE, 0.0f, 1.0f, 0.0f, 30.0f);
		} else if (p->GetAnimId() == 0x79 || p->GetAnimId() == 0x89 || p->GetAnimId() == 0xE0) {
			SetPlayerIdleAnim(p, 0x11);
			p->mIdleLoops = 3;
		}
	}
	if (p->mPopper.mState == 8 && ((XObjPopper*)p->mPopperObj)->GetAnimId() == 5 && ((XObjPopper*)p->mPopperObj)->GetAnimFrame() <= 10.0f) {
		if (gGame.mStage != 0xB || (p->GetAnimId() != 0x35 && p->GetAnimId() != 0x36)) {
			p->SetAnim(0x822, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
		}
		return;
	}
	if ((p->mLift.mState == 0 || p->mLift.mState == 4) && !p->mHang.mState && !p->mJump.mState && !p->mLadder.mState && !p->mRope.mState
	    && !p->mPush.mState && !p->mTumble.mState && !p->mCopter.mState
	    && (p->mPopper.mState == 0 || p->mPopper.mState == 8 || p->mPopper.mState == 4) && !p->mRadar.mState) {
		switch (p->GetAnimId()) {
		case 0x160:
		case 0x161:
		case 0x162:
		case 0x163:
			p->SetAnim(0x11, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
			p->mIdleLoops = rand() % 10 + 5;
			break;
		// standing and idle anims: moving starts the walk, standing still he may fidget
		case 0:
		case 1:
		case 2:
		case 8:
		case 0x11:
		case 0x12:
		case 0x4A:
		case 0xA1:
		case 0xA3:
		case 0x822:
		case 0x874:
		case 0x894:
		case 0x8B2:
		case 0xCF7:
		case 0xCF8:
		case 0xCF9:
		case 0xCFA:
		case 0xCFB:
		case 0xCFC:
		case 0xCFD: {
			BOOL script = gGame.mEvt.mData.mScript != NULL;
			if (script && __fpclassifyf(p->mWalkHeading) == FP_NAN
			    && (((XObjAttachment*)p->mCopterObj)->mVisible || ((XObjAttachment*)p->mPopperObj)->mVisible
			        || ((XObjAttachment*)p->mRadarObj)->mVisible || ((XObjAttachment*)p->mOXObj)->mVisible))
				break;
			if (p->_1C9B8[2] > 0.0f) {
				if (p->mLift.mState == 4) {
					if (!IsCarryingToothbrush(&p->mLift) && !IsCarryingSpoon(&p->mLift) && !IsCarryingSquirter(&p->mLift))
						p->SetAnim(0x162, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
					else
						p->SetAnim(0x163, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
				} else if (p->GetAttachment() == 2) {
					p->SetAnim(0x161, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
				} else {
					p->SetAnim(0x160, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
				}
				break;
			}
			if (motion != 0.0f) {
				SetPlayerIdleAnim(p, 3);
				p->mIdleLoops = 0;
				break;
			}
			if (p->mLift.mState != 0 || p->mFloorContactObjects[2] >= 0 || p->mSuits.mChangeStep)
				break;
			if (p->GetAttachment() == 1 || p->GetAttachment() == 3) {
				SetPlayerIdleAnim(p, 0x4A);
			} else if (p->GetAttachment() == 2) {
				SetPlayerIdleAnim(p, 0x822);
			} else if ((gGame.GetStatusFlags(0x100)) || script) {
				SetPlayerIdleAnim(p, 0x11);
				p->mIdleLoops = 3;
			} else if (model->IsAnimDone()) {
				if (p->mIdleLoops > 0)
					--p->mIdleLoops;
				if (p->mIdleLoops == 0 && (p->mPlayerFlags & 4)) {
					s32 id = 0x11;
					if (!gGame.mEvt.mData.mScript && (p->mHitObjectIndex < 0 || p->mHitObjectIndex == p->mFloorContactObjects[0])) {
						// one time in four he fidgets, bits 4-5 of the same roll pick which
						s32 pick = rand();
						if (!(pick & 3)) {
							switch (pick & 0x30) {
							case 0x10:
								id = 1;
								break;
							case 0x20:
								id = 2;
								break;
							case 0x30:
								id = 0x12;
								break;
							default:
								id = gPlayerSuitIdleAnimations[p->mSuits.mCurrent];
								break;
							}
							p->mIdleLoops = rand() % 10 + 5;
						}
					}
					SetPlayerIdleAnim(p, id);
				}
			}
			break;
		}
		// walking, anim 3 and its variants (gPlayerAnimVariants)
		case 3:
		case 0xC:
		case 0x818:
		case 0x896:
			if (motion == 0.0f) {
				if (p->GetAttachment() == 1 || p->GetAttachment() == 3)
					SetPlayerIdleAnim(p, 0x4A);
				else {
					SetPlayerIdleAnim(p, 0x11);
					p->mIdleLoops = 3;
				}
			} else if (speed >= 1.25f) {
				ChangePlayerGait(p, 0x77, PlayerGaitRate(speed, false));
			} else {
				p->SetAnimRate(PlayerGaitRate(speed, true));
			}
			break;
		// jogging, anim 0x77 and its variants
		case 0x77:
		case 0x88:
		case 0x882:
		case 0x897:
			if (speed < 1.25f) {
				f32 rate = speed > 1.0f ? 1.0f : speed;
				if (rate < 0.5f)
					rate = 0.5f;
				ChangePlayerGait(p, 3, rate);
			} else if (speed >= 1.9f)
				ChangePlayerGait(p, 4, PlayerGaitRate(speed, false));
			else
				p->SetAnimRate(PlayerGaitRate(speed, false));
			break;
		// running, anim 4 and its variants
		case 4:
		case 0xD:
		case 0x819:
		case 0x898:
			if (speed < 1.25f) {
				f32 rate = speed > 1.0f ? 1.0f : speed;
				if (rate < 0.5f)
					rate = 0.5f;
				ChangePlayerGait(p, 3, rate);
			} else if (speed < 1.9f) {
				f32 rate = speed > 1.0f ? 1.0f : speed;
				if (rate < 0.5f)
					rate = 0.5f;
				ChangePlayerGait(p, 0x77, rate);
			} else
				p->SetAnimRate(PlayerGaitRate(speed, false));
			break;
		}
	}
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

static inline bool IsActionIdleOr(const XAction& action, s32 rest)
{
	s32 state = action.mState;
	return state == 0 || state == rest;
}

static inline bool IsActiveActionTarget(const XAction& action, const XObj* object)
{
	return action.mState != 0 && action.mTarget == object;
}

// NONMATCHING: retail allocates the current object to r30 and its priority to r29, the build uses r25/r30;
// the idle-action tests also use the complementary branch shape
void UpdatePlayerLookAtObjects(WiiPlayerState* player, u32 frames)
{
	BOOL enabled;
	if (gGame.mEvt.mData.mScript != NULL || gGame.GetStatusFlags(0x100) || player->mPreviousContactObject >= 0)
		enabled = FALSE;
	else if (player->GetAttachment() == 1 || player->GetAttachment() == 2 || player->GetAttachment() == 3)
		enabled = FALSE;
	else if (IsActionIdleOr(player->mLift, 4) && IsActionIdleOr(player->mHang, 5) && player->mJump.mState == 0
	         && IsActionIdleOr(player->mLadder, 5) && IsActionIdleOr(player->mRope, 5) && player->mPush.mState == 0
	         && player->mTumble.mState == 0 && player->mCopter.mState == 0 && player->mPopper.mState == 0 && player->mRadar.mState == 0)
		enabled = TRUE;
	else
		enabled = FALSE;

	CVec pos(*player->GetPos());
	CVec head  = player->GetJointWorldPos("head_chn");
	XObj* best = NULL;
	if (enabled) {
		f32 nearest      = NAN;
		s16 bestPriority = 0;
		for (s32 i = 0; i < gGame.mXObjs.mObjectCount; ++i) {
			XObj* object = gGame.mXObjs.mObjects[i];
			if (!(object->mStatus.mFlags & 1) || !(object->mStatus.mFlags & 0x10000) || (object->mStatus.mFlags & 0x4000000000000000ULL)
			    || object == player || object->GetKind() >= 10000)
				continue;
			if (IsActiveActionTarget(player->mLift, object) || IsActiveActionTarget(player->mHang, object)
			    || IsActiveActionTarget(player->mJump, object) || IsActiveActionTarget(player->mLadder, object)
			    || IsActiveActionTarget(player->mRope, object) || IsActiveActionTarget(player->mPush, object)
			    || IsActiveActionTarget(player->mTumble, object) || IsActiveActionTarget(player->mCopter, object)
			    || IsActiveActionTarget(player->mPopper, object) || IsActiveActionTarget(player->mRadar, object))
				continue;
			s16 priority = object->GetLookPriority();
			if (priority == 0)
				continue;
			CVec target(*object->GetPos());
			if (target.y > pos.y - 4.0f && target.y < 4.0f + head.y) {
				f32 dx       = pos.x - target.x;
				f32 dz       = pos.z - target.z;
				f32 distance = FSqrt(dx * dx + dz * dz);
				if (distance < object->GetLookRange()) {
					f32 yaw = WrapAngle((f32)atan2(target.x - pos.x, target.z - pos.z) - player->mMoveRotate.y);
					if (yaw <= HALF_PI || yaw >= 3.0f * HALF_PI) {
						BOOL select = FALSE;
						if (best == NULL)
							select = TRUE;
						else if (priority > bestPriority)
							select = TRUE;
						else if (priority >= bestPriority && distance < nearest)
							select = TRUE;
						if (select) {
							nearest      = distance;
							best         = object;
							bestPriority = priority;
						}
					}
				}
			}
		}
	}

	if (best != NULL) {
		player->mEyeAimRecord.mSnap[0] = player->mEyeAimRecord.mSnap[1] = 1;
		player->mEyeAimRecord.m_0C                                      = best;
		player->mEyeAimRecord.mTarget.x                                 = NAN;
		player->mEyeAimRecord.m_20                                      = NULL;
		player->mEyeAimRecord.m_24                                      = NAN;
		player->mEyeAimRecord.m_28                                      = NAN;
	} else {
		BOOL update = FALSE;
		if (player->mEyeAimRecord.mSnap[0] == 0 && player->mEyeAimRecord.mSnap[1] == 0)
			update = TRUE;
		if (!update) {
			player->mEyeAimRecord.mSnap[0] = player->mEyeAimRecord.mSnap[1] = 2;
			player->mEyeAimRecord.m_0C                                      = NULL;
			player->mEyeAimRecord.mTarget.x                                 = NAN;
			player->mEyeAimRecord.m_20                                      = NULL;
			player->mEyeAimRecord.m_24                                      = NAN;
			player->mEyeAimRecord.m_28                                      = NAN;
			player->mEyeAimRecord.m_30                                      = 0.0f;
			player->mEyeAimRecord.m_2C                                      = 0.0f;
			player->mEyeAimRecord.m_4C.Set(0.0f, 0.0f, 0.0f);
		}
	}
	BOOL update = FALSE;
	if (player->mEyeAimRecord.mSnap[0] == 0 && player->mEyeAimRecord.mSnap[1] == 0)
		update = TRUE;
	if (!update)
		fn_800C6CF0(&player->mEyeAimRecord, frames);
}

void UpdatePlayerActionAnimations(WiiPlayerState* player, u32 frames)
{
	if (!(player->mPlayerFlags & 0x80000000)) {
		s32 anim  = player->GetAnimId();
		f32 frame = player->GetAnimFrame();
		f32 end   = player->GetAnimEndFrame();
		UpdateLiftActionAnimation((XActLift*)&player->mLift, anim, player, frames, frame, end);
		UpdatePlayerHangAction(&player->mHang, anim, player, frames, frame, end);
		UpdatePlayerJumpAction((XActJump*)&player->mJump, anim, player, frames, frame, end);
		UpdateLadderActionAnimation((XActLadder*)&player->mLadder, anim, player, frames, frame, end);
		UpdatePlayerRopeAction((XActRope*)&player->mRope, anim, player, frames, frame, end);
		UpdatePushActionAnimation((XActPush*)&player->mPush, anim, player, frames, frame, end);
		UpdatePlayerTumbleAnimation(&player->mTumble, anim, player, frames, frame, end);
		UpdatePlayerCopterFlight(&player->mCopter, anim, player, frames, frame, end);
		UpdatePlayerPopperAction((XActPopper*)&player->mPopper, anim, player, frames, frame, end);
		UpdateRadarTargeting((XActRadar*)&player->mRadar, anim, player, frames, frame, end);
		if (player->mAttachmentRequest < 0 && !gGame.GetStatusFlags(0x400)) {
			UpdatePlayerLocomotionAnimation(player, anim, NULL, frames, frame, end);
		}
		switch (anim) {
		case 0x5D:
			if (player->IsAnimDone()) {
				player->SetAnim(0x5E, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
			}
			break;
		case 0x5F:
			if (player->IsAnimDone()) {
				player->SetAnim(0x11, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
				player->mIdleLoops = 3;
			}
			break;
		case 0x78:
		case 0x13B:
		case 0xEF:
			if (player->IsAnimDone()) {
				player->SetAnim(0x11, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
				player->mIdleLoops = 3;
			}
			break;
		}
	}
	fn_800DDF2C(player, frames);
	UpdatePlayerLookAtObjects(player, frames);
	BOOL skipHeadAim = FALSE;
	if (player->mHeadAimRecord.mSnap[0] == 0 && player->mHeadAimRecord.mSnap[1] == 0) {
		skipHeadAim = TRUE;
	}
	if (!skipHeadAim) {
		fn_800C6CF0(&player->mHeadAimRecord, frames);
	}
	if (player->mFlashTimer > 0) {
		if ((player->mFlashTimer -= frames) < 0) {
			player->mFlashTimer = 0;
		}
		player->mFlashCount++;
		if (player->mFlashTimer > 0) {
			f32 flash = (f32)player->mFlashTimer / gRefreshRate;
			player->SetColor(1, 128.0f * flash, 0, 0, 0.0f);
			StartScreenFade(&gGame.mScrnTex.mColor, 128.0f * flash, 0, 0, 0, 128.0f * flash, 0, 0, 0, 0.0f);
		} else {
			player->SetColor(1, 0, 0, 0, 0.0f);
			StartScreenFade(&gGame.mScrnTex.mColor, 0, 0, 0, 0, 100, 100, 200, 0, 0.0f);
		}
	}
}

static inline bool InAnimRange(s32 anim, s32 first, s32 last)
{
	return anim >= first && anim <= last;
}

// NONMATCHING: Wii action costs, battery depletion, effect cleanup and warning fades
// are present; the ordered call set matches; switch layout and register scheduling differ
void UpdatePlayerBattery(WiiPlayerState* p, u32 frames)
{
	const CPadOne* pad = &p->mPad;
	// the row of the action battery costs (GetPlayerActionParameter) for what he does: 0-1 standing, 2 walk, 3 jog, 4 run; -1 none
	s32 index = -1;
	if (p->mLift.mState && IsJumpActionState(&p->mJump, 0)) {
		switch (p->mLift.mState) {
		case 1:
		case 2:
		case 21:
			index = 7;
			break;
		case 6:
		case 20:
			index = 8;
			break;
		case 3:
		case 4:
		case 29:
		case 30:
			if (IsJumpActionState(&p->mJump, 0)) {
				switch (p->GetAnimId()) {
				case 0x89:
				case 0xE0:
					index = 5;
					break;
				case 0x896:
				case 0xC:
					index = 2;
					break;
				case 0x897:
				case 0x88:
					index = 3;
					break;
				case 0x898:
				case 0xD:
					index = 4;
					break;
				case 0x895:
					index = 34;
					break;
				case 0x8B7:
				case 0x8AF:
				case 0x157:
					index = 35;
					break;
				case 0x8B1:
					index = 36;
					break;
				default:
					index = 0;
					break;
				}
			}
			break;
		case 28: // using the toothbrush
		case 31:
			if (p->GetAnimId() == 0x895)
				index = 34;
			else if (p->GetAnimId() == 0x152)
				index = 4;
			break;
		case 32: // the spoon
		case 33:
			switch (p->GetAnimId()) {
			case 0x8AF:
			case 0x157:
				index = 35;
				break;
			case 0x8B7:
				index = 4;
				break;
			case 0x156:
				index = 0;
				break;
			}
			break;
		case 34: // the mug
		case 35:
			if (p->GetAnimId() == 0x8B1)
				index = 36;
			break;
		case 38: // the squirter
		case 39:
			switch (p->GetAnimId()) {
			case 0x8C0:
			case 0x8C1:
				index = 37;
				break;
			case 0x8C2:
			case 0x8C3:
			case 0x8C4:
			case 0x8C5:
				index = 38;
				break;
			}
			break;
		case 14:
			index = 9;
			break;
		}
	} else if (p->mHang.mState) {
		switch (p->mHang.mState) {
		case 1:
		case 2:
			index = 10;
			break;
		case 4:
			index = 14;
			break;
		case 5:
			if (p->mHang.mMoveDirection == -1 || p->mHang.mMoveDirection == 1)
				index = 11;
			else
				index = 0;
			break;
		case 6:
		case 7:
			index = 12;
			break;
		case 9:
			index = 13;
			break;
		}
	} else if (p->mJump.mState) {
		switch (p->mJump.mState) {
		case 8:
		case 9:
		case 11:
		case 12:
		case 14:
		case 15:
			index = 15;
			break;
		case 6:
			index = 16;
			break;
		}
	} else if (p->mLadder.mState) {
		switch (p->mLadder.mState) {
		case 1:
		case 2:
			index = 17;
			break;
		case 5:
			if (p->mLadder.mMoveDirection == 1)
				index = 18;
			else if (p->mLadder.mMoveDirection == -1)
				index = 19;
			else
				index = 0;
			break;
		case 6:
			index = 20;
			break;
		case 7:
		case 9:
			index = 21;
			break;
		}
	} else if (p->mRope.mState) {
		switch (p->mRope.mState) {
		case 1:
		case 2:
			index = 22;
			break;
		case 5:
			if (p->mRope.mMoveDirection == 1 || p->mRope.mMoveDirection == 2)
				index = 23;
			else if (p->mRope.mMoveDirection == -1)
				index = 24;
			else
				index = 0;
			break;
		case 6:
			index = 25;
			break;
		case 8:
			index = 26;
			break;
		}
	} else if (p->mPush.mState) {
		switch (p->mPush.mState) {
		case 2:
			if (p->mPush.mMoveDirection == 1 || p->mPush.mMoveDirection == -1)
				index = 27;
			else
				index = 0;
			break;
		}
	} else if (p->mCopter.mState) {
		switch (p->mCopter.mState) {
		case 1:
		case 2:
			index = 6;
			break;
		case 3:
			index = 28;
			break;
		case 4:
			index = 29;
			break;
		}
	} else {
		if (p->mPopper.mState != 0 && p->mPopper.mState != 4) {
			switch (p->mPopper.mState) {
			case 1:
			case 2:
			case 9:
			case 10:
				index = 6;
				break;
			case 3:
				index = 0;
				break;
			case 6:
			case 7:
			case 8:
				if (((XActPopper*)&p->mPopper)->mShotStrength)
					index = 30;
				break;
			default:
				break;
			}
		} else if (p->mRadar.mState != 0) {
			switch (p->mRadar.mState) {
			case 1:
			case 2:
				index = 6;
				break;
			case 3:
			case 4:
			case 5:
				index = 32;
				break;
			default:
				if (p->mRadar.mTarget)
					index = 33;
				break;
			}
		} else if (p->mRadar.mTarget) {
			index = 33;
		} else {
			switch (p->GetAnimId()) {
			case 0x79:
				index = 5;
				break;
			case 3:
			case 0xA3:
			case 0x818:
			case 0x874:
				index = 2;
				break;
			case 0x77:
			case 0x882:
				index = 3;
				break;
			case 4:
			case 0x819:
				index = 4;
				break;
			case 1:
			case 0x11:
			case 0x4A:
			case 0xA1:
			case 0x822:
				index = 0;
				break;
			case 2:
			case 0x12:
			case 0xCF7:
			case 0xCF8:
			case 0xCF9:
			case 0xCFA:
			case 0xCFB:
			case 0xCFC:
			case 0xCFD:
				index = 1;
				break;
			}
		}
	}
	// battery per second, the table holds hundredths; row 9 instead drains about the lift's mBatteryCost each update
	f32 drain = 0.0f;
	if (index >= 0) {
		if (index == 9) {
			f32 rate = (u32)gRefreshRate / frames;
			drain    = rate * ((XActLift*)&p->mLift)->mBatteryCost;
		} else
			drain = GetPlayerActionParameter(index) / 100.0f;
	}
	if ((s32)p->mParams._1C[0] > 0) { // unlimited battery, script commands 850/851
		drain = 0.0f;
		p->mParams.SetBattery(p->mParams.mMaxBattery);
	} else if (p->mPlug.mConnection > 0 && (u8)p->mPlug.mConnection) {
		drain = 0.0f;
		p->mParams.SetBattery(p->mParams.mMaxBattery);
	} else if (gGame.mEvt.mData.mScript || (gGame.GetStatusFlags(0xC00))) {
		drain = 0.0f;
	} else if ((gGame.GetStatusFlags(0x80000000)) && !(gGame.GetStatusFlags(0x120)) && pad->mSubStick.y != 0.0f) {
		drain = 0.0f;
	}
	if (drain > 0.0f)
		drain /= (f32)gRefreshRate;
	// mIdleTimer runs down while he stands or idles in place, else resets to a second; at 0 the HUD slides its counters in
	s32 anim = p->GetAnimId();
	if (!gGame.mEvt.mData.mScript
	    && (p->mAttachmentRequest >= 0 || (anim >= 1 && anim <= 2) || (anim >= 0x11 && anim <= 0x12) || (anim >= 0xCF7 && anim <= 0xCFD)
	        || (anim >= 5 && anim <= 11) || (anim >= 0x158 && anim <= 0x159) || (anim >= 0x894 && anim <= 0x895)
	        || (anim >= 0x8AE && anim <= 0x8AF) || (anim >= 0x8B1 && anim <= 0x8B3) || anim == 0x14 || (anim >= 0x67 && anim <= 0x68)
	        || anim == 0x8A || anim == 0x8C || anim == 0x3F || anim == 0x4A || anim == 0xA1 || anim == 0x822
	        || (anim >= 0x5D && anim <= 0x5E) || anim == 0x5F)) {
		if (p->mIdleTimer > 0.0f) {
			if ((p->mIdleTimer -= (f32)frames) < 0.0f)
				p->mIdleTimer = 0.0f;
		}
	} else {
		p->mIdleTimer = (f32)gRefreshRate;
	}
	if (p->mBatteryDrains) {
		f32 battery;
		if ((p->mTumble.mState == 8 || p->mTumble.mState == 9) && p->mHitStrength > 0.0f) {
			battery         = p->mParams.mBattery - p->mHitStrength;
			p->mHitStrength = 0.0f;
		} else {
			battery = p->mParams.mBattery - drain * (f32)frames;
		}
		if (battery < 0.0f)
			battery = 0.0f;
		if (battery != p->mParams.mBattery)
			p->mParams.SetBattery(battery);
		// the battery ran out: he collapses (tumble 4), the camera turns to him and the sounds of what he was doing stop
		if (IsStopped(p) && !gGame.mEvt.mData.mScript && gGame.GetStatusFlags(0x40000000) && GetStageBatteryDepletedScript()
		    && p->mAttachmentRequest < 0
		    && (p->mLift.mState == 0 || p->mLift.mState == 4 || p->mLift.mState == 29 || p->mLift.mState == 33 || p->mLift.mState == 35
		        || (p->mLift.mState == 2 && p->GetAnimId() == 0x159))
		    && p->mCopter.mState == 0 && p->mTumble.mState != 4) {
			ResetPlayerAttachment(p, -1);
			p->mTumble.mState = 4;
			p->SetColor(1, 255, 0, 0, 0.0f);
			p->SetColor(1, 0, 0, 0, 0.4f);
			StartScreenFade(&gGame.mScrnTex.mColor, 255, 0, 0, 0, 100, 100, 200, 0, 0.4f);
			p->mPlug.mConnection = 0;
			gGame.mMotionControlCamera.EndEye();
			SetAreaCameraInterestTarget((CCameraActionArea*)gGame.mCameraActions.mTable[1], 4, p, 0.5f);
			SelectActiveAreaCameraEntryByViewDirection((CCameraActionArea*)gGame.mCameraActions.mTable[1], 1, 1.0f);
			gGame.mSubwin.mEnabled = 0;
			fn_800A12F8((EffectInstance*)&gGame.mEvtPtcl, 0x1D, FALSE);
			if (IsWiiSfxPlaying(&gGame.mSound, 0x75))
				StopWiiSfx(&gGame.mSound, 0x75, 0.0f);
			if (IsWiiSfxPlaying(&gGame.mSound, 499))
				StopWiiSfx(&gGame.mSound, 499, 0.0f);
			if (IsWiiSfxPlaying(&gGame.mSound, 0x1F8))
				StopWiiSfx(&gGame.mSound, 0x1F8, 0.0f);
			if (IsWiiSfxPlaying(&gGame.mSound, 0x1F9))
				StopWiiSfx(&gGame.mSound, 0x1F9, 0.0f);
			if (IsWiiSfxPlaying(&gGame.mSound, 0x1FA))
				StopWiiSfx(&gGame.mSound, 0x1FA, 0.0f);
			if (IsWiiSfxPlaying(&gGame.mSound, 0x1FB))
				StopWiiSfx(&gGame.mSound, 0x1FB, 0.0f);
		}
	}
	if (p->mParams.mBattery > 0.0f && p->mParams.mBattery <= 30.0f) {
		p->mBatteryWarningFrames += frames;
		if (p->mAnmQueue.mColorStep[1].mSpeed.x == 0.0f && p->mAnmQueue.mColorStep[1].mSpeed.y == 0.0f
		    && p->mAnmQueue.mColorStep[1].mSpeed.z == 0.0f && p->mBatteryWarningFrames >= gRefreshRate && !gGame.mEvt.mData.mScript) {
			p->SetColor(1, 255, 100, 0, 0.0f);
			p->SetColor(1, 0, 0, 0, 1.0f);
			StartScreenFade(&gGame.mScrnTex.mColor, 255, 100, 0, 0, 100, 100, 200, 0, 1.0f);
			p->mBatteryWarningFrames = 0;
			p->mBatteryWarningFade   = 1.0f;
			StopWiiSfx(&gGame.mSound, 0x11E, 0.0f);
			PlayWiiSoundEffect(&gGame.mSound, 0x11E, 1.0f, 0.0f);
		}
	} else if (p->mBatteryWarningFrames > 0) {
		p->mBatteryWarningFrames = 0;
		p->mBatteryWarningFade   = 0.0f;
	}
	if (p->mBatteryWarningFade > 0.0f) {
		if ((p->mBatteryWarningFade -= 0.02f * (f32)frames) < 0.0f)
			p->mBatteryWarningFade = 0.0f;
		p->mBatteryWarningAngle -= DEG2RAD * 5.0f * (f32)frames;
	}
	if ((p->mParams.mBattery <= 0.0f || p->mParams.mBattery > 30.0f
	     || (gGame.mEvt.mData.mScript != NULL && gGame.mEvt.mData.mScript != GetStageZButtonScript()))
	    && IsWiiSfxPlaying(&gGame.mSound, 0x11E)) {
		StopWiiSfx(&gGame.mSound, 0x11E, 0.0f);
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

// NONMATCHING: variant selection, archives, suit parts and plug rate are present;
// temporary lifetime and register allocation still differ
static inline void FindPlayerAnimVariant(s32& id, s32 variant)
{
	for (s32 i = 0; gPlayerAnimVariants[i].normal >= 0; ++i) {
		if (gPlayerAnimVariants[i].normal != id)
			continue;
		if (variant == 0 && gPlayerAnimVariants[i].carrying >= 0)
			id = gPlayerAnimVariants[i].carrying;
		else if (variant == 2) {
			if (gPlayerAnimVariants[i].attachment >= 0)
				id = gPlayerAnimVariants[i].attachment;
		} else if (gPlayerAnimVariants[i].lifted >= 0)
			id = gPlayerAnimVariants[i].lifted;
		break;
	}
}

void XObjChr::SetAnim(s32 id, BOOL force, void*, void*, void*, f32 frame, f32 rate, f32 blend, f32 blendRate)
{
	WiiPlayerState* chr = (WiiPlayerState*)this;
	if (chr->_969C != 0) {
		frame      = 1.0f;
		chr->_969C = 0;
	}
	if (IsCarryingToothbrush(&chr->mLift) || IsCarryingSpoon(&chr->mLift) || IsCarryingSquirter(&chr->mLift))
		FindPlayerAnimVariant(id, 0);
	else if (chr->mLift.mState == 4)
		FindPlayerAnimVariant(id, 1);
	else if (chr->mLift.mState == 0 && (chr->GetAttachment() == 1 || chr->GetAttachment() == 3))
		FindPlayerAnimVariant(id, 2);
	if (id >= 3000 && chr->mSuitAnm.mAnm.mFile.IsLoaded()) {
		XObj::SetAnim(id, force, frame, rate * (chr->mLift.mState == 4 && IsLiftTargetPlug(&chr->mLift) && id != 0x13B ? 0.4f : 1.0f),
		              blend, blendRate);
	} else if (id >= 2000 && chr->mAttachmentAnm.mAnm.mFile.IsLoaded()) {
		XObj::SetAnim(id, force, frame, rate * (chr->mLift.mState == 4 && IsLiftTargetPlug(&chr->mLift) && id != 0x13B ? 0.4f : 1.0f),
		              blend, blendRate);
	} else {
		XObj::SetAnim(id, force, frame, rate * (chr->mLift.mState == 4 && IsLiftTargetPlug(&chr->mLift) && id != 0x13B ? 0.4f : 1.0f),
		              blend, blendRate);
	}
	WiiSuitObject* suit = &chr->mSuits.mObjects[chr->mSuits.mCurrent];
	if (IsObjectActive(suit)) {
		suit->SetAnim(id, force, frame, rate * (chr->mLift.mState == 4 && IsLiftTargetPlug(&chr->mLift) && id != 0x13B ? 0.4f : 1.0f),
		              blend, blendRate);
		if (chr->mSuits.mCurrent == 6) {
			WiiSuitPart* part = &chr->mSuitParts[6];
			if (id == 0xCE8) {
				if (part->GetAnimId() != 2)
					part->SetAnim(2, force, frame,
					              rate * (chr->mLift.mState == 4 && IsLiftTargetPlug(&chr->mLift) && id != 0x13B ? 0.4f : 1.0f), blend,
					              blendRate);
			} else if (id == 0xCE7) {
				if (part->GetAnimId() != 1)
					part->SetAnim(1, force, frame,
					              rate * (chr->mLift.mState == 4 && IsLiftTargetPlug(&chr->mLift) && id != 0x13B ? 0.4f : 1.0f), blend,
					              blendRate);
			} else if (id == 0xCE6) {
				if (part->GetAnimId() != 0)
					part->SetAnim(0, force, frame,
					              rate * (chr->mLift.mState == 4 && IsLiftTargetPlug(&chr->mLift) && id != 0x13B ? 0.4f : 1.0f), blend,
					              blendRate);
			}
		}
	}
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

// NONMATCHING: all animation target cases and the target history are present;
// aggregate return homes and float/register scheduling remain different
Vec XObjChr::GetCameraTarget()
{
	WiiPlayerState* chr = (WiiPlayerState*)this;
	if ((chr->mStatus.mFlags & 1)) {
		s32 anim = chr->GetAnimId();
		Vec target;
		if (anim == 0xaf || anim == 0xea) {
			CVec joint = GetJointWorldPos("second_null");
			CVec start = chr->mMoveTranslate;
			f32 end    = GetAnimEndFrame();
			f32 t      = GetAnimFrame() / end;
			if (t > 1.0f)
				t = 1.0f;
			target = start + (joint - start) * t + (Vec) { 0, 10, 0 };
		} else if (anim == 0x17 || anim == 0x7d || anim == 0x1f || anim == 0x20) {
			CVec joint = GetJointWorldPos("second_null");
			CVec start = chr->mMoveTranslate;
			start.x    = joint.x;
			start.z    = joint.z;
			f32 end    = GetAnimEndFrame();
			f32 t      = GetAnimFrame() / end;
			if (t > 1.0f)
				t = 1.0f;
			target = start + (joint - start) * t + (Vec) { 0, 10, 0 };
		} else if (anim == 0x76) {
			CVec joint = GetJointWorldPos("second_null");
			CVec start = chr->mMoveTranslate;
			start.x    = joint.x;
			start.z    = joint.z;
			f32 end    = GetAnimEndFrame();
			f32 t      = 1.7f * (GetAnimFrame() / end);
			if (t > 1.f)
				t = 1.f;
			target = joint + (start - joint) * t + (Vec) { 0, 10, 0 };
		} else if (!IsRopeActionState(&chr->mRope, 0)) {
			if (IsRopeActionState(&chr->mRope, 2) && anim == 0x98) {
				CVec joint = GetJointWorldPos("second_null");
				CVec start = chr->mMoveTranslate;
				if (GetAnimFrame() > 80.f) {
					joint = chr->mMoveTranslate;
					start.y -= chr->mRope.mTargetOffset.y;
					f32 end = GetAnimEndFrame() - 80.f;
					f32 t   = (GetAnimFrame() - 80.f) / end;
					if (t > 1.0f)
						t = 1.0f;
					target = joint + (start - joint) * t + (Vec) { 0, 10, 0 };
				} else {
					start.x = joint.x;
					start.z = joint.z;
					target  = start + (Vec) { 0, 10, 0 };
				}
			} else if (IsRopeActionState(&chr->mRope, 6) && anim == 0x24) {
				CVec start = chr->mMoveTranslate;
				CVec joint = GetJointWorldPos("second_null");
				start.x    = joint.x;
				start.z    = joint.z;
				f32 end    = GetAnimEndFrame();
				f32 t      = GetAnimFrame() / end;
				if (t > 1.0f)
					t = 1.0f;
				target = start + (joint - start) * t + (Vec) { 0, 10, 0 };
			} else if (anim == 0x85) {
				CVec joint = GetJointWorldPos("second_null");
				CVec start = chr->mMoveTranslate;
				start.x    = joint.x;
				start.z    = joint.z;
				f32 end    = GetAnimEndFrame();
				f32 t      = 1.7f * (GetAnimFrame() / end);
				if (t > 1.f)
					t = 1.f;
				target = joint + (start - joint) * t + (Vec) { 0, 10, 0 };
			} else {
				CVec joint = GetJointWorldPos("second_null");
				CVec start = chr->mMoveTranslate;
				start.x    = joint.x;
				start.z    = joint.z;
				target     = start + (Vec) { 0, 10, 0 };
			}
		} else if (!IsLadderActionState(&chr->mLadder, 0)) {
			if (anim == 0x87) {
				CVec joint = GetJointWorldPos("second_null");
				CVec start = chr->mMoveTranslate;
				start.x    = joint.x;
				start.z    = joint.z;
				f32 end    = GetAnimEndFrame();
				f32 t      = 1.7f * (GetAnimFrame() / end);
				if (t > 1.f)
					t = 1.f;
				target = start + (joint - start) * t + (Vec) { 0, 10, 0 };
			} else if (anim == 0x72 || anim == 0x73 || anim == 0x74) {
				target = GetJointWorldPos("second_null") + (Vec) { 0, 10, 0 };
			} else {
				CVec joint = GetJointWorldPos("second_null");
				CVec start = chr->mMoveTranslate;
				start.x    = joint.x;
				start.z    = joint.z;
				f32 end    = GetAnimEndFrame();
				f32 t      = GetAnimFrame() / end;
				if (t > 1.0f)
					t = 1.0f;
				target = start + (joint - start) * t + (Vec) { 0, 10, 0 };
			}
		} else if (anim == 0x13 || anim == 0x14 || anim == 0x15 || anim == 0x16 || anim == 0x19 || anim == 0x1a || anim == 0x1b) {
			CVec joint = GetJointWorldPos("second_null");
			CVec start = chr->mMoveTranslate;
			start.x    = joint.x;
			start.z    = joint.z;
			target     = start + (Vec) { 0, 10, 0 };
		} else if (anim == 0x18) {
			Vec offset;
			offset.x = chr->mHang.mTargetOffset.x;
			offset.y = -chr->mHang.mTargetOffset.y;
			offset.z = chr->mHang.mTargetOffset.z;
			Mtx rot;
			PSMTXRotRad(rot, 'y', chr->mMoveRotate.y);
			PSMTXMultVec(rot, &offset, &offset);
			CVec end   = GetJointWorldPos("second_null") + offset;
			CVec start = chr->mMoveTranslate;

			start.x      = end.x;
			start.z      = end.z;
			f32 frame    = GetAnimFrame();
			f32 duration = GetAnimEndFrame();
			f32 t        = frame / duration;
			f32 x = start.x + t * (end.x - start.x), z = start.z + t * (end.z - start.z);
			f32 threshold = .91f * duration;
			f32 elapsed   = GetAnimFrame() - threshold;
			f32 remaining = GetAnimEndFrame() - threshold;
			f32 dy        = elapsed > 0 ? (end.y - start.y) * (elapsed / remaining) : 0.f;
			target        = (Vec) { x, 10.f + (start.y + dy), z };

		} else if (IsJumpActionState(&chr->mJump, 9) || IsJumpActionState(&chr->mJump, 12) || IsJumpActionState(&chr->mJump, 15)
		           || IsJumpActionState(&chr->mJump, 17)) {
			target   = chr->mMoveTranslate + (Vec) { 0, 10, 0 };
			target.y = 10.f + ((XActJump*)&chr->mJump)->mArcPosition.y;
		} else if (IsJumpActionState(&chr->mJump, 10) || IsJumpActionState(&chr->mJump, 13) || IsJumpActionState(&chr->mJump, 16)) {
			CVec start = chr->mMoveTranslate;
			CVec arc   = ((XActJump*)&chr->mJump)->mArcPosition;
			f32 end    = GetAnimEndFrame();
			f32 t      = GetAnimFrame() / end;
			if (t > 1.0f)
				t = 1.0f;
			target = arc + (start - arc) * t + (Vec) { 0, 10, 0 };
		} else if (IsCopterActionState(&chr->mCopter, 7) || IsCopterActionState(&chr->mCopter, 8)
		           || IsCopterActionState(&chr->mCopter, 9)) {
			CVec joint = GetJointWorldPos("second_null");
			CVec start = chr->mMoveTranslate;
			start.x    = joint.x;
			start.z    = joint.z;
			target     = start + (Vec) { 0, 10, 0 };
		} else if (anim == 0xcea) {
			CVec root   = GetJointWorldPos("second_null") + (Vec) { 0, 10, 0 };
			CVec middle = GetJointWorldPos("l_mabuta_down") + (GetJointWorldPos("r_mabuta_down") - GetJointWorldPos("l_mabuta_down")) / 2.f;
			f32 end     = GetAnimEndFrame();
			f32 t       = GetAnimFrame() / end;
			if (t > 1.0f)
				t = 1.0f;
			target = root + (middle - root) * t;
		} else if (anim == 0x43 || anim == 0x10e) {
			CVec root   = GetJointWorldPos("second_null") + (Vec) { 0, 10, 0 };
			CVec middle = GetJointWorldPos("l_mabuta_down") + (GetJointWorldPos("r_mabuta_down") - GetJointWorldPos("l_mabuta_down")) / 2.f;
			f32 end     = GetAnimEndFrame();
			f32 t       = GetAnimFrame() / end;
			if (t > 1.0f)
				t = 1.0f;
			target = root + (middle - root) * t;
		} else if (anim == 0xed) {
			CVec middle = GetJointWorldPos("l_mabuta_down") + (GetJointWorldPos("r_mabuta_down") - GetJointWorldPos("l_mabuta_down")) / 2.f;
			target      = middle;
		} else if (anim == 0xee) {
			CVec root   = GetJointWorldPos("second_null") + (Vec) { 0, 10, 0 };
			CVec middle = GetJointWorldPos("l_mabuta_down") + (GetJointWorldPos("r_mabuta_down") - GetJointWorldPos("l_mabuta_down")) / 2.f;
			f32 end     = GetAnimEndFrame();
			f32 t       = GetAnimFrame() / end;
			if (t > 1.0f)
				t = 1.0f;
			target = middle + (root - middle) * t;
		} else {
			CVec joint = GetJointWorldPos("second_null");
			CVec start = chr->mMoveTranslate;
			start.x    = joint.x;
			start.y    = joint.y;
			start.z    = joint.z;
			target     = start + (Vec) { 0, 10, 0 };
		}
		CMotionControlCamera& camera = gGame.mMotionControlCamera;
		if (camera.IsAction(CAMACTION_AREA) || camera.IsAction(CAMACTION_MAP)) {
			f32 rate = GetAreaCameraFovRatio(&camera);
			if (rate > 1.f)
				rate = 1.f;
			if (rate < .8f)
				rate = .8f;
			f32 leadX = chr->mHeadOffset * (f32)sin(chr->mMoveRotate.y);
			f32 leadZ = chr->mHeadOffset * (f32)cos(chr->mMoveRotate.y);
			f32 moveX = (7.f * rate) * (chr->mMoveTranslate.x - chr->mLastPos.x);
			f32 moveZ = (7.f * rate) * (chr->mMoveTranslate.z - chr->mLastPos.z);
			target.x += leadX + moveX;
			target.z += leadZ + moveZ;
		}
		f32* coord = &target.x;
		f32* last  = chr->mCameraTarget;
		for (s32 i = 0; i < 3; ++i) {
			f32& old = last[i];
			if (__fpclassifyf(old) == FP_NAN) {
				old = coord[i];
				continue;
			}
			f32 margin = chr->mCameraTargetMargin;
			if (old > coord[i] + margin)
				old += (coord[i] + margin) - old;
			else if (old < coord[i] - margin)
				old += (coord[i] - margin) - old;
			coord[i] = old;
		}
		return target;
	}
	return (Vec) { 0, 0, 0 };
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
f32 GetPlayerCollisionRadius(WiiPlayerState* chr)
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

// NONMATCHING: retail keeps the angle in f30 and its sine in f29, the build swaps those roles;
// the origin->y store and the surrounding float schedule also differ
void SpawnPlayerDust(WiiPlayerState* player, CVec* origin)
{
	f32 angle  = DEG2RAD * (rand() % 360);
	f32 sine   = (f32)sin(angle);
	f32 vx     = sine * (rand() % 100) / 1000.0f;
	f32 cosine = (f32)cos(angle);
	f32 vz     = cosine * (rand() % 100) / 1000.0f;
	f32 vy     = (rand() % 100) / 800.0f;
	Vec position;
	position.x = origin->x + 16.0f * (vx * (rand() % 100) / 100.0f);
	position.z = origin->z + 16.0f * (vz * (rand() % 100) / 100.0f);
	position.y = origin->y;
	f32 width  = 1.0f + (rand() % 200) / 200.0f;
	Vec size;
	size.x = size.y = width;
	size.z          = 1.0f;
	Vec velocity    = { vx, vy, vz };
	CPtclOne* dust  = SpawnPoolParticle(&player->mDust.mPtcl, &position, &size, &velocity, 0xFFFFFFA0);
	if (dust != NULL) {
		if (player->mGroundType == 10) {
			dust->mAngle      = DEG2RAD * (rand() % 360);
			dust->mAngleSpeed = DEG2RAD * (10 - rand() % 20);
			dust->mColor      = 0xFFFFFFC8;
			dust->mTex        = &player->mTex[1];
		} else {
			dust->mAngle      = DEG2RAD * (rand() % 360);
			dust->mAngleSpeed = DEG2RAD * (20 - rand() % 40);
			dust->mColor      = 0xFFFFFFA0;
			dust->mTex        = &player->mTex[0];
		}
	}
}

// NONMATCHING: the Wii command switch, scene/suit stepping and ordered call set match;
// switch-arm layout and register allocation remain
s32 XObjChr::HandleEventCommand(s32 count, s32* args)
{
	WiiPlayerState* chr = (WiiPlayerState*)this;
	s32 result          = 0;
	if (count > 0) {
		switch (args[0]) {
		case 100:
			return chr->mPlug.mConnection;
		case 101: {
			XObj* obj = (gGame.mEvt.mData.mScript != NULL ? ResolveEventObjectId(&gGame.mEvt.mData, args[1]) : NULL);
			return obj->GetCategory() == 9 ? GetClearLetickerApproachSide(obj, chr->GetPos()) : 1;
		}
		case 102: {
			s32 connection       = args[2];
			XObj* target         = (gGame.mEvt.mData.mScript != NULL ? ResolveEventObjectId(&gGame.mEvt.mData, args[1]) : NULL);
			chr->mLift.mState    = 7;
			chr->mLiftPlugTarget = target;
			SetLiftPlugConnection((XActLift*)&chr->mLift, connection, FALSE);
			result = -1;
			break;
		}
		case 103: {
			XObj* target = (gGame.mEvt.mData.mScript != NULL ? ResolveEventObjectId(&gGame.mEvt.mData, args[1]) : NULL);
			SetPos(*target->GetPos());
			SetRotationY(PI + target->mMoveRotate.y);
			s32 connection       = args[2];
			chr->mLift.mState    = 7;
			chr->mLiftPlugTarget = target;
			SetLiftPlugConnection((XActLift*)&chr->mLift, connection, FALSE);
			chr->_0AF8 = 1;
			do {
				f32 end   = GetAnimEndFrame();
				f32 frame = GetAnimFrame();
				UpdateLiftActionAnimation((XActLift*)&chr->mLift, GetAnimId(), this, 2, frame, end);
				XObj::UpdateObject(2);
				UpdateHeldObject(&chr->mLift, 2);
				chr->mPlug.UpdateObject(2);
				f32 sceneFrame = 1.0f + GetAnimFrame();
				SetModelFrame(this, sceneFrame);
				WiiSuitObject* suit = &chr->mSuits.mObjects[chr->mSuits.mCurrent];
				if (IsObjectActive(suit)) {
					SetModelFrame(suit, sceneFrame);
				}
				gGame.mGs.mScene.mHead->vf034(gGame.mGs.mScene.mHead->mPending, chr->mObject.m_pScnMdl);
				gGame.mGs.mScene.mHead->vf034(gGame.mGs.mScene.mHead->mPending, suit->mObject.m_pScnMdl);
				fn_802AEDA0(&gGame.mGs.mScene.mHead, 0);
				WiiGsSceneObj* scene = gGame.mGs.mScene.mHead;
				while (scene->mPending != 0) {
					scene->PopPending();
				}
			} while (chr->mLift.mState == 7);
			chr->_0AF8 = 0;
			result     = -1;
			break;
		}
		case 104: {
			XObj* target         = (gGame.mEvt.mData.mScript != NULL ? ResolveEventObjectId(&gGame.mEvt.mData, args[1]) : NULL);
			chr->mLift.mState    = 12;
			chr->mLiftPlugTarget = target;
			result               = -1;
			break;
		}
		case 105:
			if (chr->mPlug.mConnection) {
				PlayWiiSoundEffect(&gGame.mSound, 0xA4, 1.0f, 0.0f);
				chr->mPlug.mConnection = 0;
			}
			break;
		case 106: {
			XObj* target         = (gGame.mEvt.mData.mScript != NULL ? ResolveEventObjectId(&gGame.mEvt.mData, args[1]) : NULL);
			chr->mLift.mState    = 8;
			chr->mLiftPlugTarget = target;
			result               = -1;
			break;
		}
		case 107:
			(gGame.mEvt.mData.mScript != NULL ? ResolveEventObjectId(&gGame.mEvt.mData, args[1]) : NULL);
			chr->mPlug.mConnection = 0;
			result                 = -1;
			break;
		case 108: {
			XObj* target         = (gGame.mEvt.mData.mScript != NULL ? ResolveEventObjectId(&gGame.mEvt.mData, args[1]) : NULL);
			chr->mLift.mState    = 9;
			chr->mLiftPlugTarget = target;
			result               = -1;
			break;
		}
		case 109:
			(gGame.mEvt.mData.mScript != NULL ? ResolveEventObjectId(&gGame.mEvt.mData, args[1]) : NULL);
			chr->mPlug.mConnection = 0;
			result                 = -1;
			break;
		case 110:
			chr->mPlug.mConnection   = 0;
			chr->mPlug.mEndRotationZ = 0.0f;
			result                   = -1;
			break;
		case 111:
			SetLiftPlugConnection((XActLift*)&chr->mLift, args[1], TRUE);
			break;
		case 120:
			// shows or hides his plug
			if (args[1])
				gGame.SetStatusFlags(0x0040000000000000ULL);
			else
				gGame.ClearStatusFlags(0x0040000000000000ULL);
			break;
		case 200:
			switch (args[1]) {
			case 205:
				chr->mHeadAimRecord.mSnap[0] = chr->mHeadAimRecord.mSnap[1] = 1;
				chr->mHeadAimRecord.m_0C                                    = NULL;
				chr->mHeadAimRecord.mTarget.x                               = NAN;
				chr->mHeadAimRecord.m_20                                    = NULL;
				chr->mHeadAimRecord.m_24                                    = 0.0f;
				chr->mHeadAimRecord.m_28                                    = DEG2RAD * args[2];
				break;
			case 207:
				chr->mHeadAimRecord.mSnap[0] = chr->mHeadAimRecord.mSnap[1] = 1;
				chr->mHeadAimRecord.m_0C                                    = NULL;
				chr->mHeadAimRecord.mTarget.x                               = args[2] / 100.0f;
				chr->mHeadAimRecord.mTarget.y                               = args[3] / 100.0f;
				chr->mHeadAimRecord.mTarget.z                               = args[4] / 100.0f;
				chr->mHeadAimRecord.m_20                                    = NULL;
				chr->mHeadAimRecord.m_24                                    = NAN;
				chr->mHeadAimRecord.m_28                                    = NAN;
				break;
			case 208: {
				XObj* target                 = (gGame.mEvt.mData.mScript != NULL ? ResolveEventObjectId(&gGame.mEvt.mData, args[2]) : NULL);
				chr->mHeadAimRecord.mSnap[0] = chr->mHeadAimRecord.mSnap[1] = 1;
				chr->mHeadAimRecord.m_0C                                    = target;
				chr->mHeadAimRecord.mTarget.x                               = NAN;
				chr->mHeadAimRecord.m_20                                    = NULL;
				chr->mHeadAimRecord.m_24                                    = NAN;
				chr->mHeadAimRecord.m_28                                    = NAN;
				break;
			}
			case 220: {
				XObj* target                 = (gGame.mEvt.mData.mScript != NULL ? ResolveEventObjectId(&gGame.mEvt.mData, args[2]) : NULL);
				chr->mHeadAimRecord.mSnap[0] = chr->mHeadAimRecord.mSnap[1] = 1;
				chr->mHeadAimRecord.m_0C                                    = target;
				chr->mHeadAimRecord.mTarget.x                               = NAN;
				chr->mHeadAimRecord.m_20                                    = NULL;
				chr->mHeadAimRecord.m_24                                    = NAN;
				chr->mHeadAimRecord.m_28                                    = NAN;
				break;
			}
			case 225:
				chr->mHeadAimRecord.mSnap[0] = chr->mHeadAimRecord.mSnap[1] = 1;
				chr->mHeadAimRecord.m_0C                                    = NULL;
				chr->mHeadAimRecord.mTarget.x                               = NAN;
				chr->mHeadAimRecord.m_20                                    = &gGame.mMotionControlCamera;
				chr->mHeadAimRecord.m_24                                    = NAN;
				chr->mHeadAimRecord.m_28                                    = NAN;
				break;
			}
			return 0;
		case 201:
			chr->mHeadAimRecord.mSnap[0] = chr->mHeadAimRecord.mSnap[1] = 2;
			chr->mHeadAimRecord.m_0C                                    = NULL;
			chr->mHeadAimRecord.mTarget.x                               = NAN;
			chr->mHeadAimRecord.m_20                                    = NULL;
			chr->mHeadAimRecord.m_24                                    = NAN;
			chr->mHeadAimRecord.m_28                                    = NAN;
			chr->mHeadAimRecord.m_2C                                    = 0.0f;
			chr->mHeadAimRecord.m_30                                    = 0.0f;
			chr->mHeadAimRecord.m_4C.Set(0.0f, 0.0f, 0.0f);
			return 0;
		case 202:
			return chr->mHeadAimRecord.mSnap[0] <= 0 && chr->mHeadAimRecord.mSnap[1] <= 0;
		case 300:
			chr->mRope.mState = 4;
			SetPlayerActionTarget(&chr->mRope,
			                      (gGame.mEvt.mData.mScript != NULL ? ResolveEventObjectId(&gGame.mEvt.mData, args[1]) : NULL));
			result = -1;
			break;
		case 301:
			chr->mLadder.mState = 4;
			SetPlayerActionTarget(&chr->mLadder,
			                      (gGame.mEvt.mData.mScript != NULL ? ResolveEventObjectId(&gGame.mEvt.mData, args[1]) : NULL));
			result = -1;
			break;
		case 302:
			chr->mJump.mState = 14;
			SetPlayerActionTarget(&chr->mJump,
			                      (gGame.mEvt.mData.mScript != NULL ? ResolveEventObjectId(&gGame.mEvt.mData, args[1]) : NULL));
			result = -1;
			break;
		case 303:
			if (args[1])
				chr->mRadar.mState = 1;
			else
				chr->mRadar.mState = 2;
			result = -1;
			break;
		case 400:
			return chr->mAttachmentRequest < 0;
		case 450:
			if (args[1] >= 0 && args[1] < 3)
				chr->mSuits.mSpecial = args[1];
			break;
		case 451:
			return chr->mSuits.mSpecial;
		case 503: {
			s32 item = args[1];
			if (item == 16) {
				chr->mAttachmentRequest = 4;
				chr->mAttachmentStep    = 0;
				chr->mScriptAttach      = 1;
			} else if (item == 21) {
				chr->mAttachmentRequest = 5;
				chr->mAttachmentStep    = 0;
				chr->mScriptAttach      = 1;
			} else if (item == 22) {
				chr->mAttachmentRequest = 6;
				chr->mAttachmentStep    = 0;
				chr->mScriptAttach      = 1;
			} else {
				chr->mLift.mState                        = 25;
				((XActLift*)&chr->mLift)->mPendingItemId = item;
			}
			result = -1;
			break;
		}
		case 513:
			return ((u32)(chr->mAttachmentRequest - 4) > 3 && (chr->mLift.mState == 0 || chr->mLift.mState == 4));
		case 500:
			// takes out an item he owns: the toothbrush (16), spoon (21), mug (22) and squirter (56) go on as attachments
			// 4 to 7, anything else he lifts
			if (FindOwnedInventoryItemIndex(chr->mParams.mInventory.mItems, args[1]) >= 0) {
				s32 item = args[1];
				if (item == 16) {
					chr->mAttachmentRequest = 4;
					chr->mAttachmentStep    = 0;
				} else if (item == 21) {
					chr->mAttachmentRequest = 5;
					chr->mAttachmentStep    = 0;
				} else if (item == 22) {
					chr->mAttachmentRequest = 6;
					chr->mAttachmentStep    = 0;
				} else if (item == 56) {
					chr->mAttachmentRequest = 7;
					chr->mAttachmentStep    = 0;
				} else {
					chr->mLift.mState                        = 21;
					((XActLift*)&chr->mLift)->mPendingItemId = item;
				}
			}
			result = -1;
			break;
		case 510:
			return ((u32)(chr->mAttachmentRequest - 4) > 3 && (chr->mLift.mState == 0 || chr->mLift.mState == 4));
		case 501:
			if (chr->mLift.mTarget) {
				if (chr->GetAttachment() == 4 || chr->GetAttachment() == 5 || chr->GetAttachment() == 6 || chr->GetAttachment() == 7
				    || chr->GetAttachment() == 8) {
					chr->mAttachmentRequest = 0;
					chr->mAttachmentStep    = 0;
				} else if (chr->mLift.mTarget->GetCategory() == 8)
					chr->mLift.mState = 20;
			}
			result = -1;
			break;
		case 514:
			return ((u32)(chr->mAttachmentRequest - 4) > 3 && (chr->mLift.mState == 0 || chr->mLift.mState == 4));
		case 502:
			if (chr->mLift.mTarget && chr->mLift.mTarget->GetCategory() == 8 && chr->mLift.mTargetState != 15)
				chr->mLift.mState = 24;
			result = -1;
			break;
		case 550: {
			Inventory* inventory = &gGame.GetPlayer()->mParams.mInventory;
			inventory->mCompact  = TRUE;
			CompactInventory(inventory);
			break;
		}
		case 551:
			gGame.GetPlayer()->mParams.mInventory.mCompact = FALSE;
			break;
		case 600:
			fn_801105C4((XObjPlug*)&chr->mPlug, args[1]);
			result = -1;
			break;
		case 601:
			fn_801107AC((XObjPlug*)&chr->mPlug);
			result = -1;
			break;
		case 700:
			chr->mOnGround &= ~1;
			break;
		case 701:
			chr->mOnGround |= 1;
			break;
		case 702:
			return (chr->mOnGround & 1) ^ 1;
		case 800:
			chr->mBatteryDrains = 1;
			break;
		case 801:
			chr->mBatteryDrains = 0;
			break;
		case 802:
			return chr->mBatteryDrains;
		case 850:
			chr->mParams._1C[0] = 1;
			break;
		case 851:
			chr->mParams._1C[0] = 0;
			break;
		case 852:
			return (s32)chr->mParams._1C[0] > 0;
		case 900:
			chr->mSquirterContent = args[1];
			chr->mItemTintWord    = ((u32)args[2] << 24) | ((u32)args[3] << 16) | ((u32)args[4] << 8) | 255;
			break;
		case 901:
			chr->mCameraTargetMarginSetting = args[1] / 100.f;
			break;
		case 902:
			XActPopper* popper = (XActPopper*)&chr->mPopper;
			for (s32 i = 0; i < 10; ++i)
				ResetPopperShot(&popper->mShots.mShot[i]);
			break;
		case -1:
			ResetPlayerAttachment(chr, 17);
			break;
		case -2: {
			CVec position = GetJointWorldPos("second_null");
			SetPos(position);
			if (IsObjectActive(&chr->mSuits.mObjects[chr->mSuits.mCurrent])) {
				chr->mSuits.mObjects[chr->mSuits.mCurrent].SetPos(GetJointWorldPos("second_null"));
			}
			chr->_969C = 1;
			break;
		}
		}
	}
	return result;
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
// NONMATCHING: only register allocation and literal-pool labels still differ
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
	type &= 0xFF;
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

// NONMATCHING: effect calls, spiral geometry and color ramps are present; float
// register allocation and the compiler's unrolled loop schedule still differ
void XObjChr::DrawObjectEffects()
{
	WiiPlayerState* chr = (WiiPlayerState*)this;
	if (gGame.GetStatusFlags(0x0040000000000000ULL))
		chr->mPlug.DrawObjectEffects();
	((XObj*)chr->mCopterObj)->DrawObjectEffects();
	((XObj*)chr->mPopperObj)->DrawObjectEffects();
	((XObj*)chr->mRadarObj)->DrawObjectEffects();
	((XObj*)chr->mOXObj)->DrawObjectEffects();
	if (chr->mBatteryWarningFade > 0) {
		f32 radius = 1.5f * chr->mMoveScale.x;
		if (IsCarryingMug(&chr->mLift) && !(chr->GetAnimId() == 11 && chr->GetAnimFrame() < 45.0f)
		    && !(chr->GetAnimId() == 10 && chr->GetAnimFrame() > 22.0f)) {
			CVec jointScale;
			chr->GetJointScale(chr->FindResNode("carry_obj_nll"), &jointScale);
			radius += 2.5f * jointScale.x;
		}
		radius *= 1.2f;
		CVec from = chr->GetJointPos(chr->FindResNode("koshi_model"));
		CVec to   = chr->GetJointPos(chr->FindResNode("head"));
		if (chr->mLift.mState && IsCarryingMug(&chr->mLift)) {
			from.x = chr->mLift.mTarget->mMoveTranslate.x;
			from.z = chr->mLift.mTarget->mMoveTranslate.z;
			Vec up = { 0, 1, 0 };
			to     = from + up;
		}
		f32 yaw = atan2(to.x - from.x, to.z - from.z);
		f32 dz = to.z - from.z, dx = to.x - from.x, dy = to.y - from.y;
		f32 pitch = atan2(FSqrt(dx * dx + dz * dz), FSqrt(dy * dy));
		pitch     = dy < 0 ? HALF_PI - pitch : pitch - HALF_PI;
		Mtx trans, scale, rotY, rotX, model;
		PSMTXTrans(trans, from.x, from.y, from.z);
		PSMTXScale(scale, 1, 1, 1);
		PSMTXRotRad(rotY, 'y', yaw);
		PSMTXRotRad(rotX, 'x', pitch);
		PSMTXConcat(trans, rotY, model);
		PSMTXConcat(model, rotX, model);
		PSMTXConcat(model, scale, model);
		Vec vertices[256];
		u32 colors[256];
		f32 z = -3, phase = chr->mBatteryWarningAngle;
		u16 n = 0;
		for (int turn = 0; turn < 8; ++turn)
			for (int step = 0; step < 16; ++step) {
				f32 angle     = (TWO_PI / 16) * step;
				vertices[n].x = (2.f * (1.f - chr->mBatteryWarningFade) + radius) * std::sinf(angle);
				vertices[n].y = (2.f * (1.f - chr->mBatteryWarningFade) + radius) * std::cosf(angle);
				vertices[n].z = z;
				f32 wave      = phase;
				while (wave < 0.0f)
					wave += TWO_PI;
				while (wave >= TWO_PI)
					wave -= TWO_PI;
				if (wave > PI)
					wave = 0;
				vertices[n + 1].x = vertices[n].x;
				vertices[n + 1].y = vertices[n].y;
				vertices[n + 1].z = vertices[n].z - 1.5f * std::sinf(wave);
				z += .08f;
				phase += DEG2RAD * 30;
				n += 2;
			}
		PSMTXMultVecArray(model, vertices, vertices, n);
		n = 0;
		for (int turn = 0; turn < 4; ++turn)
			for (int step = 0; step < 16; ++step) {
				f32 ratio     = (1.f / 128) * n;
				colors[n]     = 0xff000000 | (u32)(200.f * ratio * chr->mBatteryWarningFade);
				colors[n + 1] = 0xff000000;
				n += 2;
			}
		for (int turn = 0; turn < 4; ++turn)
			for (int step = 0; step < 16; ++step) {
				f32 ratio     = 1.f - (1.f / 128) * (turn * 32 + step);
				colors[n]     = 0xff000000 | (u32)(200.f * ratio * chr->mBatteryWarningFade);
				colors[n + 1] = 0xff000000;
				n += 2;
			}
		gGame.mGs.mCurRenderState->SetCullMode((GXCullMode)0);
		gGame.mGs.mCurRenderState->SetBlendMode((GXBlendMode)1, (GXBlendFactor)4, (GXBlendFactor)5, (GXLogicOp)15);
		DrawGsColoredTrianglePrimitive(&gGame.mGs, GX_TRIANGLESTRIP, gGame.mGs.mCamera->mViewMtx, vertices, colors, n);
		n = 0;
		for (int turn = 0; turn < 4; ++turn)
			for (int step = 0; step < 16; ++step) {
				f32 ratio     = (1.f / 128) * n;
				colors[n]     = 0xffffff00 | (u32)(255.f * ratio * chr->mBatteryWarningFade);
				colors[n + 1] = 0xffffff00;
				n += 2;
			}
		for (int turn = 0; turn < 4; ++turn)
			for (int step = 0; step < 16; ++step) {
				f32 ratio     = 1.f - (1.f / 128) * (turn * 32 + step);
				colors[n]     = 0xffffff00 | (u32)(255.f * ratio * chr->mBatteryWarningFade);
				colors[n + 1] = 0xffffff00;
				n += 2;
			}
		gGame.mGs.mCurRenderState->SetBlendMode((GXBlendMode)1, (GXBlendFactor)2, (GXBlendFactor)4, (GXLogicOp)15);
		DrawGsColoredTrianglePrimitive(&gGame.mGs, GX_TRIANGLESTRIP, gGame.mGs.mCamera->mViewMtx, vertices, colors, n);
		gGame.mGs.mCurRenderState->SetBlendMode((GXBlendMode)1, (GXBlendFactor)4, (GXBlendFactor)5, (GXLogicOp)15);
		gGame.mGs.mCurRenderState->SetCullMode((GXCullMode)2);
	}
	DrawTumbleRing(&chr->mTumble.mFade, (Vec) { chr->mMoveTranslate.x, chr->mFloorHit.mHeight, chr->mMoveTranslate.z }, chr->mMoveRotate.y);
	DrawRadarTargetBeam((XActRadar*)&chr->mRadar);
	DrawPopperShotParticles((XActPopper*)&chr->mPopper);
	DrawParticlePool(&chr->mDust.mPtcl, &gGame.mGs);
	DrawSpiralTrail(&lbl_8045BBD0, GetPos());
}

// NONMATCHING: the floor, lift and player collision draws are present; matrix
// temporary order and the remaining render-state schedule differ
void XObjChr::DrawCollisionShapes()
{
	WiiPlayerState* chr = (WiiPlayerState*)this;
	SetGsChannelEnabled(&gGame.mGs, 0, FALSE);
	gGame.mGs.mCurRenderState->SetBlendMode((GXBlendMode)1, (GXBlendFactor)4, (GXBlendFactor)5, (GXLogicOp)15);
	gGame.mGs.mCurRenderState->SetZMode(TRUE, (GXCompare)7, FALSE);
	PrimVertex triangle[3];
	triangle[0].pos   = chr->mFloorTriangle[0];
	triangle[0].color = 0xFF000080;
	triangle[1].pos   = chr->mFloorTriangle[1];
	triangle[1].color = 0x00FF0080;
	triangle[2].pos   = chr->mFloorTriangle[2];
	triangle[2].color = 0x0000FF80;
	DrawGsTriangleVertices(&gGame.mGs, GX_TRIANGLESTRIP, gGame.mGs.mCamera->mViewMtx, triangle, 3);
	SetGsChannelEnabled(&gGame.mGs, 0, TRUE);
	gGame.mGs.mCurRenderState->SetBlendMode((GXBlendMode)1, (GXBlendFactor)4, (GXBlendFactor)5, (GXLogicOp)15);
	gGame.mGs.mCurRenderState->SetZMode(TRUE, (GXCompare)3, TRUE);
	if (chr->mLift.mState != 0) {
		if (IsCarryingToothbrush(&chr->mLift)) {
			CVec pos = chr->mLift.mTarget->GetJointPos("null");
			DrawGsCylinder(&gGame.mGs, 0x00FFFF80, 16, pos.x, pos.y, pos.z, HALF_PI, 0.0f, 0.0f, 3.0f, 3.0f, 1.0f);
		}
		if (IsCarryingSpoon(&chr->mLift)) {
			f32 yaw = chr->mMoveRotate.y;
			Vec pos = *GetPos();
			pos.x += 3.0f * sin(yaw);
			pos.z += 3.0f * cos(yaw);
			DrawGsCylinder(&gGame.mGs, 0x00FFFF80, 16, pos.x, pos.y, pos.z, HALF_PI, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
		}
		if (IsCarryingSquirter(&chr->mLift)) {
			f32 yaw = chr->mMoveRotate.y;
			Vec pos = *GetPos();
			pos.x += 2.0f * sin(yaw);
			pos.z += 2.0f * cos(yaw);
			DrawGsCylinder(&gGame.mGs, 0x00FFFF80, 16, pos.x, pos.y, pos.z, HALF_PI, 0.0f, 0.0f, 2.0f, 2.0f, 1.0f);
			pos.x += 2.0f * sin(yaw);
			pos.z += 2.0f * cos(yaw);
			pos.y += 7.0f;
			DrawGsCylinder(&gGame.mGs, 0x00FFFF80, 16, pos.x, pos.y, pos.z, HALF_PI, 0.0f, 0.0f, 2.0f, 2.0f, 3.0f);
			if (IsCarryingSquirter(&chr->mLift)) {
				s32 anim = chr->GetAnimId();
				if (anim == 0x8C2 || anim == 0x8C3 || anim == 0x8C4 || anim == 0x8C5) {
					XObj* object = chr->mLift.mTarget;
					CVec origin  = object->GetJointPos(object->FindResNode("atari"));
					CVec tip     = object->GetJointPos("saki");
					CVec delta   = tip - origin;
					if (PSVECMag(&delta) > 0.1f) {
						CVec scale = object->GetJointScale(object->FindResNode("atari"));
						DrawGsCylinder(&gGame.mGs, chr->mItemTintWord, 16, origin.x, origin.y, origin.z, HALF_PI, 0.0f, 0.0f, scale.x,
						               scale.z, 0.5f * scale.y);
					}
				}
			}
		}
	}
	if (chr->mStatus.mFlags & 0x0200000000000000ULL) {
		SetGsChannelEnabled(&gGame.mGs, 0, TRUE);
		gGame.mGs.mCurRenderState->SetBlendMode((GXBlendMode)1, (GXBlendFactor)4, (GXBlendFactor)5, (GXLogicOp)15);
		gGame.mGs.mCurRenderState->SetZMode(TRUE, (GXCompare)3, TRUE);
		CVec pos(chr->mMoveTranslate);
		Vec offset   = chr->mMoveScale * GetFloorCenterOffset(&chr->mFloors, 0);
		Vec extent   = chr->mMoveScale * GetFloorExtent(&chr->mFloors, 0);
		f32 floorYaw = DEG2RAD * chr->mFloors->mYawDegrees;
		f32 yaw      = chr->mMoveRotate.y + floorYaw;
		pos.x        = chr->mMoveTranslate.x + 0.5f * extent.x * sin(chr->mMoveRotate.y);
		pos.z        = chr->mMoveTranslate.z + 0.5f * extent.z * cos(chr->mMoveRotate.y);
		Mtx translate, shift, turn, tilt, scale, model, view;
		PSMTXTrans(translate, pos.x, pos.y, pos.z);
		PSMTXTrans(shift, offset.x, offset.y, offset.z);
		PSMTXScale(scale, 0.9f * extent.x, extent.y, 0.9f * extent.z);
		PSMTXRotRad(turn, 'y', yaw);
		PSMTXRotRad(tilt, 'x', HALF_PI);
		PSMTXConcat(translate, turn, model);
		PSMTXConcat(model, shift, model);
		PSMTXConcat(model, scale, model);
		PSMTXConcat(model, tilt, model);
		PSMTXConcat(gGame.mGs.mCamera->mViewMtx, model, view);
		DrawGsCylinderWithMatrix(&gGame.mGs, view, 0x9696FF60, 16);
		XObj::DrawCollisionShapes();
		if (gGame.GetStatusFlags(0x0040000000000000ULL))
			chr->mPlug.DrawCollisionShapes();
		ClearActiveObjectFloors(&chr->mPlug);
	}
}

// NONMATCHING: the gauge, look/radar/popper and point-particle draws are present;
// render-state scheduling and gauge float register lifetimes still differ
static inline void DrawPlayerGauge(f32 x, const f32& y, f32 width, f32 fraction, u32 color)
{
	CGs* gs = &gGame.mGs;
	gs->mCurRenderState->SetBlendMode((GXBlendMode)1, (GXBlendFactor)4, (GXBlendFactor)5, (GXLogicOp)15);
	gs->mCurRenderState->SetZMode(TRUE, (GXCompare)7, TRUE);
	gs->mCurRenderState->SetLineWidth(30, (GXTexOffset)5);
	DrawGsCircle(gs, 16, color, color, x - 8.0f, y, 0.0f, 16.0f, 16.0f, 0.0f, 0.0f, 0.0f);
	f32 filled = width * fraction;
	DrawGsLineSegment(gs, NULL, x - 2.0f, y, 0.0f, color, x + filled, y, 0.0f, color);
	DrawGsLineSegment(gs, NULL, (x - 2.0f) + filled, y, 0.0f, 0x64, x + width, y, 0.0f, 0x64);
	gs->mCurRenderState->SetZMode(TRUE, (GXCompare)1, FALSE);
	gs->mCurRenderState->SetLineWidth(42, (GXTexOffset)5);
	DrawGsCircle(gs, 16, 0x96, 0x96, x - 8.0f, y, 0.0f, 18.4f, 18.4f, 0.0f, 0.0f, 0.0f);
	DrawGsLineSegment(gs, NULL, x - 2.0f, y, 0.0f, 0x96, 2.0f + (x + width), y, 0.0f, 0x96);
	gs->mCurRenderState->SetBlendMode((GXBlendMode)1, (GXBlendFactor)4, (GXBlendFactor)5, (GXLogicOp)15);
	gs->mCurRenderState->SetZMode(TRUE, (GXCompare)3, FALSE);
	gs->mCurRenderState->SetLineWidth(12, (GXTexOffset)5);
}

void XObjChr::DrawScreenOverlay()
{
	WiiPlayerState* chr = (WiiPlayerState*)this;
	XObj::DrawScreenOverlay();
	DrawPopperShotRings((XActPopper*)&chr->mPopper);
	DrawPlayerRadarEffects((XActRadar*)&chr->mRadar);
	if (!(gGame.GetStatusFlags(0x100)))
		DrawPlayerLookMarker(&chr->mLook);

	CMotionControlCamera* camera = &gGame.mMotionControlCamera;
	if (!camera->IsAction(CAMACTION_SCROLL2) && chr->mGaugeTime > 0.0f) {
		Vec offset = { 0.0f, 0.0f, 0.0f };
		f32 x, y;
		ProjectOffsetToScreen(&offset, &x, &y, NULL, NULL);
		f32 left     = x - 31.0f;
		f32 duration = (f32)(s32)GetInteractionParameter(0) / 100.0f;
		DrawPlayerGauge(left, y, 62.0f, chr->mGaugeTime / (gRefreshRate * duration), 0x00FF0064);
	}
	if (!camera->IsAction(CAMACTION_SCROLL2) && (GetAnimId() == 0x7A || GetAnimId() == 0xCA)) {
		Vec offset = { 0.0f, 0.0f, 0.0f };
		f32 x, y;
		ProjectOffsetToScreen(&offset, &x, &y, NULL, NULL);
		f32 left     = x - 36.0f;
		f32 frame    = GetAnimFrame();
		f32 fraction = frame / GetAnimEndFrame();
		u32 color;
		if (fraction <= 0.6f)
			color = 0xFFC80064;
		else
			color = 0xFF000064;
		DrawPlayerGauge(left, y, 72.0f, fraction, color);
	}
	s32 row = 0;
	for (s32 i = 0; i < 10; ++i) {
		if (gPointParticleSlots[i] != NULL) {
			DrawPointParticleDisplay(gPointParticleSlots[i], (f32)row);
			if (gPointParticleSlots[i]->mType != 1)
				++row;
		}
	}
}

// _1A0, the dst-alpha flags go to every model he carries: the plug and attachments when
// they have one, all nine suits and their extra parts
void XObjChr::ApplyMaterialPresets(const u32* flags)
{
	XObj::ApplyMaterialPresets(flags);
	WiiPlayerState* chr = (WiiPlayerState*)this;
	if (chr->mPlug.HasModel()) {
		chr->mPlug.ApplyMaterialPresets(flags);
	}
	if (((XObj*)chr->mCopterObj)->HasModel()) {
		((XObj*)chr->mCopterObj)->ApplyMaterialPresets(flags);
	}
	if (((XObj*)chr->mPopperObj)->HasModel()) {
		((XObj*)chr->mPopperObj)->ApplyMaterialPresets(flags);
	}
	if (((XObj*)chr->mRadarObj)->HasModel()) {
		((XObj*)chr->mRadarObj)->ApplyMaterialPresets(flags);
	}
	if (((XObj*)chr->mOXObj)->HasModel()) {
		((XObj*)chr->mOXObj)->ApplyMaterialPresets(flags);
	}
	for (s32 i = 0; i < 9; i++) {
		if (chr->mSuits.mObjects[i].HasModel()) {
			chr->mSuits.mObjects[i].ApplyMaterialPresets(flags);
		}
		if (chr->mSuitParts[i].HasModel()) {
			chr->mSuitParts[i].ApplyMaterialPresets(flags);
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

// NONMATCHING: retail keeps a second table cursor in r22 and uses a 0x140-byte frame; the build uses one cursor,
// a 0x130-byte frame and different saved registers, with one redundant sign extension in the item-sound path
PlayerFrameParticle ProcessPlayerFrameEvents(XObj* object, FrameEventTime* frames, const FrameEvent* table)
{
	PlayerFrameParticle particle = { -1 };
	s32 anim                     = object->GetAnimId();
	if (anim == 0xCFA || anim == 0xCFD)
		anim = 0x12;
	FrameEventTime time;
	if (frames != NULL) {
		time.mFrame     = frames->mFrame;
		time.mLastFrame = frames->mLastFrame;
	} else {
		time.mFrame     = object->GetAnimFrame();
		time.mLastFrame = time.mFrame - 1.0f;
	}
	for (const FrameEvent* event = table; event->mAnim >= 0; ++event) {
		s32 result = 0;
		if (anim == event->mAnim && event->mWhenAnimMatches != NULL)
			result = event->mWhenAnimMatches(object, &time, event);
		else if (anim != event->mAnim && event->mWhenAnimDiffers != NULL)
			result = event->mWhenAnimDiffers(object, &time, event);
		if (result > 0) {
			WiiPlayerState* player = gGame.GetPlayer();
			if (event->mId == 0x74 && anim == 7 && player->mLift.mTarget != NULL) {
				XObj* held = player->mLift.mTarget;
				if ((s16)held->GetPool() == 8) {
					s8 material = GetItemSoundIndex(held->GetItem(), 0);
					if (material >= 0)
						PlayWiiSoundEffect(&gGame.mSound, GetMaterialSoundId(material), event->mVolume / 255.0f, 0.0f);
				} else {
					PlayWiiSoundEffect(&gGame.mSound, GetMaterialSoundId(0), event->mVolume / 255.0f, 0.0f);
				}
			} else {
				PlayWiiSoundEffect(&gGame.mSound, event->mId, event->mVolume / 255.0f, 0.0f);
			}
			if (event->mId == 0x72) {
				particle.mCode      = 0x18;
				particle.mTranslate = player->GetJointWorldPos("neck") + (Vec) { 0.0f, 2.0f, 0.0f };
				particle.mScale     = *player->GetScale();
				particle.mRotate    = player->mMoveRotate;
			} else if (event->mId == 0x73) {
				particle.mCode      = 0x19;
				particle.mTranslate = player->GetJointWorldPos("neck") + (Vec) { 0.0f, 2.0f, 0.0f };
				particle.mScale     = *player->GetScale();
				particle.mRotate    = player->mMoveRotate;
			} else if (event->mId == 0xA5 && (anim == 0x9F || anim == 0x15C)) {
				const Vec& tint = player->mAnmQueue.mColorStep[0].mValue;
				u32 color       = ((u32)tint.x << 24) | ((u32)tint.y << 16) | ((u32)tint.z << 8);
				if (color < 0xFFFFFF00) {
					particle.mCode      = 0x55;
					particle.mTranslate = player->GetJointWorldPos("carry_obj_nll");
					particle.mTranslate.x += 2.4f * (f32)sin(player->mMoveRotate.y);
					particle.mTranslate.z += 2.4f * (f32)cos(player->mMoveRotate.y);
					particle.mTranslate.y += 0.5f;
					particle.mScale  = *player->GetScale();
					particle.mRotate = player->mMoveRotate;
				}
			}
		} else if (result < 0) {
			StopWiiSfx(&gGame.mSound, event->mId, 0.0f);
		}
	}
	return particle;
}

// drops the stale object indexes first, then draws himself or the current suit and the
// attachments; the popper turns with the camera while the menu flag is up
// NONMATCHING (92 %): retail's register and schedule residue over the five index cleanups
void XObjChr::DrawWithView(Mtx view)
{
	WiiPlayerState* chr = (WiiPlayerState*)this;
	if (chr->mHitObjectIndex >= 0 && !IsObjectActive(gGame.mXObjs.mObjects[chr->mHitObjectIndex])) {
		chr->mHitObjectIndex = -1;
	}
	if (chr->mFloorContactObjects[0] >= 0 && !IsObjectActive(gGame.mXObjs.mObjects[chr->mFloorContactObjects[0]])) {
		chr->mFloorContactObjects[0] = chr->mFloorContactObjects[1] = -1;
	}
	if (chr->mFloorContactObjects[2] >= 0 && !IsObjectActive(gGame.mXObjs.mObjects[chr->mFloorContactObjects[2]])) {
		chr->mFloorContactObjects[2] = -1;
	}
	if (chr->mActionObjectIndex >= 0 && !IsObjectActive(gGame.mXObjs.mObjects[chr->mActionObjectIndex])) {
		chr->mActionObjectIndex = -1;
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
		if (chr->m_9C8 == 0 && (chr->mSuits.mCurrent == 3 || chr->mSuits.mCurrent == 6)) {
			chr->mSuitParts[chr->mSuits.mCurrent].DrawWithView(view);
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
		while (head->mPending != 0)
			head->PopPending();
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

// the marker over what chibi robo looks at, ported from the GameCube's
// DrawPlayerLookMarker; on the Wii _30 dims the marker to half and drops its
// glow, and it picks the second look texture, and the color fold is spelled
// out at each corner instead of once per quad
// NONMATCHING: literal-pool slots only (@ labels vs the unit's shared pool);
// the instruction stream is identical
/**
 * @note Address: 0x800E8AA0
 * @note Size: 0x864
 */
extern "C" void DrawPlayerLookMarker(XObjChrLook* look)
{
	if (!gGame.mMotionControlCamera.IsAction(CAMACTION_SCROLL2) && look->mFade > 0.0f) {
		CTexObj tex;
		f32 scale = look->mFade + 0.5f * (f32)sin(PI * look->mFade);
		f32 alpha = look->_30 ? look->mFade : 0.5f * look->mFade;
		if (look->_30) {
			gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[111], 0);
			tex = gGame.mTpl.mTex;
		} else {
			gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[112], 0);
			tex = gGame.mTpl.mTex;
		}
		f32 width  = scale * GXGetTexObjWidth(&tex);
		f32 height = scale * GXGetTexObjHeight(&tex);
		Vec pos;
		look->mObject->ProjectOffsetToScreen(&look->mOffset, &pos.x, &pos.y, NULL, NULL);
		pos.x += width / 2.0f;
		pos.y -= height / 2.0f;
		pos.z = 0.0f;
		pos.y += 4.0f * (f32)sin(DEG2RAD * look->mPhase);

		gGame.mGs.mCurRenderState->SetZCompLoc(FALSE);
		gGame.mGs.mCurRenderState->SetAlphaCompare(GX_GREATER, 0, GX_AOP_AND, GX_LEQUAL, 0xFF);
		gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
		gGame.mGs.mCurRenderState->SetZMode(TRUE, GX_ALWAYS, FALSE);
		gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
		Vec rotation = { 0.0f, 0.0f, 0.0f };
		DrawGsTexturedQuad(&gGame.mGs, &tex, 0xFFFFFF00 | (u32)(255.0f * alpha), 0xFFFFFF00 | (u32)(255.0f * alpha),
		                   0xFFFFFF00 | (u32)(255.0f * alpha), 0xFFFFFF00 | (u32)(255.0f * alpha), pos.x, pos.y, pos.z, width, height,
		                   rotation.x, rotation.y, rotation.z);

		f32 glow = alpha * (look->_30 ? 0.5f + 0.5f * (f32)sin(PI * look->mPulse) : 0.5f);
		if (glow < 0.0f)
			glow = 0.0f;
		if (look->_30) {
			gGame.mGs.mCurRenderState->SetZMode(TRUE, GX_ALWAYS, TRUE);
			Vec maskRotation = { 0.0f, 0.0f, 0.0f };
			DrawGsTexturedQuad(&gGame.mGs, &tex, 0xFFFFFF01, 0xFFFFFF01, 0xFFFFFF01, 0xFFFFFF01, pos.x, pos.y, pos.z, 0.99f * width,
			                   0.99f * height, maskRotation.x, maskRotation.y, maskRotation.z);
			gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCCOL, GX_BL_SRCALPHA, GX_LO_SET);
			gGame.mGs.mCurRenderState->SetZMode(TRUE, GX_EQUAL, FALSE);
			gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
			Vec glowRotation = { 0.0f, 0.0f, 0.0f };
			DrawGsQuad(&gGame.mGs, 0xFFFFFF00 | (u32)(255.0f * glow), 0xFFFFFF00 | (u32)(255.0f * glow), 0xFFFFFF00 | (u32)(255.0f * glow),
			           0xFFFFFF00 | (u32)(255.0f * glow), pos.x, pos.y, pos.z, width, height, glowRotation.x, glowRotation.y,
			           glowRotation.z);
		}

		if (look->mMode >= 0) {
			gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[look->mMode], 0);
			tex = gGame.mTpl.mTex;
			gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
			gGame.mGs.mCurRenderState->SetZMode(TRUE, GX_ALWAYS, FALSE);
			gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
			DrawGsTexturedQuad(&gGame.mGs, &tex, 0xFFFFFF00 | (u32)(255.0f * alpha), 0xFFFFFF00 | (u32)(255.0f * alpha),
			                   0xFFFFFF00 | (u32)(255.0f * alpha), 0xFFFFFF00 | (u32)(255.0f * alpha), pos.x, pos.y, pos.z, width, height,
			                   look->mTarget.x, look->mTarget.y, look->mTarget.z);
		}
		gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
		gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
		gGame.mGs.mCurRenderState->SetZMode(TRUE, GX_LEQUAL, FALSE);
		gGame.mGs.mCurRenderState->SetZCompLoc(TRUE);
	}
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
		XObjScnCamInitRec* aim = &((WiiPlayerState*)mChr)->mHeadAimRecord;
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
		XObjScnCamInitRec* aim = &((WiiPlayerState*)mChr)->mHeadAimRecord;
		aim->mSnap[0] = aim->mSnap[1] = 2;
		aim->m_0C                     = NULL;
		aim->mTarget.x                = NAN;
		aim->m_20                     = NULL;
		aim->m_24                     = NAN;
		aim->m_28                     = NAN;
		aim->m_30                     = 0.0f;
		aim->m_2C                     = 0.0f;
		aim->m_4C.x                   = 0.0f;
		aim->m_4C.y                   = 0.0f;
		aim->m_4C.z                   = 0.0f;
	}
	mPointedItem = mPreviousPointedItem = -1;
}

// the head-aim yaw for the row menu's seven visible slots, degrees; the octa ring's
// per-slot aims keep the GameCube's gEquipmentHeadYaw/gEquipmentHeadPitch names and
// its cursor+1 indexing, the Wii retuned the pitch table's tail
f32 gEquipmentRowHeadYaw[7] = { -30, -20, -10, 0, 10, 20, 30 };
f32 gEquipmentHeadYaw[9]    = { 0, -20, 0, 20, 20, 20, 0, -20, -20 };
f32 gEquipmentHeadPitch[9]  = { -30, -20, -10, 0, 10, 0, -10, -3, -2 };

extern "C" void ResetPopperShot(XActPopperShot* shot);

// HandleEquipmentRingInput, the GameCube's function of the same name: the remote's
// direction codes sit in the high word of the pad's u64 button fields, the GameCube
// pad's in the low. The row menu (type 3) scrolls its seven slots with the dpad, the
// octa ring maps a direction straight to a slot; where the GameCube turned the
// neck/head joints the Wii writes the head-aim record at WiiPlayerState _987C.
// 1 picked an item, -1 closed the ring
// NONMATCHING (~81%): retail anchors the three aim tables at the unit's .data
// jumptable (r31 + 0x1A4C/0x1A68/0x1A8C) — that anchor resolves only once this
// unit's .data is complete; the rest is the register-allocation and scheduling
// fallout of the missing anchor
/**
 * @note Address: 0x800E96CC
 * @note Size: 0x660
 */
extern "C" s32 HandleEquipmentRingInput(OctaMenu* menu, BOOL active)
{
	BOOL settled = menu->mOpen > 1 && (menu->mGlow == 0.0f || menu->mGlow == 1.0f);
	if (settled && menu->mPointedItem >= 0 && menu->mFocus != 0) {
		return 1;
	}
	if (menu->mOpen > 0 && menu->mFocus == 0) {
		s32 cursor;
		if (menu->mMenuType == 3) {
			if ((gGame.mMenuControls & 2) && (!(gGame.mMenuControls & 1) || !gWiiPointer.mPointerValid)) {
				u64 dirs = active ? gWiiPointer.mRepeat & 0x0F000000001E0000 : 0;
				if (dirs & 0x0100000000020000) {
					if (--menu->mLastPointedItem < 0) {
						menu->mLastPointedItem = 6;
					}
					PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
				} else if (dirs & 0x0200000000040000) {
					if (++menu->mLastPointedItem >= 7) {
						menu->mLastPointedItem = 0;
					}
					PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
				}
			}
			if (menu->mFocus != 0) {
				cursor = menu->mCursorIndex = -1;
			} else {
				cursor = menu->mCursorIndex = (menu->mFirstVisibleItem + menu->mLastPointedItem) % 7;
			}
		} else {
			u64 dirs = active ? gWiiPointer.mButton & 0x0F000000001E0000 : 0;
			if (dirs == 0x120000 || dirs == 0x0900000000000000) {
				cursor = 0;
			} else if (dirs == 0x100000 || dirs == 0x0800000000000000) {
				cursor = 1;
			} else if (dirs == 0x140000 || dirs == 0x0A00000000000000) {
				cursor = 2;
			} else if (dirs == 0x040000 || dirs == 0x0200000000000000) {
				cursor = 3;
			} else if (dirs == 0x0C0000 || dirs == 0x0600000000000000) {
				cursor = 4;
			} else if (dirs == 0x080000 || dirs == 0x0400000000000000) {
				cursor = 5;
			} else if (dirs == 0x0A0000 || dirs == 0x0500000000000000) {
				cursor = 6;
			} else if (dirs == 0x020000 || dirs == 0x0100000000000000) {
				cursor = 7;
			} else {
				cursor = menu->mCursorIndex;
			}
		}
		XObjScnCamInitRec* aim = &((WiiPlayerState*)menu->mChr)->mHeadAimRecord;
		if (menu->mMenuType == 3) {
			if (__fpclassifyf(menu->mHeadTurn) == FP_NAN) {
				aim->mSnap[0] = aim->mSnap[1] = 1;
				aim->m_0C                     = NULL;
				aim->mTarget.x                = NAN;
				aim->m_20                     = NULL;
				aim->m_24                     = DEG2RAD * (6.0f * (menu->mRowOffset * menu->mRowOffset * menu->mRowOffset));
				aim->m_28                     = DEG2RAD * gEquipmentRowHeadYaw[menu->mLastPointedItem];
			} else {
				aim->mSnap[0] = aim->mSnap[1] = 1;
				aim->m_0C                     = NULL;
				aim->mTarget.x                = NAN;
				aim->m_20                     = NULL;
				aim->m_24                     = menu->mHeadTurn;
				aim->m_28                     = DEG2RAD * gEquipmentRowHeadYaw[menu->mLastPointedItem];
			}
		} else {
			if (__fpclassifyf(menu->mHeadTurn) == FP_NAN) {
				aim->mSnap[0] = aim->mSnap[1] = 1;
				aim->m_0C                     = NULL;
				aim->mTarget.x                = NAN;
				aim->m_20                     = NULL;
				aim->m_24                     = DEG2RAD * gEquipmentHeadPitch[cursor + 1];
				aim->m_28                     = DEG2RAD * gEquipmentHeadYaw[cursor + 1];
			} else {
				aim->mSnap[0] = aim->mSnap[1] = 1;
				aim->m_0C                     = NULL;
				aim->mTarget.x                = NAN;
				aim->m_20                     = NULL;
				aim->m_24                     = menu->mHeadTurn;
				aim->m_28                     = DEG2RAD * gEquipmentHeadYaw[cursor + 1];
			}
		}
		if (menu->mCursorIndex != cursor || (menu->mPointedItem >= 0 && menu->mPointedItem != menu->mPreviousPointedItem)) {
			PlayWiiSoundEffect(&gGame.mSound, 0xF4, 1.0f, 0.0f);
		}
		menu->mCursorIndex = cursor;
		if (active && menu->mFade >= 1.0f) {
			if (gWiiPointer.Check(1)) {
				if (menu->mPointedItem >= 0 || ((gGame.mMenuControls & 2) && (!(gGame.mMenuControls & 1) || !gWiiPointer.mPointerValid))) {
					if (menu->mCursorIndex < 0) {
						PlayWiiSoundEffect(&gGame.mSound, 0xF2, 1.0f, 0.0f);
					} else if (menu->mItems[menu->mCursorIndex].mAvailable == 0) {
						PlayWiiSoundEffect(&gGame.mSound, 0xF2, 1.0f, 0.0f);
					} else {
						menu->ConfirmSelection();
						XActPopper* popper = (XActPopper*)&((WiiPlayerState*)menu->mChr)->mPopper;
						for (s32 i = 0; i < 10; i++) {
							ResetPopperShot(&popper->mShots.mShot[i]);
						}
						return 1;
					}
				}
			} else if (gWiiPointer.Check(2) || gWiiPointer.Check(0x100)) {
				return -1;
			}
		}
	}
	return 0;
}

// nothing to decide on the base menu
void OctaMenu::ConfirmSelection()
{
}
