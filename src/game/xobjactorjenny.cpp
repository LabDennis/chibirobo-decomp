#include "game/xobjactor.h"
#include "game/heldpropmatrix.h"
#include "game/xchr.h"
#include "game/gmath.h"
#include <stdlib.h>
#include <math.h>

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
void XObjActorJenny::vf148(u32 frames)
{
	vf148__9XObjActorFUl(this, frames);
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
		default:
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
	if ((_998.mFlags & 0x200) && (player->_980.mFlags & 0x40000000)) {
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
	if ((_998.mFlags & 0x100) && (player->_980.mFlags & 0x40000000)) {
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
