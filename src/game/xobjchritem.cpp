#include "game/xobjchritem.h"

/**
 * @note Address: 0x800B8EC4
 * @note Size: 0x270
 */
// loads the item he's asked to hold, true once it's ready
extern "C" BOOL LoadRequestedHeldItem(XObjChr* chr)
{
	if (chr->mSuits.mLoadStep == 0)
		chr->mSuits.mLoadStep = 1;
	if (chr->mSuits.mCurrent != chr->mSuits.mRequested) {
		chr->mSuits.mChangeStep = TRUE;
		chr->mSuits.mCurrent    = chr->mSuits.mRequested;
	}
	if (LoadPlayerSuitItem(chr)) {
		chr->mSuits.mChangeStep = FALSE;
		ApplyPlayerSuit(chr);
		return TRUE;
	}
	return FALSE;
}
