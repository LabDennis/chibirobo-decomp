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

// Carries the selected prop at its hand joint and positions the player when held.
void XObjActorPapa::vf148(u32 frames)
{
	vf148__9XObjActorFUl(this, frames);
	CJObj joint;
	XObjChr* player = gGame.GetPlayer();
	if (_C8C.mFlags & 0x3f) {
		CVec position;
		s32 kind = -1;
		if (_C8C.Check(1)) {
			kind        = 0x1f4;
			s32 path[6] = { 0, 2, 2, 0, 0, 0 };
			Attach__5CJObjFP9_HSD_JObj(&joint, FindJointByPath(mJObj, path, 6));
			position = joint.GetPos();
		} else if (_C8C.Check(2)) {
			kind        = 0x360;
			s32 path[6] = { 0, 2, 2, 0, 0, 0 };
			Attach__5CJObjFP9_HSD_JObj(&joint, FindJointByPath(mJObj, path, 6));
			position = joint.GetPos();
		} else if (_C8C.Check(4)) {
			kind        = 0x1d3;
			s32 path[6] = { 0, 2, 2, 0, 0, 0 };
			Attach__5CJObjFP9_HSD_JObj(&joint, FindJointByPath(mJObj, path, 6));
			position = joint.GetPos();
		} else if (_C8C.Check(8)) {
			kind        = 0x375;
			s32 path[6] = { 0, 2, 2, 0, 0, 0 };
			Attach__5CJObjFP9_HSD_JObj(&joint, FindJointByPath(mJObj, path, 6));
			position = joint.GetPos();
		} else if (_C8C.Check(16)) {
			kind        = 0x3fa;
			s32 path[6] = { 0, 2, 1, 0, 0, 0 };
			Attach__5CJObjFP9_HSD_JObj(&joint, FindJointByPath(mJObj, path, 6));
			position = joint.GetPos();
		} else if (_C8C.Check(32)) {
			kind        = 0x401;
			s32 path[6] = { 0, 2, 2, 0, 0, 0 };
			Attach__5CJObjFP9_HSD_JObj(&joint, FindJointByPath(mJObj, path, 6));
			position = joint.GetPos();
		}
		Mtx mount, translation, rotation, result;
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
					CopyHeldPropVisibility(this, prop);
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
					CopyHeldPropVisibility(this, prop);
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
					CopyHeldPropVisibility(this, prop);
				}
			}
			break;
		}
	}
	if ((_998.mFlags & 0x100) && (player->_980.mFlags & 0x40000000)) {
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
	if (!_C94) {
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
