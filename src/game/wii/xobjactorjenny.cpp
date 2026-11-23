#include "game/wii/cgame.h"
#include "game/wii/xobj.h"
#include "game/gmath.h"
#include <math.h>
#include <stdlib.h>


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
		CVec pos  = GetJointPos(FindResNode("hand_l_4"));
		const CVec* self = GetPos();
		f32 angle        = atan2(pos.x - self->x, pos.z - self->z);
		f32 sx    = (f32)sin(angle - HALF_PI);
		f32 cz    = (f32)cos(angle - HALF_PI);
		f32 dx    = 3.0f * sx;
		f32 dz    = 3.0f * cz;
		pos.x += dx;
		pos.y += 2.0f;
		pos.z += dz;
		player->SetPos(pos);
		pos = GetJointPos(FindResNode("head"));
		const CVec* at = player->GetPos();
		player->SetRotateY(atan2(pos.x - at->x, pos.z - at->z), 0.0f);
	}
	if ((mActorStatus.mFlags & 0x100) && (player->mPlayerFlags & 0x40000000)) {
		CVec pos = GetJointPos(FindResNode("hand_l_4"));
		pos.y += 2.0f;
		player->SetPos(pos);
		pos = GetJointPos(FindResNode("head"));
		const CVec* at = player->GetPos();
		player->SetRotateY(atan2(pos.x - at->x, pos.z - at->z), 0.0f);
	}
}

void XObjActorJenny::StateNone(u32 frames)
{
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
