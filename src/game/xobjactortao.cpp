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

/**
 * @note Address: 0x8012C858
 * @note Size: 0x24
 */
void XObjActorTao::ClearStateHandler()
{
	mState = NULL;
}

/**
 * @note Address: 0x8012C87C
 * @note Size: 0x58
 */
void XObjActorTao::RunStateHandler(u32 frames)
{
	if (mState != NULL) {
		(this->*mState)(frames);
	}
}

/**
 * @note Address: 0x8012C8D4
 * @note Size: 0x78
 */
XObjActorTao::~XObjActorTao()
{
}

/**
 * @note Address: 0x8012C94C
 * @note Size: 0xAC
 */
HSD_JObj* XObjActorTao::GetHeadJoint()
{
	s32 path[4] = { 0, 0, 0, 0 };
	return FindJointByPath(mJObj, path, 4);
}

/**
 * @note Address: 0x8012C9F8
 * @note Size: 0x5C
 */
CVec XObjActorTao::GetHeadWorldPosition()
{
	s32 path[4] = { 0, 0, 0, 0 };
	return GetEventJointWorldPosition(this, 4, path);
}

/**
 * @note Address: 0x8012CA54
 * @note Size: 0x44
 */
bool XObjActorTao::IsFree()
{
	if ((mStatus.mFlags & 1) == 0) {
		return true;
	}
	return !(_998.mFlags & 0x2000);
}

/**
 * @note Address: 0x8012CA98
 * @note Size: 0x770
 */
s32 XObjActorTao::HandleEventCommand(s32 count, s32* command)
{
	XObjActor::HandleEventCommand(count, command);
	if (count == 0) {
		return -1;
	}
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
	case 2:
		Drop();
		switch (command[1]) {
		case 0x102:
			for (s32 i = 0; i < 64; i++) {
				XObj* obj = gGame.mXObjs.mPools[8].Get(i);
				if (command[1] == obj->GetKind()) {
					mHeldObjects[0] = obj;
					obj->mStatus.mFlags |= 0x10000ULL;
					switch (command[1]) {
					case 0x102:
						_C8C.mFlags |= 2;
						break;
					}
				}
			}
			break;
		case 0x159:
			for (s32 i = 0; i < 15; i++) {
				XObj* obj = gGame.mXObjs.mPools[10].Get(i);
				if (command[1] == obj->GetKind()) {
					mHeldObjects[0] = obj;
					obj->mStatus.mFlags |= 0x10000ULL;
					_C8C.mFlags |= 1;
				}
			}
			break;
		case 0xF8: {
			mCmd[2] = command[2];
			s32 n   = -1;
			for (s32 i = 0; i < 21; i++) {
				XObj* obj = gGame.mXObjs.mPools[16].Get(i);
				if (command[1] == obj->GetKind()) {
					if (++n == mCmd[2]) {
						obj->mStatus.mFlags |= 0x10000ULL;
						mHeldObjects[0] = obj;
						_C8C.mFlags |= 4;
						break;
					}
				}
			}
			break;
		}
		case 0x2A3:
			for (s32 i = 0; i < 288; i++) {
				XObj* obj = gGame.mXObjs.mPools[22].Get(i);
				if (command[1] == obj->GetKind()) {
					obj->mStatus.mFlags |= 0x10000ULL;
					_C8C.mFlags |= 8;
					mHeldObjects[0] = obj;
				}
			}
			break;
		case 0x46F:
			for (s32 i = 0; i < 288; i++) {
				XObj* obj = gGame.mXObjs.mPools[22].Get(i);
				if (command[1] == obj->GetKind()) {
					obj->mStatus.mFlags |= 0x10000ULL;
					_C8C.mFlags |= 0x20;
					mHeldObjects[0] = obj;
				}
			}
			break;
		default:
			if (mHeldObjects[0] != NULL) {
				HSD_JObjClearMtxFlags(mHeldObjects[0]->mJObj);
				mHeldObjects[0] = NULL;
			}
			break;
		}
		break;
	case 3:
		_A4C   = command[1];
		mState = &XObjActorTao::StatePickUp;
		ResetActorMovementStep(this);
		mCmd[0] = 0;
		break;
	case 4:
		_A4C = -1;
		if (_A50 != NULL) {
			_A50->Release();
		}
		mCmd[0] = 0;
		break;
	case 5:
		if (command[1] != 0) {
			_CF4 = 0x82;
			_CF8 = 1;
		} else {
			_CF8 = 0;
			_CF4 = -1;
			if (_CFC != NULL) {
				_CFC->Release();
			}
		}
		mCmd[0] = 0;
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
 * @note Address: 0x8012D208
 * @note Size: 0x664
 */
// picks up the object _A4C names and holds it in its mouth
void XObjActorTao::StatePickUp(u32 frames)
{
	switch (_990) {
	case 0:
		if (_A50 != NULL) {
			_A50->Release();
		}
		if (_A4C >= 0) {
			_A50 = ReadObj(_A4C);
		} else {
			_A50 = NULL;
		}
		_990++;
	case 1:
		if (_A50 != NULL) {
			if (_A50->mStatus.mFlags & 0x0040000000000000ULL) {
				return;
			}
			gGame.mStatus.mFlags |= 0x1000;
		}
		_990++;
	case 2:
		if (_A4C >= 0 && _A50 != NULL) {
			_A50->SetScale(1.0f, 1.0f, 1.0f, 0);
			CJObj jobj;
			s32 path[6] = { 0, 0, 0, 0, 0, 0 };
			jobj.Attach(FindJointByPath(mJObj, path, 6));
			Vec pos = jobj.GetPos();
			_A50->SetPos(pos.x, HSD_JObjGetTranslationY(mJObj), pos.z, 0);
			PlaceInTaoMouth(jobj, _A50, _C94, 0.0f);
			_A50->mStatus.mFlags &= ~0x0200000000000000ULL;
		}
		break;
	}
}

/**
 * @note Address: 0x8012D86C
 * @note Size: 0x4
 */
void XObjActorTao::StateNone(u32 frames)
{
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

/**
 * @note Address: 0x8012F364
 * @note Size: 0xC4
 */
void XObjActorTao::ResetActorState()
{
	_998.mFlags = 0;
	_C8C.mFlags = 0;
	_980        = 0.0f;
	_984        = 0.0f;
	_988        = 0.0f;
	_C84        = _988;
	_C88        = 0;
	mState      = &XObjActorTao::StateNone;
	ResetActorMovementStep(this);
	mCmd[2] = 0;
	mCmd[3] = 0;
	mCmd[4] = 0;
	mCmd[5] = 0;
	mCmd[6] = 0;
	mCmd[7] = 0;
	mCmd[8] = 0;
	mCmd[9] = 0;
	_CF4    = -1;
	_CF8    = 0;
	_CFC    = NULL;
	_CF4    = 0x82;
	_CF8    = 1;
	_998.mFlags |= 0x2000;
}
