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
#include "game/gmath.h"
#include "game/globals.h"
#include <math.h>

extern "C" {

void SetVectorComponents(CVec* value, f32 x, f32 y, f32 z)
{
	value->Set(x, y, z);
}

void ClearObjectStatusFlags(XObj* object, u64 mask)
{
	object->mStatus.mFlags &= ~mask;
}

BOOL HasPlayerActionTarget(const XAction* action)
{
	return action->mTarget != NULL;
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

BOOL IsTumbleActionState(const XAction* act, s32 wanted)
{
	return (u8)(wanted == act->mState);
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

u8 IsPlayerFlashingFromHit(WiiPlayerState* chr)
{
	return chr->mFlashTimer > 0;
}

} // extern "C"

// nothing to decide on the base menu
void OctaMenu::ConfirmSelection()
{
}
