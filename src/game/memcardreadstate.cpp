#include "Dolphin/mtx.h"
#include "game/ccard.h"
#include "game/memcard.h"
#include <stdio.h>

char gSaveReadFileName[0xC8];
// CARDRead's DMA target, so the buffer itself is 32-byte aligned
SaveFile gSavePreviewFileBuffer[2] ATTRIBUTE_ALIGN(32);

// NONMATCHING: retail keeps the card high base and derives the stat pointer from it,
// ours materializes the stat pointer first; it also copies CARDStat's pad byte at 0x2F
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
			card->mAllSaves[slot].mHeader._34         = header._34;
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
