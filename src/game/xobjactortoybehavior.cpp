#include "game/xobjactor.h"
#include "game/xchr.h"
#include "game/cscrn.h"
#include "game/globals.h"

extern "C" {
void UpdateToy0FCarriedObjects(XObjActorToy* object, u32 frames);
void UpdateToy1FBJointScales(XObjActorToy* object, u32 frames);
void UpdateToy159JointScales(XObjActorToy* object, u32 frames);
void UpdateToy117JointVisibility(XObjActorToy* object, u32 frames);
void UpdateToy6FJointSpin(XObjActorToy* object, u32 frames);
void UpdateToy25EHeldObjects(XObjActorToy* object, u32 frames);
void UpdateToyJointSpin(XObjActorToy* object, u32 frames);
void UpdateToyJointVerticalOffset(XObjActorToy* object, u32 frames);
void UpdateToyJointExtension(XObjActorToy* object, u32 frames);
}

/**
 * @note Address: 0x8011E3A4
 * @note Size: 0x2E4
 */
// hides the item the toy dropped, the mHideKinds bits say which
extern "C" void HideToyDroppedItem(XObjActorToy* object)
{
	s32 id = -1;
	if (object->mHideKinds.Check(1)) {
		id = 0x3D;
		object->mHideKinds.mFlags &= ~1;
	} else if (object->mHideKinds.Check(2)) {
		id = 0x3E;
		object->mHideKinds.mFlags &= ~2;
	} else if (object->mHideKinds.Check(4)) {
		id = 0x3F;
		object->mHideKinds.mFlags &= ~4;
	} else if (object->mHideKinds.Check(8)) {
		id = 0x3B;
		object->mHideKinds.mFlags &= ~8;
	} else if (object->mHideKinds.Check(0x80)) {
		id = 0x21D;
		object->mHideKinds.mFlags &= ~0x80;
	} else if (object->mHideKinds.Check(0x10)) {
		id = 0x21E;
		object->mHideKinds.mFlags &= ~0x10;
	} else if (object->mHideKinds.Check(0x40)) {
		id = 0x341;
		object->mHideKinds.mFlags &= ~0x40;
	}
	if (id < 0) {
		return;
	}
	switch (id) {
	case 0x3B:
	case 0x3D:
	case 0x3E:
	case 0x3F:
	case 0x21D:
	case 0x21E:
		for (s32 i = 0; i < 64; i++) {
			XObj* obj = gGame.mXObjs.mPools[8].Get(i);
			if (id == obj->GetKind()) {
				obj->mStatus.mFlags &= ~0x10000ULL;
			}
		}
		break;
	case 0x341:
		for (s32 i = 0; i < 288; i++) {
			XObj* obj = gGame.mXObjs.mPools[22].Get(i);
			if (id == obj->GetKind()) {
				obj->mStatus.mFlags &= ~0x10000ULL;
			}
		}
		break;
	}
}

/**
 * @note Address: 0x8011E688
 * @note Size: 0x254
 */
extern "C" void HandleToyKind25ECommand(XObjActorToy* object, u32 frames)
{
	switch (object->mCmd[0]) {
	case 0:
		break;
	case 1:
		object->mAttachedId = object->mCmd[1];
		object->mState      = &XObjActorToy::StatePickUp;
		object->ResetMovementStep();
		object->mCmd[0] = 0;
		break;
	case 2:
		object->mAttachedId = -1;
		if (object->mAttached != NULL) {
			object->mAttached->Release();
		}
		object->mCmd[0] = 0;
		break;
	case 3:
		HideToyDroppedItem(object);
		switch (object->mCmd[1]) {
		case 0x341:
			for (s32 i = 0; i < 288; i++) {
				XObj* obj = gGame.mXObjs.mPools[22].Get(i);
				if (object->mCmd[1] == obj->GetKind()) {
					object->mHeldObjects[0] = obj;
					obj->mStatus.mFlags |= 0x10000ULL;
					switch (object->mCmd[1]) {
					case 0x341:
						object->mHideKinds.mFlags |= 0x40;
						break;
					}
				}
			}
			break;
		default:
			if (object->mHeldObjects[0] != NULL) {
				HSD_JObjClearMtxFlags(object->mHeldObjects[0]->mJObj);
				object->mHeldObjects[0] = NULL;
			}
			break;
		}
		object->mCmd[0] = 0;
		break;
	}
}

/**
 * @note Address: 0x8011E8DC
 * @note Size: 0xD0
 */
extern "C" void HandleToyScreenShakeCommand(XObjActorToy* object, u32 frames)
{
	switch (object->mCmd[0]) {
	case 1:
		StartScreenShake(&gGame.mQuake, object->mCmd[1], object->mCmd[2], object->mCmd[3], 0.0f, 0.0f, 0.0f, object->mCmd[4] / 100.0f);
		object->mCmd[0] = 0;
		break;
	}
}

/**
 * @note Address: 0x8011E9AC
 * @note Size: 0x140
 */
extern "C" void HandleToyKind1FCCommand(XObjActorToy* object, u32 frames)
{
	switch (object->mCmd[0]) {
	case 0:
		object->mCmd[0] = 0;
		break;
	case 1:
		StartScreenShake(&gGame.mQuake, object->mCmd[1], object->mCmd[2], object->mCmd[3], 0.0f, 0.0f, 0.0f, object->mCmd[4] / 100.0f);
		object->mCmd[0] = 0;
		break;
	case 2:
		switch (object->mCmd[1]) {
		case 0:
			object->mKindFlags.mFlags |= 1;
			object->mCmd[0] = 0;
			break;
		case 1:
			object->mKindFlags.mFlags &= ~1;
			object->mCmd[0] = 0;
			break;
		}
		break;
	}
}

/**
 * @note Address: 0x8011EAEC
 * @note Size: 0xAC
 */
extern "C" void HandleToyKind159Command(XObjActorToy* object, u32 frames)
{
	switch (object->mCmd[0]) {
	case 0:
		break;
	case 1:
		object->mAttachedId = object->mCmd[1];
		object->mState      = &XObjActorToy::StatePickUp;
		object->ResetMovementStep();
		object->mCmd[0] = 0;
		break;
	case 2:
		object->mAttachedId = -1;
		if (object->mAttached != NULL) {
			object->mAttached->Release();
		}
		object->mCmd[0] = 0;
		break;
	}
}

/**
 * @note Address: 0x8011EB98
 * @note Size: 0x80
 */
extern "C" void HandleToyKind16CCommand(XObjActorToy* object, u32 frames)
{
	switch (object->mCmd[0]) {
	case 0:
		break;
	// 1 starts its blinking away (step 2 in ProcessPendingEvents), 2 tells the script whether it is still drawn
	case 1:
		object->mKindFlags.mFlags |= 8;
		object->mCmd[0] = 0;
		break;
	case 2:
		if (object->mStatus.mFlags & 0x10000) {
			gEventVars.mVars[object->mCmd[1]] = 1;
		}
		object->mCmd[0] = 0;
		break;
	}
}

/**
 * @note Address: 0x8011EC18
 * @note Size: 0x130
 */
extern "C" void HandleToyKind156Command(XObjActorToy* object, u32 frames)
{
	switch (object->mCmd[0]) {
	case 1:
		object->mCmd[0] = 0;
		break;
	case 2:
		if (object->mMoveTranslate.y <= 2.0f) {
			gEventVars.mVars[object->mCmd[1]] = 1;
		}
		object->mCmd[0] = 0;
		break;
	case 3:
		if (object->mKindFlags.mFlags & 0x10) {
			gEventVars.mVars[object->mCmd[1]] = 1;
		}
		object->mCmd[0] = 0;
		break;
	case 4:
		object->mAttachedId = object->mCmd[1];
		object->mState      = &XObjActorToy::StatePickUp;
		object->ResetMovementStep();
		object->mCmd[0] = 0;
		break;
	case 5:
		object->mAttachedId = -1;
		if (object->mAttached != NULL) {
			object->mAttached->Release();
		}
		object->mCmd[0] = 0;
		break;
	}
}

/**
 * @note Address: 0x8011ED48
 * @note Size: 0x144
 */
extern "C" void HandleToyKind110Command(XObjActorToy* object, u32 frames)
{
	switch (object->mCmd[0]) {
	case 0:
		break;
	case 1:
		switch (object->mCmd[1]) {
		case 0: {
			object->mKindFlags.mFlags &= ~1;
			object->mMoveSpeed = 0.0f;
			object->mKindStep  = 0;
			CVec pos(object->mMoveTranslate.x, object->mMoveTranslate.y, object->mMoveTranslate.z);
			object->SetMoveDir(&pos, 0.0f);
			break;
		}
		case 1:
			object->mKindFlags.mFlags |= 1;
			object->mVelocity.x = object->mCmd[2] / 100.0f;
			object->mVelocity.y = object->mCmd[3] / 100.0f;
			object->mVelocity.z = object->mCmd[4] / 100.0f;
			object->mKindStep   = 1;
			break;
		}
		object->mCmd[0] = 0;
		break;
	}
}

/**
 * @note Address: 0x8011EE8C
 * @note Size: 0x410
 */
extern "C" void HandleToyKind0FCommand(XObjActorToy* object, u32 frames)
{
	s32 count;
	switch (object->mCmd[0]) {
	case 0:
		break;
	case 1:
		switch (object->mCmd[1]) {
		// flag 1 keeps it turned to the player (UpdateToy0FCarriedObjects)
		case 0:
			object->mKindFlags.mFlags &= ~1;
			break;
		case 1:
			object->mKindFlags.mFlags |= 1;
			break;
		}
		object->mCmd[0] = 0;
		break;
	case 2:
		HideToyDroppedItem(object);
		switch (object->mCmd[1]) {
		case 0x3B:
		case 0x3D:
		case 0x3E:
		case 0x3F:
		case 0x21D:
		case 0x21E:
			for (s32 i = 0; i < 64; i++) {
				XObj* obj = gGame.mXObjs.mPools[8].Get(i);
				if (object->mCmd[1] == obj->GetKind()) {
					obj->mStatus.mFlags |= 0x10000ULL;
					object->mHeldObjects[0] = obj;
					switch (object->mCmd[1]) {
					case 0x3B:
						object->mHideKinds.mFlags |= 8;
						break;
					case 0x3D:
						object->mHideKinds.mFlags |= 1;
						break;
					case 0x3E:
						object->mHideKinds.mFlags |= 2;
						break;
					case 0x3F:
						object->mHideKinds.mFlags |= 4;
						break;
					case 0x21D:
						object->mHideKinds.mFlags |= 0x80;
						break;
					case 0x21E:
						object->mHideKinds.mFlags |= 0x10;
						break;
					}
				}
			}
			break;
		default:
			if (object->mHeldObjects[0] != NULL) {
				HSD_JObjClearMtxFlags(object->mHeldObjects[0]->mJObj);
				object->mHeldObjects[0] = NULL;
			}
			break;
		}
		object->mCmd[0] = 0;
		break;
	case 3:
		gEventVars.mVars[object->mCmd[1]] = CountSmallSpidersGrabbingPlayer();
		object->mCmd[0]                   = 0;
		break;
	case 4:
		count = 0;
		for (s32 i = 0; i < 0x100; i++) {
			InventoryItem* items = gGame.GetPlayer()->mParams.mInventory.mItems;
			s16 num              = items[i].GetCount();
			s16 id               = items[i].GetId();
			if (GetItemFlagsMasked(id, 4) && id >= 0 && num > 0) {
				count++;
			}
		}
		gGame.mEvt.mSelectResult = count;
		object->mCmd[0]          = 0;
		break;
	case 5:
		object->mAttachedId = object->mCmd[1];
		object->mState      = &XObjActorToy::StatePickUp;
		object->ResetMovementStep();
		object->mCmd[0] = 0;
		break;
	case 6:
		object->mAttachedId = -1;
		if (object->mAttached != NULL) {
			object->mAttached->Release();
		}
		object->mCmd[0] = 0;
		break;
	case 7:
		if (object->mCmd[1]) {
			object->mPieces[0].mId    = object->mCmd[2];
			object->mPieces[0].mState = 1;
		} else {
			for (u32 j = 0; j < 4; j++) {
				if (object->mPieces[j].mObj != NULL) {
					object->mPieces[j].mObj->Release();
				}
				object->mPieces[j].mId    = -1;
				object->mPieces[j].mState = 0;
				object->mPieces[j].mObj   = NULL;
			}
		}
		object->mCmd[0] = 0;
		break;
	case 8:
		gEventVars.mVars[object->mCmd[1]] = CountEmergedSmallSpiders();
		object->mCmd[0]                   = 0;
		break;
	}
}

/**
 * @note Address: 0x8011F29C
 * @note Size: 0x220
 */
void XObjActorToy::UpdateAfterAnim(u32 frames)
{
	XObjActor::UpdateAfterAnim(frames);
	switch (GetKind()) {
	case 0xF:
		UpdateToy0FCarriedObjects(this, frames);
		break;
	case 0x6F:
		UpdateToy6FJointSpin(this, frames);
		break;
	case 0x110:
		break;
	case 0x117:
		UpdateToy117JointVisibility(this, frames);
		break;
	case 0x156:
		// flag 1: it rides on toy 0x157 (OnPushDisplacement)
		if (mKindFlags.mFlags & 1) {
			for (s32 i = 0; i < 15; i++) {
				if (GetToy(i)->GetKind() == 0x157) {
					Vec pos          = GetToy(i)->mMoveTranslate;
					mMoveTranslate.x = pos.x;
					mMoveTranslate.y = pos.y;
					mMoveTranslate.z = pos.z;
				}
			}
		}
		break;
	case 0x158:
		break;
	case 0x159:
		UpdateToy159JointScales(this, frames);
		break;
	case 0x16C:
		break;
	case 0x1D3:
		break;
	case 0x1FB:
		UpdateToy1FBJointScales(this, frames);
		break;
	case 0x1FC:
		UpdateToyJointExtension(this, frames);
		break;
	case 0x26C:
	case 0x485:
		UpdateToyJointVerticalOffset(this, frames);
		break;
	case 0x26D:
		UpdateToyJointSpin(this, frames);
		break;
	case 0x25E:
		UpdateToy25EHeldObjects(this, frames);
		break;
	}
}
