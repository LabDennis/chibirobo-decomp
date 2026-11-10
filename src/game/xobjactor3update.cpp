#include "game/xobjactor.h"
#include "game/xchr.h"
#include <stdlib.h>

// XObjActor3's update

extern "C" {
CVec GetStagePlacementScale(XObj* obj);
void HSD_MtxGetScale(Mtx mat, Vec* vec);
void HandleSpiderCounterCommand(XObjActor3* actor);
void CarryRiderAtActorJoint(XObjActor3* actor, u32 frames);
}

/**
 * @note Address: 0x80109E80
 * @note Size: 0x680
 */
// the per-kind script commands, the kind's own function, the carried objects
extern "C" void UpdateObject__10XObjActor3FUl(XObjActor3* actor, u32 frames)
{
	switch (actor->GetItem()) {
	case 8:
		actor->_B8C.mFlags |= 4;
		break;
	case 9:
		actor->UpdateAnimationSounds();
		if (gGame.mStatus.mFlags & 0x400) {
			return;
		}
		break;
	case 10:
		if (gGame.mStatus.mFlags & 0x400) {
			return;
		}
		switch (actor->mEventCommand) {
		case 1:
			actor->_B8C.mFlags &= ~4;
			actor->mEventCommand = 0;
			actor->mEventArg1    = 0;
			break;
		}
		break;
	case 11:
	case 12:
		if (gGame.mStatus.mFlags & 0x400) {
			return;
		}
		switch (actor->mEventCommand) {
		case 1:
			if (actor->mEventArg1 != 0) {
				actor->_B8C.mFlags &= ~4;
			} else {
				actor->_B8C.mFlags |= 4;
				actor->_B80 = 0;
			}
			actor->mEventCommand = 0;
			actor->mEventArg1    = 0;
			break;
		case 2:
			actor->_B80          = 2;
			actor->_B78          = actor->mEventArg2 / 100.0f;
			actor->mEventCommand = 0;
			actor->mEventArg1    = 0;
			break;
		}
		break;
	case 13:
		switch (actor->mRumble) {
		case 0:
			break;
		case 1:
			if (gGame.mStatus.mFlags & 0x800000000ULL) {
				CPadOne* rumble = &gGame.mPad.mPad[0];
				if (rumble->mConnected) {
					HSD_PadRumbleOn(rumble->mPort);
				}
				actor->mRumbleTimer = 24 / frames;
				actor->mRumble++;
			} else {
				actor->mRumble = 0;
			}
			break;
		case 2:
			if (--actor->mRumbleTimer < 0) {
				CPadOne* rumble = &gGame.mPad.mPad[0];
				if (rumble->mConnected) {
					HSD_PadRumbleOffN(rumble->mPort);
				}
				if (rumble->mConnected) {
					HSD_PadRumbleRemove(rumble->mPort);
				}
				actor->mRumble = 0;
			}
			break;
		}
		if (gGame.mStatus.mFlags & 0x400) {
			return;
		}
		switch (actor->mEventCommand) {
		case 1:
			actor->_B8C.mFlags &= ~4;
			actor->mEventCommand = 0;
			actor->mEventArg1    = 0;
			break;
		}
		break;
	case 14:
		if (gGame.mStatus.mFlags & 0x400) {
			return;
		}
		HandleSpiderCounterCommand(actor);
		break;
	}
	if (actor->_B8C.mFlags & 0x10) {
		return;
	}
	(actor->*gActor3UpdateHandlers[actor->GetItem()])(frames);
	if (!(actor->_B8C.mFlags & 4)) {
		actor->SetRotateY(actor->_B7C, 0.0f);
		actor->SetPos(actor->mMoveTranslate, 0);
	}
	UpdateObject__4XObjFUl(actor, frames);
	if (actor->GetKind() == 0x31B) {
		UpdateParticlePool(&actor->mDust.mPtcl, frames);
	}
	if (actor->GetKind() == 0x2C3) {
		CarryRiderAtActorJoint(actor, frames);
	}
	if (actor->_B8C.mFlags & 2) {
		for (u32 i = 0; i < 8; i++) {
			if (actor->mParts[i].mJoint != NULL && actor->mParts[i].mObj != NULL) {
				Mtx t;
				XObj* obj = actor->mParts[i].mObj;
				PSMTXTrans(t, obj->mMoveTranslate.x, obj->mMoveTranslate.y, obj->mMoveTranslate.z);
				PSMTXCopy(HSD_JObjGetMtxPtr(actor->mParts[i].mJoint), actor->mParts[i].mMtx);
				Vec scale;
				HSD_MtxGetScale(actor->mParts[i].mMtx, &scale);
				scale.x = GetStagePlacementScale(actor->mParts[i].mObj).x / scale.x;
				scale.y = GetStagePlacementScale(actor->mParts[i].mObj).y / scale.y;
				scale.z = GetStagePlacementScale(actor->mParts[i].mObj).z / scale.z;
				Mtx s;
				PSMTXScale(s, scale.x, scale.y, scale.z);
				Mtx m;
				PSMTXConcat(actor->mParts[i].mMtx, s, m);
				PSMTXConcat(m, t, m);
				HSD_JObj* joint = actor->mParts[i].mObj->mJObj;
				MtxPtr mp       = m;
				HSD_JObjCopyMtx(joint, mp);
			}
		}
	}
}
