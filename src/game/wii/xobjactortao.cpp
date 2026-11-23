#include "game/wii/cgame.h"
#include "game/wii/xobj.h"
#include "game/gmath.h"

extern "C" CVec GetStagePlacementScale(const XObj* obj);
extern "C" void HSD_MtxGetScale(Mtx matrix, Vec* scale);
extern "C" void ResetActorLook(XObjActor* actor, f32 rate);
extern "C" void SetActorLookAtPosition(XObjActor* actor, const Vec* target, f32 rate);
extern "C" void SetActorLookTarget(XObjActor* actor, const Vec* target, f32 rate);
extern "C" void SetActorLookAngle(XObjActor* actor, f32 value, f32 rate);
extern "C" StagePlacement* GetStageObjectPlacementByIndex(s32 index);
extern "C" XObj* SpawnObjectFromPlacement(XOBJS* list, CoordInfo* info, s32 direct, BOOL async);
extern "C" XObj* ResolveEventObjectId(const CEvtData* data, s16 id);

static inline XObj* ReadObj(s16 id)
{
	return SpawnObjectFromPlacement(&gGame.mXObjs, &CoordInfo(GetStageObjectPlacementByIndex(id)), 0, TRUE);
}

static inline XObj* FindObj(s16 id)
{
	return gGame.mEvt.mData.mScript != NULL ? ResolveEventObjectId(&gGame.mEvt.mData, id) : NULL;
}

static inline void TaoPropVisibility(XObjActorTao* tao, XObj* object)
{
	if (tao->mStatus.mFlags & 0x10000) {
		object->mStatus.mFlags |= 0x10000ULL;
	} else {
		object->mStatus.mFlags &= ~0x10000ULL;
	}
}

void XObjActorTao::ResetActorState()
{
	mActorStatus.mFlags = 0;
	mHideKinds.mFlags   = 0;
	mHeading            = 0.0f;
	mMoveSpeed          = 0.0f;
	mFacing             = 0.0f;
	_D78                = mFacing;
	_D7C                = 0;
	mState              = static_cast<StateFunc>(&XObjActorTao::StateNone);
	ResetMovementStep();
	for (s32 i = 2; i < 10; ++i) {
		mCmd[i] = 0;
	}
	mMountObj   = NULL;
	mMountObjId = 0x82;
	mMountState = 1;
	mActorStatus.mFlags |= 0x2000;
}

void XObjActorTao::ProcessPendingEvents(u32 frames)
{
	if (mActorStatus.Check(2)) {
		mActorStatus.mFlags &= ~2;
		PlayWiiSoundEffect(&gGame.mSound, 0x1CA, 1.0f, 0.0f);
	}
}

void XObjActorTao::Drop()
{
	s32 kind = -1;
	if (mHideKinds.Check(2)) {
		mHideKinds.mFlags &= ~2;
		kind = 0x102;
	} else if (mHideKinds.Check(1)) {
		mHideKinds.mFlags &= ~1;
		kind = 0x159;
	} else if (mHideKinds.Check(4)) {
		mHideKinds.mFlags &= ~4;
		kind = 0xF8;
	} else if (mHideKinds.Check(8)) {
		mHideKinds.mFlags &= ~8;
		kind = 0x2A3;
	} else if (mHideKinds.Check(0x20)) {
		mHideKinds.mFlags &= ~0x20;
		kind = 0x46F;
	}
	if (kind < 0) {
		return;
	}
	switch (kind) {
	case 0x102:
		for (s32 i = 0; i < 64; ++i) {
			XObj* obj = gGame.mXObjs.mPools[8].Get(i);
			if (kind == obj->GetKind()) {
				obj->mStatus.mFlags &= ~0x10000ULL;
			}
		}
		break;
	case 0x159:
		for (s32 i = 0; i < 15; ++i) {
			XObj* obj = gGame.mXObjs.mPools[10].Get(i);
			(void)(kind == obj->GetKind());
		}
		break;
	case 0xF8:
		for (s32 i = 0; i < 21; ++i) {
			XObj* obj = gGame.mXObjs.mPools[16].Get(i);
			(void)(kind == obj->GetKind());
		}
		break;
	case 0x2A3:
	case 0x46F:
		for (s32 i = 0; i < 400; ++i) {
			XObj* obj = gGame.mXObjs.mPools[22].Get(i);
			if (kind == obj->GetKind()) {
				obj->mStatus.mFlags &= ~0x10000ULL;
			}
		}
		break;
	}
}

void XObjActorTao::UpdateAfterAnim(u32 frames)
{
	XObjActor::UpdateAfterAnim(frames);
	if (mHideKinds.mFlags & 0x2F) {
		nw4r::g3d::ResNode joint = FindResNode("hana");
		CVec position            = GetJointPos(joint);
		Vec scale;
		Mtx scaling, translation, result;
		s32 kind = -1;
		if (mHideKinds.mFlags & 2) {
			kind = 0x102;
		} else if (mHideKinds.mFlags & 1) {
			kind = 0x159;
		} else if (mHideKinds.mFlags & 4) {
			kind = 0xF8;
		} else if (mHideKinds.mFlags & 8) {
			kind  = 0x2A3;
			joint = FindResNode("head");
		} else if (mHideKinds.mFlags & 0x20) {
			kind  = 0x46F;
			joint = FindResNode("neck");
		}
		if (kind >= 0) {
			switch (kind) {
			case 0x102:
				for (s32 i = 0; i < 64; ++i) {
					XObj* obj = gGame.mXObjs.mPools[8].Get(i);
					if (kind == obj->GetKind()) {
						switch (kind) {
						case 0x102:
							PSMTXCopy(GetWorldMtx(joint), mCarryMtx);
							HSD_MtxGetScale(mCarryMtx, &scale);
							scale.x = GetStagePlacementScale(obj).x / scale.x;
							scale.y = GetStagePlacementScale(obj).y / scale.y;
							scale.z = GetStagePlacementScale(obj).z / scale.z;
							PSMTXScale(scaling, scale.x, scale.y, scale.z);
							PSMTXConcat(mCarryMtx, scaling, result);
							PSMTXTrans(translation, 0.0f, -10.0f, 0.0f);
							PSMTXConcat(result, translation, result);
							obj->SetBaseMtx(result);
							break;
						default:
							obj->SetBaseMtx(GetWorldMtx(joint));
							break;
						}
					}
					TaoPropVisibility(this, obj);
				}
				break;
			case 0x159:
				for (s32 i = 0; i < 15; ++i) {
					XObj* obj = gGame.mXObjs.mPools[10].Get(i);
					if (kind == obj->GetKind()) {
						switch (kind) {
						case 0x159:
							PSMTXCopy(GetWorldMtx(joint), mCarryMtx);
							HSD_MtxGetScale(mCarryMtx, &scale);
							scale.x = GetStagePlacementScale(obj).x / scale.x;
							scale.y = GetStagePlacementScale(obj).y / scale.y;
							scale.z = GetStagePlacementScale(obj).z / scale.z;
							PSMTXScale(scaling, scale.x, scale.y, scale.z);
							PSMTXConcat(mCarryMtx, scaling, mCarryMtx);
							obj->SetBaseMtx(mCarryMtx);
							break;
						default:
							obj->SetBaseMtx(GetWorldMtx(joint));
							break;
						}
						TaoPropVisibility(this, obj);
					}
				}
				break;
			case 0xF8: {
				s32 ordinal = -1;
				for (s32 i = 0; i < 21; ++i) {
					XObj* obj = gGame.mXObjs.mPools[16].Get(i);
					if (kind == obj->GetKind()) {
						switch (kind) {
						case 0xF8:
							if (++ordinal != mCmd[2]) {
								break;
							}
							PSMTXCopy(GetWorldMtx(joint), mCarryMtx);
							HSD_MtxGetScale(mCarryMtx, &scale);
							scale.x = GetStagePlacementScale(obj).x / scale.x;
							scale.y = GetStagePlacementScale(obj).y / scale.y;
							scale.z = GetStagePlacementScale(obj).z / scale.z;
							PSMTXScale(scaling, scale.x, scale.y, scale.z);
							PSMTXConcat(mCarryMtx, scaling, result);
							PSMTXTrans(translation, 0.0f, -10.0f, 3.0f);
							PSMTXConcat(result, translation, result);
							obj->SetBaseMtx(result);
							return;
						default:
							obj->SetBaseMtx(GetWorldMtx(joint));
							TaoPropVisibility(this, obj);
							break;
						}
					}
				}
				break;
			}
			case 0x2A3:
			case 0x46F:
				for (s32 i = 0; i < 400; ++i) {
					XObj* obj = gGame.mXObjs.mPools[22].Get(i);
					if (kind == obj->GetKind()) {
						switch (kind) {
						case 0x2A3:
							obj->SetBaseMtx(GetWorldMtx(joint));
							break;
						case 0x46F:
							obj->SetBaseMtx(GetWorldMtx(joint));
							break;
						default:
							obj->SetBaseMtx(GetWorldMtx(joint));
							break;
						}
						TaoPropVisibility(this, obj);
					}
				}
				break;
			}
		}
	}
	switch (mMountState) {
	case 0:
		break;
	case 1:
		if (mMountObj != NULL) {
			mMountObj->Release();
		}
		if (mMountObjId >= 0) {
			mMountObj = ReadObj(mMountObjId);
			++mMountState;
		} else {
			mMountObj = NULL;
		}
		break;
	case 2:
		if (mMountObj != NULL && !(mMountObj->mStatus.mFlags & 0x0040000000000000ULL)) {
			mMountObj->mStatus.mFlags |= 0x10000ULL;
			gGame.SetStatusFlags(0x1000);
			++mMountState;
		}
		break;
	case 3:
		if (mMountObjId >= 0 && mMountObj != NULL) {
			mMountObj->SetScale(1.0f, 1.0f, 1.0f);
			nw4r::g3d::ResNode joint = FindResNode("neck");
			PSMTXCopy(GetWorldMtx(joint), mMountMtx);
			Vec scale;
			HSD_MtxGetScale(mMountMtx, &scale);
			scale.x = GetStagePlacementScale(mMountObj).x / scale.x;
			scale.y = GetStagePlacementScale(mMountObj).y / scale.y;
			scale.z = GetStagePlacementScale(mMountObj).z / scale.z;
			Mtx scaling, result;
			PSMTXScale(scaling, scale.x, scale.y, scale.z);
			PSMTXConcat(mMountMtx, scaling, result);
			mMountObj->SetBaseMtx(result);
			TaoPropVisibility(this, mMountObj);
			mMountObj->mStatus.mFlags &= ~0x0200000000000000ULL;
		}
		mActorStatus.mFlags &= ~0x2000;
		break;
	}
}

void XObjActorTao::StateNone(u32 frames)
{
}

void XObjActorTao::StatePickUp(u32 frames)
{
	switch (mKindStep) {
	case 0:
		if (mAttached != NULL) {
			mAttached->Release();
		}
		if (mAttachedId >= 0) {
			mAttached = ReadObj(mAttachedId);
		} else {
			mAttached = NULL;
		}
		++mKindStep;
	case 1:
		if (mAttached != NULL) {
			if (mAttached->mStatus.mFlags & 0x0040000000000000ULL) {
				return;
			}
			mAttached->mStatus.mFlags |= 0x10000ULL;
			gGame.SetStatusFlags(0x1000);
		}
		++mKindStep;
	case 2:
		if (mAttachedId >= 0 && mAttached != NULL) {
			mAttached->SetScale(1.0f, 1.0f, 1.0f);
			nw4r::g3d::ResNode joint = FindResNode("hana");
			CVec position            = GetJointPos(joint);
			mAttached->SetPos(position.x, GetPosY(), position.z);
			PSMTXCopy(GetWorldMtx(joint), mCarryMtx);
			Vec scale;
			HSD_MtxGetScale(mCarryMtx, &scale);
			scale.x = GetStagePlacementScale(mAttached).x / scale.x;
			scale.y = GetStagePlacementScale(mAttached).y / scale.y;
			scale.z = GetStagePlacementScale(mAttached).z / scale.z;
			Mtx scaling, translation, result;
			PSMTXScale(scaling, scale.x, scale.y, scale.z);
			PSMTXConcat(mCarryMtx, scaling, result);
			PSMTXTrans(translation, 0.0f, -10.0f, 0.0f);
			PSMTXConcat(result, translation, result);
			mAttached->SetBaseMtx(result);
			mAttached->mStatus.mFlags &= ~0x0200000000000000ULL;
		}
		break;
	}
}

s32 XObjActorTao::HandleEventCommand(s32 count, s32* command)
{
	XObjActor::HandleEventCommand(count, command);
	if (count == 0) {
		return -1;
	}
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
	case 2:
		Drop();
		switch (command[1]) {
		case 0x102:
			for (s32 i = 0; i < 64; ++i) {
				XObj* obj = gGame.mXObjs.mPools[8].Get(i);
				if (command[1] == obj->GetKind()) {
					mHeldObjects[0] = obj;
					obj->mStatus.mFlags |= 0x10000ULL;
					switch (command[1]) {
					case 0x102:
						mHideKinds.mFlags |= 2;
						break;
					}
				}
			}
			break;
		case 0x159:
			for (s32 i = 0; i < 15; ++i) {
				XObj* obj = gGame.mXObjs.mPools[10].Get(i);
				if (command[1] == obj->GetKind()) {
					mHeldObjects[0] = obj;
					obj->mStatus.mFlags |= 0x10000ULL;
					mHideKinds.mFlags |= 1;
				}
			}
			break;
		case 0xF8: {
			mCmd[2]     = command[2];
			s32 ordinal = -1;
			for (s32 i = 0; i < 21; ++i) {
				XObj* obj = gGame.mXObjs.mPools[16].Get(i);
				if (command[1] == obj->GetKind()) {
					if (++ordinal == mCmd[2]) {
						obj->mStatus.mFlags |= 0x10000ULL;
						mHeldObjects[0] = obj;
						mHideKinds.mFlags |= 4;
						break;
					}
				}
			}
			break;
		}
		case 0x2A3:
			for (s32 i = 0; i < 400; ++i) {
				XObj* obj = gGame.mXObjs.mPools[22].Get(i);
				if (command[1] == obj->GetKind()) {
					obj->mStatus.mFlags |= 0x10000ULL;
					mHeldObjects[0] = obj;
					mHideKinds.mFlags |= 8;
					for (s32 j = 0; j < 6; ++j) {
						nw4r::g3d::AnmObj* anm = mObject.m_pScnMdl->GetAnmObj((nw4r::g3d::ScnMdlSimple::AnmObjType)j);
						if (anm != NULL) {
							anm->SetFrame(0.0f);
						}
					}
					obj->SetAnim(0, TRUE, 1.0f, 1.0f, 0.0f, 15.0f);
				}
			}
			break;
		case 0x46F:
			for (s32 i = 0; i < 400; ++i) {
				XObj* obj = gGame.mXObjs.mPools[22].Get(i);
				if (command[1] == obj->GetKind()) {
					obj->mStatus.mFlags |= 0x10000ULL;
					mHeldObjects[0] = obj;
					mHideKinds.mFlags |= 0x20;
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
		break;
	case 3:
		mAttachedId = command[1];
		mState      = static_cast<StateFunc>(&XObjActorTao::StatePickUp);
		ResetMovementStep();
		mCmd[0] = 0;
		break;
	case 4:
		mAttachedId = -1;
		if (mAttached != NULL) {
			mAttached->Release();
		}
		mCmd[0] = 0;
		break;
	case 5:
		if (command[1] != 0) {
			mMountObjId = 0x82;
			mMountState = 1;
		} else {
			mMountState = 0;
			mMountObjId = -1;
			if (mMountObj != NULL) {
				mMountObj->Release();
			}
		}
		mCmd[0] = 0;
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

BOOL XObjActorTao::IsFree()
{
	if ((mStatus.mFlags & 1) == 0) {
		return TRUE;
	}
	return !(mActorStatus.mFlags & 0x2000);
}

CVec XObjActorTao::GetHeadWorldPosition()
{
	return GetJointWorldPos("seigyo");
}

nw4r::g3d::ResNode XObjActorTao::GetHeadJoint()
{
	return FindResNode("seigyo");
}

void XObjActorTao::RunStateHandler(u32 frames)
{
	if (mState != NULL) {
		(this->*mState)(frames);
	}
}

void XObjActorTao::ClearStateHandler()
{
	mState = NULL;
}
