#include "game/xobjactor.h"
#include "game/xchr.h"
#include "game/cscrn.h"
#include "game/gmath.h"
#include "game/heldpropmatrix.h"
#include <math.h>
#include <stdlib.h>

extern "C" {
void Attach__5CJObjFP9_HSD_JObj(CJObj* obj, HSD_JObj* joint);
void HSD_MtxGetScale(Mtx mtx, Vec* scale);
void PSMTXRotRad(Mtx mtx, char axis, f32 angle);
void PSMTXTrans(Mtx mtx, f32 x, f32 y, f32 z);
CVec GetStagePlacementScale(XObj* object); // the object's scale as it is drawn
}

static inline void LockPlayer(XObjChr* player)
{
	player->mPlayerStatus.mFlags = 0;
	player->mPlayerStatus.mFlags |= 0x40000000;
	player->mPad.mReset = FALSE;
}

/**
 * @note Address: 0x8012931C
 * @note Size: 0x24
 */
void XObjActorPapa::ClearStateHandler()
{
	mState = NULL;
}

/**
 * @note Address: 0x80129340
 * @note Size: 0x58
 */
void XObjActorPapa::RunStateHandler(u32 frames)
{
	if (mState != NULL) {
		(this->*mState)(frames);
	}
}

/**
 * @note Address: 0x80129398
 * @note Size: 0x78
 */
XObjActorPapa::~XObjActorPapa()
{
}

/**
 * @note Address: 0x80129410
 * @note Size: 0xA4
 */
HSD_JObj* XObjActorPapa::GetHeadJoint()
{
	s32 path[3] = { 0, 2, 0 };
	return FindJointByPath(mJObj, path, 3);
}

/**
 * @note Address: 0x801294B4
 * @note Size: 0x54
 */
CVec XObjActorPapa::GetHeadWorldPosition()
{
	s32 path[3] = { 0, 2, 0 };
	return GetEventJointWorldPosition(this, 3, path);
}

/**
 * @note Address: 0x80129508
 * @note Size: 0x44
 */
bool XObjActorPapa::IsFree()
{
	if ((mStatus.mFlags & 1) == 0) {
		return true;
	}
	return !(mActorStatus.mFlags & 0x2000);
}

/**
 * @note Address: 0x8012954C
 * @note Size: 0x8
 */
f32 XObjActorPapa::GetMinHeadPitch()
{
	return -0.17453292f;
}

/**
 * @note Address: 0x80129554
 * @note Size: 0x8
 */
f32 XObjActorPapa::GetMaxHeadPitch()
{
	return 0.17453292f;
}

/**
 * @note Address: 0x8012955C
 * @note Size: 0x210
 */
s32 XObjActorPapa::HandleEventCommand(s32 count, s32* command)
{
	XObjActor::HandleEventCommand(count, command);
	switch (command[0]) {
	case 1:
		switch (command[1]) {
		case 0:
			ResetActorLook(this, 0.08726646f);
			break;
		case 1:
			SetActorLookAtPosition(this, &gGame.GetPlayer()->mMoveTranslate, 0.08726646f);
			break;
		case 2:
			if (FindObj(command[2]) != NULL) {
				SetActorLookAtPosition(this, &FindObj(command[2])->mMoveTranslate, DEG2RAD * command[3]);
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

/**
 * @note Address: 0x8012976C
 * @note Size: 0x76C
 */
// picks up the object _A4C names and holds it in his hand
void XObjActorPapa::StatePickUp(u32 frames)
{
	switch (mKindStep) {
	case 0: {
		XObjActorTao* tao = ((XObjActorTao*)gGame.mXObjs.mPools[14].mObjects);
		if ((tao->mStatus.mFlags & 1) && !tao->IsFree()) {
			return;
		}
		bool busy = false;
		for (s32 i = 0; i < 15; i++) {
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
		if (mAttached != NULL) {
			mAttached->Release();
		}
		if (mAttachedId >= 0) {
			mAttached = ReadObj(mAttachedId);
		} else {
			mAttached = NULL;
		}
		mActorStatus.mFlags |= 0x2000;
		mKindStep++;
	}
	case 1:
		if (mAttached != NULL) {
			if (mAttached->mStatus.mFlags & 0x0040000000000000ULL) {
				return;
			}
			gGame.mStatus.mFlags |= 0x1000;
		}
		mKindStep++;
	case 2:
		if (mAttachedId >= 0 && mAttached != NULL) {
			mAttached->SetScale(1.0f, 1.0f, 1.0f, 0);
			CJObj jobj;
			s32 path[6] = { 0, 2, 2, 0, 0, 0 };
			jobj.Attach(FindJointByPath(mJObj, path, 6));
			Vec pos;
			pos = GetJointPosition(jobj.mJObj);
			Vec scale;
			scale.x = pos.x;
			scale.y = pos.y;
			scale.z = pos.z;
			mAttached->SetPos(pos.x, pos.y + 2.0f, pos.z, 0);
			PSMTXCopy(jobj.GetMtx(), mAttachedMtx);
			HSD_MtxGetScale(mAttachedMtx, &scale);
			scale.x = GetStagePlacementScale(mAttached).x / scale.x;
			scale.y = GetStagePlacementScale(mAttached).y / scale.y;
			scale.z = GetStagePlacementScale(mAttached).z / scale.z;
			Mtx scaleMtx;
			Mtx rotMtx;
			Mtx transMtx;
			Mtx m;
			PSMTXScale(scaleMtx, scale.x, scale.y, scale.z);
			PSMTXConcat(mAttachedMtx, scaleMtx, m);
			PSMTXRotRad(rotMtx, 'z', 1.5707964f);
			PSMTXConcat(m, rotMtx, m);
			PSMTXTrans(transMtx, -5.5f, 3.5f, 0.0f);
			PSMTXConcat(m, transMtx, m);
			HSD_JObj* joint = mAttached->mJObj;
			MtxPtr mp       = m;
			HSD_JObjCopyMtx(joint, mp);

			mAttached->mStatus.mFlags &= ~0x0200000000000000ULL;
		}
		mActorStatus.mFlags &= ~0x2000;
		break;
	}
}

/**
 * @note Address: 0x80129ED8
 * @note Size: 0x4
 */
void XObjActorPapa::StateNone(u32 frames)
{
}

// Carries the selected prop at its hand joint and positions the player when held.
void XObjActorPapa::UpdateAfterAnim(u32 frames)
{
	UpdateAfterAnim__9XObjActorFUl(this, frames);
	CJObj joint;
	XObjChr* player = gGame.GetPlayer();
	if (mHideKinds.mFlags & 0x3f) {
		CVec position;
		s32 kind = -1;
		if (mHideKinds.Check(1)) {
			kind        = 0x1f4;
			s32 path[6] = { 0, 2, 2, 0, 0, 0 };
			Attach__5CJObjFP9_HSD_JObj(&joint, FindJointByPath(mJObj, path, 6));
			position = joint.GetPos();
		} else if (mHideKinds.Check(2)) {
			kind        = 0x360;
			s32 path[6] = { 0, 2, 2, 0, 0, 0 };
			Attach__5CJObjFP9_HSD_JObj(&joint, FindJointByPath(mJObj, path, 6));
			position = joint.GetPos();
		} else if (mHideKinds.Check(4)) {
			kind        = 0x1d3;
			s32 path[6] = { 0, 2, 2, 0, 0, 0 };
			Attach__5CJObjFP9_HSD_JObj(&joint, FindJointByPath(mJObj, path, 6));
			position = joint.GetPos();
		} else if (mHideKinds.Check(8)) {
			kind        = 0x375;
			s32 path[6] = { 0, 2, 2, 0, 0, 0 };
			Attach__5CJObjFP9_HSD_JObj(&joint, FindJointByPath(mJObj, path, 6));
			position = joint.GetPos();
		} else if (mHideKinds.Check(16)) {
			kind        = 0x3fa;
			s32 path[6] = { 0, 2, 1, 0, 0, 0 };
			Attach__5CJObjFP9_HSD_JObj(&joint, FindJointByPath(mJObj, path, 6));
			position = joint.GetPos();
		} else if (mHideKinds.Check(32)) {
			kind        = 0x401;
			s32 path[6] = { 0, 2, 2, 0, 0, 0 };
			Attach__5CJObjFP9_HSD_JObj(&joint, FindJointByPath(mJObj, path, 6));
			position = joint.GetPos();
		}
		Mtx mount, translation, rotation, result;
		if (kind >= 0)
			switch (kind) {
			case 0x1f4:
			case 0x360:
			case 0x3fa:
			case 0x401:
				for (s32 i = 0; i < 288; ++i) {
					XObj* prop = gGame.mXObjs.mPools[22].Get(i);
					if (kind == prop->GetKind()) {
						switch (kind) {
						case 0x1f4:
							PSMTXCopy(joint.GetMtx(), mount);
							RotateHeldProp(mount, result, rotation, 'z', -HALF_PI);
							OffsetHeldProp(result, translation, 10, -4, 0);
							SetHeldPropMatrix(prop, result);
							break;
						case 0x360:
							PSMTXCopy(joint.GetMtx(), mount);
							RotateHeldProp(mount, result, rotation, 'x', -HALF_PI);
							RotateHeldProp(result, result, rotation, 'y', -HALF_PI);
							OffsetHeldProp(result, translation, -1, -12, 6);
							SetHeldPropMatrix(prop, result);
							break;
						case 0x3fa:
							PSMTXCopy(joint.GetMtx(), mount);
							RotateHeldProp(mount, result, rotation, 'z', -HALF_PI);
							OffsetHeldProp(result, translation, 0, -10, 2);
							SetHeldPropMatrix(prop, result);
							break;
						case 0x401:
							PSMTXCopy(joint.GetMtx(), mount);
							RotateHeldProp(mount, result, rotation, 'z', HALF_PI);
							OffsetHeldProp(result, translation, -4, 4.5f, 0);
							SetHeldPropMatrix(prop, result);
							break;
						default:
							SetHeldPropMatrix(prop, joint.GetMtx());
							break;
						}
						if (mStatus.mFlags & 0x10000)
							prop->mStatus.mFlags |= 0x10000ULL;
						else
							prop->mStatus.mFlags &= ~0x10000ULL;
					}
				}
				break;
			case 0x1d3:
				for (s32 i = 0; i < 15; ++i) {
					XObj* prop = gGame.mXObjs.mPools[10].Get(i);
					if (kind == prop->GetKind()) {
						switch (kind) {
						case 0x1d3:
							PSMTXCopy(joint.GetMtx(), mount);
							RotateHeldProp(mount, result, rotation, 'z', HALF_PI);
							RotateHeldProp(result, result, rotation, 'y', .19634955f);
							OffsetHeldProp(result, translation, -1, 3, -4);
							SetHeldPropMatrix(prop, result);
							break;
						default:
							SetHeldPropMatrix(prop, joint.GetMtx());
							break;
						}
						if (mStatus.mFlags & 0x10000)
							prop->mStatus.mFlags |= 0x10000ULL;
						else
							prop->mStatus.mFlags &= ~0x10000ULL;
					}
				}
				break;
			case 0x375:
				for (s32 i = 0; i < 64; ++i) {
					XObj* prop = gGame.mXObjs.mPools[8].Get(i);
					if (kind == prop->GetKind()) {
						switch (kind) {
						case 0x375:
							PSMTXCopy(joint.GetMtx(), mount);
							RotateHeldProp(mount, result, rotation, 'z', HALF_PI);
							OffsetHeldProp(result, translation, -5.5f, 3.5f, 0);
							SetHeldPropMatrix(prop, result);
							break;
						default:
							SetHeldPropMatrix(prop, joint.GetMtx());
							break;
						}
						if (mStatus.mFlags & 0x10000)
							prop->mStatus.mFlags |= 0x10000ULL;
						else
							prop->mStatus.mFlags &= ~0x10000ULL;
					}
				}
				break;
			}
	}
	if ((mActorStatus.mFlags & 0x100) && (player->mPlayerStatus.mFlags & 0x40000000)) {
		s32 held[6] = { 0, 2, 2, 0, 0, 0 };
		Attach__5CJObjFP9_HSD_JObj(&joint, FindJointByPath(mJObj, held, 6));
		CVec pos = joint.GetPos();
		pos.y += 4;
		player->SetPos(pos, 0);
		s32 face[4] = { 0, 2, 0, 0 };
		Attach__5CJObjFP9_HSD_JObj(&joint, FindJointByPath(mJObj, face, 4));
		pos = joint.GetPos();
		player->SetRotateY(atan2(pos.x - player->mMoveTranslate.x, pos.z - player->mMoveTranslate.z), 0);
	}
	if (!mJointShown) {
		s32 path[5] = { 0, 2, 0, 0, 1 };
		Attach__5CJObjFP9_HSD_JObj(&joint, FindJointByPath(mJObj, path, 5));
		HSD_JObjSetFlags(joint.mJObj, 0x10);
	} else {
		s32 path[5] = { 0, 2, 0, 0, 1 };
		Attach__5CJObjFP9_HSD_JObj(&joint, FindJointByPath(mJObj, path, 5));
		HSD_JObjClearFlags(joint.mJObj, 0x10);
	}
}

/**
 * @note Address: 0x8012BB5C
 * @note Size: 0x288
 */
// lets go of what he carries
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
		// Retail walks these two pools without changing the matching objects.
		for (s32 i = 0; i < 15; ++i) {
			XObj* obj = gGame.mXObjs.mPools[10].Get(i);
			(void)(kind == obj->GetKind());
		}
		break;
	case 0x1F4:
	case 0x360:
	case 0x3FA:
	case 0x401:
		for (s32 i = 0; i < 288; i++) {
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

/**
 * @note Address: 0x8012BDE4
 * @note Size: 0x9BC
 */
void XObjActorPapa::ProcessPendingEvents(u32 frames)
{
	CJObj jobj;
	if (mActorStatus.Check(0x400)) {
		mActorStatus.mFlags &= ~0x400;
		mHeadPitch = 0.0f;
	}
	if (mActorStatus.Check(2)) {
		mActorStatus.mFlags &= ~2;
		PlayGameSound(&gGame.mSound, rand() % 10 + 0xB7, 0xFF);
	}
	switch (mCmd[0]) {
	case 2:
		Drop();
		switch (mCmd[1]) {
		case 0x1F4:
		case 0x360:
		case 0x3FA:
		case 0x401:
			for (s32 i = 0; i < 288; i++) {
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
					switch (mCmd[1]) {
					case 0x1D3:
						mHideKinds.mFlags |= 4;
						break;
					}
				}
			}
			break;
		case 0x375:
			for (s32 i = 0; i < 64; i++) {
				XObj* obj = gGame.mXObjs.mPools[8].Get(i);
				if (mCmd[1] == obj->GetKind()) {
					obj->mStatus.mFlags |= 0x10000ULL;
					mHeldObjects[0] = obj;
					switch (mCmd[1]) {
					case 0x375:
						mHideKinds.mFlags |= 8;
						break;
					}
				}
			}
			break;
		default:
			if (mHeldObjects[0] != NULL) {
				HSD_JObj* held = mHeldObjects[0]->mJObj;
				HSD_JObjClearMtxFlags(held);
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
			LockPlayer(gGame.GetPlayer());
			gGame.GetPlayer()->SetAnim(1, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);

			break;
		}
		default: {
			mActorStatus.mFlags &= ~0x100;
			s32 path[6] = { 0, 2, 2, 0, 0, 0 };
			jobj.Attach(FindJointByPath(mJObj, path, 6));
			Vec pos;
			pos = GetJointPosition(jobj.mJObj);
			Vec target;
			target.x = pos.x;
			target.y = pos.y;
			target.z = pos.z;
			target.y = mMoveTranslate.y;
			gGame.GetPlayer()->SetPos(target, 0);
			XObjChr* player              = gGame.GetPlayer();
			player->mPlayerStatus.mFlags = 0;
			player->mPlayerStatus.mFlags |= 0x20000000;
			player->mPlayerStatus.mFlags |= 7;
			player->mPad.mReset = TRUE;
			break;
		}
		}
		mCmd[0] = 0;
		break;
	case 4:
		StartScreenShake(&gGame.mQuake, mCmd[1], mCmd[2], mCmd[3], 0.0f, 0.0f, 0.0f, mCmd[4] / 100.0f);
		mCmd[0] = 0;
		break;
	case 5: {
		HSD_AnimJoint* joint      = GetJointAnimation(mCmd[1]);
		HSD_MatAnimJoint* mat     = GetMaterialAnimation(mCmd[2]);
		HSD_ShapeAnimJoint* shape = GetShapeAnimation(mCmd[1]);
		SetAnim(0, TRUE, joint, mat, shape, 0.0f, 1.0f, 1.0f, 10.0f);
		mStatus.mFlags |= 0x0008000000000000ULL;
		mCmd[0] = 0;
		break;
	}
	case 6:
		mAttachedId = mCmd[1];
		mState      = &XObjActorPapa::StatePickUp;
		ResetActorMovementStep(this);
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

/**
 * @note Address: 0x8012C7A0
 * @note Size: 0xB8
 */
void XObjActorPapa::ResetActorState()
{
	XObjChr* player = gGame.GetPlayer();
	if (player->mPlayerStatus.mFlags & 0x40000000) {
		player->mPlayerStatus.mFlags = 0;
		player->mPlayerStatus.mFlags |= 0x20000000;
		player->mPlayerStatus.mFlags |= 7;
		player->mPad.mReset = TRUE;
	}
	mActorStatus.mFlags = 0;
	mHideKinds.mFlags   = 0;
	mHeading            = 0.0f;
	mMoveSpeed          = 0.0f;
	mFacing             = 0.0f;
	_C84                = mFacing;
	_C88                = 0;
	mJointShown         = 1;
	mState              = &XObjActorPapa::StateNone;
	ResetActorMovementStep(this);
}
