#include "game/xobjactor.h"
#include "game/xchr.h"
#include "game/gmath.h"

extern "C" {
void PSMTXTrans(Mtx mtx, f32 x, f32 y, f32 z);
void ApproachHudValue(f32* value, f32* target, f32 step);
CVec GetStagePlacementScale(XObj* object);
void HSD_MtxGetScale(Mtx matrix, Vec* scale);
}

/**
 * @note Address: 0x8012F428
 * @note Size: 0x24
 */
void XObjActorDeca::ClearStateHandler()
{
	mState = NULL;
}

/**
 * @note Address: 0x8012F44C
 * @note Size: 0x58
 */
void XObjActorDeca::RunStateHandler(u32 frames)
{
	if (mState != NULL) {
		(this->*mState)(frames);
	}
}

/**
 * @note Address: 0x8012F4A4
 * @note Size: 0x8C
 */
XObjActorDeca::~XObjActorDeca()
{
}

/**
 * @note Address: 0x8012F530
 * @note Size: 0xA4
 */
HSD_JObj* XObjActorDeca::GetHeadJoint()
{
	s32 path[3] = { 0, 3, 0 };
	return FindJointByPath(mJObj, path, 3);
}

/**
 * @note Address: 0x8012F5D4
 * @note Size: 0x54
 */
CVec XObjActorDeca::GetHeadWorldPosition()
{
	s32 path[3] = { 0, 3, 0 };
	return GetEventJointWorldPosition(this, 3, path);
}

/**
 * @note Address: 0x8012F628
 * @note Size: 0x318
 */
Vec XObjActorDeca::GetCameraTarget()
{
	if (_C90.mFlags & 4) {
		CJObj jobj;
		s32 path[2] = { 0, 3 };
		jobj.Attach(FindJointByPath(mJObj, path, 2));
		Vec pos;
		pos = GetJointPosition(jobj.mJObj);
		Vec target;
		target = pos;
		target.y -= 0.5f;
		return target;
	}
	return XObjActor::GetCameraTarget();
}

/**
 * @note Address: 0x8012F940
 * @note Size: 0x8B8
 */
// NONMATCHING: regalloc
s32 XObjActorDeca::HandleEventCommand(s32 count, s32* command)
{
	XObjChr* player = gGame.GetPlayer();
	CJObj jobj;
	CVec unused; // never read, the dol still constructs it
	HandleEventCommand__9XObjActorFlPl(this, count, command);
	switch (command[0]) {
	case 1:
		switch (command[1]) {
		case 0:
			ResetActorLook(this, 0.08726646f);
			break;
		case 1:
			SetActorLookTarget(this, &gGame.GetPlayer()->mMoveTranslate, 0.08726646f);
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
		mCmd[2] = command[2];
		mCmd[3] = command[3];
		mCmd[4] = command[4];
		mCmd[5] = command[5];
		mCmd[6] = command[6];
		mCmd[7] = command[7];
		mCmd[8] = command[8];
		mCmd[9] = command[9];
		if (mCmd[1] == 0xD0) {
			mCmd[19] = (s32)FindObj(mCmd[2]);
		}
		switch (mCmd[0]) {
		case 2:
			switch (mCmd[1]) {
			case 0:
				_C90.mFlags |= 1;
				break;
			case 1:
				_C90.mFlags &= ~1;
				break;
			}
			mCmd[0] = 0;
			break;
		case 3:
			PlayGameSound(&gGame.mSound, 0x50014, 0xFF);
			switch (mCmd[1]) {
			case 0:
				_C90.mFlags |= 2;
				_CA4 = PI;
				break;
			case 1:
				_C90.mFlags |= 2;
				_CA4 = 0.0f;
				break;
			}
			mCmd[0] = 0;
			break;
		case 4:
			_A4C   = mCmd[1];
			mState = &XObjActorDeca::StatePickUp;
			ResetActorMovementStep(this);
			mCmd[0] = 0;
			break;
		case 5:
			_A4C = -1;
			if (_A50 != NULL) {
				_A50->Release();
			}
			mCmd[0] = 0;
			break;
		case 6:
			if (mCmd[1] == 1) {
				InfoScrn_ApplyScanlineHudCommand(&gGame.mInfoScrn, 1, 1);
			} else {
				InfoScrn_ApplyScanlineHudCommand(&gGame.mInfoScrn, 0, 1);
			}
			mCmd[0] = 0;
			break;
		case 7:
			switch (mCmd[1]) {
			case 1:
				_998.mFlags |= 0x100;
				player->_980.mFlags = 0;
				player->_980.mFlags |= 0x40000000;
				player->mPad.mReset = FALSE;
				break;
			default: {
				_998.mFlags &= ~0x100;
				s32 path[5] = { 0, 6, 0, 0, 1 };
				jobj.Attach(FindJointByPath(mJObj, path, 5));
				Vec position;
				position = GetJointPosition(jobj.mJObj);
				CVec target(position);
				target.y = 0.0f;
				player->SetPos(target, 0);
				player->_980.mFlags = 0;
				player->_980.mFlags |= 0x20000000;
				player->_980.mFlags |= 7;
				player->mPad.mReset = TRUE;
				HSD_JObjClearMtxFlags(player->mJObj);
				break;
			}
			}
			mCmd[0] = 0;
			break;
		case 8:
			switch (mCmd[1]) {
			case 1:
				_C90.mFlags |= 4;
				break;
			default:
				_C90.mFlags &= ~4;
				break;
			}
			mCmd[0] = 0;
			break;
		case 9:
			if (_A50 != NULL) {
				_A50->SetAnim(mCmd[1], FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			}
			mCmd[0] = 0;
			break;
		}
		break;
	}
	return 0;
}

/**
 * @note Address: 0x801301F8
 * @note Size: 0x6EC
 */
// picks up the object _A4C names, then holds it
// NONMATCHING: regalloc
void XObjActorDeca::StatePickUp(u32 frames)
{
	switch (_990) {
	case 0: {
		XObjActorTao* tao = ((XObjActorTao*)gGame.mXObjs.mPools[14].mObjects);
		if ((tao->mStatus.mFlags & 1) && !tao->IsFree()) {
			return;
		}
		bool busy = false;
		for (s32 i = 0; i < 15; i++) {
			XObjActorToy* toy = (XObjActorToy*)gGame.mXObjs.mPools[10].Get(i);
			if (toy->GetKind() == 0x159) {
				if (!(toy->mStatus.mFlags & 1)) {
					break;
				}
				if (!toy->IsFree()) {
					busy = true;
					break;
				}
			}
		}
		if (busy) {
			return;
		}
		XObjActorPapa* papa = ((XObjActorPapa*)gGame.mXObjs.mPools[13].mObjects);
		if ((papa->mStatus.mFlags & 1) && !papa->IsFree()) {
			return;
		}
		if (_A50 != NULL) {
			_A50->Release();
		}
		if (_A4C >= 0) {
			_A50 = ReadObj(_A4C);
		} else {
			_A50 = NULL;
		}
		_998.mFlags |= 0x2000;
		_990++;
	}
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
			s32 path[2] = { 0, 5 };
			jobj.Attach(FindJointByPath(mJObj, path, 2));
			_A50->SetAnim(0, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			Mtx m;
			Mtx transMtx;
			Mtx hand;
			PSMTXCopy(HSD_JObjGetMtxPtr(jobj.mJObj), m);
			PSMTXTrans(transMtx, 0.0f, -3.0f, -4.0f);
			PSMTXConcat(m, transMtx, hand);
			HSD_JObj* joint = _A50->mJObj;
			MtxPtr mp       = hand;
			HSD_JObjCopyMtx(joint, mp);

			_A50->mStatus.mFlags &= ~0x0200000000000000ULL;
			if (mStatus.mFlags & 0x10000) {
				_A50->mStatus.mFlags |= 0x10000ULL;
			} else {
				_A50->mStatus.mFlags &= ~0x10000ULL;
			}
			_A50->mStatus.mFlags &= ~0x0200000000000000ULL;
		}
		_998.mFlags &= ~0x2000;
		mState = &XObjActorDeca::StateHold;
		ResetActorMovementStep(this);
		break;
	}
}

/**
 * @note Address: 0x801308E4
 * @note Size: 0x434
 */
// keeps what it carries in its hand
void XObjActorDeca::StateHold(u32 frames)
{
	if (_A4C >= 0 && _A50 != NULL) {
		_A50->SetScale(1.0f, 1.0f, 1.0f, 0);
		CJObj jobj;
		s32 path[2] = { 0, 5 };
		jobj.Attach(FindJointByPath(mJObj, path, 2));
		Mtx m;
		Mtx transMtx;
		Mtx hand;
		PSMTXCopy(jobj.GetMtx(), m);
		PSMTXTrans(transMtx, 0.0f, -3.0f, -4.0f);
		PSMTXConcat(m, transMtx, hand);
		HSD_JObj* joint = _A50->mJObj;
		MtxPtr mp       = hand;
		HSD_JObjCopyMtx(joint, mp);
		if (mStatus.mFlags & 0x10000) {
			_A50->mStatus.mFlags |= 0x10000ULL;
		} else {
			_A50->mStatus.mFlags &= ~0x10000ULL;
		}
		_A50->mStatus.mFlags &= ~0x0200000000000000ULL;
	}
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

/**
 * @note Address: 0x80131AB0
 * @note Size: 0x20
 */
void XObjActorDeca::UpdateAnim(u32 frames)
{
	CXObjIntp::UpdateAnim(frames);
}

/**
 * @note Address: 0x80131AD0
 * @note Size: 0x20
 */
void XObjActorDeca::UpdateObject(u32 frames)
{
	XObjActor::UpdateObject(frames);
}

/**
 * @note Address: 0x80131AF0
 * @note Size: 0x1C
 */
void XObjActorDeca::ProcessPendingEvents(u32 frames)
{
	if (_998.Check(2)) {
		_998.mFlags &= ~2;
	}
}

/**
 * @note Address: 0x80131B0C
 * @note Size: 0xDC
 */
void XObjActorDeca::ResetActorState()
{
	_998.mFlags = 0;
	_C90.mFlags = 0;
	_C98.mFlags = 0;
	_980        = 0.0f;
	_984        = 0.0f;
	_988        = 0.0f;
	_C84        = _988;
	_C88        = 0.0f;
	_C8C        = 0;
	_CA0        = PI;
	_CA4        = PI;
	mState      = &XObjActorDeca::StateHold;
	ResetActorMovementStep(this);
	for (u32 i = 0; i < 20; i++) {
		mCmd[i] = 0;
	}
}
