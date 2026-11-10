#include "game/xobjactor.h"

static inline u32 GetTaoDropFlags(const CStatus& status, u32 mask)
{
	return status.mFlags & mask;
}

/**
 * @note Address: 0x8012EFF4
 * @note Size: 0x320
 */
// lets go of what it carries
// The two scans for 0x159 and 0xF8 advance through the pools without changing any objects.
void XObjActorTao::Drop()
{
	s32 kind = -1;
	if (GetTaoDropFlags(_C8C, 2)) {
		_C8C.mFlags &= ~2;
		kind = 0x102;
	} else if (GetTaoDropFlags(_C8C, 1)) {
		_C8C.mFlags &= ~1;
		kind = 0x159;
	} else if (GetTaoDropFlags(_C8C, 4)) {
		_C8C.mFlags &= ~4;
		kind = 0xF8;
	} else if (GetTaoDropFlags(_C8C, 8)) {
		_C8C.mFlags &= ~8;
		kind = 0x2A3;
	} else if (GetTaoDropFlags(_C8C, 0x20)) {
		_C8C.mFlags &= ~0x20;
		kind = 0x46F;
	}
	if (kind < 0) {
		return;
	}
	switch (kind) {
	case 0x102:
		for (s32 i = 0; i < 64; i++) {
			XObj* obj = gGame.mXObjs.mPools[8].Get(i);
			if (kind == obj->GetKind()) {
				obj->mStatus.mFlags &= ~0x10000ULL;
			}
		}
		break;
	case 0x159:
		for (s32 i = 0; i < 15; ++i) {
			XObj* obj = gGame.mXObjs.mPools[10].Get(i);
			(void)(kind == obj->GetKind());
		}
		break;
	case 0xF8:
		for (s32 i = 0; i < 21; ++i) {
			XObj* obj = gGame.mXObjs.mPools[16].Get(i);
			(void)(kind == obj->GetKind());
		}
		break;
	case 0x2A3:
	case 0x46F:
		for (s32 i = 0; i < 288; i++) {
			XObj* obj = gGame.mXObjs.mPools[22].Get(i);
			if (kind == obj->GetKind()) {
				obj->mStatus.mFlags &= ~0x10000ULL;
			}
		}
		break;
	}
}

/**
 * @note Address: 0x8012F314
 * @note Size: 0x50
 */
void XObjActorTao::ProcessPendingEvents(u32)
{
	if (_998.Check(2)) {
		_998.mFlags &= ~2;
		PlayGameSound(&gGame.mSound, 0x15D, 0xFF);
	}
}
