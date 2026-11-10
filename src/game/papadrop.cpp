#include "game/xobjactor.h"

// lets go of what he carries
void XObjActorPapa::Drop()
{
	s32 kind = -1;
	if (_C8C.Check(1)) {
		_C8C.mFlags &= ~1;
		kind = 0x1F4;
	} else if (_C8C.Check(2)) {
		_C8C.mFlags &= ~2;
		kind = 0x360;
	} else if (_C8C.Check(4)) {
		_C8C.mFlags &= ~4;
		kind = 0x1D3;
	} else if (_C8C.Check(8)) {
		_C8C.mFlags &= ~8;
		kind = 0x375;
	} else if (_C8C.Check(0x10)) {
		_C8C.mFlags &= ~0x10;
		kind = 0x3FA;
	} else if (_C8C.Check(0x20)) {
		_C8C.mFlags &= ~0x20;
		kind = 0x401;
	}
	if (kind < 0) {
		return;
	}
	switch (kind) {
	case 0x1D3:
		// Retail walks these two pools without changing the matching objects.
		for (s32 i = 0; i < 15; ++i) {
			XObj* obj = gGame.mXObjs.mPools[10].Get(i);
			(void)(kind == obj->GetKind());
		}
		break;
	case 0x1F4:
	case 0x360:
	case 0x3FA:
	case 0x401:
		for (s32 i = 0; i < 288; i++) {
			XObj* obj = gGame.mXObjs.mPools[22].Get(i);
			if (kind == obj->GetKind()) {
				obj->mStatus.mFlags &= ~0x10000ULL;
			}
		}
		break;
	case 0x375:
		for (s32 i = 0; i < 64; ++i) {
			XObj* obj = gGame.mXObjs.mPools[8].Get(i);
			(void)(kind == obj->GetKind());
		}
		break;
	}
}
