#include "game/wii/cgame.h"
#include "game/wii/xobj.h"
#include "game/gmath.h"
#include <math.h>
#include <stdlib.h>

extern "C" {
XObj* ResolveEventObjectId(const CEvtData* data, s16 id);
void ResetActorLook(XObjActor* actor, f32 rate);
void SetActorLookAtPosition(XObjActor* actor, const Vec* target, f32 rate);
void SetActorLookTarget(XObjActor* actor, const Vec* target, f32 rate);
void SetActorLookAngle(XObjActor* actor, f32 value, f32 rate);
extern s32 gRefreshRate;
}

static inline XObj* FindObj(s16 id)
{
	return gGame.mEvt.mData.mScript != NULL ? ResolveEventObjectId(&gGame.mEvt.mData, id) : NULL;
}

static inline void JennyPropVisibility(XObjActorJenny* jenny, XObj* object)
{
	if (jenny->mStatus.mFlags & 0x10000) {
		object->mStatus.mFlags |= 0x10000ULL;
	} else {
		object->mStatus.mFlags &= ~0x10000ULL;
	}
}

static inline void HoldPlayerForJenny(WiiPlayerState* player)
{
	player->LockControl();
	player->SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
}

static inline void ReleasePlayerAt(WiiPlayerState* player, const Vec& position)
{
	player->SetPos(position);
	player->RestoreControl();
}

BOOL XObjActorJenny::LoadFromPlacement(const CoordInfo& info)
{
	if (XObjActor::LoadFromPlacement(info)) {
		if (mObject.m_pScnMdl != NULL) {
			mObject.m_pScnMdl->EnableScnObjCallbackExecOp(nw4r::g3d::ScnObj::EXECOP_CALC_WORLD);
		}
		return TRUE;
	}
	return FALSE;
}

void XObjActorJenny::ResetActorState()
{
	WiiPlayerState* player = gGame.GetPlayer();
	if (player->mPlayerFlags & 0x40000000) {
		player->RestoreControl();
	}
	mActorStatus.mFlags = 0;
	mJointFlags.mFlags  = 0;
	mHideKinds.mFlags   = 0;
	mHeading            = 0.0f;
	mMoveSpeed          = 0.0f;
	mFacing             = 0.0f;
	_D78                = mFacing;
	_D7C                = 0.0f;
	_D80                = 0;
	mJointHidden        = 0;
	mState              = static_cast<StateFunc>(&XObjActorJenny::StateNone);
	ResetMovementStep();
	for (u32 i = 0; i < 20; i++) {
		mCmd[i] = 0;
	}
	mJointFlags.mFlags &= ~8;
}

void XObjActorJenny::Drop()
{
	s32 kind = -1;
	if (mHideKinds.Check(1)) {
		mHideKinds.mFlags &= ~1;
		kind = 0x3D;
	} else if (mHideKinds.Check(2)) {
		mHideKinds.mFlags &= ~2;
		kind = 0x3E;
	} else if (mHideKinds.Check(4)) {
		mHideKinds.mFlags &= ~4;
		kind = 0x3F;
	} else if (mHideKinds.Check(8)) {
		mHideKinds.mFlags &= ~8;
		kind = 0x3B;
	} else if (mHideKinds.Check(0x10)) {
		mHideKinds.mFlags &= ~0x10;
		kind = 0x401;
	}
	if (kind < 0) {
		return;
	}
	switch (kind) {
	case 0x3B:
	case 0x3D:
	case 0x3E:
	case 0x3F:
		for (s32 i = 0; i < 64; i++) {
			XObj* obj = gGame.mXObjs.mPools[8].Get(i);
			if (kind == obj->GetKind()) {
				obj->mStatus.mFlags &= ~0x10000ULL;
			}
		}
		break;
	case 0x401:
		for (s32 i = 0; i < 400; i++) {
			XObj* obj = gGame.mXObjs.mPools[22].Get(i);
			if (kind == obj->GetKind()) {
				obj->mStatus.mFlags &= ~0x10000ULL;
			}
		}
		break;
	}
}

void XObjActorJenny::ProcessPendingEvents(u32 frames)
{
	WiiPlayerState* player = gGame.GetPlayer();
	if (mActorStatus.Check(2)) {
		mActorStatus.mFlags &= ~2;
		PlayWiiSoundEffect(&gGame.mSound, rand() % 10 + 0xFF, 1.0f, 0.0f);
	}
	switch (mCmd[0]) {
	case 2:
		switch (mCmd[1]) {
		case 8:
			mJointFlags.mFlags |= 8;
			break;
		default:
			mJointFlags.mFlags &= ~8;
			break;
		}
		mCmd[0] = 0;
		break;
	case 3:
		Drop();
		switch (mCmd[1]) {
		case 0x3B:
		case 0x3D:
		case 0x3E:
		case 0x3F:
			for (s32 i = 0; i < 64; ++i) {
				XObj* object = gGame.mXObjs.mPools[8].Get(i);
				if (mCmd[1] == object->GetKind()) {
					object->mStatus.mFlags |= 0x10000ULL;
					mHeldObjects[0] = object;
					switch (mCmd[1]) {
					case 0x3B:
						mHideKinds.mFlags |= 8;
						break;
					case 0x3D:
						mHideKinds.mFlags |= 1;
						break;
					case 0x3E:
						mHideKinds.mFlags |= 2;
						break;
					case 0x3F:
						mHideKinds.mFlags |= 4;
						break;
					}
				}
			}
			break;
		case 0x401:
			for (s32 i = 0; i < 400; ++i) {
				XObj* object = gGame.mXObjs.mPools[22].Get(i);
				if (mCmd[1] == object->GetKind()) {
					mHeldObjects[0] = object;
					object->mStatus.mFlags |= 0x10000ULL;
					mHideKinds.mFlags |= 0x10;
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
	case 4:
		switch (mCmd[1]) {
		case 1:
			mActorStatus.mFlags |= 0x200;
			HoldPlayerForJenny(player);
			break;
		default: {
			mActorStatus.mFlags &= ~0x200;
			CVec firstPos  = GetJointPos(FindResNode("hand_r_4"));
			CVec secondPos = GetJointPos(FindResNode("hand_l_4"));
			firstPos.x -= (firstPos.x - secondPos.x) / 2.0f;
			firstPos.z -= (firstPos.z - secondPos.z) / 2.0f;
			firstPos.y = GetPos()->y;
			ReleasePlayerAt(player, secondPos);
			break;
		}
		}
		mCmd[0] = 0;
		break;
	case 5:
		switch (mCmd[1]) {
		case 1:
			mActorStatus.mFlags |= 0x100;
			HoldPlayerForJenny(player);
			break;
		default: {
			mActorStatus.mFlags &= ~0x100;
			CVec position = GetJointPos(FindResNode("hand_l_4"));
			ReleasePlayerAt(player, position);
			break;
		}
		}
		mCmd[0] = 0;
		break;
	case 7:
		mCmd[0] = 0;
		break;
	case 8:
		switch (mCmd[1]) {
		case 0:
			mJointHidden = 1;
			mCmd[0]      = 0;
			break;
		case 1:
			mJointHidden = 0;
			mCmd[0]      = 0;
			break;
		}
		break;
	}
}

void XObjActorJenny::UpdateObject(u32 frames)
{
	XObjActor::UpdateObject(frames);
	if (GetAnimId() == 0x47) {
		if (mSfxActors[0].GetHandle() == NULL) {
			CSound3DActor* actor = mSfxActors[0].mActor;
			if (actor != NULL && actor->StartSound(&actor->mHandle, 0x208)) {
				actor->mHandle.SetVolume(1.0f, 0);
			}
		}
	} else if (mSfxActors[0].GetHandle() != NULL) {
		mSfxActors[0].Stop(0.0f);
	}
}

void XObjActorJenny::UpdateAnim(f32 speed)
{
	XObjActor::UpdateAnim(speed);
}

void XObjActorJenny::UpdateAfterAnim(u32 frames)
{
	XObjActor::UpdateAfterAnim(frames);
	WiiPlayerState* player = gGame.GetPlayer();
	if (mHideKinds.Check(0x1F)) {
		nw4r::g3d::ResNode joint;
		CVec position;
		s32 kind = -1;
		if (mHideKinds.Check(1)) {
			kind     = 0x3D;
			joint    = FindResNode("hand_r_4");
			position = GetJointPos(joint);
		} else if (mHideKinds.Check(2)) {
			kind     = 0x3E;
			joint    = FindResNode("hand_r_4");
			position = GetJointPos(joint);
		} else if (mHideKinds.Check(4)) {
			kind     = 0x3F;
			joint    = FindResNode("hand_r_4");
			position = GetJointPos(joint);
		} else if (mHideKinds.Check(8)) {
			kind     = 0x3B;
			joint    = FindResNode("hand_r_4");
			position = GetJointPos(joint);
		} else if (mHideKinds.Check(16)) {
			kind     = 0x401;
			joint    = FindResNode("hand_l_4");
			position = GetJointPos(joint);
		}
		Mtx mount, translation, rotation, result;
		if (kind >= 0) {
			switch (kind) {
			case 0x401:
				for (s32 i = 0; i < 400; ++i) {
					XObj* prop = gGame.mXObjs.mPools[22].Get(i);
					if (kind == prop->GetKind()) {
						switch (kind) {
						case 0x401:
							PSMTXCopy(GetWorldMtx(joint), mount);
							PSMTXRotRad(rotation, 'z', HALF_PI);
							PSMTXConcat(mount, rotation, result);
							PSMTXTrans(translation, -4.0f, 4.0f, 0.0f);
							PSMTXConcat(result, translation, result);
							prop->SetBaseMtx(result);
							break;
						default:
							prop->SetBaseMtx(GetWorldMtx(joint));
							break;
						}
						JennyPropVisibility(this, prop);
					}
				}
				break;
			case 0x3B:
			case 0x3D:
			case 0x3E:
			case 0x3F:
				for (s32 i = 0; i < 64; ++i) {
					XObj* prop = gGame.mXObjs.mPools[8].Get(i);
					if (kind == prop->GetKind()) {
						switch (kind) {
						case 0x3D:
						case 0x3E:
						case 0x3F:
							PSMTXCopy(GetWorldMtx(joint), mount);
							PSMTXRotRad(rotation, 'x', PI);
							PSMTXConcat(mount, rotation, result);
							PSMTXTrans(translation, 0.0f, 0.0f, 10.0f);
							PSMTXConcat(result, translation, result);
							prop->SetBaseMtx(result);
							break;
						case 0x3B: {
							MtxP source = GetWorldMtx(joint);
							PSMTXCopy(source, mount);
							PSMTXTrans(translation, -2.0f, -2.0f, -10.0f);
							PSMTXConcat(source, translation, result);
							prop->SetBaseMtx(result);
							break;
						}
						default:
							prop->SetBaseMtx(GetWorldMtx(joint));
							break;
						}
						JennyPropVisibility(this, prop);
					}
				}
				break;
			}
		}
	}
	if ((mActorStatus.mFlags & 0x200) && (player->mPlayerFlags & 0x40000000)) {
		CVec pos         = GetJointPos(FindResNode("hand_l_4"));
		const CVec* self = GetPos();
		f32 angle        = atan2(pos.x - self->x, pos.z - self->z);
		f32 sx           = (f32)sin(angle - HALF_PI);
		f32 cz           = (f32)cos(angle - HALF_PI);
		f32 dx           = 3.0f * sx;
		f32 dz           = 3.0f * cz;
		pos.x += dx;
		pos.y += 2.0f;
		pos.z += dz;
		player->SetPos(pos);
		pos            = GetJointPos(FindResNode("head"));
		const CVec* at = player->GetPos();
		player->SetRotateY(atan2(pos.x - at->x, pos.z - at->z), 0.0f);
	}
	if ((mActorStatus.mFlags & 0x100) && (player->mPlayerFlags & 0x40000000)) {
		CVec pos = GetJointPos(FindResNode("hand_l_4"));
		pos.y += 2.0f;
		player->SetPos(pos);
		pos            = GetJointPos(FindResNode("head"));
		const CVec* at = player->GetPos();
		player->SetRotateY(atan2(pos.x - at->x, pos.z - at->z), 0.0f);
	}
}

void XObjActorJenny::StateNone(u32 frames)
{
}

s32 XObjActorJenny::HandleEventCommand(s32 count, s32* command)
{
	XObjActor::HandleEventCommand(count, command);
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
		for (s32 i = 0; i < 8; i++) {
			mCmd[i + 2] = command[i + 2];
		}
		if (mCmd[1] == 0xD0) {
			mCmd[19] = (s32)FindObj(mCmd[2]);
			((XObj*)mCmd[19])->GetPos();
		}
		break;
	}
	return 0;
}

f32 XObjActorJenny::GetMaxHeadPitch()
{
	switch (GetAnimId()) {
	case 0x412:
	case 0x413:
	case 0x414:
		return 0.0f;
	default:
		return DEG2RAD * 10.0f;
	}
}

f32 XObjActorJenny::GetMinHeadPitch()
{
	switch (GetAnimId()) {
	case 0x412:
	case 0x413:
	case 0x414:
		return 0.0f;
	default:
		return -(DEG2RAD * 20.0f);
	}
}

void XObjActorJenny::ExecCallback_CALC_WORLD(int timing, nw4r::g3d::ScnObj* obj, u32 param, void* info)
{
	if (timing == 4) {
		nw4r::g3d::ResNode node = GetResNode("kaeru_mask");
		nw4r::g3d::ScnMdl::CopiedVisAccess access(mObject.m_pScnMdl, node.GetID());
		if (access.IsValid()) {
			access.SetVisibilityEx(!mJointHidden);
		} else {
			node.SetVisibility(!mJointHidden);
		}
	}
}

CVec XObjActorJenny::GetHeadWorldPosition()
{
	return GetJointWorldPos("head");
}

nw4r::g3d::ResNode XObjActorJenny::GetHeadJoint()
{
	return FindResNode("head");
}

void XObjActorJenny::RunStateHandler(u32 frames)
{
	if (mState != NULL) {
		(this->*mState)(frames);
	}
}

void XObjActorJenny::ClearStateHandler()
{
	mState = NULL;
}
