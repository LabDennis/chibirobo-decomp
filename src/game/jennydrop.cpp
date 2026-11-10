#include "game/xobjactor.h"

/**
 * @note Address: 0x8012527C
 * @note Size: 0x298
 */
// lets go of what she carries
void XObjActorJenny::Drop()
{
	s32 kind = -1;
	if (_C98.Check(1)) {
		_C98.mFlags &= ~1;
		kind = 0x3D;
	} else if (_C98.Check(2)) {
		_C98.mFlags &= ~2;
		kind = 0x3E;
	} else if (_C98.Check(4)) {
		_C98.mFlags &= ~4;
		kind = 0x3F;
	} else if (_C98.Check(8)) {
		_C98.mFlags &= ~8;
		kind = 0x3B;
	} else if (_C98.Check(0x10)) {
		_C98.mFlags &= ~0x10;
		kind = 0x401;
	}
	if (kind < 0) {
		return;
	}
	switch (kind) {
	case 0x3B:
	case 0x3D:
	case 0x3E:
	case 0x3F:
		for (s32 i = 0; i < 64; i++) {
			XObj* obj = gGame.mXObjs.mPools[8].Get(i);
			if (kind == obj->GetKind()) {
				obj->mStatus.mFlags &= ~0x10000ULL;
			}
		}
		break;
	case 0x401:
		for (s32 i = 0; i < 288; i++) {
			XObj* obj = gGame.mXObjs.mPools[22].Get(i);
			if (kind == obj->GetKind()) {
				obj->mStatus.mFlags &= ~0x10000ULL;
			}
		}
		break;
	}
}
