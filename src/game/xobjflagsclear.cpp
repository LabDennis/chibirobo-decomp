#include "game/xobjflags.h"

/**
 * @note Address: 0x800E2300
 * @note Size: 0x9C
 */
void XObjFlags::ResetForNewGame()
{
	u32* flags = mFlags->mFlags;
	for (u32 i = 0; i < ARRAY_SIZE(mFlags->mFlags); ++i) {
		flags[i] = 0;
	}
}
