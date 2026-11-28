#include "game/wii/cgame.h"
#include "game/wii/xobj.h"
#include "game/gmath.h"

extern "C" void ApproachHudValue(f32* value, f32* target, f32 step);
extern "C" StagePlacement* GetStageObjectPlacementByIndex(s32 index);
extern "C" XObj* SpawnObjectFromPlacement(XOBJS* list, CoordInfo* info, s32 direct, BOOL async);
extern "C" XObj* ResolveEventObjectId(const CEvtData* data, s16 id);
extern "C" void ResetActorLook(XObjActor* actor, f32 rate);
extern "C" void SetActorLookTarget(XObjActor* actor, const Vec* target, f32 rate);
extern "C" void SetActorLookAngle(XObjActor* actor, f32 value, f32 rate);
extern "C" void InfoScrn_ApplyScanlineHudCommand(InfoScrn* scrn, u8 show, s32 mode);
extern "C" void fn_800E7EA8(WiiPlayerState* player);

static inline XObj* ReadObj(s16 id)
{
	return SpawnObjectFromPlacement(&gGame.mXObjs, &CoordInfo(GetStageObjectPlacementByIndex(id)), 0, TRUE);
}

static inline XObj* FindObj(s16 id)
{
	return gGame.mEvt.mData.mScript != NULL ? ResolveEventObjectId(&gGame.mEvt.mData, id) : NULL;
}

static inline void DecaPropVisibility(XObjActorDeca* deca)
{
	if (deca->mStatus.mFlags & 0x10000) {
		deca->mAttached->mStatus.mFlags |= 0x10000ULL;
	} else {
		deca->mAttached->mStatus.mFlags &= ~0x10000ULL;
	}
}

void XObjActorDeca::ResetActorState()
{
	mActorStatus.mFlags = 0;
	mJointFlags.mFlags  = 0;
	_D8C.mFlags         = 0;
	mHeading            = 0.0f;
	mMoveSpeed          = 0.0f;
	mFacing             = 0.0f;
	_D78                = mFacing;
	_D7C                = 0.0f;
	_D80                = 0;
	mJointYaw           = PI;
	mJointYawTarget     = PI;
	mState              = static_cast<StateFunc>(&XObjActorDeca::StateHold);
	ResetMovementStep();
	for (u32 i = 0; i < 20; ++i) {
		mCmd[i] = 0;
	}
}

BOOL XObjActorDeca::LoadFromPlacement(const CoordInfo& info)
{
	if (XObjActor::LoadFromPlacement(info)) {
		if (mObject.m_pScnMdl != NULL) {
			mObject.m_pScnMdl->EnableScnObjCallbackExecOp(nw4r::g3d::ScnObj::EXECOP_CALC_WORLD);
		}
		return TRUE;
	}
	return FALSE;
}

void XObjActorDeca::ProcessPendingEvents(u32 frames)
{
	if (mActorStatus.Check(2)) {
		mActorStatus.mFlags &= ~2;
	}
}

void XObjActorDeca::UpdateObject(u32 frames)
{
	XObjActor::UpdateObject(frames);
}

void XObjActorDeca::UpdateAnim(f32 frames)
{
	CObj::UpdateAnim(frames);
}

void XObjActorDeca::UpdateAfterAnim(u32 frames)
{
	XObjActor::UpdateAfterAnim(frames);
	ApproachHudValue(&mJointYaw, &mJointYawTarget, 0.1f);
	nw4r::g3d::ResNode joint = FindResNode("cube41");
	CVec rotation            = mObject.GetNodeRotate(joint);
	joint.SetRotate(57.295776f * rotation.x, 57.295776f * mJointYaw, 57.295776f * rotation.z);
}

void XObjActorDeca::StateHold(u32 frames)
{
	if (mAttachedId >= 0 && mAttached != NULL) {
		mAttached->SetScale(1.0f, 1.0f, 1.0f);
		nw4r::g3d::ResNode joint = FindResNode("obj16");
		Mtx mount, translation, result;
		PSMTXCopy(GetWorldMtx(joint), mount);
		PSMTXTrans(translation, 0.0f, -3.0f, -4.0f);
		PSMTXConcat(mount, translation, result);
		mAttached->SetBaseMtx(result);
		DecaPropVisibility(this);
		mAttached->mStatus.mFlags &= ~0x0200000000000000ULL;
	}
}

void XObjActorDeca::StatePickUp(u32 frames)
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
				if (!toy->IsFree()) {
					busy = true;
					break;
				}
			}
		}
		if (busy) {
			return;
		}
		XObjActorPapa* papa = (XObjActorPapa*)gGame.mXObjs.mPools[13].mObjects;
		if ((papa->mStatus.mFlags & 1) && !papa->IsFree()) {
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
			mAttached->mStatus.mFlags |= 0x10000ULL;
			gGame.SetStatusFlags(0x1000);
		}
		++mKindStep;
	case 2:
		if (mAttachedId >= 0 && mAttached != NULL) {
			mAttached->SetScale(1.0f, 1.0f, 1.0f);
			nw4r::g3d::ResNode joint = FindResNode("obj16");
			mAttached->SetAnim(0, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			Mtx mount, translation, result;
			PSMTXCopy(GetWorldMtx(joint), mount);
			PSMTXTrans(translation, 0.0f, -3.0f, -4.0f);
			PSMTXConcat(mount, translation, result);
			mAttached->SetBaseMtx(result);
			mAttached->mStatus.mFlags &= ~0x0200000000000000ULL;
			DecaPropVisibility(this);
			mAttached->mStatus.mFlags &= ~0x0200000000000000ULL;
		}
		mActorStatus.mFlags &= ~0x2000;
		mState = static_cast<StateFunc>(&XObjActorDeca::StateHold);
		ResetMovementStep();
		break;
	}
}

s32 XObjActorDeca::HandleEventCommand(s32 count, s32* command)
{
	WiiPlayerState* player = gGame.GetPlayer();
	XObjActor::HandleEventCommand(count, command);
	switch (command[0]) {
	case 1:
		switch (command[1]) {
		case 0:
			ResetActorLook(this, DEG2RAD * 5.0f);
			break;
		case 1:
			SetActorLookTarget(this, gGame.GetPlayer()->GetPos(), DEG2RAD * 5.0f);
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
		if (mCmd[1] == 0xD0) {
			mCmd[19] = (s32)FindObj(mCmd[2]);
			(void)((XObj*)mCmd[19])->GetPos();
		}
		switch (mCmd[0]) {
		case 2:
			switch (mCmd[1]) {
			case 0:
				mJointFlags.mFlags |= 1;
				break;
			case 1:
				mJointFlags.mFlags &= ~1;
				break;
			}
			mCmd[0] = 0;
			break;
		case 3:
			PlayWiiSoundEffect(&gGame.mSound, 0x2F5, 1.0f, 0.0f);
			switch (mCmd[1]) {
			case 0:
				mJointFlags.mFlags |= 2;
				mJointYawTarget = PI;
				break;
			case 1:
				mJointFlags.mFlags |= 2;
				mJointYawTarget = 0.0f;
				break;
			}
			mCmd[0] = 0;
			break;
		case 4:
			mAttachedId = mCmd[1];
			mState      = static_cast<StateFunc>(&XObjActorDeca::StatePickUp);
			ResetMovementStep();
			mCmd[0] = 0;
			break;
		case 5:
			mAttachedId = -1;
			if (mAttached != NULL) {
				mAttached->Release();
			}
			mCmd[0] = 0;
			break;
		case 6:
			if (mCmd[1] == 1) {
				InfoScrn_ApplyScanlineHudCommand(&gGame.mInfoScrn, 1, 1);
			} else {
				InfoScrn_ApplyScanlineHudCommand(&gGame.mInfoScrn, 0, 1);
			}
			mCmd[0] = 0;
			break;
		case 7:
			switch (mCmd[1]) {
			case 1:
				mActorStatus.mFlags |= 0x100;
				player->LockControl();
				break;
			default: {
				mActorStatus.mFlags &= ~0x100;
				nw4r::g3d::ResNode joint = FindResNode("cyl77");
				CVec position            = GetJointPos(joint);
				position.y               = 0.0f;
				player->SetPos(position);
				player->RestoreControl();
				player->ClearBaseMtx();
				fn_800E7EA8(player);
				break;
			}
			}
			mCmd[0] = 0;
			break;
		case 8:
			switch (mCmd[1]) {
			case 1:
				mJointFlags.mFlags |= 4;
				break;
			default:
				mJointFlags.mFlags &= ~4;
				break;
			}
			mCmd[0] = 0;
			break;
		case 9:
			if (mAttached != NULL) {
				mAttached->SetAnim(mCmd[1], FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			}
			mCmd[0] = 0;
			break;
		}
		break;
	}
	return 0;
}

Vec XObjActorDeca::GetCameraTarget()
{
	if (mJointFlags.mFlags & 4) {
		CVec target = GetJointPos(FindResNode("obj12"));
		target.y -= 0.5f;
		return target;
	}
	return XObjActor::GetCameraTarget();
}

void XObjActorDeca::ExecCallbackB(nw4r::g3d::ChrAnmResult* pResult, nw4r::g3d::ResMdl mdl, nw4r::g3d::FuncObjCalcWorld* pFuncObj)
{
	u16 nodeId = pFuncObj->GetCallbackNodeID();
	XObjActor::ExecCallbackB(pResult, mdl, pFuncObj);
	if (nodeId == 0) {
		nw4r::g3d::ResNode node = GetResNode("cube41");
		if (node.IsValid()) {
			pFuncObj->SetCallbackNodeID(node.GetID());
		}
	} else if (nodeId == 8) {
		Mtx mount, rotation;
		PSMTXCopy(pResult->mpMtx, mount);
		PSMTXRotRad(rotation, 'y', PI + mJointYaw);
		PSMTXConcat(mount, rotation, mount);
		PSMTXCopy(mount, pResult->mpMtx);
	}
}

void XObjActorDeca::ExecCallbackC()
{
	if ((mActorStatus.mFlags & 0x100) && (gGame.GetPlayer()->mPlayerFlags & 0x40000000)) {
		WiiPlayerState* player = gGame.GetPlayer();
		Mtx translation, result;
		PSMTXTrans(translation, 4.5f, -3.3f, 0.0f);
		PSMTXConcat(GetWorldMtx("obj12"), translation, result);
		player->SetBaseMtx(result);
	}
}

void XObjActorDeca::ExecCallback_CALC_WORLD(int timing, nw4r::g3d::ScnObj* obj, u32 param, void* info)
{
	if (timing == 4) {
		static const char* nodeNames[] = { "obj8", "obj9", NULL };
		const bool visible             = !(mJointFlags.mFlags & 1);
		for (const char** p = nodeNames; *p != NULL; ++p) {
			nw4r::g3d::ResNode node = GetResNode(*p);
			nw4r::g3d::ScnMdl::CopiedVisAccess access(mObject.m_pScnMdl, node.GetID());
			if (access.IsValid()) {
				access.SetVisibilityEx(visible);
			} else {
				node.SetVisibility(visible);
			}
		}
	}
}

CVec XObjActorDeca::GetHeadWorldPosition()
{
	return GetJointWorldPos("obj13");
}

nw4r::g3d::ResNode XObjActorDeca::GetHeadJoint()
{
	return FindResNode("obj13");
}

void XObjActorDeca::RunStateHandler(u32 frames)
{
	if (mState != NULL) {
		(this->*mState)(frames);
	}
}

void XObjActorDeca::ClearStateHandler()
{
	mState = NULL;
}
