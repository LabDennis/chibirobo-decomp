#include "game/xobjflags.h"

/**
 * @note Address: 0x800E13C4
 * @note Size: 0x7C
 */
extern "C" s32 HandleEventCommand__8XObjCoinFlPl(XObjCoin* coin, s32 count, s32* args)
{
	if (count < 1)
		return 0;
	coin->mParam0 = args[0];
	if (count < 2)
		coin->mParam1 = 0;
	else
		coin->mParam1 = args[1];
	// the other eight go over whatever the count
	coin->mParams[0] = args[2];
	coin->mParams[1] = args[3];
	coin->mParams[2] = args[4];
	coin->mParams[3] = args[5];
	coin->mParams[4] = args[6];
	coin->mParams[5] = args[7];
	coin->mParams[6] = args[8];
	coin->mParams[7] = args[9];
	return 0;
}
