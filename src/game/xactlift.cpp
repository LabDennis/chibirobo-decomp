#include "game/xact.h"
#include "game/cgame.h"
#include "game/cvec.h"
#include "game/globals.h"
#include "game/gmath.h"
#include "game/hsd_dobj.h"
#include "game/objectfloorquery.h"
#include "game/xchr.h"
#include "game/xobjflags.h"

extern "C" {
#include <dolphin/mtx/mtx.h>
#include <sysdolphin/particle.h>
f32 GetFloorYawRadians(XObjFloor* const* floors, s32 index);
}

/**
 * @note Address: 0x800D4368
 * @note Size: 0x48C
 */
void XActLift::HandleInput(const CPadOne& pad, f32& moveWeight)
{
	if (mState == 4) {
		XObjChr* chr = mOwner;
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
			moveWeight = 0.0f;
			return;
		}
		// a sets it down (6), b puts an item away (19) and sets anything else down
		if (pad.mTrigger & PAD_BUTTON_A) {
			if (mTarget == NULL) {
				return;
			}
			if (mTarget->GetCategory() == 8) {
				return;
			}
			if (chr->mFloorContacts[4].objectIndex >= 0 && (gGame.GetObject(chr->mFloorContacts[4].objectIndex)->mStatus.mFlags & 0x20)) {
				moveWeight = 0.0f;
				return;
			}
			if (mTarget->GetCategory() == 0x18) {
				return;
			}
			mState     = 6;
			moveWeight = 0.0f;
		} else if (pad.mTrigger & PAD_BUTTON_B) {
			if (mTarget != NULL) {
				if (mTarget->GetCategory() == 8) {
					if (chr->GetAttachment() == AttachmentNone) {
						mState = 19;
					}
				} else {
					mState = 6;
				}
			}
			moveWeight = 0.0f;
		}
	} else if (mState == 12 || mState == 13) {
		// a or b stops giving power and pulls the plug (14)
		if (gGame.IsRunning() && gGame.mEvt.mData.mScript == NULL && ((pad.mTrigger & PAD_BUTTON_A) || (pad.mTrigger & PAD_BUTTON_B))) {
			mState = 14;
			mTimer = 0.0f;
		}
		moveWeight = 0.0f;
	} else if (mState == 15) {
		if (gGame.IsRunning()) {
			if (pad.mStickX > 0.8f) {
				mState = 16;
				mTimer = 0.0f;
			} else if (pad.mTrigger & PAD_BUTTON_B) {
				mState = 17;
				mTimer = 0.0f;
			}
		}
		moveWeight = 0.0f;
	} else if (mState == 31 || mState == 32) {
		// he uses the mug for as long as a is held
		if (!(pad.mButton & PAD_BUTTON_A)) {
			mState = 33;
		}
		moveWeight = 0.0f;
	} else if (mState != 0 && mState != 27 && mState != 28) {
		moveWeight = 0.0f;
	}
}

// the item particles' color callback, they take his item tint
/**
 * @note Address: 0x800D47F4
 * @note Size: 0x38
 */
extern "C" {

int ApplyPlayerItemParticleColor(HSD_Particle* particle)
{
	const GXColor& color = gGame.GetPlayer()->mItemTint;
	particle->primCol.r  = color.r;
	particle->primCol.g  = color.g;
	particle->primCol.b  = color.b;
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
				tobj->flags = (tobj->flags & ~TEX_COLORMAP_MASK) | TEX_COLORMAP_PASS;
			}
			HSD_MObjCompileTev(mobj);
		}
	}
}

/**
 * @note Address: 0x800D4998
 * @note Size: 0x1C8
 */
void PutAwayLiftedObject(XActLift* act)
{
	XObjChr* chr = act->mOwner;
	if (chr->mAnimId == 0x8B1 || chr->mAnimId == 0x8B3) {
		chr->SetAnim(0x8B2, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
	} else if (chr->mAnimId == 11) {
		chr->SetAnim(8, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
	}
	act->mState              = 0;
	act->mOwner->mAttachment = AttachmentNone;
	act->mTarget->mStatus.mFlags &= ~0x0200000000000000ULL;
	act->mTarget->SetAnim(7, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
	act->mTargetState = 20;
	XObjItem* item    = static_cast<XObjItem*>(act->mTarget);
	SetOrAddInventoryItemCount(&act->mOwner->mParams.mInventory, item->GetItem(), 1, TRUE);
	item->SetFlag();
	PlayGameSound(&gGame.mSound, 0x39, 0xFF);
}

static bool IsCarryingTool(const XActLift& action, s16 item)
{
	const XObj* target = action.mTarget;
	return target != NULL && (target->mStatus.mFlags & 0x400000000000ULL) && target->GetCategory() == 8 && target->GetItem() == item
	    && action.mTargetState != 15;
}

/**
 * @note Address: 0x800D4B60
 * @note Size: 0x118
 */
BOOL IsUsingMug(XActLift* act)
{
	return IsCarryingTool(*act, 0x16) && (act->mState >= 31 && act->mState <= 33);
}

/**
 * @note Address: 0x800D4C78
 * @note Size: 0xDC
 */
BOOL IsCarryingSquirter(XActLift* act)
{
	return IsCarryingTool(*act, 0x38);
}

/**
 * @note Address: 0x800D4D54
 * @note Size: 0xDC
 */
BOOL IsCarryingMug(XActLift* act)
{
	return IsCarryingTool(*act, 0x16);
}

/**
 * @note Address: 0x800D4E30
 * @note Size: 0xDC
 */
BOOL IsCarryingSpoon(XActLift* act)
{
	return IsCarryingTool(*act, 0x15);
}

/**
 * @note Address: 0x800D4F0C
 * @note Size: 0xDC
 */
BOOL IsCarryingToothbrush(XActLift* act)
{
	return IsCarryingTool(*act, 0x10);
}

/**
 * @note Address: 0x800D4FE8
 * @note Size: 0x4C
 */
// NONMATCHING: MWCC folds the two failure exits into beqlr/bnelr; retail branches to the shared clrlwi/blr return.
BOOL IsLiftTargetPlug(XActLift* act)
{
	bool result = false;
	if (act->mTarget != NULL && act->mTarget->GetCategory() == 24) {
		result = true;
	}
	return result;
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
	act->mTimer         = 0.0f;
	act->mPendingItemId = -1;
	ResetPlayerLiftAction(act);
}

/**
 * @note Address: 0x800D5190
 * @note Size: 0x39C
 */
Vec GetTargetFloorPosition(XAction* act)
{
	Vec position       = { 0.0f, 0.0f, 0.0f };
	const XObj* target = act->mTarget;
	if (target != NULL) {
		const XObjFloor& floor = target->mFloors[act->mTargetFloor];
		const Vec offset       = target->mMoveScale * ReadObjectFloorOffset(floor);
		const Vec extent       = target->mMoveScale * ReadObjectFloorExtent(floor);
		// Round the floor's local angle before composing it with the object's yaw.
		const f32 yaw = target->mMoveRotate.y + GetFloorYawRadians(&target->mFloors, act->mTargetFloor);
		Mtx transform;
		MakeFloorTransform(target, offset, extent, yaw, transform);
		PSMTXMultVec(transform, &position, &position);
	}
	return position;
}

} // extern "C"
