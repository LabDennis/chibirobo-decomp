#include "game/xobjactor.h"
#include "game/heldpropmatrix.h"
#include "game/xchr.h"
#include "game/gmath.h"
#include <stdlib.h>
#include <math.h>

void XObjActorJenny::ClearStateHandler()
{
	mState = NULL;
}

/**
 * @note Address: 0x80122E30
 * @note Size: 0x58
 */
void XObjActorJenny::RunStateHandler(u32 frames)
{
	if (mState != NULL) {
		(this->*mState)(frames);
	}
}

/**
 * @note Address: 0x80122E88
 * @note Size: 0x8C
 */
XObjActorJenny::~XObjActorJenny()
{
}

/**
 * @note Address: 0x80122F14
 * @note Size: 0xA4
 */
HSD_JObj* XObjActorJenny::GetHeadJoint()
{
	s32 path[3] = { 0, 3, 0 };
	return FindJointByPath(mJObj, path, 3);
}

/**
 * @note Address: 0x80122FB8
 * @note Size: 0x54
 */
CVec XObjActorJenny::GetHeadWorldPosition()
{
	s32 path[3] = { 0, 3, 0 };
	return GetEventJointWorldPosition(this, 3, path);
}

/**
 * @note Address: 0x8012300C
 * @note Size: 0x28
 */
f32 XObjActorJenny::GetMinHeadPitch()
{
	switch (mAnimId) {
	case 0x412:
	case 0x413:
	case 0x414:
		return 0.34906584f;
	default:
		return -0.34906584f;
	}
}

/**
 * @note Address: 0x80123034
 * @note Size: 0x28
 */
f32 XObjActorJenny::GetMaxHeadPitch()
{
	switch (mAnimId) {
	case 0x412:
	case 0x413:
	case 0x414:
		return 1.0471976f;
	default:
		return 0.17453292f;
	}
}

/**
 * @note Address: 0x8012305C
 * @note Size: 0x24C
 */
s32 XObjActorJenny::HandleEventCommand(s32 count, s32* command)
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
				SetActorLookTarget(this, &FindObj(command[2])->mMoveTranslate, DEG2RAD * command[3]);
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
		for (int i = 0; i < 8; i++) {
			mCmd[i + 2] = command[i + 2];
		}
		if (mCmd[1] == 0xD0) {
			mCmd[19] = (s32)FindObj(mCmd[2]);
		}
		break;
	}
	return 0;
}

/**
 * @note Address: 0x801232A8
 * @note Size: 0x4
 */
void XObjActorJenny::StateNone(u32 frames)
{
}

/**
 * @note Address: 0x80124980
 * @note Size: 0x430
 */
// NONMATCHING: the paths sit at other .rodata offsets
void XObjActorJenny::UpdateAnim(u32 frames)
{
	CXObjIntp::UpdateAnim(frames);
	CJObj joint;
	if (_C90.mFlags & 8) {
		s32 path[7] = { 0, 3, 2, 0, 0, 1, 0 };
		joint.Attach(GetJointFromRoot(mJObj, 7, path));
		HSD_JObjClearFlags(joint.mJObj, 0x10);
	} else {
		s32 path[7] = { 0, 3, 2, 0, 0, 1, 0 };
		joint.Attach(GetJointFromRoot(mJObj, 7, path));
		HSD_JObjSetFlags(joint.mJObj, 0x10);
	}
}

/**
 * @note Address: 0x80124DB0
 * @note Size: 0x4CC
 */
void XObjActorJenny::UpdateObject(u32 frames)
{
	XObjActor::UpdateObject(frames);
	CJObj joint;
	if (mStatus.mFlags & 0x10000) {
		if (_CA0) {
			s32 path[] = { 0, 3, 0, 0, 4 };
			joint.Attach(GetJointFromRoot(mJObj, 5, path));
			HSD_JObjSetFlags(joint.mJObj, 0x10);
		} else {
			s32 path[] = { 0, 3, 0, 0, 4 };
			joint.Attach(GetJointFromRoot(mJObj, 5, path));
			HSD_JObjClearFlags(joint.mJObj, 0x10);
		}
	}

	XSound& sound = gGame.mSound;
	if (mAnimId == 0x47) {
		if (!IsGameSoundPlaying(&sound, 0x19B)) {
			u8 volume = (_998.mFlags & 0x800) ? 0xFF : GetSoundVolumeAtPosition(&sound, &mMoveTranslate, 0);
			PlayGameSound(&sound, 0x19B, volume);
		}
	} else {
		sound.StopIfPlaying(0x19B);
	}
}

extern "C" void Attach__5CJObjFP9_HSD_JObj(CJObj*, HSD_JObj*);

static inline void JennyPropVisibility(XObjActorJenny* jenny, XObj* object)
{
	if (jenny->mStatus.mFlags & 0x10000)
		object->mStatus.mFlags |= 0x10000ULL;
	else
		object->mStatus.mFlags &= ~0x10000ULL;
}

// Positions the props Jenny holds and turns the carried player toward her head.
void XObjActorJenny::UpdateAfterAnim(u32 frames)
{
	UpdateAfterAnim__9XObjActorFUl(this, frames);
	CJObj joint;
	XObjChr* player = gGame.GetPlayer();
	if (_C98.mFlags & 0x1f) {
		CVec position;
		s32 kind = -1;
		if (_C98.Check(1)) {
			kind        = 0x3d;
			s32 hand[6] = { 0, 3, 1, 0, 0, 0 };
			Attach__5CJObjFP9_HSD_JObj(&joint, FindJointByPath(mJObj, hand, 6));
			position = joint.GetPos();
		} else if (_C98.Check(2)) {
			kind        = 0x3e;
			s32 hand[6] = { 0, 3, 1, 0, 0, 0 };
			Attach__5CJObjFP9_HSD_JObj(&joint, FindJointByPath(mJObj, hand, 6));
			position = joint.GetPos();
		} else if (_C98.Check(4)) {
			kind        = 0x3f;
			s32 hand[6] = { 0, 3, 1, 0, 0, 0 };
			Attach__5CJObjFP9_HSD_JObj(&joint, FindJointByPath(mJObj, hand, 6));
			position = joint.GetPos();
		} else if (_C98.Check(8)) {
			kind        = 0x3b;
			s32 hand[6] = { 0, 3, 1, 0, 0, 0 };
			Attach__5CJObjFP9_HSD_JObj(&joint, FindJointByPath(mJObj, hand, 6));
			position = joint.GetPos();
		} else if (_C98.Check(16)) {
			kind        = 0x401;
			s32 hand[6] = { 0, 3, 2, 0, 0, 0 };
			Attach__5CJObjFP9_HSD_JObj(&joint, FindJointByPath(mJObj, hand, 6));
			position = joint.GetPos();
		}
		Mtx mount, translation, rotation, result;
		switch (kind) {
		case 0x3c:
			break;
		case 0x401:
			for (s32 i = 0; i < 288; ++i) {
				XObj* prop = gGame.mXObjs.mPools[22].Get(i);
				if (kind == prop->GetKind()) {
					switch (kind) {
					case 0x401:
						PSMTXCopy(joint.GetMtx(), mount);
						PSMTXRotRad(rotation, 'z', HALF_PI);
						PSMTXConcat(mount, rotation, result);
						PSMTXTrans(translation, -4, 4, 0);
						PSMTXConcat(result, translation, result);
						SetHeldPropMatrix(prop, result);
						break;
					default:
						SetHeldPropMatrix(prop, joint.GetMtx());
						break;
					}
					JennyPropVisibility(this, prop);
				}
			}
			break;
		case 0x3b:
		case 0x3d:
		case 0x3e:
		case 0x3f:
			for (s32 i = 0; i < 64; ++i) {
				XObj* prop = gGame.mXObjs.mPools[8].Get(i);
				if (kind == prop->GetKind()) {
					switch (kind) {
					case 0x3b: {
						MtxPtr source = joint.GetMtx();
						PSMTXCopy(source, mount);
						PSMTXTrans(translation, -2, -2, -10);
						PSMTXConcat(source, translation, result);
						SetHeldPropMatrix(prop, result);
						break;
					}
					case 0x3d:
					case 0x3e:
					case 0x3f:
						PSMTXCopy(joint.GetMtx(), mount);
						PSMTXRotRad(rotation, 'x', PI);
						PSMTXConcat(mount, rotation, result);
						PSMTXTrans(translation, 0, 0, 10);
						PSMTXConcat(result, translation, result);
						SetHeldPropMatrix(prop, result);
						break;
					default:
						SetHeldPropMatrix(prop, joint.GetMtx());
						break;
					}
					JennyPropVisibility(this, prop);
				}
			}
			break;
		}
	}
	if ((_998.mFlags & 0x200) && (player->mPlayerStatus.mFlags & 0x40000000)) {
		s32 hand[6] = { 0, 3, 2, 0, 0, 0 };
		Attach__5CJObjFP9_HSD_JObj(&joint, FindJointByPath(mJObj, hand, 6));
		CVec pos = joint.GetPos();
		{
			f32 angle = atan2(pos.x - mMoveTranslate.x, pos.z - mMoveTranslate.z);
			f32 sx    = (f32)sin(angle - HALF_PI);
			f32 cz    = (f32)cos(angle - HALF_PI);
			pos.x += 3.0f * sx;
			pos.z += 3.0f * cz;
		}
		pos.y += 2;
		player->SetPos(pos, 0);
		s32 head[4] = { 0, 3, 0, 0 };
		Attach__5CJObjFP9_HSD_JObj(&joint, FindJointByPath(mJObj, head, 4));
		pos = joint.GetPos();
		player->SetRotateY(atan2(pos.x - player->mMoveTranslate.x, pos.z - player->mMoveTranslate.z), 0);
	}
	if ((_998.mFlags & 0x100) && (player->mPlayerStatus.mFlags & 0x40000000)) {
		s32 hand[6] = { 0, 3, 2, 0, 0, 0 };
		Attach__5CJObjFP9_HSD_JObj(&joint, FindJointByPath(mJObj, hand, 6));
		CVec pos = joint.GetPos();
		pos.y += 2;
		player->SetPos(pos, 0);
		s32 head[4] = { 0, 3, 0, 0 };
		Attach__5CJObjFP9_HSD_JObj(&joint, FindJointByPath(mJObj, head, 4));
		pos = joint.GetPos();
		player->SetRotateY(atan2(pos.x - player->mMoveTranslate.x, pos.z - player->mMoveTranslate.z), 0);
	}
}

/**
 * @note Address: 0x8012527C
 * @note Size: 0x298
 */
// lets go of what she carries
void XObjActorJenny::Drop()
{
	s32 kind = -1;
	if (_C98.Check(1)) {
		_C98.mFlags &= ~1;
		kind = 0x3D;
	} else if (_C98.Check(2)) {
		_C98.mFlags &= ~2;
		kind = 0x3E;
	} else if (_C98.Check(4)) {
		_C98.mFlags &= ~4;
		kind = 0x3F;
	} else if (_C98.Check(8)) {
		_C98.mFlags &= ~8;
		kind = 0x3B;
	} else if (_C98.Check(0x10)) {
		_C98.mFlags &= ~0x10;
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
		for (s32 i = 0; i < 288; i++) {
			XObj* obj = gGame.mXObjs.mPools[22].Get(i);
			if (kind == obj->GetKind()) {
				obj->mStatus.mFlags &= ~0x10000ULL;
			}
		}
		break;
	}
}

static inline void HoldPlayerForJenny(XObjChr* player)
{
	player->mPlayerStatus.mFlags = 0;
	player->mPlayerStatus.mFlags |= 0x40000000;
	player->mPad.mReset = FALSE;
	player->SetAnim(1, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
}

static inline void ReleasePlayerAt(XObjChr* player, const Vec& position)
{
	player->SetPos(position, 0);
	player->mPlayerStatus.mFlags = 0;
	player->mPlayerStatus.mFlags |= 0x20000000;
	player->mPlayerStatus.mFlags |= 7;
	player->mPad.mReset = TRUE;
}

/**
 * @note Address: 0x80125514
 * @note Size: 0xDDC
 */
void XObjActorJenny::ProcessPendingEvents(u32)
{
	CVec firstPos;
	CJObj joint;
	XObjChr* player = gGame.GetPlayer();
	if (_998.mFlags & 2) {
		_998.mFlags &= ~2;
		PlayGameSound(&gGame.mSound, rand() % 10 + 0x92, 0xFF);
	}
	switch (mCmd[0]) {
	case 2:
		if (mCmd[1] == 8)
			_C90.mFlags |= 8;
		else
			_C90.mFlags &= ~8;
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
						_C98.mFlags |= 8;
						break;
					case 0x3D:
						_C98.mFlags |= 1;
						break;
					case 0x3E:
						_C98.mFlags |= 2;
						break;
					case 0x3F:
						_C98.mFlags |= 4;
						break;
					}
				}
			}
			break;
		case 0x401:
			for (s32 i = 0; i < 288; ++i) {
				XObj* object = gGame.mXObjs.mPools[22].Get(i);
				if (mCmd[1] == object->GetKind()) {
					mHeldObjects[0] = object;
					object->mStatus.mFlags |= 0x10000ULL;
					if (mCmd[1] == 0x401)
						_C98.mFlags |= 0x10;
				}
			}
			break;
		default:
			if (mHeldObjects[0]) {
				HSD_JObjClearMtxFlags(mHeldObjects[0]->mJObj);
				mHeldObjects[0] = NULL;
			}
		}
		mCmd[0] = 0;
		break;
	case 4:
		if (mCmd[1] == 1) {
			_998.mFlags |= 0x200;
			HoldPlayerForJenny(player);
		} else {
			_998.mFlags &= ~0x200;
			s32 first[] = { 0, 3, 1, 0, 0, 0 };
			joint.Attach(FindJointByPath(mJObj, first, 6));
			firstPos     = CVec(joint.GetPos());
			s32 second[] = { 0, 3, 2, 0, 0, 0 };
			joint.Attach(FindJointByPath(mJObj, second, 6));
			CVec secondPos(joint.GetPos());
			// Retail computes this midpoint, then releases the player at the second joint.
			firstPos.x -= (firstPos.x - secondPos.x) * 0.5f;
			firstPos.z -= (firstPos.z - secondPos.z) * 0.5f;
			firstPos.y = mMoveTranslate.y;
			ReleasePlayerAt(player, secondPos);
		}
		mCmd[0] = 0;
		break;
	case 5:
		if (mCmd[1] == 1) {
			_998.mFlags |= 0x100;
			HoldPlayerForJenny(player);
		} else {
			_998.mFlags &= ~0x100;
			s32 path[] = { 0, 3, 2, 0, 0, 0 };
			joint.Attach(FindJointByPath(mJObj, path, 6));
			CVec position(joint.GetPos());
			ReleasePlayerAt(player, position);
		}
		mCmd[0] = 0;
		break;
	case 7: {
		void* anim  = GetJointAnimation(mCmd[1]);
		void* mat   = GetMaterialAnimation(mCmd[2]);
		void* shape = GetShapeAnimation(mCmd[1]);
		SetAnim(0, TRUE, anim, mat, shape, 0, 1, 1, 10);
		mStatus.mFlags |= 0x0008000000000000ULL;
		mCmd[0] = 0;
		break;
	}
	case 8:
		switch (mCmd[1]) {
		case 0:
			_CA0    = 1;
			mCmd[0] = 0;
			break;
		case 1:
			_CA0    = 0;
			mCmd[0] = 0;
			break;
		}
		break;
	}
}

/**
 * @note Address: 0x801262F0
 * @note Size: 0x12C
 */
void XObjActorJenny::ResetActorState()
{
	XObjChr* player = gGame.GetPlayer();
	if (player->mPlayerStatus.mFlags & 0x40000000) {
		player->mPlayerStatus.mFlags = 0;
		player->mPlayerStatus.mFlags |= 0x20000000;
		player->mPlayerStatus.mFlags |= 7;
		player->mPad.mReset = TRUE;
	}
	_998.mFlags = 0;
	_C90.mFlags = 0;
	_C98.mFlags = 0;
	_980        = 0.0f;
	_984        = 0.0f;
	_988        = 0.0f;
	_C84        = _988;
	_C88        = 0.0f;
	_C8C        = 0;
	_CA0        = 0;
	mState      = &XObjActorJenny::StateNone;
	ResetActorMovementStep(this);
	for (u32 i = 0; i < 20; i++) {
		mCmd[i] = 0;
	}
	_C90.mFlags &= ~8;
}
