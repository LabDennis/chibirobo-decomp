#include "game/xobjactor.h"

static inline u32 MaskedFlags(const CStatus& status, u32 mask)
{
	return status.mFlags & mask;
}

/**
 * @note Address: 0x80128230
 * @note Size: 0x38C
 */
// lets go of what she carries
void XObjActorMama::Drop()
{
	s32 kind = -1;
	if (MaskedFlags(_C8C, 4)) {
		_C8C.mFlags &= ~4;
		kind = 0x7D;
	} else if (MaskedFlags(_C8C, 1)) {
		_C8C.mFlags &= ~1;
		kind = 0x40;
	} else if (MaskedFlags(_C8C, 2)) {
		_C8C.mFlags &= ~2;
		kind = 0x5B;
	} else if (MaskedFlags(_C8C, 8)) {
		_C8C.mFlags &= ~8;
		kind = 0x196;
	} else if (MaskedFlags(_C8C, 0x10)) {
		_C8C.mFlags &= ~0x10;
		kind = 0x1A1;
	} else if (MaskedFlags(_C8C, 0x20)) {
		_C8C.mFlags &= ~0x20;
		kind = 0x45B;
	}
	if (kind < 0) {
		return;
	}
	switch (kind) {
	case 0x40:
		for (s32 i = 0; i < 64; i++) {
			XObj* obj = gGame.mXObjs.mPools[8].Get(i);
			if (kind == obj->GetKind()) {
				obj->mStatus.mFlags &= ~0x10000ULL;
			}
		}
		break;
	case 0x5B:
	case 0x7D:
	case 0x196:
	case 0x45B:
		for (s32 i = 0; i < 288; i++) {
			XObj* obj = gGame.mXObjs.mPools[22].Get(i);
			if (kind == obj->GetKind()) {
				obj->mStatus.mFlags &= ~0x10000ULL;
			}
		}
		break;
	case 0x1A1:
		for (s32 i = 0; i < 288; i++) {
			XObj* obj = gGame.mXObjs.mPools[22].Get(i);
			if (obj->GetKind() == 0x1A0) {
				obj->mStatus.mFlags &= ~0x10000ULL;
			} else if (kind == obj->GetKind()) {
				obj->mStatus.mFlags &= ~0x10000ULL;
			}
		}
		break;
	}
}
