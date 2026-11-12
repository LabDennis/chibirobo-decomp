#include "game/xchr.h"
#include "game/xact.h"
#include "game/xaction.h"
#include "game/xobjleticker.h"
#include "game/xobjflags.h"
#include "game/xobjs.h"
#include "game/cgame.h"
#include "game/cgs.h"
#include "game/ccamera.h"
#include "game/cjobjset.h"
#include "game/cparticle.h"
#include "game/cscrn.h"
#include "game/csound.h"
#include "game/fbtexeff.h"
#include "game/globals.h"
#include "game/hsd_dobj.h"
#include "game/stagedata.h"
#include "game/video.h"
#include "game/gmath.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
extern "C" {
#include <dolphin/mtx/mtx.h>
#include <dolphin/mtx/vec.h>
#include <sysdolphin/mtx_transform.h>
}

// chibi robo's actions, the XAct kinds of XAction, and the game helpers they use

extern "C" {
void ConfigureSubwindowCamera(FbSubwin* subwin, const Vec& interest, f32 angleY, f32 angleX, f32 distance, f32 fov);
void SetObjectJointTranslationByPath(CXObj* obj, s32 depth, const s32* path, f32 x, f32 y, f32 z); // puts a joint at a translation
f32 GetJointAnimationEndFrame(const CJObj* obj);                                                   // the end frame of the anim
HSD_JObj* GetObjectJointByPath(const CJObj* obj, s32 depth, s32* path);
CVec GetPlayerJointLocalTranslationByPath(CJObj* obj, s32 depth, s32* path);
void SetJointYawByPath(CXObj* obj, s32 depth, s32* path, f32 angle);
void SetJointTranslationFromVectorByPath(CJObj* obj, s32 depth, s32* path, const Vec& pos);
void HSD_JObjSetRotationY__FP9_HSD_JObjf(HSD_JObj* jobj, f32 angle);
void RecordObjectPositionAndHeightChange(XObj* obj);
void ApplyPlayerJumpStrength(XActJump* act, f32 strength);
void TryLeaveHang(XActHang* act);
Vec GetLiftTargetContactOffset(XActLift* act, XObj* target);
Vec GetTargetFloorPosition(XAction* act); // where the held floor of the target is
u32 CheckStageFlag(s32 stage, u32 mask);
s32 GetObjectPoolId(const XObj*);
s16 GetObjectItemId(XObj*);
s16 fn_8006E364(s32);
f32 GetHeadingBetweenPositions(const Vec*, const Vec*);
void SetPlugCordSpeed(XObjPlug*, f32);
void SetLiftActionState(XActLift*, s32);
void fn_800B503C(XActLift*, s32);
XObj* GetGameObjectByIndex(const CGame*, s32);
void TryStartObjectEvent(CGame*, const void*, void*, s16, s16);
BOOL IsEventScriptActive(const CGame*);
Params* GetPlayerParamsByIndex(CGame*, s32);
s32 GetInventoryCountByIndex(Params*, u32);
void fn_800D4294(XObjChr*, s32, f32);
void SetClampedPlayerBattery(Params*, f32);
HSD_Generator* CreateParticleByBankAndId(CGame*, s32, s32, u8, HSD_JObj*, BOOL);
XObj* SpawnObjectFromPlacementIndex(CGame*, s16);
BOOL fn_800D3CDC(XObj*);
void ClearObjectStatusFlags(XObj*, u64);
void SetPlayerActionTarget(XAction*, XObj*);
s32 RemovePlayerInventoryItemCount(Params*, s16, s32);
void PutAwayLiftedObject(XActLift*);
Vec GetJointMatrixTranslation(void*, HSD_JObj*);
Vec GetJointMatrixScale(void*, HSD_JObj*);
HSD_Generator* CreateParticleAtPosition(CGame*, u32, u32, s32, const Vec&, BOOL);
struct HSD_PSUserFunc;
extern HSD_PSUserFunc lbl_80228270;
void psSetUserFunc(HSD_Generator*, HSD_PSUserFunc*);
void fn_800D3EB0(CGame*, f32, f32, f32, f32);
void SetSubwindowCamera(CGame*, const Vec&, f32, f32, f32, f32);
void EnableSubwindowWithFlags(CGame*, u32);
Vec GetObjectJointWorldPosition(const CJObj*);
BOOL IsLiftActionState(const XActLift*, s32);
f32 GetObjectJointRotationY(CJObj*);
void ClearObjectAnimationQueue(XObj*);
Vec GetObjectJointMatrixScale(const CJObj*);
HSD_Generator* CreateScaledParticleAtPosition(CGame*, u32, u32, s32, const Vec&, const Vec&, BOOL);

extern const s32 lbl_8020A5E8[];
f32 GetJointAnimationCurrentFrame(CJObj*);
BOOL CheckCarriedObjectContacts(XActLift*, const Vec*, f32, f32, BOOL, BOOL, BOOL, Vec*);
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
	act->mPendingItem = act->_48 >= 0 ? SpawnObjectFromPlacementIndex(&gGame, act->_48) : NULL;
	SetLiftActionState(act, next);
}
static inline void TakeLiftItem(XActLift* act, s32 anim, HSD_AObj* aobj, bool useCount)
{
	s32 wanted = useCount ? 11 : 0xec;
	if (anim != wanted) {
		HeldAnim(act->mOwner, wanted, TRUE, 1);
		fn_800B503C(act, useCount ? 3 : 18);
		if (IsLiftTargetPlug(act)) {
			ClearObjectStatusFlags(act->mTarget, 0x0400000000000000ULL);
			act->mTarget = NULL;
		}
		if (act->_48 >= 0 && act->mPendingItem) {
			act->mPendingItem->mStatus.mFlags |= 0x0400000000000000ULL;
			act->mPendingItem->SetScale(0, 0);
			SetPlayerActionTarget(act, act->mPendingItem);
			if (useCount) {
				s16 id         = GetObjectItemId(act->mTarget);
				s32 count      = RemovePlayerInventoryItemCount(GetPlayerParamsByIndex(&gGame, 0), id, 1);
				s32 difference = count - GetStageCollectionFlagBase(0);
				if (difference > 0)
					act->mPendingItem->_260 = difference;
			}
			act->_48 = -1;
			if (useCount && IsCarryingSquirter(act))
				ApplyPlayerItemTintToTarget(act);
		}
	} else if (aobj->flags & 0x44000000) {
		HeldAnim(act->mOwner, 8);
		SetLiftActionState(act, 4);
		fn_800B503C(act, useCount ? 4 : 19);
	}
}
static inline void ReleasePlugTarget(XActLift* act, s32 anim, HSD_AObj* aobj, s32 next)
{
	if (anim != next) {
		HeldAnim(act->mOwner, next, FALSE, 1);
		act->mOwner->UpdateAnim(0);
		fn_800B503C(act, 13);
	} else if (aobj->flags & 0x44000000) {
		HeldAnim(act->mOwner, 8);
		SetLiftActionState(act, 4);
		act->mPlugTarget = NULL;
	}
}
static inline void UseToolCharge(XActLift* act, s16 count, s32 level)
{
	if (lbl_8020A5E8[count] == level)
		HeldAnim(act->mTarget, level + 1);
	else
		HeldAnim(act->mTarget, level);
	SetOrAddInventoryItemCount(&act->mOwner->mParams.mInventory, 0x38, count, FALSE);
}

void UpdateLiftActionAnimation(XActLift* act, s32 anim, HSD_AObj* aobj, u32 frames, f32 frame, f32)
{
	s32 placement     = act->mTarget ? act->mTarget->mCoord._00 - GetStagePlacementByIndex(0) : -1;
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
		if (!(aobj->flags & 0x44000000))
			return;
		HeldAnim(act->mOwner, 6, TRUE, 1);
		SetLiftActionState(act, 2);
		if (placement < 0) {
			SetPlugCordSpeed(&act->mOwner->mPlug, 0.5f);
			act->_2C = 0;
			PlayGameSound(&gGame.mSound, 0x3a, 255);
		} else if ((s16)GetObjectPoolId(act->mTarget) == 8) {
			act->_2C = FRAME_RATE * ((f32)fn_8006E364(GetObjectItemId(act->mTarget)) / 100.0f);
		} else {
			act->_2C = 0.5f * FRAME_RATE;
		}
		// fall through
	case 2:
		act->_2C -= frames;
		if (!(act->_2C <= 0))
			return;
		SetLiftActionState(act, 3);
		// fall through
	case 3:
		if (anim != 7) {
			if (!act->mTarget || (s16)GetObjectPoolId(act->mTarget) != 8 || HasRoomForItem(act->mTarget)) {
				fn_800B503C(act, 1);
				HeldAnim(act->mOwner, 7, TRUE, 1);
			} else {
				SetLiftActionState(act, 5);
			}
			if (act->mTarget && act->mTarget->vf128() && (act->mTarget->mStatus.mFlags & 0x10000000000ULL)) {
				XObj* target = act->mTarget;
				TryStartObjectEvent(&gGame, GetGameObjectByIndex(&gGame, placement)->vf128(), target, -1, -1);
			}
		} else if (aobj->flags & 0x44000000) {
			HeldAnim(act->mOwner, 8);
			if (act->mTarget) {
				if (act->mTarget->vf128())
					SetLiftActionState(act, 5);
				else if ((s16)GetObjectPoolId(act->mTarget) == 8 && !(act->mTarget->mStatus.mFlags & 0x400000000000ULL))
					SetLiftActionState(act, 19);
				else
					SetLiftActionState(act, 4);
				fn_800B503C(act, 4);
			}
		}
		break;
	case 4:
		if (anim == 0x895) {
			if (aobj->flags & 0x44000000) {
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
			if (aobj->flags & 0x44000000)
				HeldAnim(act->mOwner, 0x894);
		}
		if (IsCarryingToothbrush(act)) {
			if (anim != 0x895 && act->mContactEffect) {
				gGame.mParticle.Kill(act->mContactEffectId);
				act->mContactEffect = NULL;
			}
		} else if (IsCarryingSquirter(act) && (u32)(anim - 0x8c2) > 2 && act->mToolEffect) {
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
					fn_800D4294(act->mOwner, 17, 0);
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
			fn_800B503C(act, 5);
		} else if (aobj->flags & 0x44000000) {
			fn_800D4294(act->mOwner, 17, 0);
			SetLiftActionState(act, 0);
		}
		break;
	case 7:
	case 8:
	case 9:
		if (anim != 0x9f && anim != 0xa0 && anim != 0xbc) {
			SetPlayerActionTarget(act, &act->mOwner->mPlug);
			XObjPlug* plug = (XObjPlug*)act->mTarget;
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
				fn_800B503C(act, 6);
			} else if (IsLiftActionState(act, 8)) {
				HeldAnim(act->mOwner, 0xa0);
				fn_800B503C(act, 8);
			} else {
				HeldAnim(act->mOwner, 0xbc);
				fn_800B503C(act, 11);
			}
		} else if (aobj->flags & 0x44000000) {
			if (IsLiftActionState(act, 7)) {
				fn_800D4294(act->mOwner, 17, 0);
				SetLiftActionState(act, 0);
				if (act->mPlugTarget)
					act->mPlugTarget->vf134(0);
			} else if (IsLiftActionState(act, 8)) {
				SetLiftActionState(act, 12);
				act->_2C = 0.2f * FRAME_RATE;
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
			fn_800B503C(act, 7);
			act->mYawOffset = 0;
		} else if (aobj->flags & 0x44000000) {
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
		act->_2C -= frames;
		if (act->_2C <= 0) {
			act->_2C = 0;
			if (act->mPlugTarget && !act->mPlugTarget->vf134(0)) {
				SetLiftActionState(act, 14);
				break;
			}
			HeldAnim(act->mOwner, 0xab);
			SetLiftActionState(act, 13);
			if (act->mPlugTarget && !GetObjectItemId(act->mPlugTarget)) {
				fn_800D3EB0(&gGame, NAN, NAN, NAN, NAN);
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
			if (!act->mPlugTarget->vf134(act->mBatteryCost)) {
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
			fn_800B503C(act, 10);
			gGame.mSubwin.mEnabled = FALSE;
			if (act->mPlugTarget && GetObjData(&act->mPlugTarget->mCoord)->_14 == 9)
				((XObjLeticker*)act->mPlugTarget)->vf148();
		} else if (aobj->flags & 0x44000000) {
			HeldAnim(act->mOwner, 8);
			SetLiftActionState(act, 4);
			act->mPlugTarget = NULL;
		}
		break;
	case 16: {
		XObjPlug* plug = (XObjPlug*)act->mTarget;
		if (anim != 0xbf) {
			HeldAnim(act->mOwner, 0xbf, FALSE, 1);
			if (act->mPlugTarget)
				HeldAnim(act->mPlugTarget, 1, FALSE, 1);
		} else if (aobj->flags & 0x44000000) {
			HeldAnim(act->mOwner, 8);
			SetLiftActionState(act, 4);
			fn_800B503C(act, 4);
			act->mPlugTarget = NULL;
			ResetPlayerPlugState((XObjPlug*)act->mTarget, TRUE);
		} else if (frame > 142.0f) {
			plug->_D3C = 0;
		} else if (frame > 85.0f) {
			if (plug->_D3C != -1) {
				f32 remaining = act->mOwner->mParams.mBattery - 10.0f;
				if (remaining < 0)
					remaining = 0;
				SetClampedPlayerBattery(&act->mOwner->mParams, remaining);
			}
			((XObjPlug*)act->mTarget)->_D3C = -1;
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
			fn_800B503C(act, 14);
			if (act->mContactEffect) {
				gGame.mParticle.Kill(act->mContactEffectId);
				act->mContactEffect = NULL;
			}
			if (act->mToolEffect) {
				gGame.mParticle.Kill(act->mToolEffectId);
				// This path clears the first effect pointer again in all three editions.
				act->mContactEffect = NULL;
			}
		} else if (aobj->flags & 0x44000000) {
			SetLiftActionState(act, 0);
			fn_800B503C(act, 15);
			fn_800D4294(act->mOwner, 17, 0);
			if (act->mTarget && (s16)GetObjectPoolId(act->mTarget) == 8) {
				s8 sound = (s8)GetItemSoundIndex(GetObjectItemId(act->mTarget), 1);
				if (sound >= 0)
					PlayGameSound(&gGame.mSound, GetItemReleaseSoundId(sound), 255);
			}
		}
		break;
	case 20:
		LoadLiftItem(act, 21);
		// fall through
	case 21:
		if (act->mPendingItem) {
			if (!fn_800D3CDC(act->mPendingItem))
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
			fn_800B503C(act, 16);
		} else if (aobj->flags & 0x44000000) {
			fn_800D4294(act->mOwner, 17, 0);
			SetLiftActionState(act, 0);
			fn_800B503C(act, 17);
		}
		break;
	case 24:
		LoadLiftItem(act, 25);
		// fall through
	case 25:
		if (act->mPendingItem) {
			if (!fn_800D3CDC(act->mPendingItem))
				return;
			gGame.mStatus.mFlags |= 0x1000;
		}
		SetLiftActionState(act, 26);
		// fall through
	case 26:
		TakeLiftItem(act, anim, aobj, false);
		break;
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
		if (aobj->flags & 0x44000000) {
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
		} else if (aobj->flags & 0x44000000) {
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
		} else if (aobj->flags & 0x44000000) {
			HeldAnim(act->mOwner, 8);
			SetLiftActionState(act, 4);
		}
		break;
	case 34:
		PutAwayLiftedObject(act);
		break;
	case 35: {
		Vec point = act->mOwner->mMoveTranslate;
		f32 angle = act->mOwner->mMoveRotate.y;
		point.x += 2.0f * (f32)sin(angle);
		point.z += 2.0f * (f32)cos(angle);
		if (CheckCarriedObjectContacts(act, &point, 2, 1, TRUE, FALSE, FALSE, NULL)) {
			act->mOwner->_158A8        = 0;
			act->mOwner->mItemTintWord = -1;
			HeldAnim(act->mOwner, 0x8d2);
			HeldAnim(act->mTarget, 17);
		} else {
			point.x += 2.0f * (f32)sin(angle);
			point.z += 2.0f * (f32)cos(angle);
			point.y += 7.0f;
			if (CheckCarriedObjectContacts(act, &point, 2, 3, TRUE, FALSE, FALSE, NULL)) {
				act->mOwner->_158A8        = 0;
				act->mOwner->mItemTintWord = -1;
				HeldAnim(act->mOwner, 0x8c1);
				HeldAnim(act->mTarget, 10);
			}
		}
		if (act->mOwner->mAnimId != 0x8d2 && act->mOwner->mAnimId != 0x8c1) {
			s16 count       = (s16)GetOwnedInventoryItemCount(&act->mOwner->mParams, 0x38);
			s32 chargeLevel = lbl_8020A5E8[count];
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
				act->mOwner->_158A8        = 0;
				act->mOwner->mItemTintWord = -1;
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
		if (aobj->flags & 0x44000000) {
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
							psSetUserFunc(act->mToolEffect, &lbl_80228270);
					}
				}
			}
		} else {
			ApplyPlayerItemTintToTarget(act);
		}
		break;
	}
}
}

#include "game/cjobjinlineattach.h"

// the part of a particle the item tint callback writes
struct ItemPtclColor {
	u8 _00[0x12]; // _00
	u8 mRed;      // _12
	u8 mGreen;    // _13
	u8 mBlue;     // _14
};

// a += b, b by value like the other CVec operators
static inline void AddVec(CVec& a, Vec b)
{
	PSVECAdd(&a, &b, &a);
}

// a held object of the given pool and kind
static inline bool IsHolding(XAction* act)
{
	bool result = false;
	if (act->mTarget != NULL && (act->mTarget->mStatus.mFlags & 0x400000000000ULL)) {
		result = true;
	}
	return result;
}

static inline bool IsHoldingPool(XAction* act, s16 pool)
{
	bool result = false;
	if (IsHolding(act) && act->mTarget->GetCategory() == pool) {
		result = true;
	}
	return result;
}

static inline bool IsHoldingKind(XAction* act, s16 pool, s16 kind)
{
	bool result = false;
	if (IsHoldingPool(act, pool) && act->mTarget->GetItem() == kind) {
		result = true;
	}
	return result;
}

// holding the item and not throwing it
static inline bool IsCarrying(XAction* act, s16 kind)
{
	bool result = false;
	if (IsHoldingKind(act, 8, kind) && act->_24 != 15) {
		result = true;
	}
	return result;
}

static inline bool IsTargetPool(XAction* act, s16 pool)
{
	bool result = false;
	if (act->mTarget != NULL && act->mTarget->GetCategory() == pool) {
		result = true;
	}
	return result;
}

static inline bool IsStill(XAction* act)
{
	bool result = false;
	if (act->mState == 31 || act->mState == 32 || act->mState == 33) {
		result = true;
	}
	return result;
}

static inline bool IsCarryingStill(XAction* act, s16 kind)
{
	bool result = false;
	if (IsCarrying(act, kind) && IsStill(act)) {
		result = true;
	}
	return result;
}

// lets go of a pushed object and resets
static inline void Release(XAction* act)
{
	XObj* target = act->mTarget;
	if (target != NULL && (target->mStatus.mFlags & 0x0800000000000000ULL)) {
		target->mStatus.mFlags &= ~0x0800000000000000ULL;
	}
	act->Reset();
}

static inline Vec UnitScale()
{
	Vec scale = { 1.0f, 1.0f, 1.0f };
	return scale;
}

// whether his hands are up at the top of the ladder
static inline bool AtLadderTop(XActLadder* act)
{
	if (act->mTarget != NULL) {
		s32 path[] = { 0 };
		if (14.0f + GetEventJointWorldPosition(act->mOwner, 1, path).y >= act->mTarget->mFloorHeights[0]) {
			return true;
		}
	}
	return false;
}

// drops the item he's carrying, it goes in his inventory
static inline void PutAway(XActLift* act)
{
	XObjChr* chr = act->mOwner;
	if (chr->mAnimId == 0x8B1 || chr->mAnimId == 0x8B3) {
		chr->SetAnim(0x8B2, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
	} else if (chr->mAnimId == 11) {
		chr->SetAnim(8, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
	}
	act->mState              = 0;
	act->mOwner->mAttachment = 0;
	act->mTarget->mStatus.mFlags &= ~0x0200000000000000ULL;
	act->mTarget->SetAnim(7, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
	act->_24       = 20;
	XObjItem* item = (XObjItem*)act->mTarget;
	SetOrAddInventoryItemCount(&act->mOwner->mParams.mInventory, item->GetItem(), 1, TRUE);
	item->SetFlag();
	PlayGameSound(&gGame.mSound, 0x39, 0xFF);
}

extern "C" Vec lbl_8037C340;

static inline Vec GetFloorExtent(const XObjFloor* floor)
{
	Vec value = lbl_8037C340;
	value.x   = floor->mExtent[0] / 100.0f;
	value.y   = floor->mExtent[1] / 100.0f;
	value.z   = floor->mExtent[2] / 100.0f;
	return value;
}

extern "C" {

BOOL SetClampedPushPositionXZ(XObj* target, const Vec* direction, u8 axes, f32 x, f32 z);
f32 QueryObjectFloorHeight(XObj* target, f32 radius, f32 height);

static const f32 pushAngleTolerance = 0.1f * DEG2RAD;

static inline Vec PushEdgePoint(const XObj* obj, s32 index, bool first, f32 height)
{
	const Vec& point = first ? obj->mEdges[index].mFrom : obj->mEdges[index].mTo;
	Vec v            = { obj->mMoveTranslate.x + point.x, height, obj->mMoveTranslate.z + point.z };
	return v;
}
static inline f32 FacingPushEdge(XObj* edgeObject, XObjChr* owner)
{
	s32 index   = edgeObject->mSelectedFloorIndex;
	Vec& player = owner->mMoveTranslate;
	Vec a       = PushEdgePoint(edgeObject, index, true, player.y);
	Vec b       = PushEdgePoint(edgeObject, index, false, player.y);
	CVec normal;
	GetOutwardPerpendicularToEdge(&player, &a, &b, &normal);
	return WrapAngle(atan2(normal.x, normal.z));
}
void UpdatePushedObject(XActPush* act)
{
	if (!act->_24)
		return;
	XObj* target = act->mTarget;
	if (!target || !(target->mStatus.mFlags & 1))
		return;
	switch (act->_24) {
	case 1: {
		target->mStatus.mFlags |= 0x0800000000000000ULL;
		Vec hand         = GetPlayerCarryJointWorldPosition(act->mOwner);
		const CVec& diff = target->mMoveTranslate - hand;
		act->_10         = CVec(diff.x, diff.y, diff.z);
		break;
	}
	case 2: {
		Vec hand         = GetPlayerCarryJointWorldPosition(act->mOwner);
		XObj* edgeObject = act->mTarget;
		f32 angle        = WrapAngle(FacingPushEdge(edgeObject, act->mOwner) - edgeObject->mMoveRotate.y);
		Mtx rotate;
		PSMTXRotRad(rotate, 'y', angle);
		CVec direction(0, 0, act->_0C);
		PSMTXMultVec(rotate, &direction, &direction);
		if ((angle > (90 * DEG2RAD - pushAngleTolerance) && angle < (90 * DEG2RAD + pushAngleTolerance))
		    || (angle > (270 * DEG2RAD - pushAngleTolerance) && angle < (270 * DEG2RAD + pushAngleTolerance)))
			act->mPushPositionClamped = SetClampedPushPositionXZ(target, &direction, 1, hand.x + act->_10.x, hand.z + act->_10.z);
		else
			act->mPushPositionClamped = SetClampedPushPositionXZ(target, &direction, 2, hand.x + act->_10.x, hand.z + act->_10.z);

		target->OnPushDisplacement(target->mMoveTranslate - target->_264);
		if ((target->mStatus.mFlags & 0x4000000000ULL) && QueryObjectFloorHeight(target, 0.1f, 0.1f) < target->mMoveTranslate.y - 5.0f) {
			target->mStatus.mFlags &= ~0x0800000000000000ULL;
			Release(act);
			XObjChr* owner = act->mOwner;
			owner->SetAnim(17, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
			owner->_A2C = 3;
			act->mState = 0;
			target->mStatus.mFlags |= 0x1000000000000000ULL;

			Vec rotation = target->mMoveRotate, speed = target->mMoveRotate.mSpeed;
			Vec extent = GetFloorExtent(target->mFloors);
			f32 dx, dz;
			if ((angle > (90 * DEG2RAD - pushAngleTolerance) && angle < (90 * DEG2RAD + pushAngleTolerance))) {
				rotation.z -= HALF_PI;
				speed.z = 0.1f;
				dz      = 0;
				dx      = extent.x * (2.0f / sqrtf(3.0f)) * 0.5f;
			} else if ((angle > (270 * DEG2RAD - pushAngleTolerance) && angle < (270 * DEG2RAD + pushAngleTolerance))) {
				rotation.z += HALF_PI;
				speed.z = 0.1f;
				dz      = 0;
				dx      = -(extent.x * (2.0f / sqrtf(3.0f))) * 0.5f;
			} else if ((angle > (0 * DEG2RAD - pushAngleTolerance) && angle < (0 * DEG2RAD + pushAngleTolerance))) {
				rotation.x += HALF_PI;
				speed.x = 0.1f;
				dx      = 0;
				dz      = extent.z * (2.0f / sqrtf(3.0f)) * 0.5f;
			} else if ((angle > (180 * DEG2RAD - pushAngleTolerance) && angle < (180 * DEG2RAD + pushAngleTolerance))) {
				rotation.x -= HALF_PI;
				speed.x = 0.1f;
				dx      = 0;
				dz      = -(extent.z * (2.0f / sqrtf(3.0f))) * 0.5f;
			} else {
				dx = dz = 0;
			}
			target->mMoveRotate.mTarget.Set(rotation);
			target->mMoveRotate.mSpeed.Set(speed);
			target->_794 = CVec(dx, -(extent.y * (2.0f / sqrtf(3.0f))) * 0.5f, dz);
		}
		break;
	}
	}
}

/**
 * @note Address: 0x800C5270
 * @note Size: 0x464
 */
void UpdatePushActionAnimation(XActPush* act, s32 anim, HSD_AObj* aobj, u32 frames, f32 frame, f32 end)
{
	switch (act->mState) {
	case 1:
		if (anim != 0x3E) {
			if (act->mTarget != NULL) {
				f32 angle = GetFloorFacingAngle(act->mTarget, &act->mOwner->mMoveTranslate, act->mTargetFloor, 1, 0, 0);
				if (fpclassify(angle) != FP_NAN) {
					act->mOwner->SetAnim(0x3E, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
					act->mOwner->SetRotationY(angle, 0);
				} else {
					Release(act);
					act->mState = 0;
				}
			}
		} else if (aobj->flags & 0x44000000) {
			act->mOwner->SetAnim(0x3F, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
			act->mState = 2;
			act->_24    = 1;
		}
		break;
	case 2:
		if (act->_0C == 1) {
			if (anim == 0x4B) {
				if (aobj->flags & 0x44000000) {
					act->mOwner->SetAnim(0x40, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
					act->_24 = 2;
				}
			} else if (anim != 0x40) {
				act->mOwner->SetAnim(0x4B, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				act->_24 = 0;
			}
		} else if (act->_0C == -1) {
			if (anim == 0x4C) {
				if (aobj->flags & 0x44000000) {
					act->mOwner->SetAnim(0x41, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
					act->_24 = 2;
				}
			} else if (anim != 0x41) {
				act->mOwner->SetAnim(0x4C, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				act->_24 = 0;
			}
		} else if (anim != 0x3F) {
			act->mOwner->SetAnim(0x3F, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			act->_24 = 0;
		}
		break;
	case 3:
		if (anim != 0x42) {
			act->mOwner->SetAnim(0x42, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
		} else if (aobj->flags & 0x44000000) {
			XObjChr* chr = act->mOwner;
			chr->SetAnim(0x11, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
			chr->_A2C   = 3;
			act->mState = 0;
		}
		break;
	}
}

// lets go of what he's pushing once a is let go, japan uses b while the game flag is up
/**
 * @note Address: 0x800C56D4
 * @note Size: 0x248
 */
void HandlePushActionInput(XActPush* act, const CPadOne* pad, f32* weight)
{
	if (act->mState == 2) {
		if (gGame.IsRunning() && gGame.mEvt.mData.mScript == NULL) {
#if defined(VERSION_JP)
			if (gGame.mStatus.mFlags & 0x0000080000000000ULL) {
				if (pad->mTrigger & 0x200) {
					Release(act);
					act->mState = 3;
					*weight     = 0.0f;
				}
			} else
#endif
			    if (!(pad->mButton & 0x100)) {
				Release(act);
				act->mState = 3;
				*weight     = 0.0f;
			}
		}
	} else if (act->mState != 0) {
		if (act->mState != 3 && !(pad->mButton & 0x100)
#if defined(VERSION_JP)
		    && !(gGame.mStatus.mFlags & 0x0000080000000000ULL)
#endif
		) {
			Release(act);
			XObjChr* chr = act->mOwner;
			chr->SetAnim(17, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			chr->_A2C   = 3;
			act->mState = 0;
		}
		*weight = 0.0f;
	}
}

/**
 * @note Address: 0x800C591C
 * @note Size: 0xE4
 */
void CancelPlayerPushAction(XActPush* act)
{
	if (act->mState != 0) {
		Release(act);
		act->mState  = 0;
		XObjChr* chr = act->mOwner;
		chr->SetAnim(0x11, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
		chr->_A2C = 3;
	}
}

/**
 * @note Address: 0x800C5A00
 * @note Size: 0x7C
 */
void ReleasePushAction(XActPush* act)
{
	Release(act);
}

void PositionPlayerAroundRopeTarget(XActRope*, f32);

static inline void RopeAnim(XActRope* act, s32 id, f32 loop, BOOL force = FALSE, f32 start = 0)
{
	act->mOwner->SetAnim(id, force, NULL, NULL, NULL, start, 1, loop, 10);
}
static inline void LocateRope(XActRope* act, bool grab)
{
	CVec player(act->mOwner->mMoveTranslate);
	player.y = 0;
	CVec rope(act->mTarget->mMoveTranslate);
	rope.y = 0;
	CVec offset;
	if (grab) {
		offset.Set(act->_10);
		offset.y = 0;
	} else
		offset.Set(0, 0, -.5f);
	Mtx matrix;
	PSMTXRotRad(matrix, 'y', grab ? act->mOwner->mMoveRotate.y : act->mTarget->mMoveRotate.y);
	PSMTXMultVec(matrix, &offset, &offset);
	if (grab) {
		Vec delta = offset;
		PSVECSubtract(&rope, &delta, &rope);
	} else
		AddVec(rope, offset);
	act->mPos   = rope;
	act->mDist  = PSVECDistance(&rope, &player);
	act->mAngle = atan2(player.x - rope.x, player.z - rope.z);
}
static inline void ApproachRope(XActRope* act, u32 frames, bool move, bool snap, f32 speed)
{
	if (!act->mTarget)
		return;
	Vec delta = act->mTarget->mMoveTranslate - act->mTarget->_264;
	act->mPos.x += delta.x;
	act->mPos.z += delta.z;
	if (act->mDist > 0 && move) {
		if ((act->mDist -= speed * frames) < 0)
			act->mDist = 0;
		act->mOwner->SetTranslateX(act->mDist * (f32)sin(act->mAngle) + act->mPos.x, 0);
		act->mOwner->SetTranslateZ(act->mDist * (f32)cos(act->mAngle) + act->mPos.z, 0);
	} else if (snap) {
		act->mOwner->SetTranslateX(act->mPos.x, 0);
		act->mOwner->SetTranslateZ(act->mPos.z, 0);
	}
}
static inline void ReleaseRopePlug(XActRope* act, f32 frame, f32 threshold)
{
	if (CheckStageFlag(gGame.mStage, 1)) {
		XObjPlug* plug = &act->mOwner->mPlug;
		if (plug->_D3C && frame >= threshold) {
			ResetPlayerPlugState(plug, TRUE);
			PlayGameSound(&gGame.mSound, 0x37, 0xff);
		}
	}
}
static inline void SyncRopePosition(XObjChr* owner)
{
	if (owner->mMoveTranslate.y != owner->_264.y)
		owner->mStatus.mFlags |= 0x0010000000000000ULL;
	owner->_264.Set(owner->mMoveTranslate);
}

/**
 * @note Address: 0x800C5B98
 * @note Size: 0x1C78 (US)
 */
void UpdatePlayerRopeAction(XActRope* act, s32 anim, HSD_AObj* aobj, u32 frames, f32 frame, f32)
{
	switch (act->mState) {
	case 1:
		if (anim != 0x21) {
			RopeAnim(act, 0x21, 0);
			if (act->mTarget) {
				act->mOwner->SetRotationY(WrapAngle(atan2(act->mTarget->mMoveTranslate.x - act->mOwner->mMoveTranslate.x,
				                                          act->mTarget->mMoveTranslate.z - act->mOwner->mMoveTranslate.z)),
				                          0);
				LocateRope(act, true);
			}
		} else if (aobj->flags & 0x44000000) {
			RopeAnim(act, 0x8c, 0);
			act->mState = 5;
		} else {
			ApproachRope(act, frames, frame > 20, true, .2f);
			ReleaseRopePlug(act, frame, 28);
		}
		break;
	case 2:
		if (anim != 0x98) {
			RopeAnim(act, 0x98, 0);
			if (act->mTarget) {
				act->mOwner->SetRotationY(WrapAngle(act->mTarget->mMoveRotate.y), 0);
				LocateRope(act, false);
			}
		} else if (aobj->flags & 0x44000000) {
			s32 path[] = { 0 };
			CVec pos   = GetEventJointWorldPosition(act->mOwner, 1, path);
			Vec offset = act->_10;
			Mtx matrix;
			PSMTXRotRad(matrix, 'y', act->mTarget->mMoveRotate.y);
			PSMTXMultVec(matrix, &offset, &offset);
			act->mOwner->SetPos(pos.x + offset.x, pos.y - offset.y, pos.z + offset.z, 0);
			SyncRopePosition(act->mOwner);
			act->mOwner->SetRotationY(PI + act->mOwner->mMoveRotate.y, 0);
			s32 p2[] = { 0 };
			SetJointYawByPath(act->mOwner, 1, p2, 0);
			s32 p3[] = { 0 };
			SetJointTranslationFromVectorByPath(act->mOwner, 1, p3, act->_10);
			RopeAnim(act, 0x8c, 0);
			act->mState = 5;
		} else {
			ApproachRope(act, frames, frame > 80, false, .2f);
			ReleaseRopePlug(act, frame, 80);
		}
		break;
	case 3:
		if (anim != 0x85) {
			if (act->mTarget) {
				act->mOwner->SetRotationY(act->mTarget->mMoveRotate.y, 0);
				RopeAnim(act, 0x8a, 1, TRUE);
				act->mOwner->UpdateAnim(0);
				s32 p1[]   = { 0 };
				Vec local  = GetPlayerJointLocalTranslationByPath(act->mOwner, 1, p1);
				s32 p2[]   = { 0 };
				CVec delta = GetEventJointWorldPosition(act->mOwner, 1, p2) - act->mOwner->mMoveTranslate;
				act->mOwner->SetTranslateY(act->mOwner->mMoveTranslate.y - delta.y, 0);
				RopeAnim(act, 0x85, 1);
				LocateRope(act, false);
			}
			ReleaseRopePlug(act, 0, 0);
		} else if (aobj->flags & 0x44000000) {
			s32 p1[]        = { 0 };
			HSD_JObj* child = GetJointByPath(act->mOwner->mJObj, p1, 1);
			f32 turn        = HSD_JObjGetRotationY(child);
			act->mOwner->SetRotationY(HSD_JObjGetRotationY(act->mOwner->mJObj) + turn, 0);
			HSD_JObjSetRotationY(child, 0);
			s32 p2[]   = { 0 };
			CVec delta = GetEventJointWorldPosition(act->mOwner, 1, p2) - act->mOwner->mMoveTranslate;
			delta.y    = 0;
			CVec offset(act->_10);
			Mtx matrix;
			PSMTXRotRad(matrix, 'y', HSD_JObjGetRotationY(act->mOwner->mJObj));
			PSMTXMultVec(matrix, &offset, &offset);
			offset.y = 0;
			act->mOwner->SetPos(act->mOwner->mMoveTranslate - (delta + offset), 0);
			SyncRopePosition(act->mOwner);
			s32 p3[] = { 0 };
			SetJointTranslationFromVectorByPath(act->mOwner, 1, p3, act->_10);
			RopeAnim(act, 0x8c, 0);
			act->mState = 5;
		} else
			ApproachRope(act, frames, true, true, .4f);
		break;
	case 4:
		RopeAnim(act, 0x8c, 1);
		act->mState = 5;
		break;
	case 5:
		if (anim == 0x8b) {
			if (aobj->flags & 0x44000000) {
				if (act->_0C == 1)
					RopeAnim(act, 0x8d, 1);
				else
					RopeAnim(act, 0x8c, 1);
			}
		} else if (anim == 0x8d) {
			if (aobj->flags & 0x44000000) {
				if (act->_0C == 1)
					RopeAnim(act, 0x8b, 1);
				else
					RopeAnim(act, 0x8a, 1);
			}
		} else if (act->_0C == 1 || act->_0C == 2) {
			if (anim == 0x8a)
				RopeAnim(act, 0x8b, 1);
			else if (anim == 0x8c)
				RopeAnim(act, 0x8d, 1);
#if defined(VERSION_PAL)
			else if (anim == 0x23)
				RopeAnim(act, 0x8c, 0);
#endif
		} else if (act->_0C == -1) {
			if (act->mOwner->GetHeight() > 0) {
				if (anim != 0x23)
					RopeAnim(act, 0x23, 0);
			} else {
				act->mState = 8;
				RopeAnim(act, 0x25, 0);
			}
		} else if (anim == 0x23)
			RopeAnim(act, 0x8c, 0);
		if (act->_0C == 1 || act->_0C == 2) {
			s32 path[] = { 0, 0 };
			if (act->mTarget && GetEventJointWorldPosition(act->mOwner, 2, path).y >= act->mTarget->mFloorHeights[0]) {
				act->mOwner->SetTranslateY(act->mTarget->mFloorHeights[0] - act->_10.y, 0);
				RopeAnim(act, 0x24, 0);
				act->mState = 6;
				break;
			}
			PlayGameSound(&gGame.mSound, 0x11, 0xff);
		} else if (act->_0C == -1)
			PlayGameSound(&gGame.mSound, 0x15, 0xff);
		if (act->mTarget) {
			f32 target = WrapAngle(PI + act->mTarget->mMoveRotate.y);
			f32 angle  = act->mOwner->mMoveRotate.y;
			if (WrapAngle(target - angle) >= PI) {
				angle -= DEG2RAD * 2.f * frames;
				if (WrapAngle(target - angle) < PI)
					angle = target;
			} else {
				angle += DEG2RAD * 2.f * frames;
				if (WrapAngle(target - angle) > PI)
					angle = target;
			}
			if (anim == 0x34 || anim == 0x8b || anim == 0x8d || anim == 0x23)
				act->mOwner->SetRotationY(angle, 0);
			PositionPlayerAroundRopeTarget(act, angle);
		}
		break;
	case 6:
		if (anim != 0x24)
			RopeAnim(act, 0x24, 0);
		else if (aobj->flags & 0x44000000) {
			s32 p[] = { 0 };
			act->mOwner->SetPos(GetEventJointWorldPosition(act->mOwner, 1, p), 0);
			SyncRopePosition(act->mOwner);
			s32 p2[] = { 0 };
			SetObjectJointTranslationByPath(act->mOwner, 1, p2, 0, 0, 0);
			XObjChr* owner = act->mOwner;
			owner->SetAnim(0x11, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
			owner->_A2C  = 3;
			act->mState  = 0;
			act->mTarget = NULL;
		}
		break;
	case 7:
		RopeAnim(act, 0x25, 1, TRUE, 20);
		act->mState = 8;
	// fall through
	case 8:
		if (anim != 0x25)
			RopeAnim(act, 0x25, 0);
		else if (aobj->flags & 0x44000000) {
			if (act->mOwner->GetAttachment() == 1) {
				act->mState                 = 0;
				act->mOwner->mCopter.mState = 7;
			} else {
				RopeAnim(act, 0x38, 0);
				act->mState = 9;
			}
			act->mTarget = NULL;
		}
		break;
	case 9:
		if (act->mOwner->GetHeight() <= 0) {
			if (act->mOwner->_9A90 > 30) {
				RopeAnim(act, 0x91, 1);
				act->mState = 11;
				ApplyPlayerJumpStrength(&act->mOwner->mJump, act->mOwner->_9A90);
				act->mOwner->vf12C(1, 255, 0, 0, 0);
				act->mOwner->vf12C(1, 0, 0, 0, .3f);
				StartScreenFade(&gGame.mScrnTex.mColor, 255, 0, 0, 0, 100, 100, 200, 0, .3f);
			} else {
				RopeAnim(act, 0x59, 1);
				act->mState = 10;
			}
		}
		break;
	case 10:
		if (aobj->flags & 0x44000000) {
			XObjChr* owner = act->mOwner;
			owner->SetAnim(0x11, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
			owner->_A2C = 3;
			act->mState = 0;
		}
		break;
	case 11:
		if (aobj->flags & 0x44000000) {
			act->mState = 0;
			RopeAnim(act, 0x8f, 1);
			act->mOwner->mJump.mState = 6;
			gGame.mParticle.Create(0x10015, 0, act->mOwner->mJObj, TRUE);
			PlayGameSound(&gGame.mSound, 0x1a, 255);
			PlayGameSound(&gGame.mSound, 0x36, 255);
		}
		break;
	}
	if (act->mState == 5) {
		s32 p[]         = { 0 };
		HSD_JObj* joint = GetJointByPath(act->mOwner->mJObj, p, 1);
		if (joint)
			act->mOwner->Push(joint, 2, act->_10.y);
	}
}

// walks him around the target at the same distance
/**
 * @note Address: 0x800C5A7C
 * @note Size: 0x11C
 */
void PositionPlayerAroundRopeTarget(XActRope* act, f32 angle)
{
	if (act->mTarget != NULL) {
		CVec dir((Vec) { act->_10.x, 0.0f, act->_10.z });
		f32 dist = dir.Length();
		act->mOwner->SetTranslateX(act->mTarget->mMoveTranslate.x - dist * (f32)sin(angle), 0);
		act->mOwner->SetTranslateZ(act->mTarget->mMoveTranslate.z - dist * (f32)cos(angle), 0);
	}
}

/**
 * @note Address: 0x800C7AC4
 * @note Size: 0x100
 */
void HandleRopeActionInput(XActRope* act, const CPadOne* pad, f32* weight)
{
	if (act->mState == 5) {
		if (gGame.IsRunning() && gGame.mEvt.mData.mScript == NULL) {
			if (pad->mTrigger & 0x200) {
				act->mState = 8;
			}
			if (act->mOwner->mAnimId == 0x8A || act->mOwner->mAnimId == 0x8C) {
				*weight = 0.0f;
			}
		}
	} else if (act->mState != 0) {
		*weight = 0.0f;
	}
}

/**
 * @note Address: 0x800C7BC4
 * @note Size: 0x1FB0
 */
// NONMATCHING: the target is read once too few
void UpdateLadderActionAnimation(XActLadder* act, s32 anim, HSD_AObj* aobj, u32 frames, f32 frame, f32 end)
{
	switch (act->mState) {
	case 1:
		if (anim != 0x66) {
			act->mOwner->SetAnim(0x66, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			if (act->mTarget != NULL) {
				act->mOwner->SetRotationY(WrapAngle(PI + act->mTarget->mMoveRotate.y), 0);
				CVec chrPos(act->mOwner->mMoveTranslate);
				chrPos.y = 0.0f;
				CVec ladPos(act->mTarget->mMoveTranslate);
				ladPos.y = 0.0f;
				CVec off(0.0f, 0.0f, 4.0f);
				Mtx m;
				PSMTXRotRad(m, 'y', act->mTarget->mMoveRotate.y);
				PSMTXMultVec(m, &off, &off);
				AddVec(ladPos, off);
				act->mPos.x = ladPos.x;
				act->mPos.y = ladPos.y;
				act->mPos.z = ladPos.z;
				act->mDist  = PSVECDistance(&ladPos, &chrPos);
				act->mAngle = atan2(chrPos.x - ladPos.x, chrPos.z - ladPos.z);
			}
		} else if (aobj->flags & 0x44000000) {
			s32 path[] = { 0 };
			act->mOwner->SetPos(GetEventJointWorldPosition(act->mOwner, 1, path), 0);
			RecordObjectPositionAndHeightChange(act->mOwner);
			s32 path2[] = { 0 };
			SetObjectJointTranslationByPath(act->mOwner, 1, path2, 0.0f, 0.0f, 0.0f);
			act->mOwner->SetAnim(0x67, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
			act->mState = 5;
		} else {
			if (act->mTarget != NULL) {
				Vec move = act->mTarget->mMoveTranslate - act->mTarget->_264;
				act->mPos.x += move.x;
				act->mPos.z += move.z;
				if (act->mDist > 0.0f) {
					if ((act->mDist -= 0.2f * frames) < 0.0f) {
						act->mDist = 0.0f;
					}
					act->mOwner->SetTranslateX(act->mDist * (f32)sin(act->mAngle) + act->mPos.x, 0);
					act->mOwner->SetTranslateZ(act->mDist * (f32)cos(act->mAngle) + act->mPos.z, 0);
				} else {
					act->mOwner->SetTranslateX(act->mPos.x, 0);
					act->mOwner->SetTranslateZ(act->mPos.z, 0);
				}
			}
			if (CheckStageFlag(gGame.mStage, 1)) {
				XObjPlug* plug = &act->mOwner->mPlug;
				if (plug->_D3C != 0 && frame >= 25.0f) {
					ResetPlayerPlugState(plug, TRUE);
					PlayGameSound(&gGame.mSound, 0x37, 0xFF);
				}
			}
		}
		break;
	case 2:
		if (anim != 0x6D) {
			act->mOwner->SetAnim(0x6D, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			if (act->mTarget != NULL) {
				act->mOwner->SetRotationY(WrapAngle(act->mTarget->mMoveRotate.y), 0);
				CVec chrPos(act->mOwner->mMoveTranslate);
				chrPos.y = 0.0f;
				CVec ladPos(act->mTarget->mMoveTranslate);
				ladPos.y = 0.0f;
				CVec off(0.0f, 0.0f, -1.5f);
				Mtx m;
				PSMTXRotRad(m, 'y', act->mTarget->mMoveRotate.y);
				PSMTXMultVec(m, &off, &off);
				AddVec(ladPos, off);
				act->mPos.x = ladPos.x;
				act->mPos.y = ladPos.y;
				act->mPos.z = ladPos.z;
				act->mDist  = PSVECDistance(&ladPos, &chrPos);
				act->mAngle = atan2(chrPos.x - ladPos.x, chrPos.z - ladPos.z);
			}
		} else if (aobj->flags & 0x44000000) {
			s32 path[] = { 0 };
			act->mOwner->SetPos(GetEventJointWorldPosition(act->mOwner, 1, path), 0);
			RecordObjectPositionAndHeightChange(act->mOwner);
			s32 path2[] = { 0 };
			SetObjectJointTranslationByPath(act->mOwner, 1, path2, 0.0f, 0.0f, 0.0f);
			act->mOwner->SetRotationY(PI + act->mOwner->mMoveRotate.y, 0);
			s32 path3[] = { 0 };
			SetJointYawByPath(act->mOwner, 1, path3, 0.0f);
			act->mOwner->SetAnim(0x68, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
			act->mState = 5;
		} else {
			if (act->mTarget != NULL) {
				Vec move = act->mTarget->mMoveTranslate - act->mTarget->_264;
				act->mPos.x += move.x;
				act->mPos.z += move.z;
				if (act->mDist > 0.0f && frame > 20.0f) {
					if ((act->mDist -= 0.2f * frames) < 0.0f) {
						act->mDist = 0.0f;
					}
					act->mOwner->SetTranslateX(act->mDist * (f32)sin(act->mAngle) + act->mPos.x, 0);
					act->mOwner->SetTranslateZ(act->mDist * (f32)cos(act->mAngle) + act->mPos.z, 0);
				}
			}
			if (CheckStageFlag(gGame.mStage, 1)) {
				XObjPlug* plug = &act->mOwner->mPlug;
				if (plug->_D3C != 0 && frame >= 50.0f) {
					ResetPlayerPlugState(plug, TRUE);
					PlayGameSound(&gGame.mSound, 0x37, 0xFF);
				}
			}
		}
		break;
	case 3:
		if (anim != 0x87) {
			if (act->mTarget != NULL) {
				if (act->mTarget->mFloors[act->mTargetFloor].mId == 0) {
					CVec chrPos(act->mOwner->mMoveTranslate);
					chrPos.y = 0.0f;
					CVec pt(GetTargetFloorPosition(act));
					pt.y = 0.0f;
					act->mOwner->SetRotationY(PI + (f32)atan2(pt.x - chrPos.x, pt.z - chrPos.z), 0);
				} else {
					act->mOwner->SetRotationY(act->mTarget->mMoveRotate.y, 0);
				}
				act->mOwner->SetAnim(0x87, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
				act->mOwner->SetAnim(0x87, TRUE, NULL, NULL, NULL, GetJointAnimationEndFrame(act->mOwner), 1.0f, 1.0f, 10.0f);
				act->mOwner->UpdateAnim(0);
				s32 path[] = { 0 };
				CVec d     = GetEventJointWorldPosition(act->mOwner, 1, path) - act->mOwner->mMoveTranslate;
				act->mOwner->SetTranslateX(act->mOwner->mMoveTranslate.x - d.x, 0);
				act->mOwner->SetTranslateZ(act->mOwner->mMoveTranslate.z - d.z, 0);
				act->mOwner->SetTranslateY(act->mTarget->mMaxFloorHeight, 0);
				act->mOwner->SetAnim(0x87, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
				CVec chrPos(act->mOwner->mMoveTranslate);
				chrPos.y = 0.0f;
				CVec ladPos(act->mTarget->mMoveTranslate);
				ladPos.y = 0.0f;
				CVec off(0.0f, 0.0f, -1.0f);
				Mtx m;
				PSMTXRotRad(m, 'y', act->mTarget->mMoveRotate.y);
				PSMTXMultVec(m, &off, &off);
				AddVec(ladPos, off);
				act->mPos.x = ladPos.x;
				act->mPos.y = ladPos.y;
				act->mPos.z = ladPos.z;
				act->mDist  = PSVECDistance(&ladPos, &chrPos);
				act->mAngle = atan2(chrPos.x - ladPos.x, chrPos.z - ladPos.z);
			}
			if (CheckStageFlag(gGame.mStage, 1)) {
				XObjPlug* plug = &act->mOwner->mPlug;
				if (plug->_D3C != 0) {
					ResetPlayerPlugState(plug, TRUE);
					PlayGameSound(&gGame.mSound, 0x37, 0xFF);
				}
			}
		} else if (aobj->flags & 0x44000000) {
			s32 path[]      = { 0 };
			HSD_JObj* joint = GetObjectJointByPath(act->mOwner, 1, path);
			f32 turn        = HSD_JObjGetRotationY(joint);
			f32 facing      = HSD_JObjGetRotationY(act->mOwner->mJObj);
			act->mOwner->SetRotationY(facing + turn, 0);
			HSD_JObjSetRotationY__FP9_HSD_JObjf(joint, 0.0f);
			s32 path2[] = { 0 };
			CVec d      = GetEventJointWorldPosition(act->mOwner, 1, path2) - act->mOwner->mMoveTranslate;
			f32 angle   = HSD_JObjGetRotationY(act->mOwner->mJObj);
			d.x         = -d.x;
			d.z         = -d.z;
			act->mOwner->SetPos((act->mOwner->mMoveTranslate + d), 0);
			RecordObjectPositionAndHeightChange(act->mOwner);
			s32 path3[] = { 0 };
			SetObjectJointTranslationByPath(act->mOwner, 1, path3, 0.0f, 0.0f, 0.0f);
			act->mOwner->SetAnim(0x68, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
			act->mState = 5;
			act->_0C    = 0;
		} else if (act->mTarget != NULL) {
			Vec move = act->mTarget->mMoveTranslate - act->mTarget->_264;
			act->mPos.x += move.x;
			act->mPos.z += move.z;
			if (act->mDist > 0.0f) {
				if ((act->mDist -= 0.4f * frames) < 0.0f) {
					act->mDist = 0.0f;
				}
				act->mOwner->SetTranslateX(act->mDist * (f32)sin(act->mAngle) + act->mPos.x, 0);
				act->mOwner->SetTranslateZ(act->mDist * (f32)cos(act->mAngle) + act->mPos.z, 0);
			} else {
				act->mOwner->SetTranslateX(act->mPos.x, 0);
				act->mOwner->SetTranslateZ(act->mPos.z, 0);
			}
			act->mOwner->SetTranslateY(act->mTarget->mMaxFloorHeight, 0);
		}
		break;
	case 4:
		act->mOwner->SetAnim(0x68, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
		act->mState = 5;
		break;
	case 5:
		if (anim == 0x69 || anim == 0x6A || anim == 0x6B || anim == 0x6C) {
			if (aobj->flags & 0x44000000) {
				s32 path[] = { 0 };
				CVec hand  = GetPlayerJointLocalTranslationByPath(act->mOwner, 1, path);
				if (hand.y < 0.0f) {
					hand.y = (s32)(hand.y - 0.5f);
				}
				s32 path2[] = { 0 };
				SetJointTranslationFromVectorByPath(act->mOwner, 1, path2, hand);
				s32 path3[] = { 0 };
				act->mOwner->SetPos(GetEventJointWorldPosition(act->mOwner, 1, path3), 0);
				RecordObjectPositionAndHeightChange(act->mOwner);
				s32 path4[] = { 0 };
				SetObjectJointTranslationByPath(act->mOwner, 1, path4, 0.0f, 0.0f, 0.0f);
				if (act->_0C == 1) {
					if (AtLadderTop(act)) {
						act->mState = 6;
					} else if (anim == 0x69 || anim == 0x6B) {
						act->mOwner->SetAnim(0x6A, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
					} else if (anim == 0x6A || anim == 0x6C) {
						act->mOwner->SetAnim(0x69, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
					}
				} else if (act->_0C == -1) {
					if (act->mOwner->GetHeight() <= 2.0f) {
						act->mState = 7;
					} else if (anim == 0x69 || anim == 0x6B) {
						act->mOwner->SetAnim(0x6C, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
					} else if (anim == 0x6A || anim == 0x6C) {
						act->mOwner->SetAnim(0x6B, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
					}
				} else if (anim == 0x69 || anim == 0x6B) {
					act->mOwner->SetAnim(0x68, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
				} else if (anim == 0x6A || anim == 0x6C) {
					act->mOwner->SetAnim(0x67, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
				}
				if (act->_2C && act->mState == 5) {
					act->mState = 9;
				}
			}
		} else {
			if (act->_0C == 1) {
				if (AtLadderTop(act)) {
					act->mState = 6;
				} else if (anim == 0x67) {
					act->mOwner->SetAnim(0x69, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
				} else if (anim == 0x68) {
					act->mOwner->SetAnim(0x6A, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
				}
			} else if (act->_0C == -1) {
				if (act->mOwner->GetHeight() <= 2.0f) {
					act->mState = 7;
				} else if (anim == 0x67) {
					act->mOwner->SetAnim(0x6B, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
				} else if (anim == 0x68) {
					act->mOwner->SetAnim(0x6C, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
				}
			}
			if (act->_2C && act->mState == 5) {
				act->mState = 9;
			}
		}
		break;
	case 6:
		if (!(anim == 0x6E || anim == 0x6F)) {
			if (anim == 0x69 || anim == 0x6B) {
				act->mOwner->SetAnim(0x6F, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			} else if (anim == 0x6A || anim == 0x6C) {
				act->mOwner->SetAnim(0x6E, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			} else if (anim == 0x67) {
				act->mOwner->SetAnim(0x6E, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			} else if (anim == 0x68) {
				act->mOwner->SetAnim(0x6F, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			}
		} else if (aobj->flags & 0x44000000) {
			s32 path[] = { 0 };
			act->mOwner->SetPos(GetEventJointWorldPosition(act->mOwner, 1, path), 0);
			RecordObjectPositionAndHeightChange(act->mOwner);
			s32 path2[] = { 0 };
			SetObjectJointTranslationByPath(act->mOwner, 1, path2, 0.0f, 0.0f, 0.0f);
			XObjChr* chr = act->mOwner;
			chr->SetAnim(0x11, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			chr->_A2C    = 3;
			act->mState  = 0;
			act->mTarget = NULL;
		}
		break;
	case 7:
		if (!(anim == 0x70 || anim == 0x71)) {
			if (anim == 0x69 || anim == 0x6B) {
				act->mOwner->SetAnim(0x71, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			} else if (anim == 0x6A || anim == 0x6C) {
				act->mOwner->SetAnim(0x70, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			} else if (anim == 0x67) {
				act->mOwner->SetAnim(0x70, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			} else if (anim == 0x68) {
				act->mOwner->SetAnim(0x71, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			}
		} else if (aobj->flags & 0x44000000) {
			s32 path[] = { 0 };
			act->mOwner->SetPos(GetEventJointWorldPosition(act->mOwner, 1, path), 0);
			RecordObjectPositionAndHeightChange(act->mOwner);
			s32 path2[] = { 0 };
			SetObjectJointTranslationByPath(act->mOwner, 1, path2, 0.0f, 0.0f, 0.0f);
			XObjChr* chr = act->mOwner;
			chr->SetAnim(0x11, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			chr->_A2C    = 3;
			act->mState  = 0;
			act->mTarget = NULL;
		}
		break;
	case 8:
		if (anim == 0x69 || anim == 0x6B) {
			act->mOwner->SetAnim(0x73, TRUE, NULL, NULL, NULL, 10.0f, 1.0f, 1.0f, 10.0f);
		} else if (anim == 0x6A || anim == 0x6C) {
			act->mOwner->SetAnim(0x72, TRUE, NULL, NULL, NULL, 10.0f, 1.0f, 1.0f, 10.0f);
		} else if (anim == 0x67) {
			act->mOwner->SetAnim(0x72, TRUE, NULL, NULL, NULL, 10.0f, 1.0f, 1.0f, 10.0f);
		} else if (anim == 0x68) {
			act->mOwner->SetAnim(0x73, TRUE, NULL, NULL, NULL, 10.0f, 1.0f, 1.0f, 10.0f);
		}
		act->mState = 9;
	case 9:
		if (!(anim == 0x72 || anim == 0x73)) {
			if (anim == 0x69 || anim == 0x6B) {
				act->mOwner->SetAnim(0x73, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			} else if (anim == 0x6A || anim == 0x6C) {
				act->mOwner->SetAnim(0x72, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			} else if (anim == 0x67) {
				act->mOwner->SetAnim(0x72, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			} else if (anim == 0x68) {
				act->mOwner->SetAnim(0x73, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			}
		} else if (aobj->flags & 0x44000000) {
			s32 path[] = { 0 };
			act->mOwner->SetPos(GetEventJointWorldPosition(act->mOwner, 1, path), 0);
			RecordObjectPositionAndHeightChange(act->mOwner);
			s32 path2[] = { 0 };
			SetObjectJointTranslationByPath(act->mOwner, 1, path2, 0.0f, 0.0f, 0.0f);
			if (act->mOwner->GetAttachment() == 1) {
				act->mState                 = 0;
				act->mOwner->mCopter.mState = 4;
			} else {
				act->mOwner->SetAnim(0x74, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
				act->mState = 10;
			}
			act->mTarget = NULL;
		}
		break;
	case 10:
		if (act->mOwner->GetHeight() <= 0.0f) {
			if (act->mOwner->_9A90 > 30.0f) {
				act->mState = 0;
				act->mOwner->SetAnim(0x8E, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
				act->mOwner->mJump.mState = 5;
				ApplyPlayerJumpStrength(&act->mOwner->mJump, act->mOwner->_9A90);
				act->mOwner->vf12C(1, 0xFF, 0, 0, 0.0f);
				act->mOwner->vf12C(1, 0, 0, 0, 0.3f);
				StartScreenFade(&gGame.mScrnTex.mColor, 0xFF, 0, 0, 0, 100, 100, 200, 0, 0.3f);
			} else {
				act->mOwner->SetAnim(0x75, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
				act->mState = 11;
			}
		}
		break;
	case 11:
		if (aobj->flags & 0x44000000) {
			XObjChr* chr = act->mOwner;
			chr->SetAnim(0x11, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			chr->_A2C   = 3;
			act->mState = 0;
		}
		break;
	}
}

const Vec* GetScaledTargetFloorExtent(const XAction*);
s32 FindPlayerSupportingFloor(XObjChr*, s32, s32);
f32 GetObjectEdgeOutwardYaw(const XObj*, Vec*, s32);
f32 GetJointAnimationCurrentFrame(CJObj*);

static inline void HangAnim(XActHang* act, s32 id, f32 loop = 1, BOOL force = FALSE, f32 start = 0)
{
	act->mOwner->SetAnim(id, force, NULL, NULL, NULL, start, 1, loop, 10);
}
static inline void ReleaseHangPlug(XActHang* act)
{
	if (CheckStageFlag(gGame.mStage, 1)) {
		XObjPlug* plug = &act->mOwner->mPlug;
		if (plug->_D3C) {
			ResetPlayerPlugState(plug, TRUE);
			PlayGameSound(&gGame.mSound, 0x37, 255);
		}
	}
}
static inline bool RoundHangFloor(XActHang* act)
{
	return act->mTarget->mFloors[act->mTargetFloor].mId == 0;
}
static inline void SaveHangDistance(XActHang* act)
{
	CVec owner(act->mOwner->mMoveTranslate);
	owner.y = 0;
	CVec floor(GetTargetFloorPosition(act));
	floor.y  = 0;
	act->_3C = floor;
	act->_48 = PSVECDistance(&owner, &floor);
}
static inline void RefreshHangFloor(XActHang* act)
{
	s32 floor = FindPlayerSupportingFloor(act->mOwner, 1, act->mTargetFloor);
	if (floor >= 0)
		act->mTargetFloor = floor;
}
static inline f32 RoundHangAngle(XActHang* act)
{
	CVec owner(act->mOwner->mMoveTranslate);
	owner.y = 0;
	CVec floor(GetTargetFloorPosition(act));
	floor.y = 0;
	return PI + (f32)atan2(floor.x - owner.x, floor.z - owner.z);
}
static inline void HangRootPosition(XActHang* act)
{
	s32 path[] = { 0 };
	act->mOwner->SetPos(GetEventJointWorldPosition(act->mOwner, 1, path), 0);
	RecordObjectPositionAndHeightChange(act->mOwner);
	s32 reset[] = { 0 };
	SetObjectJointTranslationByPath(act->mOwner, 1, reset, 0, 0, 0);
}
static inline void FinishHang(XActHang* act, f32 loop)
{
	XObjChr* p = act->mOwner;
	p->SetAnim(0x11, FALSE, NULL, NULL, NULL, 0, 1, loop, 10);
	p->_A2C     = 3;
	act->mState = 0;
}

/**
 * @note Address: 0x800CBDCC
 * @note Size: 0x1CE8
 */
void UpdatePlayerHangAction(XActHang* act, s32 anim, HSD_AObj* aobj, u32, f32 frame, f32 end)
{
	switch (act->mState) {
	case 1: {
		static CVec contact, normal, start;
		if (anim != 0x13) {
			if (act->mTarget) {
				f32 angle = GetFloorFacingAngle(act->mTarget, &act->mOwner->mMoveTranslate, act->mTargetFloor, 1, &contact, &normal);
				if (__fpclassifyf(angle) == FP_NAN) {
					act->mState  = 0;
					act->mTarget = NULL;
					return;
				}
				StopPlayerMotion(act->mOwner);
				HangAnim(act, 0x13, 0);
				act->mOwner->SetRotationY(angle, 0);
				start.Set(act->mOwner->mMoveTranslate);
				contact.y = start.y;
			}
			ReleaseHangPlug(act);
		} else if (aobj->flags & 0x44000000) {
			HangAnim(act, 0x14);
			act->mState = 5;
			act->_0C    = 0;
			if (act->mTarget) {
				if (RoundHangFloor(act))
					SaveHangDistance(act);
				else
					act->mOwner->SetPos(contact - normal * 4.f, 0);
			}
		} else if (act->mTarget) {
			if (RoundHangFloor(act)) {
				XObjChr* p = act->mOwner;
				f32 radius = p->mMoveScale.x * GetScaledTargetFloorExtent(act)->x;
				CVec owner(act->mOwner->mMoveTranslate);
				owner.y = 0;
				CVec floor(GetTargetFloorPosition(act));
				floor.y = 0;
				if (PSVECDistance(&owner, &floor) < 4.f + radius) {
					f32 yaw = act->mOwner->mMoveRotate.y;
					act->mOwner->SetTranslateX(act->mOwner->mMoveTranslate.x - .2f * (f32)sin(yaw), 0);
					act->mOwner->SetTranslateZ(act->mOwner->mMoveTranslate.z - .2f * (f32)cos(yaw), 0);
				}
			} else {
				CVec delta = (contact - normal * 4.f) - start;
				act->mOwner->SetPos((contact - normal * 4.f) - delta * (1.f - frame / end), 0);
			}
		}
		break;
	}
	case 4:
		if (anim != 0x7a && anim != 0xca) {
			if (act->mTarget) {
				RefreshHangFloor(act);
				f32 yaw               = RoundHangFloor(act) ? RoundHangAngle(act)
				                                            : PI + GetObjectEdgeOutwardYaw(act->mTarget, &act->mOwner->mMoveTranslate, act->mTargetFloor);
				yaw                   = WrapAngle(yaw);
				act->mHangStartOffset = act->mOwner->mMoveTranslate - act->mTarget->mMoveTranslate;
				CVec pos(act->mOwner->mMoveTranslate);
				pos.x += 1.5f * (f32)sin(PI + yaw);
				pos.z += 1.5f * (f32)cos(PI + yaw);
				act->mOwner->SetPos(pos, 0);
				RecordObjectPositionAndHeightChange(act->mOwner);
			}
			HangAnim(act, 0x7a, 0);
			return;
		}
		if (aobj->flags & 0x44000000) {
			act->mOwner->SetPos(act->mTarget->mMoveTranslate + act->mHangStartOffset, 0);
			RecordObjectPositionAndHeightChange(act->mOwner);
			act->mState               = 0;
			act->mTarget              = NULL;
			act->mOwner->mJump.mState = 3;
			ReleaseHangPlug(act);
			return;
		}
		if (!act->_2C)
			return;
		{
			f32 last = GetJointAnimationEndFrame(act->mOwner);
			if (GetJointAnimationCurrentFrame(act->mOwner) / last <= .6f) {
				FinishHang(act, 0);
				act->mTarget = NULL;
				return;
			}
		}
		act->mOwner->SetPos(act->mTarget->mMoveTranslate + act->mHangStartOffset, 0);
		RecordObjectPositionAndHeightChange(act->mOwner);
		if (act->mOwner->GetAttachment() == 2 || act->mOwner->mLift.mState) {
			act->mOwner->mJump.mState = 2;
			act->mState               = 0;
			act->mTarget              = NULL;
			ReleaseHangPlug(act);
		} else if (act->mTarget && act->mTarget->mStatus.mFlags & 0x800000000ULL) {
			act->mOwner->mLadder.SetState(3, act->mTarget);
			act->mState  = 0;
			act->mTarget = NULL;
		} else if (act->mTarget && act->mTarget->mStatus.mFlags & 0x1000000000ULL) {
			act->mOwner->mRope.SetState(3, act->mTarget);
			act->mState  = 0;
			act->mTarget = NULL;
			return;
		} else {
			act->mState = 3;
			return;
		}
		// The jump and ladder transitions also run the stage plug check below.
		// fall through
	case 3:
		if (anim != 0x76) {
			if (act->mTarget) {
				RefreshHangFloor(act);
				CVec contact(0, 0, 0);
				f32 yaw;
				if (RoundHangFloor(act))
					yaw = RoundHangAngle(act);
				else {
					yaw = PI + GetFloorFacingAngle(act->mTarget, &act->mOwner->mMoveTranslate, act->mTargetFloor, 0, &contact, NULL);
					if (__fpclassifyf(yaw) == FP_NAN) {
						act->mOwner->mJump.mState = 2;
						act->mState               = 0;
						act->mTarget              = NULL;
						return;
					}
				}
				yaw = WrapAngle(yaw);
				act->mOwner->SetRotationY(yaw, 0);
				HangAnim(act, 0x13, 1, TRUE);
				HangAnim(act, 0x13, 1, TRUE, GetJointAnimationEndFrame(act->mOwner));
				act->mOwner->UpdateAnim(0);
				s32 p1[]   = { 0 };
				CVec local = GetPlayerJointLocalTranslationByPath(act->mOwner, 1, p1);
				s32 p2[]   = { 0 };
				CVec delta = GetEventJointWorldPosition(act->mOwner, 1, p2) - act->mOwner->mMoveTranslate;
				delta.y    = -delta.y;
				act->mOwner->SetPos(act->mOwner->mMoveTranslate + delta, 0);
				RecordObjectPositionAndHeightChange(act->mOwner);
				HangAnim(act, 0x76);
				local.x  = -local.x;
				local.z  = -local.z;
				s32 p3[] = { 0 };
				SetJointTranslationFromVectorByPath(act->mOwner, 1, p3, local);
				CVec pos(act->mOwner->mMoveTranslate);
				f32 distance;
				if (RoundHangFloor(act)) {
					CVec owner(act->mOwner->mMoveTranslate);
					owner.y = 0;
					CVec floor(GetTargetFloorPosition(act));
					floor.y    = 0;
					yaw        = PI + (f32)atan2(floor.x - owner.x, floor.z - owner.z);
					f32 span   = PSVECDistance(&owner, &floor);
					XObjChr* p = act->mOwner;
					distance   = 1.f + (span - p->mMoveScale.x * GetScaledTargetFloorExtent(act)->x);
				} else {
					pos.x    = contact.x;
					pos.z    = contact.z;
					distance = 1;
				}
				yaw = WrapAngle(yaw);
				pos.x += distance * (f32)sin(PI + yaw);
				pos.z += distance * (f32)cos(PI + yaw);
				Vec offset = act->_10;
				Mtx rotate;
				PSMTXRotRad(rotate, 'y', yaw);
				PSMTXMultVec(rotate, &offset, &offset);
				pos.x += offset.x;
				pos.z += offset.z;
				act->mOwner->SetPos(pos, 0);
				RecordObjectPositionAndHeightChange(act->mOwner);
			}
			ReleaseHangPlug(act);
		} else if (aobj->flags & 0x44000000) {
			s32 childPath[] = { 0 };
			HSD_JObj* child = GetObjectJointByPath(act->mOwner, 1, childPath);
			f32 turn        = HSD_JObjGetRotationY(child);
			act->mOwner->SetRotationY(HSD_JObjGetRotationY(act->mOwner->mJObj) + turn, 0);
			HSD_JObjSetRotationY__FP9_HSD_JObjf(child, 0);
			HangAnim(act, 0x13);
			HangAnim(act, 0x13, 1, TRUE, GetJointAnimationEndFrame(act->mOwner));
			act->mOwner->UpdateAnim(0);
			s32 p1[]   = { 0 };
			CVec local = GetPlayerJointLocalTranslationByPath(act->mOwner, 1, p1);
			s32 p2[]   = { 0 };
			CVec delta = GetEventJointWorldPosition(act->mOwner, 1, p2) - act->mOwner->mMoveTranslate;
			s32 p3[]   = { 0 };
			SetJointTranslationFromVectorByPath(act->mOwner, 1, p3, local);
			HangAnim(act, 0x14);
			act->mState = 5;
			act->_0C    = 0;
			if (act->mTarget && RoundHangFloor(act))
				SaveHangDistance(act);
		}
		break;
	case 2:
		if (anim != 0x18) {
			HangAnim(act, 0x18);
			if (act->mTarget)
				act->mOwner->SetRotationY(GetObjectEdgeOutwardYaw(act->mTarget, &act->mOwner->mMoveTranslate, act->mTargetFloor), 0);
		} else if (aobj->flags & 0x44000000) {
			HangRootPosition(act);
			act->mOwner->SetRotationY(PI + act->mOwner->mMoveRotate.y, 0);
			s32 p[] = { 0 };
			SetJointYawByPath(act->mOwner, 1, p, 0);
			HangAnim(act, 0x14);
			act->mOwner->UpdateAnim(0);
			s32 child[] = { 0 };
			act->mOwner->SetPos(
			    act->mOwner->mMoveTranslate + (act->mOwner->mMoveTranslate - GetEventJointWorldPosition(act->mOwner, 1, child)), 1);
			RecordObjectPositionAndHeightChange(act->mOwner);
			act->mState = 5;
			act->_0C    = 0;
		}
		break;
	case 5:
		if (act->_0C == -1) {
			if (anim != 0x16)
				HangAnim(act, 0x16);
		} else if (act->_0C == 1) {
			if (anim != 0x15)
				HangAnim(act, 0x15);
		} else if (anim != 0x14)
			HangAnim(act, 0x14);
		break;
	case 6:
	case 7: {
		int next = act->mState == 7 ? 0x7d : 0x17;
		if (anim != next)
			HangAnim(act, next);
		else if (aobj->flags & 0x44000000) {
			HangRootPosition(act);
			FinishHang(act, 0);
			act->mTarget = NULL;
		}
		break;
	}
	case 8:
		HangAnim(act, 0x19, 1, TRUE, 15);
		act->mState = 9;
	// fall through
	case 9:
		if (anim != 0x19) {
			HangAnim(act, 0x19);
			return;
		}
		if (!(aobj->flags & 0x44000000))
			return;
		HangAnim(act, 0x1a);
		act->mState  = 10;
		act->mTarget = NULL;
	// fall through
	case 10:
		if (act->mOwner->GetHeight() <= 0) {
			if (act->mOwner->_9A90 > 30) {
				act->mState = 0;
				HangAnim(act, 0x8e);
				act->mOwner->mJump.mState = 5;
				ApplyPlayerJumpStrength(&act->mOwner->mJump, act->mOwner->_9A90);
				act->mOwner->vf12C(1, 255, 0, 0, 0);
				act->mOwner->vf12C(1, 0, 0, 0, .3f);
				StartScreenFade(&gGame.mScrnTex.mColor, 255, 0, 0, 0, 100, 100, 200, 0, .3f);
			} else {
				HangAnim(act, 0x1b);
				act->mState = 11;
			}
		} else if (act->mOwner->GetAttachment() == 1) {
			act->mState                 = 0;
			act->mOwner->mCopter.mState = 8;
		}
		break;
	case 11:
		if (aobj->flags & 0x44000000)
			FinishHang(act, 1);
		break;
	}
}

/**
 * @note Address: 0x800C9B74
 * @note Size: 0xC0
 */
void HSD_JObjSetRotationY__FP9_HSD_JObjf(HSD_JObj* jobj, f32 angle)
{
	HSD_JObjSetRotationY(jobj, angle);
}

/**
 * @note Address: 0x800C9C34
 * @note Size: 0x3C
 */
void RecordObjectPositionAndHeightChange(XObj* obj)
{
	if (obj->mMoveTranslate.y != obj->_264.y) {
		obj->mStatus.mFlags |= 0x0010000000000000ULL;
	}
	obj->_264.x = obj->mMoveTranslate.x;
	obj->_264.y = obj->mMoveTranslate.y;
	obj->_264.z = obj->mMoveTranslate.z;
}

/**
 * @note Address: 0x800C9C70
 * @note Size: 0xE4
 */
void HandleLadderActionInput(XActLadder* act, const CPadOne* pad, f32* weight)
{
	act->_2C = FALSE;
	if (act->mState == 5) {
		if (gGame.IsRunning() && gGame.mEvt.mData.mScript == NULL && (pad->mTrigger & 0x200)) {
			act->_2C = TRUE;
		}
	} else if (act->mState != 0) {
		*weight = 0.0f;
	}
}

CVec fn_800C35F0(XObjChr*);
extern Vec lbl_8037C334;

static inline Vec GetFloorOffset(const XObjFloor* floor)
{
	Vec value = lbl_8037C334;
	value.x   = floor->mCenterOffset[0] / 100.0f;
	value.y   = floor->mCenterOffset[1] / 100.0f;
	value.z   = floor->mCenterOffset[2] / 100.0f;
	return value;
}

static inline f32 JumpMotionLength(XObjChr* owner)
{
	CVec motion = fn_800C35F0(owner);
	return PSVECMag(&motion);
}

static inline f32 JumpStickLength(const CPadOne* pad)
{
	return sqrtf(pad->mStickX * pad->mStickX + pad->mStickY * pad->mStickY);
}
static inline void JumpAnim(XActJump* act, s32 id, f32 loop = 1)
{
	act->mOwner->SetAnim(id, FALSE, NULL, NULL, NULL, 0, 1, loop, 10);
}
static inline void EndJump(XActJump* act)
{
	XObjChr* owner = act->mOwner;
	owner->SetAnim(0x11, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
	owner->_A2C = 3;
	act->mState = 0;
}
static inline bool AdvanceJumpArc(XActJump* act, u32 frames)
{
	if (act->mArcAngle < HALF_PI) {
		if ((act->mArcAngle += act->mArcStep * frames) > HALF_PI)
			act->mArcAngle = HALF_PI;
		f32 lift = act->mRise * (f32)sin(act->mArcAngle);
		act->mOwner->SetTranslateY(act->mArcPosition.y + lift, 0);
		f32 angle = HSD_JObjGetRotationY(act->mOwner->mJObj);
		f32 dx    = act->mForward * (act->mArcStep * (f32)sin(angle));
		f32 dz    = act->mForward * (act->mArcStep * (f32)cos(angle));
		act->mOwner->SetTranslateX(act->mArcPosition.x += dx, 0);
		act->mOwner->SetTranslateZ(act->mArcPosition.z += dz, 0);
		return false;
	}
	return true;
}

static inline bool IsJumpAnim(s32 anim, s32 base, s32 first, s32 second)
{
	return anim == base || anim == first || anim == second;
}

/**
 * @note Address: 0x800CA0B4
 * @note Size: 0x1D00
 */
void UpdatePlayerJumpAction(XActJump* act, s32 anim, HSD_AObj* aobj, u32 frames, f32 frame, f32 end)
{
	bool interrupt = false;
	if (!gGame.mEvt.mData.mScript) {
		CPad pad(0);
		if (JumpStickLength(pad.mPad) > 0) {
			switch (act->mState) {
			case 19:
				if (frame > 1)
					interrupt = true;
				break;
			case 4:
			case 18:
				if (frame > 3)
					interrupt = true;
				break;
			case 10:
			case 13:
			case 16:
				act->mOwner->SetAnimRate(1.f + 2.f * ((frame / end) * JumpStickLength(pad.mPad)), 0x5a0);
				break;
			}
		}
	}
	s32 next     = -1;
	bool playing = false;
	switch (act->mState) {
	case 1:
		if (anim != 0x1c) {
			JumpAnim(act, 0x1c, 0);
			return;
		}
		if (!(aobj->flags & 0x44000000))
			return;
		act->mState = 2;
		// fall through
	case 2:
	case 3:
	case 17:
		if (act->mOwner->GetHeight() <= 0 || (act->mOwner->GetHeight() <= 1.f && JumpMotionLength(act->mOwner) > .5f)) {
			if (act->mOwner->mLift.mState == 19) {
				act->mState = 0;
				return;
			}
			if (act->mOwner->_9A90 > 30) {
				JumpAnim(act, 0x8e);
				act->mState = 5;
				ApplyPlayerJumpStrength(act, act->mOwner->_9A90);
				act->mOwner->vf12C(1, 255, 0, 0, 0);
				act->mOwner->vf12C(1, 0, 0, 0, .3f);
				StartScreenFade(&gGame.mScrnTex.mColor, 255, 0, 0, 0, 100, 100, 200, 0, .3f);
			} else if (act->mOwner->_9A90 > 11.5f) {
				JumpAnim(act, 0x1e);
				act->mState = 4;
				if (act->mOwner->_9A90 > 12.5f)
					gGame.mParticle.Create(0x10006, 0, act->mOwner->mJObj, FALSE);
			} else if (act->mOwner->_9A90 > 7.5f) {
				JumpAnim(act, 0x1f);
				act->mState = 18;
			} else {
				JumpAnim(act, 0x20);
				act->mState = 19;
			}
		} else if (act->mOwner->mLift.mState != 19) {
			if (act->mState == 3) {
				if (anim != 0x7c)
					JumpAnim(act, 0x7c, 0);
			} else if (anim != 0x1d && anim != 0x93)
				JumpAnim(act, 0x1d, 0);
		}
		break;
	case 4:
		if (act->mOwner->mLift.mState == 19) {
			act->mState = 0;
			return;
		}
		if ((aobj->flags & 0x44000000) || interrupt)
			EndJump(act);
		break;
	case 5:
		if (act->mOwner->mLift.mState == 19) {
			act->mState = 0;
			return;
		}
		if (aobj->flags & 0x44000000) {
			JumpAnim(act, 0x8f);
			act->mState = 6;
			gGame.mParticle.Create(0x10015, 0, act->mOwner->mJObj, TRUE);
			PlayGameSound(&gGame.mSound, 0x1a, 255);
			PlayGameSound(&gGame.mSound, 0x36, 255);
		}
		break;
	case 6:
		if (act->mOwner->mLift.mState == 19) {
			act->mState = 0;
			return;
		}
		if ((act->mStrength -= frames) <= 0) {
			s32 id = 0x90;
			if (act->mTargetStrength >= 50)
				id = 0x9a;
			else if (act->mTargetStrength >= 45 && !gGame.mEvt.mData.mScript) {
				XObjChr* p = act->mOwner;
				if (p->mLift.mState && IsCarrying(&p->mLift, 22))
					PutAway(&p->mLift);
			}
			JumpAnim(act, id);
			act->mState = 7;
			gGame.mParticle.Kill(0x15);
			StopGameSfx(&gGame.mSound, 0x36);
		} else if (!IsGameSoundPlaying(&gGame.mSound, 0x36))
			PlayGameSound(&gGame.mSound, 0x36, 255);
		break;
	case 7:
		if (act->mOwner->mLift.mState == 19) {
			act->mState = 0;
			return;
		}
		if (aobj->flags & 0x44000000)
			EndJump(act);
		break;
	case 8:
		next    = 0x5a;
		playing = IsJumpAnim(anim, next, 0x10f, 0x115);
	// fall through
	case 11:
		if (next < 0) {
			next    = 0x60;
			playing = IsJumpAnim(anim, next, 0x112, 0x118);
		}
	// fall through
	case 14:
		if (next < 0) {
			next    = 0x63;
			playing = IsJumpAnim(anim, next, 0x11b, 0x11e);
		}
		if (!playing) {
			act->mTargetHeight = act->mTarget->mSelectedFloorHeight;
			if (act->mOwner->mMoveTranslate.y >= act->mTargetHeight) {
				act->mState = 0;
				return;
			}
			if (act->mTarget) {
				f32 yaw = GetFloorFacingAngle(act->mTarget, &act->mOwner->mMoveTranslate, act->mTargetFloor, 0, NULL, NULL);
				if (__fpclassifyf(yaw) != FP_NAN) {
					StopPlayerMotion(act->mOwner);
					JumpAnim(act, next);
					act->mOwner->SetRotationY(yaw, 0);
				} else {
					act->mState  = 0;
					act->mTarget = NULL;
				}
			}
		} else if (aobj->flags & 0x44000000) {
			s32 id = act->mState == 8 ? 0x5b : act->mState == 11 ? 0x61 : 0x64;
			JumpAnim(act, id);
			++act->mState;
			if (CheckStageFlag(gGame.mStage, 1)) {
				XObjPlug* plug = &act->mOwner->mPlug;
				if (plug->_D3C) {
					plug->_D3C = 0;
					s32 path[] = { 0 };
					HSD_JObjSetRotationX(GetJointByPath(plug->mJObj, path, 1), 0);
					PlayGameSound(&gGame.mSound, 0x37, 255);
				}
			}
			if (act->mTarget) {
				act->mArcPosition = act->mOwner->mMoveTranslate;
				act->mRise        = act->mTargetHeight - act->mOwner->mMoveTranslate.y;
				act->mArcAngle    = 0;
				act->mArcStep     = act->mRise > 6 ? .8f * (HALF_PI / act->mRise) : DEG2RAD * 12;
				act->mForward     = 5;
				{
					XObjChr* p = act->mOwner;
					f32 radius = p->mMoveScale.x * GetFloorExtent(p->mFloors).x;
					f32 height = .5f * (p->mMoveScale.x * (GetFloorOffset(p->mFloors).y + GetFloorExtent(p->mFloors).y));
					Mtx rot;
					PSMTXRotRad(rot, 'y', p->mMoveRotate.y);
					Vec offset = act->mOwner->mMoveScale * GetFloorOffset(act->mOwner->mFloors);
					PSMTXMultVec(rot, &offset, &offset);
					f32 yaw = act->mOwner->mMoveRotate.y;
					f32 x   = 3.f * (f32)sin(yaw) + (offset.x + act->mOwner->mMoveTranslate.x);
					f32 z   = 3.f * (f32)cos(yaw) + (offset.z + act->mOwner->mMoveTranslate.z);
					f32 y   = .5f + act->mTargetHeight;
					for (s32 i = 0; i < gGame.mXObjs.mObjectCount; ++i) {
						XObj* obj = gGame.GetObject(i);
						if (!(obj->mStatus.mFlags & 1) || !(obj->mStatus.mFlags & 0x10000) || obj->mStatus.mFlags & 0x100000)
							continue;
						if (obj->GetCategory() == 7 || obj->GetCategory() == 20 || act->mOwner->mId == obj->mId || obj->GetKind() >= 10000)
							continue;
						f32 dx = obj->mMoveTranslate.x - x, dz = obj->mMoveTranslate.z - z;
						if (sqrtf(dx * dx + dz * dz) > 100.f + (radius + obj->mFloorXZExtent))
							continue;
						CVec push, contact, from(x, y, z), to(x, y, z);
						if ((obj->vfFC(&from, &to, radius, height, &push, &contact, 0, 0) & 3) == 3) {
							act->mForward = 0;
							act->mRise *= .8f;
							break;
						}
					}
				}
				AdvanceJumpArc(act, frames);
			}
			if (!act->mTarget || act->mRise <= 0) {
				EndJump(act);
				act->mTarget = NULL;
			}
		}
		break;
	case 9:
		next    = 0x5b;
		playing = IsJumpAnim(anim, next, 0x110, 0x116);
	// fall through
	case 12:
		if (next < 0) {
			next    = 0x61;
			playing = IsJumpAnim(anim, next, 0x113, 0x119);
		}
	// fall through
	case 15:
		if (next < 0) {
			next    = 0x64;
			playing = IsJumpAnim(anim, next, 0x11c, 0x11f);
		}
		if (!playing)
			JumpAnim(act, next);
		if (AdvanceJumpArc(act, frames)) {
			if (act->mForward == 0) {
				JumpAnim(act, 0x1d, 0);
				act->mState = 17;
				return;
			}
			s32 id = act->mState == 9 ? 0x5c : act->mState == 12 ? 0x62 : 0x65;
			JumpAnim(act, id);
			++act->mState;
		}
		break;
	case 10:
		next    = 0x5c;
		playing = IsJumpAnim(anim, next, 0x111, 0x117);
	// fall through
	case 13:
		if (next < 0) {
			next    = 0x62;
			playing = IsJumpAnim(anim, next, 0x114, 0x11a);
		}
	// fall through
	case 16:
		if (next < 0) {
			next    = 0x65;
			playing = IsJumpAnim(anim, next, 0x11d, 0x120);
		}
		if (!playing) {
			JumpAnim(act, next);
			return;
		}
		if (aobj->flags & 0x44000000) {
			EndJump(act);
			act->mTarget = NULL;
		} else if (interrupt)
			act->mOwner->SetAnimRate(4, 0x5a0);
		break;
	case 18:
	case 19:
		if ((aobj->flags & 0x44000000) || interrupt)
			EndJump(act);
		break;
	}
}

// the jump's fall, a long one makes him drop what he carries
/**
 * @note Address: 0x800C9D54
 * @note Size: 0x360
 */
void ApplyPlayerJumpStrength(XActJump* act, f32 strength)
{
	if (gGame.mEvt.mData.mScript != NULL) {
		act->mTargetStrength = 0.0f;
		act->mStrength       = 0.0f;
	} else {
		act->mTargetStrength = strength;
		act->mStrength       = strength;
		if (act->mTargetStrength >= 50.0f) {
			XObjChr* chr = act->mOwner;
			if (chr->mLift.mState != 0 && IsCarrying(&chr->mLift, 22)) {
				PutAway(&chr->mLift);
				if (act->mTargetStrength >= 60.0f) {
					act->mState                 = 0;
					act->mOwner->mTumble.mState = 1;
					act->mOwner->SetAnim(0x35, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				}
			}
		}
	}
}

/**
 * @note Address: 0x800CBDB4
 * @note Size: 0x18
 */
void SuppressMovementDuringJump(XActJump* act, const CPadOne* pad, f32* weight)
{
	if (act->mState != 0) {
		*weight = 0.0f;
	}
}

/**
 * @note Address: 0x800CE120
 * @note Size: 0x214
 */
void HandleHangActionInput(XActHang* act, const CPadOne* pad, f32* weight)
{
	act->_2C = FALSE;
	if (act->mState == 4) {
		f32 stick = -(f32)atan2(pad->mStickX, pad->mStickY);
		f32 angle = WrapAngle(stick + gGame.mMotionControlCamera.mCamera.mAngle.y - act->mOwner->mMoveRotate.y);
		if (!(angle < 0.5235988f) && !(angle > 5.7595863f)) {
			act->_2C = TRUE;
		}
		*weight = 0.0f;
	} else if (act->mState == 5) {
		if (gGame.IsRunning() && gGame.mEvt.mData.mScript == NULL) {
			if (pad->mTrigger & 0x100) {
				if (act->mTarget == NULL || (act->mTarget->mStatus.mFlags & 0x400000000ULL)) {
					TryLeaveHang(act);
					act->_0C = 0;
				}
				*weight = 0.0f;
			} else if (pad->mTrigger & 0x200) {
				act->mState = 9;
				act->_0C    = 0;
				*weight     = 0.0f;
			}
		}
	} else if (act->mState != 0) {
		*weight = 0.0f;
	}
}

/**
 * @note Address: 0x800CE334
 * @note Size: 0x20
 */
void fn_800CE334(XActLift* act, u32 value, BOOL apply)
{
	act->_54 = value;
	if (apply) {
		act->mOwner->mPlug._D3C = value | 0x10000;
	}
}

static inline bool CarriedObjectTouches(XObj* object, const Vec& pos, f32 radius, f32 height, CVec& hit, CVec& contact)
{
	CVec from(pos), to(pos);
	return (object->vfFC(&from, &to, radius, height, &hit, &contact, 0, 0) & 3) == 3;
}

/**
 * @note Address: 0x800CE354
 * @note Size: 0x5B0
 */
BOOL CheckCarriedObjectContacts(XActLift* act, const Vec* pos, f32 radius, f32 height, BOOL checkItem, BOOL checkEvent, BOOL checkOthers,
                                Vec* result)
{
	s16 item   = act->mTarget->GetItem();
	BOOL found = FALSE;
	for (s32 i = 0; i < gGame.mXObjs.mObjectCount; ++i) {
		XObj* object = gGame.GetObject(i);
		if (!(object->mStatus.mFlags & 1) || !(object->mStatus.mFlags & 0x10000))
			continue;
		f32 dx = object->mMoveTranslate.x - pos->x;
		f32 dz = object->mMoveTranslate.z - pos->z;
		if (sqrtf(dx * dx + dz * dz) > 100.0f + (radius + object->mFloorXZExtent))
			continue;
		CVec hit, contact;
		if (checkItem && (object->mStatus.mFlags & 0x40000000000ULL)) {
			if (CarriedObjectTouches(object, *pos, radius, height, hit, contact) && object->vf140(pos, item)) {
				found = TRUE;
				if (result)
					*result = contact;
			}
		}
		if (checkEvent && (object->mStatus.mFlags & 0x80000000000ULL)) {
			if (CarriedObjectTouches(object, *pos, radius, height, hit, contact)) {
				object->vf13C(&contact, act->mOwner->_158A8, act->mOwner->mItemTintWord);
				found = TRUE;
				if (result)
					*result = contact;
			}
		} else if (checkOthers && !(object->mStatus.mFlags & 0x2000000000000ULL) && !(object->mStatus.mFlags & 0x100000)
		           && object != act->mTarget && object != act->mOwner) {
			if (CarriedObjectTouches(object, *pos, radius, height, hit, contact)) {
				object->vf13C(&contact, act->mOwner->_158A8, act->mOwner->mItemTintWord);
				found = TRUE;
				if (result)
					*result = contact;
			}
		}
	}
	return found;
}

void UpdateCarriedObjectPosition(XActLift*, XObj*);
void HSD_JObjCopyMtx__FP9_HSD_JObjPA4_f(HSD_JObj*, MtxPtr);
void HSD_JObjClearMtxFlags__FP9_HSD_JObj(HSD_JObj*);
CVec GetStagePlacementScale(XObj*);
void GetJointScale(CJObj*, HSD_JObj*, Vec*);
f32 GetFirstFloorCollisionRadius(const XObj*);
extern const s32 lbl_8020A5E8[];

static inline HSD_JObj* HeldItemJoint(XActLift* act)
{
	s32 path[] = { 0, 0 };
	return GetObjectJointByPath(act->mOwner, 2, path);
}
static inline s32 HeldItemVariant(XActLift* act)
{
	return lbl_8020A5E8[(s16)GetOwnedInventoryItemCount(&act->mOwner->mParams, 0x38)];
}
static inline void ScaleHeldItem(XActLift* act, XObj* obj)
{
	Vec scale;
	GetJointScale(act->mOwner, HeldItemJoint(act), &scale);
	obj->SetScale(GetStagePlacementScale(obj) * scale, 0);
}
static inline s32 HeldIdleAnim(XActLift* act)
{
	switch (act->mOwner->mAnimId) {
	case 0x895:
		return 1;
	case 0x8b7:
		return 5;
	case 0x8ae:
		return 2;
	case 0x8af:
		return 3;
	case 0x8c5:
		return 14;
	case 0x8c0:
		return 9;
	case 0x8d2:
		return 17;
	case 0x8d3:
		return 18;
	case 0x8c1:
		return 10;
	case 0x8c2:
		return 11;
	case 0x8c3:
		return 12;
	case 0x8c4:
		return 13;
	case 0xe0:
		if (IsCarryingToothbrush(act))
			return 0;
		if (IsCarryingSpoon(act))
			return 1;
		if (IsCarryingSquirter(act))
			return HeldItemVariant(act) + 1;
		break;
	case 0xca:
		if (IsCarryingToothbrush(act))
			return 3;
		if (IsCarryingSpoon(act))
			return 4;
		if (IsCarryingSquirter(act))
			return HeldItemVariant(act) + 5;
		break;
	default:
		if (IsCarryingToothbrush(act))
			return 0;
		if (IsCarryingSpoon(act))
			return 1;
		if (IsCarryingSquirter(act))
			return HeldItemVariant(act) + 1;
		break;
	}
	return 0;
}
static inline HSD_JObj* HeldPlugEndJoint(XObj* obj)
{
	s32 path[] = { 0 };
	return GetObjectJointByPath(obj, 1, path);
}

/**
 * @note Address: 0x800CECFC
 * @note Size: 0x2040
 */
void UpdateHeldObject(XActLift* act, s32)
{
	if (!act->_24)
		return;
	XObj* obj = act->mTarget;
	if (!obj || !(obj->mStatus.mFlags & 1))
		return;
	f32 frame = GetJointAnimationCurrentFrame(act->mOwner), end = GetJointAnimationEndFrame(act->mOwner);
	switch (act->_24) {
	case 1:
		obj->mStatus.mFlags |= 0x0400000000000000ULL;
		act->_30        = obj->mMoveTranslate - GetPlayerCarryJointWorldPosition(act->mOwner);
		act->mYawOffset = obj->mMoveRotate.y - act->mOwner->mMoveRotate.y;
		obj->SetPos(GetPlayerCarryJointWorldPosition(act->mOwner) - act->_30 * (1.f - frame / end), 0);
		if (obj->GetCategory() == 24) {
			act->mYawOffset = 0;
			obj->SetRotateY(act->mYawOffset + act->mOwner->mMoveRotate.y, DEG2RAD * 6);
			obj->SetRotationX(0, 0);
			ResetPlayerPlugState((XObjPlug*)obj, TRUE);
		}
		if (obj->GetCategory() == 8 && obj->GetItem() == 22) {
			act->mYawOffset = 0;
			obj->SetRotateY(act->mYawOffset + act->mOwner->mMoveRotate.y, DEG2RAD * 6);
		}
		act->_24 = 2;
		// fall through
	case 2: {
		CVec offset(GetLiftTargetContactOffset(act, obj));
		f32 progress = 2.f * (frame / end);
		if (progress > 1)
			progress = 1;
		obj->SetPos((GetPlayerCarryJointWorldPosition(act->mOwner) + act->_30 * (1.f - progress)) - offset * progress, 0);
		break;
	}
	case 3:
		if (IsCarryingToothbrush(act) || IsCarryingSpoon(act) || IsCarryingSquirter(act)) {
			HSD_JObj* root = obj->mJObj;
			HSD_JObjCopyMtx__FP9_HSD_JObjPA4_f(root, HSD_JObjGetMtxPtr(HeldItemJoint(act)));
			if (IsCarryingSpoon(act))
				HeldAnim(obj, 4, TRUE);
			else if (IsCarryingSquirter(act))
				HeldAnim(obj, HeldItemVariant(act) + 5, TRUE);
		} else {
			UpdateCarriedObjectPosition(act, obj);
			ScaleHeldItem(act, obj);
			act->mYawOffset = 0;
			obj->SetRotationY(act->mYawOffset + act->mOwner->mMoveRotate.y, 0);
			if (IsCarryingMug(act))
				HeldAnim(obj, 2);
		}
		break;
	case 4: {
		if (obj->mStatus.mFlags & 0x800000) {
			obj->mStatus.mFlags &= ~0x800000ULL;
			obj->vfDC(FALSE, 0);
		}
		if (IsCarryingToothbrush(act) || IsCarryingSpoon(act) || IsCarryingSquirter(act)) {
			HSD_JObj* root = obj->mJObj;
			HSD_JObjCopyMtx__FP9_HSD_JObjPA4_f(root, HSD_JObjGetMtxPtr(HeldItemJoint(act)));
			HeldAnim(obj, HeldIdleAnim(act));
			break;
		}
		obj->SetRotationY(act->mYawOffset + act->mOwner->mMoveRotate.y, 0);
		UpdateCarriedObjectPosition(act, obj);
		if (IsTargetPool(act, 24)) {
			CJObj hand(HeldItemJoint(act));
			s32 path[] = { 0 };
			CJObj plug(GetObjectJointByPath(obj, 1, path));
			plug.SetRotationX(HSD_JObjGetRotationX(hand.mJObj), 0);
			plug.SetRotationZ(HSD_JObjGetRotationZ(hand.mJObj), 0);
		} else if (IsCarryingMug(act)) {
			s32 id = 4;
			if (act->mState != 31) {
				switch (act->mOwner->mAnimId) {
				case 12:
					id = 8;
					break;
				case 0x88:
					id = 9;
					break;
				case 13:
					id = 10;
					break;
				default:
					id = 1;
				}
			}
			HeldAnim(obj, id);
		}
		break;
	}
	case 5: {
		UpdateCarriedObjectPosition(act, obj);
		if (!(GetJointAnimationCurrentFrame(act->mOwner) >= 24))
			break;
		obj->SetTranslateY(act->mOwner->mMoveTranslate.y, 0);
		CVec push, contact;
		XObjChr* owner = act->mOwner;
		f32 radius     = GetFirstFloorCollisionRadius(owner);
		f32 height     = owner->GetCollisionHeight();
		if (obj->vfFC(&act->mOwner->mMoveTranslate, &act->mOwner->mMoveTranslate, radius, height, &push, &contact, 0, 0) & 1) {
			f32 distance = PSVECDistance(&act->mOwner->mMoveTranslate, &contact);
			obj->SetTranslateX(distance * (f32)sin(act->mOwner->mMoveRotate.y) + obj->mMoveTranslate.x, 0);
			obj->SetTranslateZ(distance * (f32)cos(act->mOwner->mMoveRotate.y) + obj->mMoveTranslate.z, 0);
		}
		if (obj->GetCategory() == 24)
			PlayGameSound(&gGame.mSound, 0x39, 255);
		else if (obj->GetCategory() != 8)
			PlayGameSound(&gGame.mSound, GetMaterialSoundId(obj->mFloors->mType >> 8), 255);
		obj->mStatus.mFlags &= ~0x0400000000000000ULL;
		act->mTarget = NULL;
		break;
	}
	case 6:
		UpdateCarriedObjectPosition(act, obj);
		if (obj->GetCategory() == 24) {
			CJObj hand(HeldItemJoint(act));
			CJObj end(HeldPlugEndJoint(obj));
			end.SetRotationX(HSD_JObjGetRotationX(hand.mJObj), 0);
			XObjPlug* plug = (XObjPlug*)obj;
			HSD_AObj* anim = GetAObj(act->mOwner->mJObj);
			if (anim->flags & 0x44000000)
				act->mTarget = NULL;
			else if (!plug->_D3C && (anim ? HSD_AObjGetCurrentFrame(anim) : 0.0f) >= 90) {
				plug->_D3C = act->_54 | 0x10000;
				obj->mStatus.mFlags &= ~0x0400000000000000ULL;
			} else if (!plug->_D3C && (anim ? HSD_AObjGetCurrentFrame(anim) : 0.0f) >= 34 && act->_54 == 0x200)
				end.SetRotationZ(-40.f * DEG2RAD, 0);
		}
		break;
	case 7:
		UpdateCarriedObjectPosition(act, obj);
		if (obj->GetCategory() == 24) {
			CJObj hand(HeldItemJoint(act));
			CJObj end(HeldPlugEndJoint(obj));
			end.SetRotationX(HSD_JObjGetRotationX(hand.mJObj), 0);
			XObjPlug* plug = (XObjPlug*)obj;
			HSD_AObj* anim = GetAObj(act->mOwner->mJObj);
			if (anim->flags & 0x44000000)
				act->_24 = 4;
			else if ((anim ? HSD_AObjGetCurrentFrame(anim) : 0.0f) >= 80)
				end.SetRotationZ(0, 0);
			else if (plug->_D3C && (anim ? HSD_AObjGetCurrentFrame(anim) : 0.0f) >= 76) {
				ResetPlayerPlugState(plug, FALSE);
				obj->mStatus.mFlags |= 0x0400000000000000ULL;
			}
		}
		break;
	case 8:
		UpdateCarriedObjectPosition(act, obj);
		if (obj->GetCategory() == 24) {
			CJObj hand(HeldItemJoint(act));
			CJObj end(HeldPlugEndJoint(obj));
			end.SetRotationX(HSD_JObjGetRotationX(hand.mJObj), 0);
			XObjPlug* plug = (XObjPlug*)obj;
			HSD_AObj* anim = GetAObj(act->mOwner->mJObj);
			if (anim->flags & 0x44000000)
				act->_24 = 9;
			else if (!plug->_D3C && (anim ? HSD_AObjGetCurrentFrame(anim) : 0.0f) >= 24)
				plug->_D3C = -1;
		}
		break;
	case 10:
		UpdateCarriedObjectPosition(act, obj);
		if (obj->GetCategory() == 24) {
			CJObj hand(HeldItemJoint(act));
			CJObj end(HeldPlugEndJoint(obj));
			end.SetRotationX(HSD_JObjGetRotationX(hand.mJObj), 0);
			XObjPlug* plug = (XObjPlug*)obj;
			HSD_AObj* anim = GetAObj(act->mOwner->mJObj);
			if (anim->flags & 0x44000000)
				act->_24 = 4;
			else if (plug->_D3C && (anim ? HSD_AObjGetCurrentFrame(anim) : 0.0f) >= 0)
				ResetPlayerPlugState(plug, FALSE);
		}
		break;
	case 11:
		UpdateCarriedObjectPosition(act, obj);
		if (obj->GetCategory() == 24) {
			CJObj hand(HeldItemJoint(act));
			CJObj end(HeldPlugEndJoint(obj));
			end.SetRotationX(HSD_JObjGetRotationX(hand.mJObj), 0);
			end.SetRotationZ(HSD_JObjGetRotationZ(hand.mJObj), 0);
			XObjPlug* plug = (XObjPlug*)obj;
			HSD_AObj* anim = GetAObj(act->mOwner->mJObj);
			if (anim->flags & 0x44000000)
				act->_24 = 12;
			else if (!plug->_D3C && (anim ? HSD_AObjGetCurrentFrame(anim) : 0.0f) >= 24)
				plug->_D3C = 0x10400;
		}
		break;
	case 12:
		UpdateCarriedObjectPosition(act, obj);
		if (obj->GetCategory() == 24) {
			CJObj hand(HeldItemJoint(act));
			CJObj end(HeldPlugEndJoint(obj));
			end.SetRotationX(HSD_JObjGetRotationX(hand.mJObj), 0);
			end.SetRotationZ(HSD_JObjGetRotationZ(hand.mJObj), 0);
		}
		break;
	case 13:
		UpdateCarriedObjectPosition(act, obj);
		if (obj->GetCategory() == 24) {
			CJObj hand(HeldItemJoint(act));
			CJObj end(HeldPlugEndJoint(obj));
			end.SetRotationX(HSD_JObjGetRotationX(hand.mJObj), 0);
			end.SetRotationZ(HSD_JObjGetRotationZ(hand.mJObj), 0);
			XObjPlug* plug = (XObjPlug*)obj;
			HSD_AObj* anim = GetAObj(act->mOwner->mJObj);
			if (anim->flags & 0x44000000)
				act->_24 = 4;
			else if (plug->_D3C && (anim ? HSD_AObjGetCurrentFrame(anim) : 0.0f) >= 0)
				ResetPlayerPlugState(plug, FALSE);
		}
		break;
	case 14:
		if (IsCarryingToothbrush(act) || IsCarryingSpoon(act) || IsCarryingSquirter(act)) {
			HSD_JObj* root = obj->mJObj;
			HSD_JObjCopyMtx__FP9_HSD_JObjPA4_f(root, HSD_JObjGetMtxPtr(HeldItemJoint(act)));
			if (IsCarryingSpoon(act))
				HeldAnim(obj, 4, TRUE);
			else if (IsCarryingSquirter(act))
				HeldAnim(obj, HeldItemVariant(act) + 5, TRUE);
		} else {
			UpdateCarriedObjectPosition(act, obj);
			ScaleHeldItem(act, obj);

			if (IsCarryingMug(act))
				HeldAnim(obj, 3);
		}
		break;
	case 15:
		if (obj->GetCategory() == 8) {
			SetOrAddInventoryItemCount(&gGame.GetPlayer()->mParams.mInventory, obj->GetItem(), 1, TRUE);
			((XObjItem*)obj)->SetFlag();
		}
		obj->Release();
		act->mTarget = NULL;
		break;
	case 16:
	case 18: {
		CVec offset(GetLiftTargetContactOffset(act, obj));
		Mtx trans, world;
		PSMTXTrans(trans, offset.x, -offset.y, offset.z);
		PSMTXConcat(HSD_JObjGetMtxPtr(HeldItemJoint(act)), trans, world);
		HSD_JObjCopyMtx__FP9_HSD_JObjPA4_f(obj->mJObj, world);
		Vec v;
		HSD_MtxGetTranslate(world, &v);
		obj->SetPos(v, 0);
		HSD_MtxGetScale(world, &v);
		obj->SetScale(v, 0);
		break;
	}
	case 17:
		if (obj->GetCategory() == 8) {
			((XObjItem*)obj)->SetFlag();
		}
		obj->Release();
		act->mTarget = NULL;
		break;
	case 19:
		HSD_JObjClearMtxFlags__FP9_HSD_JObj(obj->mJObj);
		act->mYawOffset = 0;
		obj->SetRotationY(act->mYawOffset + act->mOwner->mMoveRotate.y, 0);
		UpdateCarriedObjectPosition(act, obj);
		ScaleHeldItem(act, obj);
		act->_24 = 4;
		break;
	case 20:
		if (GetAObj(obj->mJObj)->flags & 0x44000000) {
			obj->Release();
			act->mTarget = NULL;
		}
		break;
	}
}

/**
 * @note Address: 0x800C7810
 * @note Size: 0x2B4
 */
void SetJointTranslationFromVectorByPath(CJObj* obj, s32 depth, s32* path, const Vec& pos)
{
	CJObj joint(GetJointByPath(obj->mJObj, path, depth));
	joint.SetTranslate(pos.x, pos.y, pos.z, TRUE);
}

// puts the carried object where he holds it
/**
 * @note Address: 0x800CE904
 * @note Size: 0xA4
 */
void UpdateCarriedObjectPosition(XActLift* act, XObj* target)
{
	target->SetPos(GetPlayerCarryJointWorldPosition(act->mOwner) - GetLiftTargetContactOffset(act, target), 0);
}

/**
 * @note Address: 0x800CE9A8
 * @note Size: 0x354
 */
// NONMATCHING: stack order
Vec GetLiftTargetContactOffset(XActLift* act, XObj* target)
{
	if (target->GetCategory() == 0x18) {
		s32 path[] = { 0 };
		CJObj joint(GetJointByPath(target->mJObj, path, 1));
		CVec pos(joint.GetPos());
		return pos - target->mMoveTranslate;
	}
	Vec offset = target->mMoveTranslate * -1.0f;
	return GetFloorContactPosition(target, act->mTargetFloor, &offset);
}

/**
 * @note Address: 0x800D0D3C
 * @note Size: 0x90
 */
void HSD_JObjClearMtxFlags__FP9_HSD_JObj(HSD_JObj* jobj)
{
	HSD_JObjClearMtxFlags(jobj);
}

// Both retail call sites read the result before the next call.
extern "C" const Vec* GetScaledTargetFloorExtent(const XAction* act)
{
	static Vec extent;
	const XObj* target = act->mTarget;
	const Vec& scaled  = target->mMoveScale * GetFloorExtent(&target->mFloors[act->mTargetFloor]);
	return static_cast<Vec*>(memcpy(&extent, &scaled, sizeof(extent)));
}

/**
 * @note Address: 0x800D0DCC
 * @note Size: 0xC0
 */
extern "C" f32 GetFirstFloorCollisionRadius(const XObj* object)
{
	return object->mMoveScale.x * GetFloorExtent(object->mFloors).x;
}

/**
 * @note Address: 0x800D0E8C
 * @note Size: 0xC0
 */
void HSD_JObjCopyMtx__FP9_HSD_JObjPA4_f(HSD_JObj* jobj, MtxPtr mtx)
{
	HSD_JObjCopyMtx(jobj, mtx);
}

/**
 * @note Address: 0x800D3A5C
 * @note Size: 0x94
 */
HSD_Generator* CreateParticleAtPosition(CGame* game, u32 bank, u32 id, s32 status, const Vec& pos, BOOL keep)
{
	Vec rot = { 0.0f, 0.0f, 0.0f };
	return (&game->mParticle)->CreateSRT(id | (bank << 16), status, pos, UnitScale(), rot, keep);
}

/**
 * @note Address: 0x800D3AF0
 * @note Size: 0xB4
 */
Vec GetJointMatrixScale(void* unused, HSD_JObj* jobj)
{
	Vec scale;
	HSD_MtxGetScale(HSD_JObjGetMtxPtr(jobj), &scale);
	return scale;
}

/**
 * @note Address: 0x800D3BA4
 * @note Size: 0x5C
 */
HSD_Generator* CreateScaledParticleAtPosition(CGame* game, u32 bank, u32 id, s32 status, const Vec& pos, const Vec& scale, BOOL keep)
{
	Vec rot = { 0.0f, 0.0f, 0.0f };
	return (&game->mParticle)->CreateSRT(id | (bank << 16), status, pos, scale, rot, keep);
}

/**
 * @note Address: 0x800D3C00
 * @note Size: 0xB8
 */
Vec GetObjectJointMatrixScale(const CJObj* obj)
{
	HSD_JObj* jobj = obj->mJObj;
	Vec scale;
	HSD_MtxGetScale(HSD_JObjGetMtxPtr(jobj), &scale);
	return scale;
}

/**
 * @note Address: 0x800D3CB8
 * @note Size: 0x24
 */
s32 RemovePlayerInventoryItemCount(Params* params, s16 id, s32 amount)
{
	return RemoveInventoryItemCount(&params->mInventory, id, amount);
}

/**
 * @note Address: 0x800D3CDC
 * @note Size: 0x30
 */
BOOL fn_800D3CDC(XObj* obj)
{
	return !(obj->mStatus.mFlags & 0x0040000000000000ULL);
}

/**
 * @note Address: 0x800D3D0C
 * @note Size: 0x64
 */
XObj* SpawnObjectFromPlacementIndex(CGame* game, s16 id)
{
	CoordInfo info(GetStageObjectPlacementByIndex(id));
	return SpawnObjectFromPlacement(&game->mXObjs, &info, 0, TRUE);
}

/**
 * @note Address: 0x800D3D70
 * @note Size: 0x44
 */
void SetClampedPlayerBattery(Params* params, f32 battery)
{
	params->SetBattery(battery);
}

/**
 * @note Address: 0x800D3DB4
 * @note Size: 0x20
 */
void EnableSubwindowWithFlags(CGame* game, u32 flags)
{
	game->mSubwin.mEnabled      = 1;
	game->mSubwin.mFlags.mFlags = 0;
	game->mSubwin.mFlags.mFlags |= flags;
}

/**
 * @note Address: 0x800D3DD4
 * @note Size: 0x24
 */
void SetSubwindowCamera(CGame* game, const Vec& interest, f32 angleY, f32 angleX, f32 distance, f32 fov)
{
	ConfigureSubwindowCamera(&game->mSubwin, interest, angleY, angleX, distance, fov);
}

/**
 * @note Address: 0x800D3DF8
 * @note Size: 0xB8
 */
Vec GetObjectJointWorldPosition(const CJObj* obj)
{
	HSD_JObj* jobj = obj->mJObj;
	Vec pos;
	HSD_MtxGetTranslate(HSD_JObjGetMtxPtr(jobj), &pos);
	return pos;
}

/**
 * @note Address: 0x800D3EB0
 * @note Size: 0x14
 */
void fn_800D3EB0(CGame* game, f32 a, f32 b, f32 c, f32 d)
{
	game->mSubwin._134 = a;
	game->mSubwin._138 = b;
	game->mSubwin._13C = c;
	game->mSubwin._140 = d;
}

/**
 * @note Address: 0x800D3EC4
 * @note Size: 0x50
 */
void ClearObjectAnimationQueue(XObj* obj)
{
	obj->mAnmQueue.Clear();
}

/**
 * @note Address: 0x800D3F14
 * @note Size: 0x1D8
 */
CXObj::CXObj(HSD_JObj* jobj)
{
	CJObj::Attach(jobj);
	SyncMoveFromJoint(this);
}

// picks the joint's srt up into the move values
/**
 * @note Address: 0x800D40EC
 * @note Size: 0x164
 */
void SyncMoveFromJoint(CXObj* obj)
{
	union {
		Vec vec;
		Quaternion quat;
	} value;
	HSD_JObjGetTranslation(obj->mJObj, &value.vec);
	obj->mMoveTranslate.Reset(value.vec);
	HSD_JObjGetRotation(obj->mJObj, (Quaternion*)&value.vec);
	obj->mMoveRotate.Reset(value.vec);
	HSD_JObjGetScale(obj->mJObj, &value.vec);
	obj->mMoveScale.Reset(value.vec);
}

/**
 * @note Address: 0x800D4250
 * @note Size: 0x44
 */
f32 GetObjectJointRotationY(CJObj* obj)
{
	return HSD_JObjGetRotationY(obj->mJObj);
}

/**
 * @note Address: 0x800D4294
 * @note Size: 0x60
 */
void fn_800D4294(XObjChr* chr, s32 anim, f32 blend)
{
	chr->SetAnim(anim, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, blend, 10.0f);
	chr->_A2C = 3;
}

/**
 * @note Address: 0x800D42F4
 * @note Size: 0x3C
 */
HSD_Generator* CreateParticleByBankAndId(CGame* game, s32 bank, s32 id, u8 type, HSD_JObj* jobj, BOOL keep)
{
	return (&game->mParticle)->Create(id | (bank << 16), type, jobj, keep);
}

/**
 * @note Address: 0x800D4330
 * @note Size: 0x10
 */
s32 GetInventoryCountByIndex(Params* params, u32 index)
{
	return params->mInventory.mItems[index].mCount;
}

/**
 * @note Address: 0x800D4340
 * @note Size: 0x28
 */
s16 GetObjectItemId(XObj* obj)
{
	return obj->GetItem();
}

/**
 * @note Address: 0x800D4368
 * @note Size: 0x48C
 */
void HandleLiftActionInput(XActLift* act, const CPadOne* pad, f32* weight)
{
	if (act->mState == 4) {
		XObjChr* chr = act->mOwner;
		if (chr->mJump.mState != 0) {
			return;
		}
		if (chr->mHang.mState != 0) {
			return;
		}
		if (!gGame.IsRunning()) {
			return;
		}
		if (gGame.mStatus.mFlags & 0x100) {
			return;
		}
		if (gGame.mEvt.mData.mScript != NULL) {
			return;
		}
		if (chr->mAnimId == 0x895 || chr->mAnimId == 0x8AE || chr->mAnimId == 0x8AF) {
			*weight = 0.0f;
			return;
		}
		if (pad->mTrigger & 0x100) {
			if (act->mTarget == NULL) {
				return;
			}
			if (act->mTarget->GetCategory() == 8) {
				return;
			}
			if (chr->mFloorContacts[4].objectIndex >= 0 && (gGame.GetObject(chr->mFloorContacts[4].objectIndex)->mStatus.mFlags & 0x20)) {
				*weight = 0.0f;
				return;
			}
			if (act->mTarget->GetCategory() == 0x18) {
				return;
			}
			act->mState = 6;
			*weight     = 0.0f;
		} else if (pad->mTrigger & 0x200) {
			if (act->mTarget != NULL) {
				if (act->mTarget->GetCategory() == 8) {
					if (chr->GetAttachment() == 0) {
						act->mState = 0x13;
					}
				} else {
					act->mState = 6;
				}
			}
			*weight = 0.0f;
		}
	} else if (act->mState == 0xC || act->mState == 0xD) {
		if (gGame.IsRunning() && gGame.mEvt.mData.mScript == NULL && ((pad->mTrigger & 0x100) || (pad->mTrigger & 0x200))) {
			act->mState = 0xE;
			act->_2C    = 0.0f;
		}
		*weight = 0.0f;
	} else if (act->mState == 0xF) {
		if (gGame.IsRunning()) {
			if (pad->mStickX > 0.8f) {
				act->mState = 0x10;
				act->_2C    = 0.0f;
			} else if (pad->mTrigger & 0x200) {
				act->mState = 0x11;
				act->_2C    = 0.0f;
			}
		}
		*weight = 0.0f;
	} else if (act->mState == 0x1B) {
		// stays as it is while the lift is thrown
	} else if (act->mState == 0x1C) {
	} else if (act->mState == 0x1F || act->mState == 0x20) {
		if (!(pad->mButton & 0x100)) {
			act->mState = 0x21;
		}
		*weight = 0.0f;
	} else if (act->mState != 0) {
		*weight = 0.0f;
	}
}

// the item particles' color callback, they take his item tint
/**
 * @note Address: 0x800D47F4
 * @note Size: 0x38
 */
s32 ApplyPlayerItemParticleColor(ItemPtclColor* particle)
{
	u32 color        = gGame.GetPlayer()->mItemTintWord;
	particle->mRed   = color >> 24;
	particle->mGreen = color >> 16;
	particle->mBlue  = color >> 8;
	return 0;
}

/**
 * @note Address: 0x800D482C
 * @note Size: 0x16C
 */
void ApplyPlayerItemTintToTarget(XAction* act)
{
	s32 path[]     = { 0, 1 };
	HSD_JObj* jobj = FindJointByPath(act->mTarget->mJObj, path, 2);
	for (HSD_DObj* dobj = HSD_JObjGetDObj(jobj); dobj != NULL; dobj = HSD_DObjGetNext(dobj)) {
		HSD_MObj* mobj = HSD_DObjGetMObj(dobj);
		if (mobj != NULL) {
			mobj->mat->diffuse = act->mOwner->mItemTint;
			for (HSD_TObj* tobj = HSD_MObjGetTObj(mobj); tobj != NULL; tobj = HSD_TObjGetNext(tobj)) {
				u32 flags   = tobj->flags & ~0xF0000;
				tobj->flags = flags;
				tobj->flags = flags | 0x60000;
			}
			HSD_MObjCompileTev(mobj);
		}
	}
}

/**
 * @note Address: 0x800D4998
 * @note Size: 0x1C8
 */
// NONMATCHING: regalloc
void PutAwayLiftedObject(XActLift* act)
{
	PutAway(act);
}

/**
 * @note Address: 0x800D4B60
 * @note Size: 0x118
 */
BOOL IsUsingMug(XActLift* act)
{
	return IsCarryingStill(act, 0x16);
}

/**
 * @note Address: 0x800D4C78
 * @note Size: 0xDC
 */
BOOL IsCarryingSquirter(XActLift* act)
{
	return IsCarrying(act, 0x38);
}

/**
 * @note Address: 0x800D4D54
 * @note Size: 0xDC
 */
BOOL IsCarryingMug(XActLift* act)
{
	return IsCarrying(act, 0x16);
}

/**
 * @note Address: 0x800D4E30
 * @note Size: 0xDC
 */
BOOL IsCarryingSpoon(XActLift* act)
{
	return IsCarrying(act, 0x15);
}

/**
 * @note Address: 0x800D4F0C
 * @note Size: 0xDC
 */
BOOL IsCarryingToothbrush(XActLift* act)
{
	return IsCarrying(act, 0x10);
}

/**
 * @note Address: 0x800D4FE8
 * @note Size: 0x4C
 */
BOOL IsLiftTargetPlug(XActLift* act)
{
	return IsTargetPool(act, 0x18);
}

/**
 * @note Address: 0x800D5034
 * @note Size: 0xA0
 */
void ResetPlayerLiftAction(XActLift* act)
{
	if (act->mTarget != NULL) {
		act->mTarget->mStatus.mFlags &= ~0x10000ULL;
		act->mTarget->mStatus.mFlags &= ~0x0400000000000000ULL;
	}
	act->Reset();
	act->mContactEffect = NULL;
	act->mUseEffect     = NULL;
	act->mToolEffect    = NULL;
}

/**
 * @note Address: 0x800D50D4
 * @note Size: 0xBC
 */
void InitializePlayerLiftAction(XActLift* act, XObjChr* owner)
{
	act->Init(owner);
	act->_2C = 0.0f;
	act->_48 = -1;
	ResetPlayerLiftAction(act);
}

/**
 * @note Address: 0x800D5190
 * @note Size: 0x39C
 */
Vec GetTargetFloorPosition(XAction* act)
{
	if (act->mTarget != NULL) {
		CVec position = act->mTarget->mMoveTranslate;
		CVec offset   = act->mTarget->mMoveScale * GetFloorOffset(&act->mTarget->mFloors[act->mTargetFloor]);
		CVec extent   = act->mTarget->mMoveScale * GetFloorExtent(&act->mTarget->mFloors[act->mTargetFloor]);
		f32 yaw       = act->mTarget->mMoveRotate.y + DEG2RAD * act->mTarget->mFloors[act->mTargetFloor].mYawDegrees;
		Mtx translate, local, rotate, scale, result;
		PSMTXTrans(translate, position.x, position.y, position.z);
		PSMTXTrans(local, offset.x, offset.y, offset.z);
		PSMTXScale(scale, extent.x, extent.y, extent.z);
		PSMTXRotRad(rotate, 'y', yaw);
		PSMTXConcat(translate, rotate, result);
		PSMTXConcat(result, local, result);
		PSMTXConcat(result, scale, result);
		Vec point = { 0.0f, 0.0f, 0.0f };
		PSMTXMultVec(result, &point, &point);
		return point;
	}
	Vec zero = { 0.0f, 0.0f, 0.0f };
	return zero;
}

/**
 * @note Address: 0x800CDBA8
 * @note Size: 0x578
 */
static inline u32 CheckHangClearance(XObj* obj, const CVec& from, const CVec& to, f32 radius, f32 height, Vec& push, Vec& contact)
{
	return obj->vfFC(&from, &to, radius, height, &push, &contact, 0, 0);
}

void TryLeaveHang(XActHang* act)
{
	const Vec& radiusExtent = GetFloorExtent(act->mOwner->mFloors);
	f32 radius              = act->mOwner->mMoveScale.x * radiusExtent.x;
	const Vec& extent       = GetFloorExtent(act->mOwner->mFloors);
	const Vec& offset       = GetFloorOffset(act->mOwner->mFloors);
	f32 halfHeight          = 0.5f * (act->mOwner->mMoveScale.x * (offset.y + extent.y));
	s32 path[]              = { 0 };
	Vec position            = GetEventJointWorldPosition(act->mOwner, 1, path);
	position.y += 0.5f;

	u32 blockingHit = 0;
	for (s32 i = 0; i < gGame.mXObjs.mObjectCount; i++) {
		XObj* obj = gGame.GetObject(i);
		if (!(obj->mStatus.mFlags & 1) || !(obj->mStatus.mFlags & 0x10000) || (obj->mStatus.mFlags & 0x100000))
			continue;
		if (obj->GetCategory() == 7 || obj->GetCategory() == 20 || (s32)act->mOwner->mId == (s32)obj->mId || obj->GetKind() >= 10000)
			continue;
		f32 dx = obj->mMoveTranslate.x - position.x;
		f32 dz = obj->mMoveTranslate.z - position.z;
		if (sqrtf(dx * dx + dz * dz) > 100.0f + (radius + obj->mFloorXZExtent))
			continue;

		CVec push, contact;
		u32 result = CheckHangClearance(obj, CVec(position), CVec(position), radius, halfHeight, push, contact);
		if ((result & 3) == 3) {
			blockingHit = result;
			break;
		}
	}
	if (blockingHit)
		return;
	if ((rand() & 7) == 0)
		act->mState = 7;
	else
		act->mState = 6;
}

} // extern "C"
