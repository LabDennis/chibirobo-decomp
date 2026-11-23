#include "game/wii/cgame.h"
#include "game/wii/xobj.h"
#include "game/cscrn.h"
#include "game/gmath.h"
#include <math.h>
#include <stdlib.h>

extern "C" void UpdateAfterAnim__9XObjActorFUl(XObjActor* actor, u32 frames);
extern "C" CVec GetStagePlacementScale(const XObj* obj);
extern "C" void HSD_MtxGetScale(Mtx matrix, Vec* scale);
extern "C" StagePlacement* GetStageObjectPlacementByIndex(s32 index);
extern "C" XObj* SpawnObjectFromPlacement(XOBJS* list, CoordInfo* info, s32 direct, BOOL async);
extern "C" XObj* ResolveEventObjectId(const CEvtData* data, s16 id);
extern "C" BOOL IsFree__12XObjActorToyFv(XObjActorToy* toy);
extern "C" void ResetActorLook(XObjActor* actor, f32 rate);
extern "C" void SetActorLookAtPosition(XObjActor* actor, const Vec* target, f32 rate);
extern "C" void SetActorLookTarget(XObjActor* actor, const Vec* target, f32 rate);
extern "C" void SetActorLookAngle(XObjActor* actor, f32 value, f32 rate);
extern "C" s32 HandleEventCommand__9XObjActorFlPl(XObjActor* actor, s32 count, s32* command);

static inline XObj* ReadObj(s16 id)
{
	return SpawnObjectFromPlacement(&gGame.mXObjs, &CoordInfo(GetStageObjectPlacementByIndex(id)), 0, TRUE);
}

static inline XObj* FindObj(s16 id)
{
	return gGame.mEvt.mData.mScript != NULL ? ResolveEventObjectId(&gGame.mEvt.mData, id) : NULL;
}

static inline void PapaPropVisibility(XObjActorPapa* papa, XObj* prop)
{
	if (papa->mStatus.mFlags & 0x10000) {
		prop->mStatus.mFlags |= 0x10000ULL;
	} else {
		prop->mStatus.mFlags &= ~0x10000ULL;
	}
}

void XObjActorPapa::ResetActorState()
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
	mJointShown         = 1;
	mState              = static_cast<StateFunc>(&XObjActorPapa::StateNone);
	ResetMovementStep();
}

void XObjActorPapa::Drop()
{
	s32 kind = -1;
	if (mHideKinds.Check(1)) {
		mHideKinds.mFlags &= ~1;
		kind = 0x1F4;
	} else if (mHideKinds.Check(2)) {
		mHideKinds.mFlags &= ~2;
		kind = 0x360;
	} else if (mHideKinds.Check(4)) {
		mHideKinds.mFlags &= ~4;
		kind = 0x1D3;
	} else if (mHideKinds.Check(8)) {
		mHideKinds.mFlags &= ~8;
		kind = 0x375;
	} else if (mHideKinds.Check(0x10)) {
		mHideKinds.mFlags &= ~0x10;
		kind = 0x3FA;
	} else if (mHideKinds.Check(0x20)) {
		mHideKinds.mFlags &= ~0x20;
		kind = 0x401;
	}
	if (kind < 0) {
		return;
	}
	switch (kind) {
	case 0x1D3:
		for (s32 i = 0; i < 15; ++i) {
			XObj* obj = gGame.mXObjs.mPools[10].Get(i);
			(void)(kind == obj->GetKind());
		}
		break;
	case 0x1F4:
	case 0x360:
	case 0x3FA:
	case 0x401:
		for (s32 i = 0; i < 400; i++) {
			XObj* obj = gGame.mXObjs.mPools[22].Get(i);
			if (kind == obj->GetKind()) {
				obj->mStatus.mFlags &= ~0x10000ULL;
			}
		}
		break;
	case 0x375:
		for (s32 i = 0; i < 64; ++i) {
			XObj* obj = gGame.mXObjs.mPools[8].Get(i);
			(void)(kind == obj->GetKind());
		}
		break;
	}
}

void XObjActorPapa::ProcessPendingEvents(u32 frames)
{
	if (mActorStatus.Check(0x400)) {
		mActorStatus.mFlags &= ~0x400;
		mHeadPitch = 0.0f;
	}
	if (mActorStatus.Check(2)) {
		mActorStatus.mFlags &= ~2;
		PlayWiiSoundEffect(&gGame.mSound, rand() % 10 + 0x124, 1.0f, 0.0f);
	}
	switch (mCmd[0]) {
	case 2:
		Drop();
		switch (mCmd[1]) {
		case 0x1F4:
		case 0x360:
		case 0x3FA:
		case 0x401:
			for (s32 i = 0; i < 400; i++) {
				XObj* obj = gGame.mXObjs.mPools[22].Get(i);
				if (mCmd[1] == obj->GetKind()) {
					mHeldObjects[0] = obj;
					obj->mStatus.mFlags |= 0x10000ULL;
					switch (mCmd[1]) {
					case 0x1F4:
						mHideKinds.mFlags |= 1;
						break;
					case 0x360:
						mHideKinds.mFlags |= 2;
						break;
					case 0x3FA:
						mHideKinds.mFlags |= 0x10;
						break;
					case 0x401:
						mHideKinds.mFlags |= 0x20;
						break;
					}
				}
			}
			break;
		case 0x1D3:
			for (s32 i = 0; i < 15; i++) {
				XObj* obj = gGame.mXObjs.mPools[10].Get(i);
				if (mCmd[1] == obj->GetKind()) {
					obj->mStatus.mFlags |= 0x10000ULL;
					mHeldObjects[0] = obj;
					mHideKinds.mFlags |= 4;
				}
			}
			break;
		case 0x375:
			for (s32 i = 0; i < 64; i++) {
				XObj* obj = gGame.mXObjs.mPools[8].Get(i);
				if (mCmd[1] == obj->GetKind()) {
					obj->mStatus.mFlags |= 0x10000ULL;
					mHeldObjects[0] = obj;
					mHideKinds.mFlags |= 8;
				}
			}
			break;
		default:
			if (mHeldObjects[0] != NULL) {
				mHeldObjects[0]->ClearBaseMtx();
				mHeldObjects[0] = NULL;
			}
			break;
		}
		mCmd[0] = 0;
		break;
	case 3:
		switch (mCmd[1]) {
		case 1: {
			mActorStatus.mFlags |= 0x100;
			WiiPlayerState* player = gGame.GetPlayer();
			player->LockControl();
			player->SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			break;
		}
		default: {
			mActorStatus.mFlags &= ~0x100;
			WiiPlayerState* player = gGame.GetPlayer();
			CVec pos               = GetJointPos(FindResNode("hand_r_4"));
			pos.y                  = mMoveTranslate.y;
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
	case 6:
		mAttachedId = mCmd[1];
		mState      = static_cast<StateFunc>(&XObjActorPapa::StatePickUp);
		ResetMovementStep();
		mCmd[0] = 0;
		break;
	case 7:
		mAttachedId = -1;
		if (mAttached != NULL) {
			mAttached->Release();
		}
		mCmd[0] = 0;
		break;
	case 8:
		switch (mCmd[1]) {
		case 1:
			mJointShown = 1;
			break;
		default:
			mJointShown = 0;
			break;
		}
		mCmd[0] = 0;
		break;
	}
}

void XObjActorPapa::UpdateAfterAnim(u32 frames)
{
	UpdateAfterAnim__9XObjActorFUl(this, frames);
	WiiPlayerState* player = gGame.GetPlayer();
	if (mHideKinds.Check(0x3F)) {
		CVec position;
		s32 kind = -1;
		if (mHideKinds.Check(1)) {
			kind     = 0x1F4;
			position = GetJointPos(FindResNode("hand_r_4"));
		} else if (mHideKinds.Check(2)) {
			kind     = 0x360;
			position = GetJointPos(FindResNode("hand_r_4"));
		} else if (mHideKinds.Check(4)) {
			kind     = 0x1D3;
			position = GetJointPos(FindResNode("hand_r_4"));
		} else if (mHideKinds.Check(8)) {
			kind     = 0x375;
			position = GetJointPos(FindResNode("hand_r_4"));
		} else if (mHideKinds.Check(16)) {
			kind     = 0x3FA;
			position = GetJointPos(FindResNode("hand_l_4"));
		} else if (mHideKinds.Check(32)) {
			kind     = 0x401;
			position = GetJointPos(FindResNode("hand_r_4"));
		}
		Mtx mount, translation, rotation, result;
		if (kind >= 0) {
			switch (kind) {
			case 0x1F4:
			case 0x360:
			case 0x3FA:
			case 0x401:
				for (s32 i = 0; i < 400; ++i) {
					XObj* prop = gGame.mXObjs.mPools[22].Get(i);
					if (kind == prop->GetKind()) {
						switch (kind) {
						case 0x1F4:
							PSMTXCopy(GetWorldMtx(FindResNode("hand_r_4")), mount);
							PSMTXRotRad(rotation, 'z', -HALF_PI);
							PSMTXConcat(mount, rotation, result);
							PSMTXTrans(translation, 10.0f, -4.0f, 0.0f);
							PSMTXConcat(result, translation, result);
							prop->SetBaseMtx(result);
							break;
						case 0x360:
							PSMTXCopy(GetWorldMtx(FindResNode("hand_r_4")), mount);
							PSMTXRotRad(rotation, 'x', -HALF_PI);
							PSMTXConcat(mount, rotation, result);
							PSMTXRotRad(rotation, 'y', -HALF_PI);
							PSMTXConcat(result, rotation, result);
							PSMTXTrans(translation, -1.0f, -12.0f, 6.0f);
							PSMTXConcat(result, translation, result);
							prop->SetBaseMtx(result);
							break;
						case 0x3FA:
							PSMTXCopy(GetWorldMtx(FindResNode("hand_l_4")), mount);
							PSMTXRotRad(rotation, 'z', -HALF_PI);
							PSMTXConcat(mount, rotation, result);
							PSMTXTrans(translation, 0.0f, -10.0f, 2.0f);
							PSMTXConcat(result, translation, result);
							prop->SetBaseMtx(result);
							break;
						case 0x401:
							PSMTXCopy(GetWorldMtx(FindResNode("hand_r_4")), mount);
							PSMTXRotRad(rotation, 'z', HALF_PI);
							PSMTXConcat(mount, rotation, result);
							PSMTXTrans(translation, -4.0f, 4.5f, 0.0f);
							PSMTXConcat(result, translation, result);
							prop->SetBaseMtx(result);
							break;
						}
						PapaPropVisibility(this, prop);
					}
				}
				break;
			case 0x1D3:
				for (s32 i = 0; i < 15; ++i) {
					XObj* prop = gGame.mXObjs.mPools[10].Get(i);
					if (kind == prop->GetKind()) {
						PSMTXCopy(GetWorldMtx(FindResNode("hand_r_4")), mount);
						PSMTXRotRad(rotation, 'z', HALF_PI);
						PSMTXConcat(mount, rotation, result);
						PSMTXRotRad(rotation, 'y', 0.19634955f);
						PSMTXConcat(result, rotation, result);
						PSMTXTrans(translation, -1.0f, 3.0f, -4.0f);
						PSMTXConcat(result, translation, result);
						prop->SetBaseMtx(result);
						PapaPropVisibility(this, prop);
					}
				}
				break;
			case 0x375:
				for (s32 i = 0; i < 64; ++i) {
					XObj* prop = gGame.mXObjs.mPools[8].Get(i);
					if (kind == prop->GetKind()) {
						PSMTXCopy(GetWorldMtx(FindResNode("hand_r_4")), mount);
						PSMTXRotRad(rotation, 'z', HALF_PI);
						PSMTXConcat(mount, rotation, result);
						PSMTXTrans(translation, -5.5f, 3.5f, 0.0f);
						PSMTXConcat(result, translation, result);
						prop->SetBaseMtx(result);
						PapaPropVisibility(this, prop);
					}
				}
				break;
			}
		}
	}
	if ((mActorStatus.mFlags & 0x100) && (player->mPlayerFlags & 0x40000000)) {
		CVec pos = GetJointPos(FindResNode("hand_r_4"));
		pos.y += 4.0f;
		player->SetPos(pos);
		pos = GetJointPos(FindResNode("face"));
		player->SetRotateY(atan2(pos.x - player->mMoveTranslate.x, pos.z - player->mMoveTranslate.z), 0.0f);
	}
	FindResNode("face").SetVisibility(mJointShown != 0);
}

void XObjActorPapa::StateNone(u32 frames)
{
}

void XObjActorPapa::StatePickUp(u32 frames)
{
	switch (mKindStep) {
	case 0: {
		XObjActorTao* tao = (XObjActorTao*)gGame.mXObjs.mPools[14].mObjects;
		if ((tao->mStatus.mFlags & 1) && !tao->IsFree()) {
			return;
		}
		bool busy = false;
		for (s32 i = 0; i < 15; ++i) {
			XObjActorToy* toy = (XObjActorToy*)gGame.mXObjs.mPools[10].Get(i);
			if (toy->GetKind() == 0x159) {
				if (!(toy->mStatus.mFlags & 1)) {
					break;
				}
				if (!IsFree__12XObjActorToyFv(toy)) {
					busy = true;
					break;
				}
			}
		}
		if (busy) {
			return;
		}
		if (mAttached != NULL) {
			mAttached->Release();
		}
		if (mAttachedId >= 0) {
			mAttached = ReadObj(mAttachedId);
		} else {
			mAttached = NULL;
		}
		mActorStatus.mFlags |= 0x2000;
		++mKindStep;
	}
	case 1:
		if (mAttached != NULL) {
			if (mAttached->mStatus.mFlags & 0x0040000000000000ULL) {
				return;
			}
			gGame.SetStatusFlags(0x1000);
		}
		++mKindStep;
	case 2:
		if (mAttachedId >= 0 && mAttached != NULL) {
			mAttached->SetScale(1.0f, 1.0f, 1.0f);
			nw4r::g3d::ResNode joint = FindResNode("hand_r_4");
			CVec position            = GetJointPos(joint);
			mAttached->SetPos(position.x, position.y + 2.0f, position.z);
			PSMTXCopy(GetWorldMtx(joint), mAttachedMtx);
			Vec scale;
			HSD_MtxGetScale(mAttachedMtx, &scale);
			scale.x = GetStagePlacementScale(mAttached).x / scale.x;
			scale.y = GetStagePlacementScale(mAttached).y / scale.y;
			scale.z = GetStagePlacementScale(mAttached).z / scale.z;
			Mtx scaling, rotation, translation, result;
			PSMTXScale(scaling, scale.x, scale.y, scale.z);
			PSMTXConcat(mAttachedMtx, scaling, result);
			PSMTXRotRad(rotation, 'z', HALF_PI);
			PSMTXConcat(result, rotation, result);
			PSMTXTrans(translation, -5.5f, 3.5f, 0.0f);
			PSMTXConcat(result, translation, result);
			mAttached->SetBaseMtx(result);
			mAttached->mStatus.mFlags &= ~0x0200000000000000ULL;
		}
		mActorStatus.mFlags &= ~0x2000;
		break;
	}
}

s32 XObjActorPapa::HandleEventCommand(s32 count, s32* command)
{
	HandleEventCommand__9XObjActorFlPl(this, count, command);
	switch (command[0]) {
	case 1:
		switch (command[1]) {
		case 0:
			ResetActorLook(this, DEG2RAD * 5.0f);
			break;
		case 1:
			SetActorLookAtPosition(this, gGame.GetPlayer()->GetPos(), DEG2RAD * 5.0f);
			break;
		case 2:
			if (FindObj(command[2]) != NULL) {
				SetActorLookTarget(this, FindObj(command[2])->GetPos(), DEG2RAD * command[3]);
			}
			break;
		case 3:
			SetActorLookAngle(this, DEG2RAD * command[2], DEG2RAD * command[3]);
			break;
		}
		break;
	default:
		if (count < 1) {
			return 0;
		}
		mCmd[0] = command[0];
		if (count < 2) {
			mCmd[1] = 0;
		} else {
			mCmd[1] = command[1];
		}
		mCmd[2] = command[2];
		mCmd[3] = command[3];
		mCmd[4] = command[4];
		mCmd[5] = command[5];
		mCmd[6] = command[6];
		mCmd[7] = command[7];
		mCmd[8] = command[8];
		mCmd[9] = command[9];
		break;
	}
	return 0;
}

BOOL XObjActorPapa::IsFree()
{
	if ((mStatus.mFlags & 1) == 0) {
		return TRUE;
	}
	return !(mActorStatus.mFlags & 0x2000);
}

CVec XObjActorPapa::GetHeadWorldPosition()
{
	return GetJointWorldPos("face");
}

nw4r::g3d::ResNode XObjActorPapa::GetHeadJoint()
{
	return FindResNode("face");
}

f32 XObjActorPapa::GetMaxHeadPitch()
{
	return DEG2RAD * 10.0f;
}

f32 XObjActorPapa::GetMinHeadPitch()
{
	return -(DEG2RAD * 10.0f);
}
