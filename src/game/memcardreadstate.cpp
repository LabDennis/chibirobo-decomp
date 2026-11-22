#include "Dolphin/mtx.h"
#include "game/ccard.h"
#include "game/memcard.h"
#include <stdio.h>

// NONMATCHING: register allocation around the loop setup: retail's copy of the card
// base lands one register later, so every callee-saved register shifts by one slot.
// Writing out &card->mStat at both uses instead of the local makes it worse (95.31).
/**
 * @note Address: 0x8008933C
 * @note Size: 0x524
 */
extern "C" BOOL ReadMemoryCardPreviews(Memcard* card, s32 channel)
{
	if (card->Probe(channel))
		return TRUE;
	if (card->Mount(channel, TRUE))
		return TRUE;
	CARDStat* stat = &card->mStat;
	for (s32 slot = 0; slot < 10; ++slot) {
		sprintf(gSaveReadFileName, gSaveFileNameFormat, slot);
		if (card->OpenStatus(gSaveReadFileName, stat))
			continue;
		if (!ReadValidatedSaveFileCopy(card, gSavePreviewFileBuffer)) {
			const SaveHeader& header                  = gSavePreviewFileBuffer[0].mData.mHeader;
			card->mAllSaves[slot].mHeader.mSignature  = header.mSignature;
			card->mAllSaves[slot].mHeader.mVersion    = header.mVersion;
			card->mAllSaves[slot].mHeader.mStage      = header.mStage;
			card->mAllSaves[slot].mHeader.mTimeMode   = header.mTimeMode;
			card->mAllSaves[slot].mHeader.mTime       = header.mTime;
			card->mAllSaves[slot].mHeader.mDuration   = header.mDuration;
			card->mAllSaves[slot].mHeader.mExp        = header.mExp;
			card->mAllSaves[slot].mHeader.mLevel      = header.mLevel;
			card->mAllSaves[slot].mHeader.mRank       = header.mRank;
			card->mAllSaves[slot].mHeader.mBattery    = header.mBattery;
			card->mAllSaves[slot].mHeader.mMaxBattery = header.mMaxBattery;
			card->mAllSaves[slot].mHeader.mMoney      = header.mMoney;
			card->mAllSaves[slot].mHeader.mScrap      = header.mScrap;
			card->mAllSaves[slot].mHeader.mActive     = header.mActive;
			card->mAllSaves[slot].mHeader.mFrames     = header.mFrames;
			card->mState                              = 9;
			card->mCard.CloseEx();
			card->LogResult();
			if (card->mCard.mResult != CARD_RESULT_READY)
				continue;
			card->mAllSaves[slot].mStatus = 0;
		} else {
			card->mAllSaves[slot].mStatus = -1;
		}
		card->mAllSaves[slot].mState = 1;
		card->mAllSaves[slot].mStat  = *stat;
	}
	if (card->GetFreeBlocks())
		return TRUE;
	card->mState = 3;
	card->mCard.UnmountEx();
	card->LogResult();
	if (card->mCard.mResult != CARD_RESULT_READY)
		return TRUE;
	card->mState = 0;
	return FALSE;
}
