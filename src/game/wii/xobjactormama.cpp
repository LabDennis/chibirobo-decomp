#include "game/wii/cgame.h"
#include "game/wii/ceffect.h"
#include "game/wii/xobj.h"
#include "game/cscrn.h"
#include "game/gmath.h"
#include <math.h>
#include <stdlib.h>

extern "C" {
void UpdateAfterAnim__9XObjActorFUl(XObjActor* actor, u32 frames);
EffectInstance* fn_800A11A4(EffectInstance* effects, u32 id, s32 group);
}

static inline void MamaPropVisibility(XObjActorMama* mama, XObj* prop)
{
	if (mama->mStatus.mFlags & 0x10000) {
		prop->mStatus.mFlags |= 0x10000ULL;
	} else {
		prop->mStatus.mFlags &= ~0x10000ULL;
	}
}

void XObjActorMama::ResetActorState()
{
	WiiPlayerState* player = gGame.GetPlayer();
	if (player->mPlayerFlags & 0x40000000) {
		player->RestoreControl();
	}
	mActorStatus.mFlags = 0;
	mHideKinds.mFlags   = 0;
	mHeading            = 0.0f;
	mMoveSpeed          = 0.0f;
	mFacing             = 0.0f;
	_D78                = mFacing;
	_D7C                = 0;
	mState              = static_cast<StateFunc>(&XObjActorMama::StateNone);
	ResetMovementStep();
}

void XObjActorMama::Drop()
{
	s32 kind = -1;
	if (mHideKinds.Check(4)) {
		mHideKinds.mFlags &= ~4;
		kind = 0x7D;
	} else if (mHideKinds.Check(1)) {
		mHideKinds.mFlags &= ~1;
		kind = 0x40;
	} else if (mHideKinds.Check(2)) {
		mHideKinds.mFlags &= ~2;
		kind = 0x5B;
	} else if (mHideKinds.Check(8)) {
		mHideKinds.mFlags &= ~8;
		kind = 0x196;
	} else if (mHideKinds.Check(0x10)) {
		mHideKinds.mFlags &= ~0x10;
		kind = 0x1A1;
	} else if (mHideKinds.Check(0x20)) {
		mHideKinds.mFlags &= ~0x20;
		kind = 0x45B;
	}
	if (kind < 0) {
		return;
	}
	switch (kind) {
	case 0x40:
		for (s32 i = 0; i < 32; i++) {
			XObj* obj = gGame.mXObjs.mPools[8].Get(i);
			if (kind == obj->GetKind()) {
				obj->mStatus.mFlags &= ~0x10000ULL;
			}
		}
		break;
	case 0x5B:
	case 0x7D:
	case 0x196:
	case 0x45B:
		for (s32 i = 0; i < 400; i++) {
			XObj* obj = gGame.mXObjs.mPools[22].Get(i);
			if (kind == obj->GetKind()) {
				obj->mStatus.mFlags &= ~0x10000ULL;
			}
		}
		break;
	case 0x1A1:
		for (s32 i = 0; i < 200; i++) {
			XObj* obj = gGame.mXObjs.mPools[22].Get(i);
			if (obj->GetKind() == 0x1A0 || kind == obj->GetKind()) {
				obj->mStatus.mFlags &= ~0x10000ULL;
			}
		}
		break;
	}
}

void XObjActorMama::ProcessPendingEvents(u32 frames)
{
	WiiPlayerState* player = gGame.GetPlayer();
	if (mActorStatus.Check(0x400)) {
		mActorStatus.mFlags &= ~0x400;
		mHeadPitch = 0.0f;
	}
	if (mActorStatus.Check(2)) {
		mActorStatus.mFlags &= ~2;
		PlayWiiSoundEffect(&gGame.mSound, rand() % 10 + 0x10B, 1.0f, 0.0f);
	}
	switch (mCmd[0]) {
	case 2:
		Drop();
		switch (mCmd[1]) {
		case 0x40:
			for (s32 i = 0; i < 32; i++) {
				XObj* obj = gGame.mXObjs.mPools[8].Get(i);
				if (mCmd[1] == obj->GetKind()) {
					mHeldObjects[0] = obj;
					obj->mStatus.mFlags |= 0x10000ULL;
					mHideKinds.mFlags |= 1;
				}
			}
			break;
		case 0x5B:
		case 0x7D:
		case 0x196:
		case 0x45B:
			for (s32 i = 0; i < 400; i++) {
				XObj* obj = gGame.mXObjs.mPools[22].Get(i);
				if (mCmd[1] == obj->GetKind()) {
					obj->mStatus.mFlags |= 0x10000ULL;
					mHeldObjects[0] = obj;
					switch (mCmd[1]) {
					case 0x5B:
						mHideKinds.mFlags |= 2;
						break;
					case 0x7D:
						mHideKinds.mFlags |= 4;
						break;
					case 0x196:
						mHideKinds.mFlags |= 8;
						break;
					case 0x45B:
						mHideKinds.mFlags |= 0x20;
						break;
					}
				}
			}
			break;
		case 0x1A1:
			for (s32 i = 0; i < 200; i++) {
				XObj* obj = gGame.mXObjs.mPools[22].Get(i);
				if (obj->GetKind() == 0x1A0) {
					obj->mStatus.mFlags |= 0x10000ULL;
					mHeldObjects[0] = obj;
				} else if (obj->GetKind() == 0x1A1) {
					obj->mStatus.mFlags |= 0x10000ULL;
					mHeldObjects[1] = obj;
				}
			}
			mHideKinds.mFlags |= 0x10;
			break;
		default:
			for (s32 i = 0; i < 2; ++i) {
				if (mHeldObjects[i] != NULL) {
					mHeldObjects[i]->ClearBaseMtx();
					mHeldObjects[i] = NULL;
				}
			}
			break;
		}
		mCmd[0] = 0;
		break;
	case 3:
		switch (mCmd[1]) {
		case 1:
			mActorStatus.mFlags |= 0x100;
			player->LockControl();
			player->SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			break;
		default: {
			mActorStatus.mFlags &= ~0x100;
			CVec pos = GetJointPos(FindResNode("hand_r_4"));
			pos.y    = mMoveTranslate.y;
			player->SetPos(pos);
			player->RestoreControl();
			break;
		}
		}
		mCmd[0] = 0;
		break;
	case 4:
		StartScreenShake(&gGame.mQuake, mCmd[1], mCmd[2], mCmd[3], 0.0f, 0.0f, 0.0f, mCmd[4] / 100.0f);
		mCmd[0] = 0;
		break;
	case 5:
		mCmd[0] = 0;
		break;
	case 6: {
		CVec pos = GetJointPos(FindResNode("hand_l_4"));
		pos.x += 8.0f * (f32)sin(GetRotY());
		pos.y += 1.0f;
		pos.z += 8.0f * (f32)cos(GetRotY());
		EffectInstance* effect = fn_800A11A4((EffectInstance*)&gGame.mEvtPtcl, 0x82, 0);
		if (effect != NULL) {
			effect->pos   = pos;
			effect->rot   = (Vec) { 0.0f, GetRotY(), 0.0f };
			effect->scale = (Vec) { 1.0f, 1.0f, 1.0f };
			UpdateEffectMtx(effect);
		}
		mCmd[0] = 0;
		break;
	}
	}
}

void XObjActorMama::UpdateAfterAnim(u32 frames)
{
	UpdateAfterAnim__9XObjActorFUl(this, frames);
	WiiPlayerState* player = gGame.GetPlayer();
	if (mHideKinds.mFlags & 0x3F) {
		CVec position = GetJointPos(FindResNode("hand_r_4"));
		s32 kind      = -1;
		if (mHideKinds.Check(1)) {
			kind = 0x40;
		} else if (mHideKinds.Check(2)) {
			kind = 0x5B;
		} else if (mHideKinds.Check(4)) {
			kind = 0x7D;
		} else if (mHideKinds.Check(8)) {
			kind = 0x196;
		} else if (mHideKinds.Check(16)) {
			kind = 0x1A1;
		} else if (mHideKinds.Check(32)) {
			kind = 0x45B;
		}
		Mtx mount, translation, rotation, result;
		if (kind >= 0) {
			switch (kind) {
			case 0x40:
				for (s32 i = 0; i < 32; ++i) {
					XObj* prop = gGame.mXObjs.mPools[8].Get(i);
					if (kind == prop->GetKind()) {
						PSMTXCopy(GetWorldMtx(FindResNode("hand_r_4")), mount);
						PSMTXRotRad(rotation, 'z', -PI / 4);
						PSMTXConcat(mount, rotation, result);
						PSMTXTrans(translation, 0.0f, -3.0f, 0.0f);
						PSMTXConcat(result, translation, result);
						prop->SetBaseMtx(result);
						MamaPropVisibility(this, prop);
					}
				}
				break;
			case 0x5B:
			case 0x7D:
			case 0x196:
			case 0x45B:
				for (s32 i = 0; i < 400; ++i) {
					XObj* prop = gGame.mXObjs.mPools[22].Get(i);
					if (kind != prop->GetKind()) {
						continue;
					}
					switch (kind) {
					case 0x5B:
						PSMTXCopy(GetWorldMtx(FindResNode("hand_r_4")), mount);
						PSMTXRotRad(rotation, 'z', -PI / 4);
						PSMTXConcat(mount, rotation, result);
						PSMTXTrans(translation, 0.0f, -3.0f, 0.0f);
						PSMTXConcat(result, translation, result);
						prop->SetBaseMtx(result);
						break;
					case 0x7D:
						PSMTXCopy(GetWorldMtx(FindResNode("hand_r_4")), mount);
						PSMTXRotRad(rotation, 'y', 1.17809725f);
						PSMTXConcat(mount, rotation, result);
						prop->SetBaseMtx(result);
						break;
					case 0x196:
						sin(DEG2RAD * ((s32)(mKindCounter * 16) % 360));
						++mKindCounter;
						PSMTXCopy(GetWorldMtx(FindResNode("hand_r_4")), mount);
						PSMTXRotRad(rotation, 'x', -HALF_PI);
						PSMTXConcat(mount, rotation, result);
						PSMTXTrans(translation, 2.5f, -3.0f, -8.5f);
						PSMTXConcat(result, translation, result);
						PSMTXRotRad(rotation, 'y', 1.17809725f);
						PSMTXConcat(result, rotation, result);
						prop->SetBaseMtx(result);
						break;
					case 0x45B:
						PSMTXCopy(GetWorldMtx(FindResNode("hand_r_4")), mount);
						PSMTXRotRad(rotation, 'y', -1.37444687f);
						PSMTXConcat(mount, rotation, result);
						PSMTXRotRad(rotation, 'z', HALF_PI);
						PSMTXConcat(result, rotation, result);
						PSMTXRotRad(rotation, 'x', 0.0f);
						PSMTXConcat(result, rotation, result);
						PSMTXTrans(translation, -4.5f, -6.0f, 0.0f);
						PSMTXConcat(result, translation, result);
						prop->SetBaseMtx(result);
						break;
					}
					MamaPropVisibility(this, prop);
				}
				break;
			case 0x1A1:
				for (s32 i = 0; i < 200; ++i) {
					XObj* prop = gGame.mXObjs.mPools[22].Get(i);
					if (prop->GetKind() == 0x1A0) {
						PSMTXCopy(GetWorldMtx(FindResNode("hand_l_4")), mount);
						PSMTXRotRad(rotation, 'z', -0.19634955f);
						PSMTXConcat(mount, rotation, result);
						PSMTXTrans(translation, 0.0f, -4.5f, 0.0f);
						PSMTXConcat(result, translation, result);
						prop->SetBaseMtx(result);
						MamaPropVisibility(this, prop);
					} else if (prop->GetKind() == 0x1A1) {
						PSMTXCopy(GetWorldMtx(FindResNode("hand_r_4")), mount);
						PSMTXRotRad(rotation, 'x', -1.17809725f);
						PSMTXConcat(mount, rotation, result);
						PSMTXTrans(translation, 0.0f, -3.5f, -3.0f);
						PSMTXConcat(result, translation, result);
						PSMTXRotRad(rotation, 'z', -HALF_PI);
						PSMTXConcat(result, rotation, result);
						prop->SetBaseMtx(result);
						MamaPropVisibility(this, prop);
					}
				}
				break;
			}
		}
	}
	if ((mActorStatus.mFlags & 0x100) && (player->mPlayerFlags & 0x40000000)) {
		CVec pos = GetJointPos(FindResNode("hand_r_4"));
		pos.y += 2.0f;
		player->SetPos(pos);
		pos = GetJointPos(FindResNode("face"));
		player->SetRotateY(atan2(pos.x - player->mMoveTranslate.x, pos.z - player->mMoveTranslate.z), 0.0f);
	}
}

void XObjActorMama::StateNone(u32 frames)
{
}
