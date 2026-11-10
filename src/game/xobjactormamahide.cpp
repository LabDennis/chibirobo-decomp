#include "game/xobjactor.h"
#include "game/xchr.h"
#include "game/cscrn.h"
extern "C" {
#include <sysdolphin/random.h>
}

void XObjActorMama::ProcessPendingEvents(u32)
{
	CJObj jobj;
	XObjChr* player = gGame.GetPlayer();
	CVec pos;
	if (_998.Check(0x400)) {
		_998.mFlags &= ~0x400;
		mHeadPitch = 0.0f;
	}
	if (_998.Check(2)) {
		_998.mFlags &= ~2;
		PlayGameSound(&gGame.mSound, HSD_Randi(10) + 0x9E, 0xFF);
	}
	switch (mCmd[0]) {
	case 2:
		Drop();
		switch (mCmd[1]) {
		case 0x40:
			for (s32 i = 0; i < 64; i++) {
				XObj* obj = gGame.mXObjs.mPools[8].Get(i);
				if (mCmd[1] == obj->GetKind()) {
					mHeldObjects[0] = obj;
					obj->mStatus.mFlags |= 0x10000ULL;
					if (mCmd[1] == 0x40)
						_C8C.mFlags |= 1;
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
						_C8C.mFlags |= 2;
						break;
					case 0x7D:
						_C8C.mFlags |= 4;
						break;
					case 0x196:
						_C8C.mFlags |= 8;
						break;
					case 0x45B:
						_C8C.mFlags |= 0x20;
						break;
					}
				}
			}
			break;
		case 0x1A1:
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
			_C8C.mFlags |= 0x10;
			break;
		default:
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
	case 3:
		switch (mCmd[1]) {
		case 1: {
			_998.mFlags |= 0x100;
			player->_980.mFlags = 0;
			player->_980.mFlags |= 0x40000000;
			player->mPad.mReset = FALSE;
			player->SetAnim(1, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);

			break;
		}
		default: {
			_998.mFlags &= ~0x100;
			s32 path[6] = { 0, 4, 2, 0, 0, 0 };
			jobj.Attach(FindJointByPath(mJObj, path, 6));
			pos   = CVec(jobj.GetPos());
			pos.y = mMoveTranslate.y;
			player->SetPos(pos, 0);
			player->_980.mFlags = 0;
			player->_980.mFlags |= 0x20000000;
			player->_980.mFlags |= 7;
			player->mPad.mReset = TRUE;
			break;
		}
		}
		mCmd[0] = 0;
		break;
	case 4:
		StartScreenShake(&gGame.mQuake, mCmd[1], mCmd[2], mCmd[3], 0.0f, 0.0f, 0.0f, mCmd[4] / 100.0f);
		mCmd[0] = 0;
		break;
	case 5: {
		void* joint = GetJointAnimation(mCmd[1]);
		void* mat   = GetMaterialAnimation(mCmd[2]);
		void* shape = GetShapeAnimation(mCmd[1]);
		SetAnim(0, TRUE, joint, mat, shape, 0.0f, 1.0f, 1.0f, 10.0f);
		mStatus.mFlags |= 0x0008000000000000ULL;
		mCmd[0] = 0;
		break;
	}
	case 6: {
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
