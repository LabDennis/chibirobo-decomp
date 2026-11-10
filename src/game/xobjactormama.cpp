#include "game/xobjactor.h"
#include "game/xchr.h"
#include "game/gmath.h"
#include "game/heldpropmatrix.h"
#include <math.h>

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
					CopyHeldPropVisibility(this, prop);
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
				CopyHeldPropVisibility(this, prop);
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
					CopyHeldPropVisibility(this, prop);
				} else if (prop->GetKind() == 0x1a1) {
					PSMTXCopy(hand.GetMtx(), mount);
					RotateHeldProp(mount, result, rotation, 'x', -1.17809725f);
					OffsetHeldProp(result, translation, 0, -3.5f, -3);
					RotateHeldProp(result, result, rotation, 'z', -HALF_PI);
					SetHeldPropMatrix(prop, result);
					CopyHeldPropVisibility(this, prop);
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
