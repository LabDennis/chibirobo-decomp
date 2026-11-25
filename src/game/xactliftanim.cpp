#include "game/xchr.h"
#include "game/xact.h"
#include "game/xaction.h"
#include "game/xobjleticker.h"
#include "game/cgame.h"
#include "game/cvec.h"
#include "game/cparticle.h"
#include "game/csound.h"
#include "game/globals.h"
#include "game/stagedata.h"
#include "game/video.h"
#include "game/gmath.h"
#include <math.h>
extern "C" {
#include <dolphin/mtx/vec.h>
#include <sysdolphin/aobj.h>
#include <sysdolphin/particle.h>
}

extern "C" {
void SetObjectJointTranslationByPath(CJObj* object, s32 depth, s32* path, f32 x, f32 y, f32 z);
void RecordObjectPositionAndHeightChange(XObj* obj);
s32 GetObjectPoolId(const XObj*);
s16 GetObjectItemId(XObj*);
s16 GetItemPickupHoldTime(s32);
f32 GetHeadingBetweenPositions(const Vec*, const Vec*);
void SetPlugCordSpeed(XObjPlug*, f32);
void SetLiftActionState(XActLift*, s32);
void SetLiftTargetState(XActLift*, s32);
XObj* GetGameObjectByIndex(const CGame*, s32);
BOOL IsEventScriptActive(const CGame*);
Params* GetPlayerParamsByIndex(CGame*, s32);
s32 GetInventoryCountByIndex(Params*, u32);
void SetPlayerStandAnim(XObjChr*, s32, f32);
void SetClampedPlayerBattery(Params*, f32);
HSD_Generator* CreateParticleByBankAndId(CGame*, s32, s32, u8, HSD_JObj*, BOOL);
XObj* SpawnObjectFromPlacementIndex(CGame*, s16);
BOOL IsObjectLoaded(XObj*);
void ClearObjectStatusFlags(XObj*, u64);
void SetPlayerActionTarget(XAction*, XObj*);
s32 RemovePlayerInventoryItemCount(Params*, s16, s32);
void PutAwayLiftedObject(XActLift*);
Vec GetJointMatrixTranslation(void*, HSD_JObj*);
Vec GetJointMatrixScale(const CJObj*, HSD_JObj*);
HSD_Generator* CreateParticleAtPosition(CGame*, u32, u32, s32, const Vec&, BOOL);
extern HSD_PSUserFunc gPlayerItemParticleCallbacks;
void SetSubwindowRect(CGame*, f32, f32, f32, f32);
void SetSubwindowCamera(CGame*, const Vec&, f32, f32, f32, f32);
void EnableSubwindowWithFlags(CGame*, u32);
Vec GetObjectJointWorldPosition(const CJObj*);
BOOL IsLiftActionState(const XActLift*, s32);
f32 GetObjectJointRotationY(CJObj*);
void ClearObjectAnimationQueue(XObj*);
Vec GetObjectJointMatrixScale(const CJObj*);
HSD_Generator* CreateScaledParticleAtPosition(CGame*, u32, u32, s32, const Vec&, const Vec&, BOOL);
extern const s32 gSquirterChargeByCount[];
}

static inline void HeldAnim(XObj* obj, s32 id, BOOL force = FALSE, f32 blend = 0)
{
	obj->SetAnim(id, force, NULL, NULL, NULL, 0, 1, blend, 10);
}
static inline bool HasRoomForItem(XObj* item)
{
	s16 id   = GetObjectItemId(item);
	s32 slot = FindOwnedInventoryItemIndex(GetPlayerParamsByIndex(&gGame, 0)->mInventory.mItems, id);
	return slot < 0 || GetInventoryCountByIndex(GetPlayerParamsByIndex(&gGame, 0), slot) < 10;
}

static inline void LoadLiftItem(XActLift* act, s32 next)
{
	act->mPendingItem = act->mPendingItemId >= 0 ? SpawnObjectFromPlacementIndex(&gGame, act->mPendingItemId) : NULL;
	SetLiftActionState(act, next);
}
static inline void TakeLiftItem(XActLift* act, s32 anim, HSD_AObj* aobj, bool useCount)
{
	s32 wanted = useCount ? 11 : 0xec;
	if (anim != wanted) {
		HeldAnim(act->mOwner, wanted, TRUE, 1);
		SetLiftTargetState(act, useCount ? 3 : 18);
		if (IsLiftTargetPlug(act)) {
			ClearObjectStatusFlags(act->mTarget, 0x0400000000000000ULL);
			act->mTarget = NULL;
		}
		if (act->mPendingItemId >= 0 && act->mPendingItem) {
			act->mPendingItem->mStatus.mFlags |= 0x0400000000000000ULL;
			act->mPendingItem->SetScale(0, 0);
			SetPlayerActionTarget(act, act->mPendingItem);
			if (useCount) {
				s16 id         = GetObjectItemId(act->mTarget);
				s32 count      = RemovePlayerInventoryItemCount(GetPlayerParamsByIndex(&gGame, 0), id, 1);
				s32 difference = count - GetStageCollectionFlagBase(0);
				if (difference > 0)
					act->mPendingItem->mPlacementParam1 = difference;
			}
			act->mPendingItemId = -1;
			if (useCount && IsCarryingSquirter(act))
				ApplyPlayerItemTintToTarget(act);
		}
	} else if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
		HeldAnim(act->mOwner, 8);
		SetLiftActionState(act, 4);
		SetLiftTargetState(act, useCount ? 4 : 19);
	}
}
static inline void ReleasePlugTarget(XActLift* act, s32 anim, HSD_AObj* aobj, s32 next)
{
	if (anim != next) {
		HeldAnim(act->mOwner, next, FALSE, 1);
		act->mOwner->UpdateAnim(0);
		SetLiftTargetState(act, 13);
	} else if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
		HeldAnim(act->mOwner, 8);
		SetLiftActionState(act, 4);
		act->mPlugTarget = NULL;
	}
}
static inline void UseToolCharge(XActLift* act, s16 count, s32 level)
{
	if (gSquirterChargeByCount[count] == level)
		HeldAnim(act->mTarget, level + 1);
	else
		HeldAnim(act->mTarget, level);
	SetOrAddInventoryItemCount(&act->mOwner->mParams.mInventory, 0x38, count, FALSE);
}

/**
 * @note Address: 0x800D0F4C
 * @note Size: 0x2B10
 */
extern "C" void UpdateLiftActionAnimation(XActLift* act, s32 anim, HSD_AObj* aobj, u32 frames, f32 frame, f32)
{
	s32 placement     = act->mTarget ? act->mTarget->mCoord.mPlacement - GetStagePlacementByIndex(0) : -1;
	act->mBatteryCost = 0;
	switch (act->mState) {
	case 1:
		if (anim != 5) {
			HeldAnim(act->mOwner, 5);
			if (act->mTarget) {
				const CVec& point = GetFloorContactPosition(act->mTarget, act->mTargetFloor, NULL);
				act->mOwner->SetRotateY(GetHeadingBetweenPositions(&act->mOwner->mMoveTranslate, &point), 12 * DEG2RAD);
			}
			if (placement < 0 && PSVECDistance(&act->mOwner->mMoveTranslate, &act->mTarget->mMoveTranslate) >= 3.0f)
				SetPlugCordSpeed(&act->mOwner->mPlug, 0.2f);
			return;
		}
		if (!(aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)))
			return;
		HeldAnim(act->mOwner, 6, TRUE, 1);
		SetLiftActionState(act, 2);
		if (placement < 0) {
			SetPlugCordSpeed(&act->mOwner->mPlug, 0.5f);
			act->mTimer = 0;
			PlayGameSound(&gGame.mSound, 0x3a, 255);
		} else if ((s16)GetObjectPoolId(act->mTarget) == 8) {
			act->mTimer = FRAME_RATE * ((f32)GetItemPickupHoldTime(GetObjectItemId(act->mTarget)) / 100.0f);
		} else {
			act->mTimer = 0.5f * FRAME_RATE;
		}
		// fall through
	case 2:
		act->mTimer -= frames;
		if (!(act->mTimer <= 0))
			return;
		SetLiftActionState(act, 3);
		// fall through
	case 3:
		if (anim != 7) {
			if (!act->mTarget || (s16)GetObjectPoolId(act->mTarget) != 8 || HasRoomForItem(act->mTarget)) {
				SetLiftTargetState(act, 1);
				HeldAnim(act->mOwner, 7, TRUE, 1);
			} else {
				SetLiftActionState(act, 5);
			}
			if (act->mTarget && act->mTarget->GetEventScript() && (act->mTarget->mStatus.mFlags & 0x10000000000ULL)) {
				XObj* target = act->mTarget;
				TryStartObjectEvent(&gGame, GetGameObjectByIndex(&gGame, placement)->GetEventScript(), target, -1, -1);
			}
		} else if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
			HeldAnim(act->mOwner, 8);
			if (act->mTarget) {
				if (act->mTarget->GetEventScript())
					SetLiftActionState(act, 5);
				else if ((s16)GetObjectPoolId(act->mTarget) == 8 && !(act->mTarget->mStatus.mFlags & 0x400000000000ULL))
					SetLiftActionState(act, 19);
				else
					SetLiftActionState(act, 4);
				SetLiftTargetState(act, 4);
			}
		}
		break;
	case 4:
		if (anim == 0x895) {
			if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
				HeldAnim(act->mOwner, 0x894);
				if (act->mContactEffect) {
					gGame.mParticle.Kill(act->mContactEffectId);
					act->mContactEffect = NULL;
				}
			} else {
				if (act->mCheckContact && frame > 6) {
					s32 path[5]  = { 0, 0, 0, 0, 0 };
					Vec position = CJObj_GetJointWorldPositionByPath(act->mTarget, 5, path);
					if (CheckCarriedObjectContacts(act, &position, 3, 1, TRUE, FALSE, FALSE, NULL))
						act->mCheckContact = FALSE;
				}
				if (!act->mContactEffect) {
					act->mContactEffectId = 29;
					s32 path[5]           = { 0, 0, 0, 0, 0 };
					act->mContactEffect
					    = CreateParticleByBankAndId(&gGame, 1, act->mContactEffectId, 0, GetObjectJointByPath(act->mTarget, 5, path), TRUE);
				}
			}
		} else if (anim == 0x8ae || anim == 0x8af) {
			if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM))
				HeldAnim(act->mOwner, 0x894);
		}
		// the brush's effect lasts while it scrubs (0x895), the squirter's while it squirts (0x8c2 to 0x8c4)
		if (IsCarryingToothbrush(act)) {
			if (anim != 0x895 && act->mContactEffect) {
				gGame.mParticle.Kill(act->mContactEffectId);
				act->mContactEffect = NULL;
			}
		} else if (IsCarryingSquirter(act) && (anim < 0x8c2 || anim > 0x8c4) && act->mToolEffect) {
			gGame.mParticle.Kill(act->mToolEffectId);
			act->mToolEffect = NULL;
		}
		break;
	case 5:
		if (!IsEventScriptActive(&gGame)) {
			if ((s16)GetObjectPoolId(act->mTarget) == 8 && !(act->mTarget->mStatus.mFlags & 0x400000000000ULL)) {
				if (HasRoomForItem(act->mTarget))
					SetLiftActionState(act, 19);
				else {
					SetLiftActionState(act, 0);
					SetPlayerStandAnim(act->mOwner, 17, 0);
					act->mTarget = NULL;
				}
			} else {
				SetLiftActionState(act, 4);
			}
		}
		break;
	case 6:
		if (anim != 9) {
			HeldAnim(act->mOwner, 9);
			SetLiftTargetState(act, 5);
		} else if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
			SetPlayerStandAnim(act->mOwner, 17, 0);
			SetLiftActionState(act, 0);
		}
		break;
	// event commands 102, 106 and 108 plug him into mPlugTarget: 7 only plugs in, 8 then gives it
	// power (12, 13), 9 then waits for the stick or b (15)
	case 7:
	case 8:
	case 9:
		if (anim != 0x9f && anim != 0xa0 && anim != 0xbc) {
			XObjPlug* plug = &act->mOwner->mPlug;
			SetPlayerActionTarget(act, plug);
			plug->mStatus.mFlags |= 0x0400000000000000ULL;
			ResetPlayerPlugState(plug, TRUE);
			act->mYawOffset = 0;
			plug->SetRotationY(GetObjectJointRotationY(act->mOwner), 0);
			plug->SetRotationX(0, 0);
			s32 path[] = { 0 };
			CXObj joint(GetObjectJointByPath(plug, 1, path));
			joint.SetRotationX(0, 0);
			joint.SetRotationY(0, 0);
		}
		// fall through
	case 10:
		if (anim != 0x9f && anim != 0xa0 && anim != 0xbc) {
			ClearObjectAnimationQueue(act->mOwner);
			if (IsLiftActionState(act, 7)) {
				HeldAnim(act->mOwner, 0x9f);
				SetLiftTargetState(act, 6);
			} else if (IsLiftActionState(act, 8)) {
				HeldAnim(act->mOwner, 0xa0);
				SetLiftTargetState(act, 8);
			} else {
				HeldAnim(act->mOwner, 0xbc);
				SetLiftTargetState(act, 11);
			}
		} else if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
			if (IsLiftActionState(act, 7)) {
				SetPlayerStandAnim(act->mOwner, 17, 0);
				SetLiftActionState(act, 0);
				if (act->mPlugTarget)
					act->mPlugTarget->ReceivePlugPower(0);
			} else if (IsLiftActionState(act, 8)) {
				SetLiftActionState(act, 12);
				act->mTimer = 0.2f * FRAME_RATE;
			} else {
				HeldAnim(act->mOwner, 0xbe, FALSE, 1);
				SetLiftActionState(act, 15);
			}
		}
		break;
	case 11:
		if (anim != 0xad) {
			HeldAnim(act->mOwner, 0xad, FALSE, 1);
			act->mOwner->UpdateAnim(0);
			SetPlayerActionTarget(act, &act->mOwner->mPlug);
			SetLiftTargetState(act, 7);
			act->mYawOffset = 0;
		} else if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
			s32 path[] = { 0 };
			act->mOwner->SetPos(GetEventJointWorldPosition(act->mOwner, 1, path), 0);
			RecordObjectPositionAndHeightChange(act->mOwner);
			s32 root[] = { 0 };
			SetObjectJointTranslationByPath(act->mOwner, 1, root, 0, 0, 0);
			HeldAnim(act->mOwner, 8, FALSE, 1);
			SetLiftActionState(act, 4);
			act->mPlugTarget = NULL;
		}
		break;
	case 12:
		act->mTimer -= frames;
		if (act->mTimer <= 0) {
			act->mTimer = 0;
			if (act->mPlugTarget && !act->mPlugTarget->ReceivePlugPower(0)) {
				SetLiftActionState(act, 14);
				break;
			}
			HeldAnim(act->mOwner, 0xab);
			SetLiftActionState(act, 13);
			if (act->mPlugTarget && !GetObjectItemId(act->mPlugTarget)) {
				SetSubwindowRect(&gGame, NAN, NAN, NAN, NAN);
				s32 path[] = { 0 };
				CJObj joint(GetObjectJointByPath(&act->mOwner->mPlug, 1, path));
				Vec at = act->mOwner->mMoveTranslate + (act->mPlugTarget->mMoveTranslate - act->mOwner->mMoveTranslate) * 0.3f;
				f32 y  = act->mOwner->mMoveTranslate.y;
				at.y   = y + 0.8f * (GetObjectJointWorldPosition(&joint).y - y);
				SetSubwindowCamera(&gGame, at, act->mOwner->mMoveRotate.y - 45 * DEG2RAD, 24 * DEG2RAD, 100, 8);
				EnableSubwindowWithFlags(&gGame, 0);
			}
		}
		break;
	case 13: {
		Params& params = act->mOwner->mParams;
		if (params.mBattery > 0 && params.mBattery / params.mMaxBattery > 0.2f) {
			if (anim != 0xab)
				HeldAnim(act->mOwner, 0xab);
		} else if (anim != 0xae) {
			HeldAnim(act->mOwner, 0xae);
		}
		if (act->mPlugTarget) {
			act->mBatteryCost = (f32)frames * (((s32)GetPlayerActionParameter(9) / 100.0f) / FRAME_RATE);
			if (!act->mPlugTarget->ReceivePlugPower(act->mBatteryCost)) {
				act->mBatteryCost = 0;
				SetLiftActionState(act, 14);
			}
		}
		break;
	}
	case 14:
		if (anim != 0xac) {
			HeldAnim(act->mOwner, 0xac);
			act->mOwner->UpdateAnim(0);
			SetLiftTargetState(act, 10);
			gGame.mSubwin.mEnabled = FALSE;
			if (act->mPlugTarget && GetObjData(&act->mPlugTarget->mCoord)->mCategory == 9)
				static_cast<XObjLeticker*>(act->mPlugTarget)->OnUnplugged();
		} else if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
			HeldAnim(act->mOwner, 8);
			SetLiftActionState(act, 4);
			act->mPlugTarget = NULL;
		}
		break;
	case 16: {
		XObjPlug* plug = static_cast<XObjPlug*>(act->mTarget);
		if (anim != 0xbf) {
			HeldAnim(act->mOwner, 0xbf, FALSE, 1);
			if (act->mPlugTarget)
				HeldAnim(act->mPlugTarget, 1, FALSE, 1);
		} else if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
			HeldAnim(act->mOwner, 8);
			SetLiftActionState(act, 4);
			SetLiftTargetState(act, 4);
			act->mPlugTarget = NULL;
			ResetPlayerPlugState(plug, TRUE);
		} else if (frame > 142.0f) {
			plug->mConnection = 0;
		} else if (frame > 85.0f) {
			if (plug->mConnection != -1) {
				f32 remaining = act->mOwner->mParams.mBattery - 10.0f;
				if (remaining < 0)
					remaining = 0;
				SetClampedPlayerBattery(&act->mOwner->mParams, remaining);
			}
			plug->mConnection = -1;
		}
		break;
	}
	case 17:
		ReleasePlugTarget(act, anim, aobj, 0xbd);
		break;
	case 18:
		ReleasePlugTarget(act, anim, aobj, 0xac);
		break;
	case 19:
		if (anim != 10) {
			HeldAnim(act->mOwner, 10);
			SetLiftTargetState(act, 14);
			if (act->mContactEffect) {
				gGame.mParticle.Kill(act->mContactEffectId);
				act->mContactEffect = NULL;
			}
			if (act->mToolEffect) {
				gGame.mParticle.Kill(act->mToolEffectId);
				// This path clears the first effect pointer again in all three editions.
				act->mContactEffect = NULL;
			}
		} else if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
			SetLiftActionState(act, 0);
			SetLiftTargetState(act, 15);
			SetPlayerStandAnim(act->mOwner, 17, 0);
			if (act->mTarget && (s16)GetObjectPoolId(act->mTarget) == 8) {
				s8 sound = (s8)GetItemSoundIndex(GetObjectItemId(act->mTarget), 1);
				if (sound >= 0)
					PlayGameSound(&gGame.mSound, GetItemReleaseSoundId(sound), 255);
			}
		}
		break;
	// 20 to 22 take an item out of his inventory, 24 to 26 the same without counting it off
	case 20:
		LoadLiftItem(act, 21);
		// fall through
	case 21:
		if (act->mPendingItem) {
			if (!IsObjectLoaded(act->mPendingItem))
				return;
			gGame.mStatus.mFlags |= 0x1000;
		}
		SetLiftActionState(act, 22);
		// fall through
	case 22:
		TakeLiftItem(act, anim, aobj, true);
		break;
	case 23:
		if (anim != 0xdd) {
			HeldAnim(act->mOwner, 0xdd);
			SetLiftTargetState(act, 16);
		} else if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
			SetPlayerStandAnim(act->mOwner, 17, 0);
			SetLiftActionState(act, 0);
			SetLiftTargetState(act, 17);
		}
		break;
	case 24:
		LoadLiftItem(act, 25);
		// fall through
	case 25:
		if (act->mPendingItem) {
			if (!IsObjectLoaded(act->mPendingItem))
				return;
			gGame.mStatus.mFlags |= 0x1000;
		}
		SetLiftActionState(act, 26);
		// fall through
	case 26:
		TakeLiftItem(act, anim, aobj, false);
		break;
	// a with a tool out: the toothbrush (27, 28), spoon (29, 30), mug (31 to 33) and squirter (35, 36)
	case 27:
		HeldAnim(act->mOwner, 0x895, TRUE);
		HeldAnim(act->mTarget, 1, TRUE);
		SetLiftActionState(act, 28);
		act->mCheckContact = TRUE;
		break;
	case 28:
		if (frame > 2) {
			SetLiftActionState(act, 4);
			s32 path[5]  = { 0, 0, 0, 0, 0 };
			Vec position = CJObj_GetJointWorldPositionByPath(act->mTarget, 5, path);
			if (CheckCarriedObjectContacts(act, &position, 3, 1, TRUE, FALSE, FALSE, NULL))
				act->mCheckContact = FALSE;
		}
		break;
	case 29:
		HeldAnim(act->mOwner, 0x8b7, TRUE);
		HeldAnim(act->mTarget, 5, TRUE);
		SetLiftActionState(act, 30);
		break;
	case 30:
		if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
			HeldAnim(act->mOwner, 0x8ae, TRUE);
			SetLiftActionState(act, 4);
			Vec pos = act->mOwner->mMoveTranslate;
			f32 yaw = act->mOwner->mMoveRotate.y;
			pos.x += 3.0f * (f32)sin(yaw);
			pos.z += 3.0f * (f32)cos(yaw);
			if (!CheckCarriedObjectContacts(act, &pos, 1, 1, TRUE, FALSE, FALSE, NULL)) {
				HeldAnim(act->mOwner, 0x8af, TRUE);
				PlayGameSound(&gGame.mSound, 0x110, 255);
				s32 path[]        = { 0, 0 };
				const CVec& point = CJObj_GetJointWorldPositionByPath(act->mTarget, 2, path);
				CreateScaledParticleAtPosition(&gGame, 1, 0x2e, 1, point, GetObjectJointMatrixScale(act->mTarget), FALSE);
			}
		}
		break;
	case 31:
		if (anim != 0x8b1) {
			HeldAnim(act->mOwner, 0x8b1);
			HeldAnim(act->mTarget, 4);
			act->mUseEffect = NULL;
		} else if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
			HeldAnim(act->mOwner, 0x8b2);
			HeldAnim(act->mTarget, 1);
			SetLiftActionState(act, 32);
		} else if (GetJointAnimationCurrentFrame(act->mOwner) > 10 && !act->mUseEffect) {
			act->mUseEffectId = 47;
			act->mUseEffect   = CreateParticleByBankAndId(&gGame, 1, 47, 1, act->mOwner->mJObj, FALSE);
		}
		break;
	case 33:
		if (anim != 0x8b3) {
			HeldAnim(act->mOwner, 0x8b3);
			HeldAnim(act->mTarget, 1);
		} else if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
			HeldAnim(act->mOwner, 8);
			SetLiftActionState(act, 4);
		}
		break;
	case 34:
		PutAwayLiftedObject(act);
		break;
	// Refill at the low or high intake point. If neither touches a source, fire at the current charge level.
	case 35: {
		Vec point = act->mOwner->mMoveTranslate;
		f32 angle = act->mOwner->mMoveRotate.y;
		point.x += 2.0f * (f32)sin(angle);
		point.z += 2.0f * (f32)cos(angle);
		if (CheckCarriedObjectContacts(act, &point, 2, 1, TRUE, FALSE, FALSE, NULL)) {
			act->mOwner->mSquirterContent = 0;
			act->mOwner->mItemTintWord    = -1;
			HeldAnim(act->mOwner, 0x8d2);
			HeldAnim(act->mTarget, 17);
		} else {
			point.x += 2.0f * (f32)sin(angle);
			point.z += 2.0f * (f32)cos(angle);
			point.y += 7.0f;
			if (CheckCarriedObjectContacts(act, &point, 2, 3, TRUE, FALSE, FALSE, NULL)) {
				act->mOwner->mSquirterContent = 0;
				act->mOwner->mItemTintWord    = -1;
				HeldAnim(act->mOwner, 0x8c1);
				HeldAnim(act->mTarget, 10);
			}
		}
		if (act->mOwner->mAnimId != 0x8d2 && act->mOwner->mAnimId != 0x8c1) {
			s16 count       = (s16)GetOwnedInventoryItemCount(&act->mOwner->mParams, 0x38);
			s32 chargeLevel = gSquirterChargeByCount[count];
			if (chargeLevel == 3) {
				HeldAnim(act->mOwner, 0x8c2);
				HeldAnim(act->mTarget, 11, TRUE, 1);
			} else if (chargeLevel == 2) {
				HeldAnim(act->mOwner, 0x8c3);
				HeldAnim(act->mTarget, 12, TRUE, 1);
			} else if (chargeLevel == 1) {
				HeldAnim(act->mOwner, 0x8c4);
				HeldAnim(act->mTarget, 13, TRUE, 1);
			} else if (count <= 1) {
				act->mOwner->mSquirterContent = 0;
				act->mOwner->mItemTintWord    = -1;
				HeldAnim(act->mOwner, 0x8c5);
				HeldAnim(act->mTarget, 14);
			}
		}
		act->mTarget->UpdateAnim(frames);
		SetLiftActionState(act, 36);
		act->mCheckToolHit = TRUE;
		break;
	}
	case 36:
		if (aobj->flags & (AOBJ_REWINDED | AOBJ_NO_ANIM)) {
			bool finished = false;
			s16 count     = (s16)(GetOwnedInventoryItemCount(&act->mOwner->mParams, 0x38) - 1);
			if (anim == 0x8c2) {
				UseToolCharge(act, count, 3);
				finished = true;
			} else if (anim == 0x8c3) {
				UseToolCharge(act, count, 2);
				finished = true;
			} else if (anim == 0x8c4) {
				UseToolCharge(act, count, 1);
				finished = true;
			} else if (anim == 0x8c5) {
				HeldAnim(act->mTarget, 1);
				finished = true;
			} else if (anim == 0x8d2) {
				HeldAnim(act->mOwner, 0x8c0);
				HeldAnim(act->mTarget, 9);
			} else if (anim == 0x8c0) {
				HeldAnim(act->mOwner, 0x8d3);
				HeldAnim(act->mTarget, 18);
			} else if (anim == 0x8d3 || anim == 0x8c1) {
				HeldAnim(act->mTarget, 4);
				// filled up, the inventory keeps the count at 10 at most
				SetOrAddInventoryItemCount(&act->mOwner->mParams.mInventory, 0x38, 11, FALSE);
				finished = true;
			}
			act->mTarget->UpdateAnim(frames);
			if (finished) {
				HeldAnim(act->mOwner, 17);
				SetLiftActionState(act, 4);
				if (act->mToolEffect) {
					gGame.mParticle.Kill(act->mToolEffectId);
					act->mToolEffect = NULL;
				}
			}
		} else if (anim >= 0x8c2 && anim <= 0x8c5) {
			if (act->mCheckToolHit) {
				s32 first[]    = { 0, 0 };
				HSD_JObj* root = GetObjectJointByPath(act->mTarget, 2, first);
				s32 second[]   = { 0, 3 };
				HSD_JObj* tip  = GetObjectJointByPath(act->mTarget, 2, second);
				CVec pos(GetJointMatrixTranslation(act->mTarget, root));
				f32 distance;
				{
					CVec delta = pos - GetJointMatrixTranslation(act->mTarget, tip);
					distance   = PSVECMag(&delta);
				}
				if (distance > 0.1f) {
					Vec scale = GetJointMatrixScale(act->mTarget, root);
					Vec hit;
					if (CheckCarriedObjectContacts(act, &pos, scale.x, 0.5f * scale.y, FALSE, TRUE, TRUE, &hit)) {
						act->mCheckToolHit = FALSE;
						if (act->mToolEffect) {
							gGame.mParticle.Kill(act->mToolEffectId);
							act->mToolEffect = NULL;
						}
						hit.x += scale.x * (f32)sin(act->mOwner->mMoveRotate.y);
						hit.z += scale.x * (f32)cos(act->mOwner->mMoveRotate.y);
						CreateParticleAtPosition(&gGame, 1, 0x5a, 1, hit, FALSE);
					}
					if (act->mCheckToolHit && !act->mToolEffect) {
						act->mToolEffectId   = -1;
						HSD_JObj* attachment = NULL;
						if (anim != 0x8c5) {
							attachment         = root;
							act->mToolEffectId = 0x3c;
						} else if (frame > 10) {
							attachment         = tip;
							act->mToolEffectId = 0x42;
						}
						if (act->mToolEffectId >= 0)
							act->mToolEffect = CreateParticleByBankAndId(&gGame, 1, act->mToolEffectId, 0, attachment, TRUE);
						if (act->mToolEffect && anim != 0x8c5)
							psSetUserFunc(act->mToolEffect, &gPlayerItemParticleCallbacks);
					}
				}
			}
		} else {
			ApplyPlayerItemTintToTarget(act);
		}
		break;
	}
}
