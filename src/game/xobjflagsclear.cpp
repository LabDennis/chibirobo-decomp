#include "game/xobjflags.h"

/**
 * @note Address: 0x800E2300
 * @note Size: 0x9C
 */
// todo: weak copy
extern "C" void ResetForNewGame__9XObjFlagsFv(XObjFlags* obj)
{
	u32* flags = obj->mFlags->mFlags;
	for (u32 i = 0; i < ARRAY_SIZE(obj->mFlags->mFlags); ++i) {
		flags[i] = 0;
	}
}
