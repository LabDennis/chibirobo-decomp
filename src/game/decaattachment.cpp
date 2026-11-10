#include "game/xobjactor.h"
#include "game/xchr.h"
#include "game/gmath.h"

extern "C" {
void ApproachHudValue(f32* value, f32* target, f32 step);
CVec GetStagePlacementScale(XObj* object);
void HSD_MtxGetScale(Mtx matrix, Vec* scale);
void PSMTXTrans(Mtx mtx, f32 x, f32 y, f32 z);
}

/**
 * @note Address: 0x80130D18
 * @note Size: 0xD98
 */
void XObjActorDeca::vf148(u32 frames)
{
	vf148__9XObjActorFUl(this, frames);
	CJObj joint;
	XObjChr* player = gGame.GetPlayer();
	if (_C90.mFlags & 1) {
		s32 first[3] = { 0, 0, 0 };
		joint.Attach(FindJointByPath(mJObj, first, 3));
		HSD_JObjSetFlags(joint.mJObj, 0x10);
		s32 second[4] = { 0, 0, 0, 0 };
		joint.Attach(FindJointByPath(mJObj, second, 4));
		HSD_JObjSetFlags(joint.mJObj, 0x10);
	} else {
		s32 first[3] = { 0, 0, 0 };
		joint.Attach(FindJointByPath(mJObj, first, 3));
		HSD_JObjClearFlags(joint.mJObj, 0x10);
		s32 second[4] = { 0, 0, 0, 0 };
		joint.Attach(FindJointByPath(mJObj, second, 4));
		HSD_JObjClearFlags(joint.mJObj, 0x10);
	}
	ApproachHudValue(&_CA0, &_CA4, 0.1f);
	s32 head[5] = { 0, 1, 0, 0, 0 };
	joint.Attach(FindJointByPath(mJObj, head, 5));
	joint.SetRotationY(_CA0, 0);
	if ((_998.mFlags & 0x100) && (player->_980.mFlags & 0x40000000)) {
		s32 mount[2] = { 0, 3 };
		joint.Attach(FindJointByPath(mJObj, mount, 2));
		Mtx scaling, rotation, translation, result, matrix;
		PSMTXCopy(joint.GetMtx(), matrix);
		Vec scale;
		HSD_MtxGetScale(matrix, &scale);
		scale.x = GetStagePlacementScale(player).x / scale.x;
		scale.y = GetStagePlacementScale(player).y / scale.y;
		scale.z = GetStagePlacementScale(player).z / scale.z;
		PSMTXScale(scaling, scale.x, scale.y, scale.z);
		PSMTXConcat(matrix, scaling, result);
		PSMTXRotRad(rotation, 'y', 0.0f);
		PSMTXConcat(result, rotation, result);
		PSMTXTrans(translation, 4.5f, -2.3f, 0.0f);
		PSMTXConcat(result, translation, result);
		HSD_JObj* root   = player->mJObj;
		MtxPtr matrixPtr = result;
		HSD_JObjCopyMtx(root, matrixPtr);
	}
}
