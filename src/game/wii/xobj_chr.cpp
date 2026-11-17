// The player object's file, .text 0x800C9E84-0x800E9D30: its .rodata names it "xobj_chr.cpp"
// (wii_units.md Nr. 60 and 61), the GameCube's obja_chr.cpp. The functions come in the reverse of the
// GameCube's order.
// NONMATCHING: only the small accessors the GameCube file shares have source so far; the player's
// update, draw and menu code is still to do. CanFallDuringLadderAction, CanFallDuringRopeAction and
// CanCopterActionSlideOnSlope (74-81 %): retail starts each bool at TRUE and clears it, as an inlined
// helper returning the first two compares would; this build tests the other way round.
#include "rvl_msl/PowerPC_EABI_Support/MSL_C/MSL_Common/math_api.h"
#include "game/attachmentvalues.h"
#include "game/ringmenu.h"
#include "game/wii/cgame.h"
#include "game/wii/xobj.h"
#include "game/wii/xchrmenu.h"
#include "game/wii/csound.h"
#include "game/playeranimationdata.h"
#include "game/pointptcl.h"
#include "game/mapfloorhit.h"
#include "game/gmath.h"
#include "game/globals.h"
#include <math.h>
#include <stdlib.h>

extern "C" void ReleasePushAction(XAction* action);

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
	if (IsStopped(chr)) {
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
	bool ret = false;
	if (chr->mTumble.mState == 4 || chr->mTumble.mState == 5 || chr->mTumble.mState == 6) {
		ret = true;
	}
	return ret;
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

static inline u8 IsMenuUp(WiiPlayerState* chr)
{
	return (u8)(chr->mMenu.mAtc.mOpen > 0 || chr->mMenu.mDrs.mOpen > 0);
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
	chr->mMotion.x         = 0.0f;
	chr->mMotion.y         = 0.0f;
	chr->mMotion.z         = 0.0f;
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

// events wait while he lies there out of battery
static inline bool IsEventBlocked(const WiiPlayerState* chr)
{
	bool ret = false;
	if (IsEventHeld(chr) && IsTumbling(chr)) {
		ret = true;
	}
	return ret;
}

extern "C" {
u32 CheckStageFlag(s32 stage, u32 mask);
BOOL fn_800C98E0(XObjMap* map, const Vec* pos, s32 start, XObjMapHit* hit);
void UpdateHeldObject(XAction* action, u32 frames);
void UpdatePushedObject(XAction* action, u32 frames);
void GetOutwardPerpendicularToEdge(Vec* a, Vec* b, Vec* c, Vec* out);
s32 GetClearLetickerApproachSide(XObj* object, const Vec* pos);
void StartPointParticles(PointPtcl* ptcl, s32 type, s32 amount);
BOOL IsLiftTargetPlug(XAction* action);
BOOL fn_800CDC98(WiiPlayerState* player);
void fn_800CDDC0(WiiPlayerState* player);
extern PointPtcl gPointParticlePool[10];
extern PointPtcl* gPointParticleSlots[10];
extern s32 gRefreshRate;

void SetVectorComponents(CVec* value, f32 x, f32 y, f32 z)
{
	value->Set(x, y, z);
}

void ClearObjectStatusFlags(XObj* object, u64 mask)
{
	object->mStatus.mFlags &= ~mask;
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
// NONMATCHING (91 %): SetAnim goes through WiiPlayerVtable, retail's virtual call loads the slot into r12
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

BOOL fn_800CDDAC(const XObj* object)
{
	return (object->mStatus.mFlags & 1) != 0;
}

// the player's suit step of the stage setup: puts on the suit the save asked for, TRUE once it is on
// NONMATCHING: retail recomputes the suits' address into r4 and reads mCurrent before mRequested
BOOL fn_800CDC08(WiiPlayerState* player)
{
	if (player->mSuits.mLoadStep == 0) {
		player->mSuits.mLoadStep = 1;
	}
	if (player->mSuits.mCurrent != player->mSuits.mRequested) {
		player->mSuits.mCurrent    = player->mSuits.mRequested;
		player->mSuits.mChangeStep = 1;
	}
	if (fn_800CDC98(player)) {
		player->mSuits.mChangeStep = 0;
		fn_800CDDC0(player);
		return TRUE;
	}
	return FALSE;
}

// reads the player's pad once more
void fn_800CE48C(WiiPlayerState* player)
{
	player->mPad.Update(gWiiGameCubePads, 0);
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
	value->y         = y;
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
			gGame.mGs.mSceneHead->vf034(gGame.mGs.mSceneHead->mPending, player->mLift.mTarget->mObject.m_pScnMdl);
		}
	}
}

// the same for what he pushes
void fn_800D1AF8(WiiPlayerState* player, u32 frames)
{
	if (player->mPush.mTarget != NULL) {
		UpdatePushedObject(&player->mPush, frames);
		if (player->mPush.mTarget != NULL) {
			gGame.mGs.mSceneHead->vf034(gGame.mGs.mSceneHead->mPending, player->mPush.mTarget->mObject.m_pScnMdl);
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

// NONMATCHING (95 %): the SetAnim call through WiiPlayerVtable as in StopPlayerMotion, and retail
// starts the ring-up flag at TRUE and the open flag at FALSE where this build does the reverse
void SelectPlayerEquipmentRing(WiiPlayerState* chr, s32 selection)
{
	if (selection >= 0) {
		chr->mMenu.mPrevSel = chr->mMenu.mSel;
		chr->mMenu.mSel     = selection;
	}
	if (IsMenuUp(chr)) {
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
	return value;
}

void SetObjectXAndTarget(CObjVec* value, f32 x)
{
	value->x         = x;
	value->mTarget.x = x;
}

void SetObjectZAndTarget(CObjVec* value, f32 z)
{
	value->z         = z;
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

// NONMATCHING (67-71 %): retail starts each bool at TRUE and clears it, as in CanFallDuringLadderAction
BOOL CanFallDuringJumpAction(const XAction* act)
{
	s32 state = act->mState;
	bool a    = state == 0 || state == 2;
	bool b    = a || state == 3;
	bool c    = b || state == 4;
	return c || state == 17;
}

// NONMATCHING (67-71 %): retail starts each bool at TRUE and clears it, as in CanFallDuringLadderAction
BOOL CanFallDuringCopterAction(const XAction* act)
{
	s32 state = act->mState;
	bool a    = state == 0 || state == 3;
	bool b    = a || state == 4;
	bool c    = b || state == 7;
	bool d    = c || state == 8;
	bool e    = d || state == 5;
	bool f    = e || state == 6;
	return f || state == 9;
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
	value->x         = x;
	value->y         = y;
	value->z         = z;
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
	value->x         = x;
	value->y         = y;
	value->z         = z;
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
	s32 state = act->mState;
	bool idle = state == 0 || state == 10;
	return idle || state == 11;
}

BOOL CanFallDuringRopeAction(const XAction* act)
{
	s32 state  = act->mState;
	bool idle  = state == 0 || state == 9;
	bool ended = idle || state == 10;
	return ended || state == 11;
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
	bool idle = act->mState == 0 || act->mState == 6;
	return idle || act->mState == 9;
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

// NONMATCHING (45 %): retail keeps each inline's bool in a register of its own (stopped, down, held) and
// tests the tumble states one compare at a time; this build folds them into one flag and a range check
BOOL IsPlayerBlockingEvents(const WiiPlayerState* player)
{
	return IsEventBlocked(player);
}

// the event call 101: which side of a clear leticker he comes from, 1 for anything else
// NONMATCHING: retail loads the data record ahead of the prologue's stores
s32 GetCharacterObjectApproachResult(XObj* chr, XObj* object)
{
	return object->GetPool() == 9 ? GetClearLetickerApproachSide(object, chr->GetPos()) : 1;
}

// the highest floor of the object under a floor group that's still below chibi robo,
// stops early on the preferred one, -1 for none. The contact flag is read off the group's own entry
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

} // extern "C"

// nothing to decide on the base menu
void OctaMenu::ConfirmSelection()
{
}
