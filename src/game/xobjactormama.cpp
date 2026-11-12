#include "game/xobjactor.h"
#include "game/xchr.h"
#include "game/gmath.h"
#include "game/heldpropmatrix.h"
#include "game/evt.h"
#include <math.h>

/**
 * @note Address: 0x8012641C
 * @note Size: 0x24
 */
void XObjActorMama::ClearStateHandler()
{
	mState = NULL;
}

/**
 * @note Address: 0x80126440
 * @note Size: 0x58
 */
void XObjActorMama::RunStateHandler(u32 frames)
{
	if (mState != NULL) {
		(this->*mState)(frames);
	}
}

/**
 * @note Address: 0x80126498
 * @note Size: 0x78
 */
XObjActorMama::~XObjActorMama()
{
}

/**
 * @note Address: 0x80126510
 * @note Size: 0xAC
 */
HSD_JObj* XObjActorMama::GetHeadJoint()
{
	s32 path[4] = { 0, 4, 0, 0 };
	return FindJointByPath(mJObj, path, 4);
}

/**
 * @note Address: 0x801265BC
 * @note Size: 0x5C
 */
CVec XObjActorMama::GetHeadWorldPosition()
{
	s32 path[4] = { 0, 4, 0, 0 };
	return GetEventJointWorldPosition(this, 4, path);
}

/**
 * @note Address: 0x80126618
 * @note Size: 0x8
 */
f32 XObjActorMama::GetMinHeadPitch()
{
	return -0.17453292f;
}

/**
 * @note Address: 0x80126620
 * @note Size: 0x8
 */
f32 XObjActorMama::GetMaxHeadPitch()
{
	return 0.17453292f;
}

/**
 * @note Address: 0x80126628
 * @note Size: 0x8
 */
f32 XObjActorMama::GetMinHeadYaw()
{
	return -0.6981317f;
}

/**
 * @note Address: 0x80126630
 * @note Size: 0x8
 */
f32 XObjActorMama::GetMaxHeadYaw()
{
	return 0.6981317f;
}

/**
 * @note Address: 0x80126638
 * @note Size: 0x210
 */
s32 XObjActorMama::HandleEventCommand(s32 count, s32* command)
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
 * @note Address: 0x80126848
 * @note Size: 0x4
 */
void XObjActorMama::StateNone(u32 frames)
{
}

void XObjActorMama::vf148(u32 frames)
{
	vf148__9XObjActorFUl(this, frames);
	CJObj hand;
	XObjChr* player = gGame.GetPlayer();
	if (_C8C.mFlags & 0x3f) {
		CVec position;
		CJObj other;
		s32 first[6] = { 0, 4, 2, 0, 0, 0 };
		hand.Attach(FindJointByPath(mJObj, first, 6));
		s32 second[6] = { 0, 4, 1, 0, 0, 0 };
		other.Attach(FindJointByPath(mJObj, second, 6));
		position = hand.GetPos();
		s32 kind = -1;
		if (_C8C.Check(1))
			kind = 0x40;
		else if (_C8C.Check(2))
			kind = 0x5b;
		else if (_C8C.Check(4))
			kind = 0x7d;
		else if (_C8C.Check(8))
			kind = 0x196;
		else if (_C8C.Check(16))
			kind = 0x1a1;
		else if (_C8C.Check(32))
			kind = 0x45b;
		Mtx mount, translation, rotation, result;
		switch (kind) {
		case 0x40:
			for (s32 i = 0; i < 64; ++i) {
				XObj* prop = gGame.mXObjs.mPools[8].Get(i);
				if (kind == prop->GetKind()) {
					switch (kind) {
					case 0x40:
						PSMTXCopy(hand.GetMtx(), mount);
						RotateHeldProp(mount, result, rotation, 'z', -PI / 4);
						OffsetHeldProp(result, translation, 0, -3, 0);
						SetHeldPropMatrix(prop, result);
						break;
					default:
						SetHeldPropMatrix(prop, hand.GetMtx());
						break;
					}
					if (mStatus.mFlags & 0x10000)
						prop->mStatus.mFlags |= 0x10000ULL;
					else
						prop->mStatus.mFlags &= ~0x10000ULL;
				}
			}
			break;
		case 0x5b:
		case 0x7d:
		case 0x196:
		case 0x45b:
			for (s32 i = 0; i < 288; ++i) {
				XObj* prop = gGame.mXObjs.mPools[22].Get(i);
				if (kind != prop->GetKind())
					continue;
				switch (kind) {
				case 0x5b:
					PSMTXCopy(hand.GetMtx(), mount);
					RotateHeldProp(mount, result, rotation, 'z', -PI / 4);
					OffsetHeldProp(result, translation, 0, -3, 0);
					SetHeldPropMatrix(prop, result);
					break;
				case 0x7d:
					PSMTXCopy(hand.GetMtx(), mount);
					RotateHeldProp(mount, result, rotation, 'y', 1.17809725f);
					SetHeldPropMatrix(prop, result);
					break;
				case 0x196:
					sin(DEG2RAD * ((s32)(_994 * 16) % 360));
					++_994;
					PSMTXCopy(hand.GetMtx(), mount);
					RotateHeldProp(mount, result, rotation, 'x', -HALF_PI);
					OffsetHeldProp(result, translation, 2.5f, -3, -8.5f);
					RotateHeldProp(result, result, rotation, 'y', 1.17809725f);
					SetHeldPropMatrix(prop, result);
					break;
				case 0x45b:
					PSMTXCopy(hand.GetMtx(), mount);
					RotateHeldProp(mount, result, rotation, 'y', -1.37444687f);
					RotateHeldProp(result, result, rotation, 'z', HALF_PI);
					RotateHeldProp(result, result, rotation, 'x', 0);
					OffsetHeldProp(result, translation, -4.5f, -6, 0);
					SetHeldPropMatrix(prop, result);
					break;
				default:
					SetHeldPropMatrix(prop, hand.GetMtx());
					break;
				}
				if (mStatus.mFlags & 0x10000)
					prop->mStatus.mFlags |= 0x10000ULL;
				else
					prop->mStatus.mFlags &= ~0x10000ULL;
			}
			break;
		case 0x1a1:
			for (s32 i = 0; i < 288; ++i) {
				XObj* prop = gGame.mXObjs.mPools[22].Get(i);
				if (prop->GetKind() == 0x1a0) {
					PSMTXCopy(other.GetMtx(), mount);
					RotateHeldProp(mount, result, rotation, 'z', -.19634955f);
					OffsetHeldProp(result, translation, 0, -4.5f, 0);
					SetHeldPropMatrix(prop, result);
					if (mStatus.mFlags & 0x10000)
						prop->mStatus.mFlags |= 0x10000ULL;
					else
						prop->mStatus.mFlags &= ~0x10000ULL;
				} else if (prop->GetKind() == 0x1a1) {
					PSMTXCopy(hand.GetMtx(), mount);
					RotateHeldProp(mount, result, rotation, 'x', -1.17809725f);
					OffsetHeldProp(result, translation, 0, -3.5f, -3);
					RotateHeldProp(result, result, rotation, 'z', -HALF_PI);
					SetHeldPropMatrix(prop, result);
					if (mStatus.mFlags & 0x10000)
						prop->mStatus.mFlags |= 0x10000ULL;
					else
						prop->mStatus.mFlags &= ~0x10000ULL;
				}
			}
			break;
		}
	}
	if ((_998.mFlags & 0x100) && (player->_980.mFlags & 0x40000000)) {
		s32 hold[6] = { 0, 4, 2, 0, 0, 0 };
		hand.Attach(FindJointByPath(mJObj, hold, 6));
		CVec pos = hand.GetPos();
		pos.y += 2;
		player->SetPos(pos, 0);
		s32 face[5] = { 0, 4, 0, 0, 0 };
		hand.Attach(FindJointByPath(mJObj, face, 5));
		pos = hand.GetPos();
		player->SetRotateY(atan2(pos.x - player->mMoveTranslate.x, pos.z - player->mMoveTranslate.z), 0);
	}
}
