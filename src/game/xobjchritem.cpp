#include "game/xobjchritem.h"

/**
 * @note Address: 0x800B8EC4
 * @note Size: 0x270
 */
// loads the item he's asked to hold, true once it's ready
extern "C" BOOL LoadRequestedHeldItem(XObjChr* chr)
{
	PlayerSuits& suits = chr->mSuits;
	if (suits.mLoadStep == 0)
		suits.mLoadStep = 1;
	if (suits.mCurrent != suits.mRequested) {
		suits.mChangeStep = TRUE;
		suits.mCurrent    = suits.mRequested;
	}
	if (LoadPlayerSuitItem(chr)) {
		suits.mChangeStep = FALSE;
		ApplyPlayerSuit(chr);
		return TRUE;
	}
	return FALSE;
}
