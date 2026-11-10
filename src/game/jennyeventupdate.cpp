#include "game/xobjactor.h"
#include "game/xchr.h"
#include <stdlib.h>

static inline void HoldPlayerForJenny(XObjChr* player)
{
	player->_980.mFlags = 0;
	player->_980.mFlags |= 0x40000000;
	player->mPad.mReset = FALSE;
	player->SetAnim(1, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
}

static inline void ReleasePlayerAt(XObjChr* player, const Vec& position)
{
	player->SetPos(position, 0);
	player->_980.mFlags = 0;
	player->_980.mFlags |= 0x20000000;
	player->_980.mFlags |= 7;
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
