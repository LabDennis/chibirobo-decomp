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
#include "game/cvec.h"
#include "game/cparticle.h"
#include "game/cscrn.h"
#include "game/csound.h"
#include "game/fbtexeff.h"
#include "game/globals.h"
#include "game/hsd_dobj.h"
#include "game/objectfloorquery.h"
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
#include <sysdolphin/particle.h>
}

// chibi robo's actions, the XAct kinds of XAction, and the game helpers they use

extern "C" {
void SetObjectJointTranslationByPath(CJObj* object, s32 depth, s32* path, f32 x, f32 y, f32 z);
CVec GetPlayerJointLocalTranslationByPath(CJObj* obj, s32 depth, s32* path);
void SetJointYawByPath(CXObj* obj, s32 depth, s32* path, f32 angle);
void SetJointTranslationFromVectorByPath(CJObj* obj, s32 depth, s32* path, const Vec& pos);
void RecordObjectPositionAndHeightChange(XObj* obj);
void ApplyPlayerJumpStrength(XActJump* act, f32 strength);
void TryLeaveHang(XActHang* act);
Vec GetTargetFloorPosition(XAction* act); // where the held floor of the target is
u32 CheckStageFlag(s32 stage, u32 mask);
void PutAwayLiftedObject(XActLift*);
}

#include "game/cjobjinlineattach.h"

extern "C" {

void GetJointScale(CJObj*, HSD_JObj*, Vec*);
f32 GetFirstFloorCollisionRadius(const XObj*);
extern const s32 gSquirterChargeByCount[];

static inline s32 HeldItemVariant(XActLift* act)
{
	return gSquirterChargeByCount[(s16)GetOwnedInventoryItemCount(&act->mOwner->mParams, 0x38)];
}

/**
 * @note Address: 0x800CECFC
 * @note Size: 0x2040
 */
void UpdateHeldObject(XActLift* act, s32)
{
	if (!act->mTargetState)
		return;
	XObj* obj = act->mTarget;
	if (!obj || !(obj->mStatus.mFlags & 1))
		return;
	f32 frame = GetJointAnimationCurrentFrame(act->mOwner), end = GetJointAnimationEndFrame(act->mOwner);
	switch (act->mTargetState) {
	case 1:
		obj->mStatus.mFlags |= 0x0400000000000000ULL;
		act->mLiftStartOffset = obj->mMoveTranslate - GetPlayerCarryJointWorldPosition(act->mOwner);
		act->mYawOffset       = obj->mMoveRotate.y - act->mOwner->mMoveRotate.y;
		obj->SetPos(GetPlayerCarryJointWorldPosition(act->mOwner) - act->mLiftStartOffset * (1.f - frame / end), 0);
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
		act->mTargetState = 2;
		// fall through
	case 2: {
		CVec offset(GetLiftTargetContactOffset(act, obj));
		f32 progress = 2.f * (frame / end);
		if (progress > 1)
			progress = 1;
		obj->SetPos((GetPlayerCarryJointWorldPosition(act->mOwner) + act->mLiftStartOffset * (1.f - progress)) - offset * progress, 0);
		break;
	}
	case 3:
		if (IsCarryingToothbrush(act) || IsCarryingSpoon(act) || IsCarryingSquirter(act)) {
			s32 path[]     = { 0, 0 };
			HSD_JObj* root = obj->mJObj;
			HSD_JObjCopyMtx(root, HSD_JObjGetMtxPtr(GetObjectJointByPath(act->mOwner, 2, path)));
			if (IsCarryingSpoon(act))
				obj->SetAnim(4, TRUE, NULL, NULL, NULL, 0, 1, 0, 10);
			else if (IsCarryingSquirter(act))
				obj->SetAnim(HeldItemVariant(act) + 5, TRUE, NULL, NULL, NULL, 0, 1, 0, 10);
		} else {
			UpdateCarriedObjectPosition(act, obj);
			s32 path[] = { 0, 0 };
			Vec scale;
			GetJointScale(act->mOwner, GetObjectJointByPath(act->mOwner, 2, path), &scale);
			obj->SetScale(GetStagePlacementScale(obj) * scale, 0);
			act->mYawOffset = 0;
			obj->SetRotationY(act->mYawOffset + act->mOwner->mMoveRotate.y, 0);
			if (IsCarryingMug(act))
				obj->SetAnim(2, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
		}
		break;
	case 4: {
		if (obj->mStatus.mFlags & 0x800000) {
			obj->mStatus.mFlags &= ~0x800000ULL;
			obj->SetDestinationAlpha(FALSE, 0);
		}
		if (IsCarryingToothbrush(act) || IsCarryingSpoon(act) || IsCarryingSquirter(act)) {
			s32 path[]     = { 0, 0 };
			HSD_JObj* root = obj->mJObj;
			HSD_JObjCopyMtx(root, HSD_JObjGetMtxPtr(GetObjectJointByPath(act->mOwner, 2, path)));
			s32 id = 0;
			switch (act->mOwner->mAnimId) {
			case 0x895:
				id = 1;
				break;
			case 0x8b7:
				id = 5;
				break;
			case 0x8ae:
				id = 2;
				break;
			case 0x8af:
				id = 3;
				break;
			case 0x8c5:
				id = 14;
				break;
			case 0x8c0:
				id = 9;
				break;
			case 0x8d2:
				id = 17;
				break;
			case 0x8d3:
				id = 18;
				break;
			case 0x8c1:
				id = 10;
				break;
			case 0x8c2:
				id = 11;
				break;
			case 0x8c3:
				id = 12;
				break;
			case 0x8c4:
				id = 13;
				break;
			case 0xe0:
				if (IsCarryingToothbrush(act))
					id = 0;
				else if (IsCarryingSpoon(act))
					id = 1;
				else if (IsCarryingSquirter(act))
					id = HeldItemVariant(act) + 1;
				break;
			case 0xca:
				if (IsCarryingToothbrush(act))
					id = 3;
				else if (IsCarryingSpoon(act))
					id = 4;
				else if (IsCarryingSquirter(act))
					id = HeldItemVariant(act) + 5;
				break;
			default:
				if (IsCarryingToothbrush(act))
					id = 0;
				else if (IsCarryingSpoon(act))
					id = 1;
				else if (IsCarryingSquirter(act))
					id = HeldItemVariant(act) + 1;
				break;
			}
			obj->SetAnim(id, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
			break;
		}
		obj->SetRotationY(act->mYawOffset + act->mOwner->mMoveRotate.y, 0);
		UpdateCarriedObjectPosition(act, obj);
		if (act->mTarget != NULL && act->mTarget->GetCategory() == 24) {
			s32 handPath[] = { 0, 0 };
			CJObj hand(GetObjectJointByPath(act->mOwner, 2, handPath));
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
			obj->SetAnim(id, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
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
		if (obj->TestFloorCollision(&act->mOwner->mMoveTranslate, &act->mOwner->mMoveTranslate, radius, height, &push, &contact, 0, 0)
		    & 1) {
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
			s32 handPath[] = { 0, 0 };
			CJObj hand(GetObjectJointByPath(act->mOwner, 2, handPath));
			s32 endPath[] = { 0 };
			CJObj end(GetObjectJointByPath(obj, 1, endPath));
			end.SetRotationX(HSD_JObjGetRotationX(hand.mJObj), 0);
			XObjPlug* plug = (XObjPlug*)obj;
			HSD_AObj* anim = GetAObj(act->mOwner->mJObj);
			if (anim->flags & 0x44000000)
				act->mTarget = NULL;
			else if (!plug->mConnection && (anim ? HSD_AObjGetCurrentFrame(anim) : 0.0f) >= 90) {
				plug->mConnection = act->mPlugConnection | 0x10000;
				obj->mStatus.mFlags &= ~0x0400000000000000ULL;
			} else if (!plug->mConnection && (anim ? HSD_AObjGetCurrentFrame(anim) : 0.0f) >= 34 && act->mPlugConnection == 0x200)
				end.SetRotationZ(-40.f * DEG2RAD, 0);
		}
		break;
	case 7:
		UpdateCarriedObjectPosition(act, obj);
		if (obj->GetCategory() == 24) {
			s32 handPath[] = { 0, 0 };
			CJObj hand(GetObjectJointByPath(act->mOwner, 2, handPath));
			s32 endPath[] = { 0 };
			CJObj end(GetObjectJointByPath(obj, 1, endPath));
			end.SetRotationX(HSD_JObjGetRotationX(hand.mJObj), 0);
			XObjPlug* plug = (XObjPlug*)obj;
			HSD_AObj* anim = GetAObj(act->mOwner->mJObj);
			if (anim->flags & 0x44000000)
				act->mTargetState = 4;
			else if ((anim ? HSD_AObjGetCurrentFrame(anim) : 0.0f) >= 80)
				end.SetRotationZ(0, 0);
			else if (plug->mConnection && (anim ? HSD_AObjGetCurrentFrame(anim) : 0.0f) >= 76) {
				ResetPlayerPlugState(plug, FALSE);
				obj->mStatus.mFlags |= 0x0400000000000000ULL;
			}
		}
		break;
	case 8:
		UpdateCarriedObjectPosition(act, obj);
		if (obj->GetCategory() == 24) {
			s32 handPath[] = { 0, 0 };
			CJObj hand(GetObjectJointByPath(act->mOwner, 2, handPath));
			s32 endPath[] = { 0 };
			CJObj end(GetObjectJointByPath(obj, 1, endPath));
			end.SetRotationX(HSD_JObjGetRotationX(hand.mJObj), 0);
			XObjPlug* plug = (XObjPlug*)obj;
			HSD_AObj* anim = GetAObj(act->mOwner->mJObj);
			if (anim->flags & 0x44000000)
				act->mTargetState = 9;
			else if (!plug->mConnection && (anim ? HSD_AObjGetCurrentFrame(anim) : 0.0f) >= 24)
				plug->mConnection = -1;
		}
		break;
	case 10:
		UpdateCarriedObjectPosition(act, obj);
		if (obj->GetCategory() == 24) {
			s32 handPath[] = { 0, 0 };
			CJObj hand(GetObjectJointByPath(act->mOwner, 2, handPath));
			s32 endPath[] = { 0 };
			CJObj end(GetObjectJointByPath(obj, 1, endPath));
			end.SetRotationX(HSD_JObjGetRotationX(hand.mJObj), 0);
			XObjPlug* plug = (XObjPlug*)obj;
			HSD_AObj* anim = GetAObj(act->mOwner->mJObj);
			if (anim->flags & 0x44000000)
				act->mTargetState = 4;
			else if (plug->mConnection && (anim ? HSD_AObjGetCurrentFrame(anim) : 0.0f) >= 0)
				ResetPlayerPlugState(plug, FALSE);
		}
		break;
	case 11:
		UpdateCarriedObjectPosition(act, obj);
		if (obj->GetCategory() == 24) {
			s32 handPath[] = { 0, 0 };
			CJObj hand(GetObjectJointByPath(act->mOwner, 2, handPath));
			s32 endPath[] = { 0 };
			CJObj end(GetObjectJointByPath(obj, 1, endPath));
			end.SetRotationX(HSD_JObjGetRotationX(hand.mJObj), 0);
			end.SetRotationZ(HSD_JObjGetRotationZ(hand.mJObj), 0);
			XObjPlug* plug = (XObjPlug*)obj;
			HSD_AObj* anim = GetAObj(act->mOwner->mJObj);
			if (anim->flags & 0x44000000)
				act->mTargetState = 12;
			else if (!plug->mConnection && (anim ? HSD_AObjGetCurrentFrame(anim) : 0.0f) >= 24)
				plug->mConnection = 0x10400;
		}
		break;
	case 12:
		UpdateCarriedObjectPosition(act, obj);
		if (obj->GetCategory() == 24) {
			s32 handPath[] = { 0, 0 };
			CJObj hand(GetObjectJointByPath(act->mOwner, 2, handPath));
			s32 endPath[] = { 0 };
			CJObj end(GetObjectJointByPath(obj, 1, endPath));
			end.SetRotationX(HSD_JObjGetRotationX(hand.mJObj), 0);
			end.SetRotationZ(HSD_JObjGetRotationZ(hand.mJObj), 0);
		}
		break;
	case 13:
		UpdateCarriedObjectPosition(act, obj);
		if (obj->GetCategory() == 24) {
			s32 handPath[] = { 0, 0 };
			CJObj hand(GetObjectJointByPath(act->mOwner, 2, handPath));
			s32 endPath[] = { 0 };
			CJObj end(GetObjectJointByPath(obj, 1, endPath));
			end.SetRotationX(HSD_JObjGetRotationX(hand.mJObj), 0);
			end.SetRotationZ(HSD_JObjGetRotationZ(hand.mJObj), 0);
			XObjPlug* plug = (XObjPlug*)obj;
			HSD_AObj* anim = GetAObj(act->mOwner->mJObj);
			if (anim->flags & 0x44000000)
				act->mTargetState = 4;
			else if (plug->mConnection && (anim ? HSD_AObjGetCurrentFrame(anim) : 0.0f) >= 0)
				ResetPlayerPlugState(plug, FALSE);
		}
		break;
	case 14:
		if (IsCarryingToothbrush(act) || IsCarryingSpoon(act) || IsCarryingSquirter(act)) {
			s32 path[]     = { 0, 0 };
			HSD_JObj* root = obj->mJObj;
			HSD_JObjCopyMtx(root, HSD_JObjGetMtxPtr(GetObjectJointByPath(act->mOwner, 2, path)));
			if (IsCarryingSpoon(act))
				obj->SetAnim(4, TRUE, NULL, NULL, NULL, 0, 1, 0, 10);
			else if (IsCarryingSquirter(act))
				obj->SetAnim(HeldItemVariant(act) + 5, TRUE, NULL, NULL, NULL, 0, 1, 0, 10);
		} else {
			UpdateCarriedObjectPosition(act, obj);
			s32 path[] = { 0, 0 };
			Vec scale;
			GetJointScale(act->mOwner, GetObjectJointByPath(act->mOwner, 2, path), &scale);
			obj->SetScale(GetStagePlacementScale(obj) * scale, 0);

			if (IsCarryingMug(act))
				obj->SetAnim(3, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
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
		s32 path[] = { 0, 0 };
		PSMTXConcat(HSD_JObjGetMtxPtr(GetObjectJointByPath(act->mOwner, 2, path)), trans, world);
		HSD_JObjCopyMtx(obj->mJObj, world);
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
	case 19: {
		HSD_JObjClearMtxFlags(obj->mJObj);
		act->mYawOffset = 0;
		obj->SetRotationY(act->mYawOffset + act->mOwner->mMoveRotate.y, 0);
		UpdateCarriedObjectPosition(act, obj);
		s32 path[] = { 0, 0 };
		Vec scale;
		GetJointScale(act->mOwner, GetObjectJointByPath(act->mOwner, 2, path), &scale);
		obj->SetScale(GetStagePlacementScale(obj) * scale, 0);
		act->mTargetState = 4;
		break;
	}
	case 20:
		if (GetAObj(obj->mJObj)->flags & 0x44000000) {
			obj->Release();
			act->mTarget = NULL;
		}
		break;
	}
}

/**
 * @note Address: 0x800D0DCC
 * @note Size: 0xC0
 */
extern "C" f32 GetFirstFloorCollisionRadius(const XObj* object)
{
	return object->mMoveScale.x * ReadObjectFloorExtent(object->mFloors[0]).x;
}

} // extern "C"
