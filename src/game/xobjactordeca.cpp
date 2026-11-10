#include "game/xobjactor.h"
#include "game/xchr.h"
#include "game/gmath.h"

extern "C" {
extern XObjActorDeca::StateFunc sDecaHoldState;
extern XObjActorDeca::StateFunc sDecaPickupState;
void PSMTXTrans(Mtx mtx, f32 x, f32 y, f32 z);
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
			mState = sDecaPickupState;
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
		mState = sDecaHoldState;
		ResetActorMovementStep(this);
		break;
	}
}
