#include "game/xobjactor.h"
#include "game/xchr.h"
#include "game/gmath.h"

extern "C" {
void HSD_MtxGetScale(Mtx mtx, Vec* scale);
void PSMTXTrans(Mtx mtx, f32 x, f32 y, f32 z);
CVec GetStagePlacementScale(XObj* object); // the object's scale as it is drawn
}

extern "C" void Attach__5CJObjFP9_HSD_JObj(CJObj*, HSD_JObj*);
static inline void CopyTaoVisibility(const XObjActorTao* tao, XObj* carried)
{
	if (tao->mStatus.mFlags & 0x10000)
		carried->mStatus.mFlags |= 0x10000ULL;
	else
		carried->mStatus.mFlags &= ~0x10000ULL;
}

static inline void ScaleTaoMount(CJObj& joint, XObj* carried, Mtx mount, Mtx result)
{
	PSMTXCopy(joint.GetMtx(), mount);
	Vec scale;
	HSD_MtxGetScale(mount, &scale);
	scale.x = GetStagePlacementScale(carried).x / scale.x;
	scale.y = GetStagePlacementScale(carried).y / scale.y;
	scale.z = GetStagePlacementScale(carried).z / scale.z;
	Mtx scaling;
	PSMTXScale(scaling, scale.x, scale.y, scale.z);
	PSMTXConcat(mount, scaling, result);
}

static inline void PlaceInTaoMouth(CJObj& joint, XObj* carried, Mtx mount, f32 z)
{
	Mtx result, translation;
	ScaleTaoMount(joint, carried, mount, result);
	PSMTXTrans(translation, 0, -10, z);
	PSMTXConcat(result, translation, result);
	HSD_JObj* root = carried->mJObj;
	MtxPtr matrix  = result;
	HSD_JObjCopyMtx(root, matrix);
}

static inline void CopyTaoJoint(CJObj& joint, XObj* obj)
{
	MtxPtr matrix  = joint.GetMtx();
	HSD_JObj* root = obj->mJObj;
	HSD_JObjCopyMtx(root, matrix);
}
void XObjActorTao::vf148(u32 frames)
{
	vf148__9XObjActorFUl(this, frames);
	CJObj joint;
	if (_C8C.mFlags & 0x2f) {
		s32 mouth[6] = { 0, 0, 0, 0, 0, 0 };
		Attach__5CJObjFP9_HSD_JObj(&joint, FindJointByPath(mJObj, mouth, 6));
		CVec mouthPosition;
		mouthPosition = joint.GetPos();
		s32 kind      = -1;
		if (_C8C.mFlags & 2)
			kind = 0x102;
		else if (_C8C.mFlags & 1)
			kind = 0x159;
		else if (_C8C.mFlags & 4)
			kind = 0xf8;
		else if (_C8C.mFlags & 8) {
			kind        = 0x2a3;
			s32 path[5] = { 0, 0, 0, 0, 0 };
			Attach__5CJObjFP9_HSD_JObj(&joint, FindJointByPath(mJObj, path, 5));
		} else if (_C8C.mFlags & 0x20) {
			kind        = 0x46f;
			s32 path[3] = { 0, 0, 0 };
			Attach__5CJObjFP9_HSD_JObj(&joint, FindJointByPath(mJObj, path, 3));
		}
		switch (kind) {
		case 0x102:
			for (s32 i = 0; i < 64; ++i) {
				XObj* obj = gGame.mXObjs.mPools[8].Get(i);
				if (kind == obj->GetKind()) {
					switch (kind) {
					case 0x102:
						PlaceInTaoMouth(joint, obj, _C94, 0);
						break;
					default:
						CopyTaoJoint(joint, obj);
						break;
					}
				}
				CopyTaoVisibility(this, obj);
			}
			break;
		case 0x159:
			for (s32 i = 0; i < 15; ++i) {
				XObj* obj = gGame.mXObjs.mPools[10].Get(i);
				if (kind == obj->GetKind()) {
					switch (kind) {
					case 0x159:
						ScaleTaoMount(joint, obj, _C94, _C94);
						HSD_JObjCopyMtx(obj->mJObj, _C94);
						break;
					default:
						CopyTaoJoint(joint, obj);
						break;
					}
					CopyTaoVisibility(this, obj);
				}
			}
			break;
		case 0xf8: {
			s32 ordinal = -1;
			for (s32 i = 0; i < 21; ++i) {
				XObj* obj = gGame.mXObjs.mPools[16].Get(i);
				if (kind == obj->GetKind()) {
					switch (kind) {
					case 0xf8:
						if (++ordinal != mCmd[2])
							break;
						PlaceInTaoMouth(joint, obj, _C94, 3);
						return;
					default:
						CopyTaoJoint(joint, obj);
						CopyTaoVisibility(this, obj);
						break;
					}
				}
			}
			break;
		}
		case 0x2a3:
		case 0x46f:
			for (s32 i = 0; i < 288; ++i) {
				XObj* obj = gGame.mXObjs.mPools[22].Get(i);
				if (kind == obj->GetKind()) {
					// The original dispatches the two stage prop kinds separately.
					switch (kind) {
					case 0x2a3:
						CopyTaoJoint(joint, obj);
						break;
					case 0x46f:
						CopyTaoJoint(joint, obj);
						break;
					default:
						CopyTaoJoint(joint, obj);
						break;
					}
					CopyTaoVisibility(this, obj);
				}
			}
			break;
		}
	}
	switch (_CF8) {
	case 1:
		if (_CFC)
			_CFC->Release();
		if (_CF4 >= 0) {
			_CFC = ReadObj(_CF4);
			++_CF8;
		} else
			_CFC = NULL;
		break;
	case 2:
		if (_CFC && !(_CFC->mStatus.mFlags & 0x0040000000000000ULL)) {
			gGame.mStatus.mFlags |= 0x1000;
			++_CF8;
		}
		break;
	case 3:
		if (_CF4 >= 0 && _CFC) {
			_CFC->SetScale(1, 1, 1, 0);
			CJObj extra;
			s32 path[3] = { 0, 0, 0 };
			Attach__5CJObjFP9_HSD_JObj(&extra, FindJointByPath(mJObj, path, 3));
			Mtx result;
			ScaleTaoMount(extra, _CFC, _CC4, result);
			HSD_JObj* root = _CFC->mJObj;
			MtxPtr matrix  = result;
			HSD_JObjCopyMtx(root, matrix);
			CopyTaoVisibility(this, _CFC);
			_CFC->mStatus.mFlags &= ~0x0200000000000000ULL;
		}
		_998.mFlags &= ~0x2000;
		break;
	}
}
