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
	s32 headPath[4] = { 0, 4, 0, 0 };
	return FindJointByPath(mJObj, headPath, 4);
}

/**
 * @note Address: 0x801265BC
 * @note Size: 0x5C
 */
CVec XObjActorMama::GetHeadWorldPosition()
{
	s32 headPath[4] = { 0, 4, 0, 0 };
	return GetEventJointWorldPosition(this, 4, headPath);
}

/**
 * @note Address: 0x80126618
 * @note Size: 0x8
 */
f32 XObjActorMama::GetMinHeadPitch()
{
	return -DEG2RAD * 10;
}

/**
 * @note Address: 0x80126620
 * @note Size: 0x8
 */
f32 XObjActorMama::GetMaxHeadPitch()
{
	return DEG2RAD * 10;
}

/**
 * @note Address: 0x80126628
 * @note Size: 0x8
 */
f32 XObjActorMama::GetMinHeadYaw()
{
	return -DEG2RAD * 40;
}

/**
 * @note Address: 0x80126630
 * @note Size: 0x8
 */
f32 XObjActorMama::GetMaxHeadYaw()
{
	return DEG2RAD * 40;
}

/**
 * @note Address: 0x80126638
 * @note Size: 0x210
 */
s32 XObjActorMama::HandleEventCommand(s32 count, s32* command)
{
	XObjActor::HandleEventCommand(count, command);
	switch (command[0]) {
	case 1: // command 1 steers her head
		switch (command[1]) {
		case 0:
			ResetActorLook(this, DEG2RAD * 5);
			break;
		case 1:
			SetActorLookAtPosition(this, &gGame.GetPlayer()->mMoveTranslate, DEG2RAD * 5);
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
	default: // stash other commands in mCmd
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

void XObjActorMama::UpdateAfterAnim(u32 frames)
{
	UpdateAfterAnim__9XObjActorFUl(this, frames);
	CJObj hand;
	XObjChr* player = gGame.GetPlayer();
	if (mHideKinds.mFlags & 0x3f) { // a held prop kind is set
		CVec position;
		CJObj other;
		s32 first[6] = { 0, 4, 2, 0, 0, 0 };
		hand.Attach(FindJointByPath(mJObj, first, 6));
		s32 second[6] = { 0, 4, 1, 0, 0, 0 };
		other.Attach(FindJointByPath(mJObj, second, 6));
		position = hand.GetPos();
		s32 kind = -1; // bits 0 to 5 pick the prop's kind
		if (mHideKinds.Check(1))
			kind = 0x40;
		else if (mHideKinds.Check(2))
			kind = 0x5b;
		else if (mHideKinds.Check(4))
			kind = 0x7d;
		else if (mHideKinds.Check(8))
			kind = 0x196;
		else if (mHideKinds.Check(16))
			kind = 0x1a1;
		else if (mHideKinds.Check(32))
			kind = 0x45b;
		Mtx mount, translation, rotation, result;
		if (kind >= 0)
			switch (kind) {
			case 0x40:
				for (s32 i = 0; i < 64; ++i) { // pool 8 holds 64 props
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
						if (mStatus.mFlags & 0x10000) // the prop draws only while her flag is set
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
				for (s32 i = 0; i < 288; ++i) { // pool 22 holds 288
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
						RotateHeldProp(mount, result, rotation, 'y', 1.17809725f); // 67.5 degrees
						SetHeldPropMatrix(prop, result);
						break;
					case 0x196:
						sin(DEG2RAD * ((s32)(mKindCounter * 16) % 360)); // the sine is unused; only the counter counts
						++mKindCounter;
						PSMTXCopy(hand.GetMtx(), mount);
						RotateHeldProp(mount, result, rotation, 'x', -HALF_PI);
						OffsetHeldProp(result, translation, 2.5f, -3, -8.5f);
						RotateHeldProp(result, result, rotation, 'y', 1.17809725f); // 67.5 degrees
						SetHeldPropMatrix(prop, result);
						break;
					case 0x45b:
						PSMTXCopy(hand.GetMtx(), mount);
						RotateHeldProp(mount, result, rotation, 'y', -1.37444687f); // 78.75 degrees
						RotateHeldProp(result, result, rotation, 'z', HALF_PI);
						RotateHeldProp(result, result, rotation, 'x', 0);
						OffsetHeldProp(result, translation, -4.5f, -6, 0);
						SetHeldPropMatrix(prop, result);
						break;
					default:
						SetHeldPropMatrix(prop, hand.GetMtx());
						break;
					}
					if (mStatus.mFlags & 0x10000) // the prop draws only while her flag is set
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
						RotateHeldProp(mount, result, rotation, 'z', -.19634955f); // 11.25 degrees
						OffsetHeldProp(result, translation, 0, -4.5f, 0);
						SetHeldPropMatrix(prop, result);
						if (mStatus.mFlags & 0x10000) // the prop draws only while her flag is set
							prop->mStatus.mFlags |= 0x10000ULL;
						else
							prop->mStatus.mFlags &= ~0x10000ULL;
					} else if (prop->GetKind() == 0x1a1) {
						PSMTXCopy(hand.GetMtx(), mount);
						RotateHeldProp(mount, result, rotation, 'x', -1.17809725f); // 67.5 degrees
						OffsetHeldProp(result, translation, 0, -3.5f, -3);
						RotateHeldProp(result, result, rotation, 'z', -HALF_PI);
						SetHeldPropMatrix(prop, result);
						if (mStatus.mFlags & 0x10000) // the prop draws only while her flag is set
							prop->mStatus.mFlags |= 0x10000ULL;
						else
							prop->mStatus.mFlags &= ~0x10000ULL;
					}
				}
				break;
			}
	}
	if ((mActorStatus.mFlags & 0x100) && (player->mPlayerStatus.mFlags & 0x40000000)) { // she carries the player
		s32 hold[6] = { 0, 4, 2, 0, 0, 0 };
		hand.Attach(FindJointByPath(mJObj, hold, 6));
		CVec pos = hand.GetPos();
		pos.y += 2; // two units above the hand
		player->SetPos(pos, 0);
		s32 face[5] = { 0, 4, 0, 0, 0 };
		hand.Attach(FindJointByPath(mJObj, face, 5));
		pos = hand.GetPos();
		player->SetRotateY(atan2(pos.x - player->mMoveTranslate.x, pos.z - player->mMoveTranslate.z), 0); // face the way her palm points
	}
}
