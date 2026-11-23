#include "game/xobjactor.h"
#include "game/xchr.h"
#include "game/cgameparticle.h"
#include "game/gmath.h"
#include "game/heldpropmatrix.h"
#include "game/evt.h"
#include "game/cscrn.h"
#include <math.h>
extern "C" {
#include <sysdolphin/random.h>
}

// NONMATCHING: ProcessPendingEvents and UpdateAfterAnim still differ from retail; in data
// (GGTE01), .rodata (0x8020BE08) lacks the header-originated six-Vec prefix (0x48 bytes),
// and .data/.sdata/.sdata2/.sbss2 differ in weak RTTI records and literal pool entries.

static inline u32 MaskedFlags(const CStatus& status, u32 mask)
{
	return status.mFlags & mask;
}

/**
 * @note Address: 0x8012926C
 * @note Size: 0xB0
 */
void XObjActorMama::ResetActorState()
{
	XObjChr* player = gGame.GetPlayer();
	if (player->mPlayerStatus.mFlags & XObjChr::ControlLocked) {
		player->RestoreControl();
	}
	mActorStatus.mFlags = 0;
	mHideKinds.mFlags   = 0;
	mHeading            = 0.0f;
	mMoveSpeed          = 0.0f;
	mFacing             = 0.0f;
	_C84                = mFacing;
	_C88                = 0;
	mState              = &XObjActorMama::StateNone;
	ResetActorMovementStep(this);
}

// NONMATCHING: case 6 builds the effect's rotation straight in CreateSRT's by-value slot (US 0x8012918C stores the
// { 0, 0, 0 } copy to 0x20(r1), 0x801291E8 its y), this build makes the local and copies it, so the frame is 0xC larger
// and one more saved register is used. Tried: { 0, rotY, 0 } as the initialiser, scale declared first, both Vecs as
// compound literals at the call (the rotation is then built where retail builds it, but still copied) and a Vec-returning
// inline, all lower or equal
void XObjActorMama::ProcessPendingEvents(u32)
{
	CJObj jobj;
	XObjChr* player = gGame.GetPlayer();
	CVec pos;
	if (mActorStatus.Check(0x400)) { // drop the player pitch-tracking and level the head
		mActorStatus.mFlags &= ~0x400;
		mHeadPitch = 0.0f;
	}
	if (mActorStatus.Check(2)) { // 2: a random sound from the 10-entry range at 0x9E
		mActorStatus.mFlags &= ~2;
		PlayGameSound(&gGame.mSound, HSD_Randi(10) + 0x9E, 0xFF);
	}
	switch (mCmd[0]) {
	case 2: // hold a scripted kind
		Drop();
		switch (mCmd[1]) {
		case 0x40:
			for (s32 i = 0; i < 64; i++) { // the kind's own pool
				XObj* obj = gGame.mXObjs.mPools[8].Get(i);
				if (mCmd[1] == obj->GetKind()) {
					mHeldObjects[0] = obj;
					obj->mStatus.mFlags |= 0x10000ULL;
					switch (mCmd[1]) {
					case 0x40:
						mHideKinds.mFlags |= 1;
						break;
					}
				}
			}
			break;
		case 0x5B:
		case 0x7D:
		case 0x196:
		case 0x45B:
			for (s32 i = 0; i < 288; i++) {
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
		case 0x1A1: // a pair: 0x1A0 into hand 0, 0x1A1 into hand 1
			for (s32 i = 0; i < 288; i++) {
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
		default: // hold nothing
			for (int i = 0; i < 2; ++i) {
				XObj*& held = mHeldObjects[i];
				if (held) {
					HSD_JObjClearMtxFlags(held->mJObj);
					held = NULL;
				}
			}
			break;
		}
		mCmd[0] = 0;
		break;
	case 3: // grab or set down the player
		switch (mCmd[1]) {
		case 1: {
			mActorStatus.mFlags |= 0x100;
			player->LockControl();
			player->SetAnim(1, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);

			break;
		}
		default: {
			mActorStatus.mFlags &= ~0x100;
			s32 path[6] = { 0, 4, 2, 0, 0, 0 };
			jobj.Attach(FindJointByPath(mJObj, path, 6));
			pos   = CVec(jobj.GetPos());
			pos.y = mMoveTranslate.y;
			player->SetPos(pos, 0);
			player->RestoreControl();
			break;
		}
		}
		mCmd[0] = 0;
		break;
	case 4: // shake the screen, arg in 1/100 units
		StartScreenShake(&gGame.mQuake, mCmd[1], mCmd[2], mCmd[3], 0.0f, 0.0f, 0.0f, mCmd[4] / 100.0f);
		mCmd[0] = 0;
		break;
	case 5: { // play a scripted animation
		void* joint = GetJointAnimation(mCmd[1]);
		void* mat   = GetMaterialAnimation(mCmd[2]);
		void* shape = GetShapeAnimation(mCmd[1]);
		SetAnim(0, TRUE, joint, mat, shape, 0.0f, 1.0f, 1.0f, 10.0f);
		mStatus.mFlags |= 0x0008000000000000ULL;
		mCmd[0] = 0;
		break;
	}
	case 6: { // a particle burst 8 units ahead
		s32 path[] = { 0, 4, 0, 0, 0 };
		jobj.Attach(FindJointByPath(mJObj, path, 5));
		pos = CVec(jobj.GetPos());
		pos.x += 8.0f * (f32)sin(HSD_JObjGetRotationY(mJObj));
		pos.y += 1.0f;
		pos.z += 8.0f * (f32)cos(HSD_JObjGetRotationY(mJObj));
		Vec rotation = { 0, 0, 0 };
		rotation.y   = HSD_JObjGetRotationY(mJObj);
		Vec scale    = { 1, 1, 1 };
		gGame.mParticle.CreateSRT(0x82, 1, pos, scale, rotation, TRUE);
		mCmd[0] = 0;
		break;
	}
	}
}

/**
 * @note Address: 0x80128230
 * @note Size: 0x38C
 */
// lets go of what she carries
void XObjActorMama::Drop()
{
	s32 kind = -1;
	if (MaskedFlags(mHideKinds, 4)) {
		mHideKinds.mFlags &= ~4;
		kind = 0x7D;
	} else if (MaskedFlags(mHideKinds, 1)) {
		mHideKinds.mFlags &= ~1;
		kind = 0x40;
	} else if (MaskedFlags(mHideKinds, 2)) {
		mHideKinds.mFlags &= ~2;
		kind = 0x5B;
	} else if (MaskedFlags(mHideKinds, 8)) {
		mHideKinds.mFlags &= ~8;
		kind = 0x196;
	} else if (MaskedFlags(mHideKinds, 0x10)) {
		mHideKinds.mFlags &= ~0x10;
		kind = 0x1A1;
	} else if (MaskedFlags(mHideKinds, 0x20)) {
		mHideKinds.mFlags &= ~0x20;
		kind = 0x45B;
	}
	if (kind < 0) {
		return;
	}
	switch (kind) {
	case 0x40:
		for (s32 i = 0; i < 64; i++) {
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
		for (s32 i = 0; i < 288; i++) {
			XObj* obj = gGame.mXObjs.mPools[22].Get(i);
			if (kind == obj->GetKind()) {
				obj->mStatus.mFlags &= ~0x10000ULL;
			}
		}
		break;
	case 0x1A1:
		for (s32 i = 0; i < 288; i++) {
			XObj* obj = gGame.mXObjs.mPools[22].Get(i);
			if (obj->GetKind() == 0x1A0) {
				obj->mStatus.mFlags &= ~0x10000ULL;
			} else if (kind == obj->GetKind()) {
				obj->mStatus.mFlags &= ~0x10000ULL;
			}
		}
		break;
	}
}

/**
 * @note Address: 0x80128210
 * @note Size: 0x20
 */
void XObjActorMama::UpdateObject(u32 frames)
{
	XObjActor::UpdateObject(frames);
}

void XObjActorMama::UpdateAfterAnim(u32 frames)
{
	XObjActor::UpdateAfterAnim(frames);
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
	if ((mActorStatus.mFlags & 0x100) && (player->mPlayerStatus.mFlags & XObjChr::ControlLocked)) { // she carries the player
		s32 hold[6] = { 0, 4, 2, 0, 0, 0 };
		hand.Attach(FindJointByPath(mJObj, hold, 6));
		CVec pos = hand.GetPos();
		pos.y += 2; // two units above the hand
		player->SetPos(pos, 0);
		s32 face[5] = { 0, 4, 0, 0, 0 };
		hand.Attach(FindJointByPath(mJObj, face, 5));
		pos = hand.GetPos();
		// turn the player toward her head joint
		player->SetRotateY(atan2(pos.x - player->mMoveTranslate.x, pos.z - player->mMoveTranslate.z), 0);
	}
}

/**
 * @note Address: 0x80126848
 * @note Size: 0x4
 */
void XObjActorMama::StateNone(u32 frames)
{
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
			SetActorLookTarget(this, &gGame.GetPlayer()->mMoveTranslate, DEG2RAD * 5);
			break;
		case 2:
			if (FindObj(command[2]) != NULL) {
				SetActorLookTarget(this, &FindObj(command[2])->mMoveTranslate, DEG2RAD * command[3]);
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
 * @note Address: 0x80126630
 * @note Size: 0x8
 */
f32 XObjActorMama::GetMaxHeadYaw()
{
	return DEG2RAD * 40;
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
 * @note Address: 0x80126620
 * @note Size: 0x8
 */
f32 XObjActorMama::GetMaxHeadPitch()
{
	return DEG2RAD * 10;
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
 * @note Address: 0x801265BC
 * @note Size: 0x5C
 */
CVec XObjActorMama::GetHeadWorldPosition()
{
	s32 headPath[4] = { 0, 4, 0, 0 };
	return GetEventJointWorldPosition(this, 4, headPath);
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
 * @note Address: 0x80126498
 * @note Size: 0x78
 */
XObjActorMama::~XObjActorMama()
{
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
 * @note Address: 0x8012641C
 * @note Size: 0x24
 */
void XObjActorMama::ClearStateHandler()
{
	mState = NULL;
}
