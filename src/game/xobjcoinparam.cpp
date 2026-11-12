#include "game/xobjflags.h"

/**
 * @note Address: 0x800E13C4
 * @note Size: 0x7C
 */
s32 XObjCoin::HandleEventCommand(s32 count, s32* args)
{
	if (count < 1)
		return 0;
	mParam0 = args[0];
	if (count < 2)
		mParam1 = 0;
	else
		mParam1 = args[1];
	// the other eight go over whatever the count
	mParams[0] = args[2];
	mParams[1] = args[3];
	mParams[2] = args[4];
	mParams[3] = args[5];
	mParams[4] = args[6];
	mParams[5] = args[7];
	mParams[6] = args[8];
	mParams[7] = args[9];
	return 0;
}
