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
void XObjActor3::UpdateObject(u32 frames)
{
	switch (GetItem()) {
	case 8:
		mActorFlags.mFlags |= 4; // flag 4 skips the heading/pos write below
		break;
	case 9:
		UpdateAnimationSounds();
		if (gGame.mStatus.mFlags & 0x400) {
			return;
		}
		break;
	case 10:
		if (gGame.mStatus.mFlags & 0x400) {
			return;
		}
		switch (mEventCommand) {
		case 1:
			mActorFlags.mFlags &= ~4;
			mEventCommand = 0;
			mEventArg1    = 0;
			break;
		}
		break;
	case 11:
	case 12:
		if (gGame.mStatus.mFlags & 0x400) {
			return;
		}
		switch (mEventCommand) {
		case 1:
			if (mEventArg1 != 0) {
				mActorFlags.mFlags &= ~4;
			} else {
				mActorFlags.mFlags |= 4;
				mKindStep = 0;
			}
			mEventCommand = 0;
			mEventArg1    = 0;
			break;
		case 2:
			mKindStep     = 2;
			mKindTimer    = mEventArg2 / 100.0f; // script units are 1/100
			mEventCommand = 0;
			mEventArg1    = 0;
			break;
		}
		break;
	case 13:
		switch (mRumble) {
		case 0:
			break;
		case 1: // start the rumble
			if (gGame.mStatus.mFlags & 0x800000000ULL) {
				CPadOne* rumble = &gGame.mPad.mPad[0];
				if (rumble->mConnected) {
					HSD_PadRumbleOn(rumble->mPort);
				}
				mRumbleTimer = 24 / frames;
				mRumble++;
			} else {
				mRumble = 0;
			}
			break;
		case 2:
			if (--mRumbleTimer < 0) {
				CPadOne* rumble = &gGame.mPad.mPad[0];
				if (rumble->mConnected) {
					HSD_PadRumbleOffN(rumble->mPort);
				}
				if (rumble->mConnected) {
					HSD_PadRumbleRemove(rumble->mPort);
				}
				mRumble = 0;
			}
			break;
		}
		if (gGame.mStatus.mFlags & 0x400) {
			return;
		}
		switch (mEventCommand) {
		case 1:
			mActorFlags.mFlags &= ~4;
			mEventCommand = 0;
			mEventArg1    = 0;
			break;
		}
		break;
	case 14:
		if (gGame.mStatus.mFlags & 0x400) {
			return;
		}
		HandleSpiderCounterCommand(this);
		break;
	}
	if (mActorFlags.mFlags & 0x10) { // flag 0x10 skips the update
		return;
	}
	(this->*gActor3UpdateHandlers[GetItem()])(frames);
	if (!(mActorFlags.mFlags & 4)) {
		SetRotateY(mHeading, 0.0f);
		SetPos(mMoveTranslate, 0);
	}
	UpdateObject__4XObjFUl(this, frames);
	if (GetKind() == 0x31B) { // the dust kind
		UpdateParticlePool(&mDust.mPtcl, frames);
	}
	if (GetKind() == 0x2C3) { // carries a rider at a joint
		CarryRiderAtActorJoint(this, frames);
	}
	if (mActorFlags.mFlags & 2) { // flag 2: carried parts follow their joints
		for (u32 i = 0; i < 8; i++) {
			if (mParts[i].mJoint != NULL && mParts[i].mObj != NULL) {
				Mtx t;
				XObj* obj = mParts[i].mObj;
				PSMTXTrans(t, obj->mMoveTranslate.x, obj->mMoveTranslate.y, obj->mMoveTranslate.z);
				PSMTXCopy(HSD_JObjGetMtxPtr(mParts[i].mJoint), mParts[i].mMtx);
				Vec scale;
				HSD_MtxGetScale(mParts[i].mMtx, &scale);
				scale.x = GetStagePlacementScale(mParts[i].mObj).x / scale.x;
				scale.y = GetStagePlacementScale(mParts[i].mObj).y / scale.y;
				scale.z = GetStagePlacementScale(mParts[i].mObj).z / scale.z;
				Mtx s;
				PSMTXScale(s, scale.x, scale.y, scale.z);
				Mtx m;
				PSMTXConcat(mParts[i].mMtx, s, m);
				PSMTXConcat(m, t, m);
				HSD_JObj* joint = mParts[i].mObj->mJObj;
				MtxPtr mp       = m;
				HSD_JObjCopyMtx(joint, mp);
			}
		}
	}
}
