// The player object's file, .text 0x800C9E84-0x800E9D30: its .rodata names it "xobj_chr.cpp"
// (wii_units.md Nr. 60 and 61), the GameCube's obja_chr.cpp. The functions come in the reverse of the
// GameCube's order.
// NONMATCHING: only the small accessors the GameCube file shares have source so far; the player's
// update, draw and menu code is still to do. CanFallDuringLadderAction, CanFallDuringRopeAction and
// CanCopterActionSlideOnSlope (74-81 %): retail starts each bool at TRUE and clears it, as an inlined
// helper returning the first two compares would; this build tests the other way round.
#include "game/attachmentvalues.h"
#include "game/ringmenu.h"
#include "game/wii/cgame.h"
#include "game/wii/xobj.h"
#include "game/wii/xchrmenu.h"
#include "game/gmath.h"
#include "game/globals.h"
#include <math.h>

static inline bool TestStatus(const XObj* object, u64 mask)
{
	return (u8)((object->mStatus.mFlags & mask) != 0);
}

extern "C" {
u32 CheckStageFlag(s32 stage, u32 mask);

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

f32 WrapPlayerActionAngle(f32 angle)
{
	angle = WrapAngle(angle);
	return angle;
}

f32 GetVectorDistance(const Vec* a, const Vec* b)
{
	return PSVECDistance((Vec*)a, (Vec*)b);
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

void SetAttachmentCountById(AttachmentValues* values, s32 id, s32 value)
{
	values->SetCount(id, value);
}

// out of battery: the battery is flat and the gauge on screen has run down to it
BOOL IsPlayerStopped(const WiiPlayerState* chr)
{
	return chr->mParams.mBattery <= 0.0f && InfoScrn_IsBatteryGaugeEmpty(&gGame.mInfoScrn);
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
