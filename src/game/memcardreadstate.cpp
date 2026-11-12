#include "Dolphin/mtx.h"
#include "game/ccard.h"
#include "game/memcard.h"
#include <stdio.h>

char gSaveReadFileName[0xC8];
// CARDRead's DMA target, so the buffer itself is 32-byte aligned
SaveFile gSavePreviewFileBuffer[2] ATTRIBUTE_ALIGN(32);

/**
 * @note Address: 0x8008933C
 * @note Size: 0x524
 */
extern "C" BOOL ReadMemoryCardPreviews(Memcard* card, s32 channel)
{
	if (card->Probe(channel) || card->Mount(channel, TRUE))
		return TRUE;
	for (s32 slot = 0; slot < 10; ++slot) {
		sprintf(gSaveReadFileName, gSaveFileNameFormat, slot);
		if (card->OpenStatus(gSaveReadFileName, &card->mStat))
			continue;
		SavePreviewRecord& record = card->mAllSaves[slot];
		if (!ReadValidatedSaveFileCopy(card, gSavePreviewFileBuffer)) {
			const SaveHeader& header   = gSavePreviewFileBuffer[0].mData.mHeader;
			record.mHeader.mSignature  = header.mSignature;
			record.mHeader.mVersion    = header.mVersion;
			record.mHeader.mStage      = header.mStage;
			record.mHeader.mTimeMode   = header.mTimeMode;
			record.mHeader.mTime       = header.mTime;
			record.mHeader.mDuration   = header.mDuration;
			record.mHeader.mExp        = header.mExp;
			record.mHeader.mLevel      = header.mLevel;
			record.mHeader.mRank       = header.mRank;
			record.mHeader.mBattery    = header.mBattery;
			record.mHeader.mMaxBattery = header.mMaxBattery;
			record.mHeader.mMoney      = header.mMoney;
			record.mHeader.mScrap      = header.mScrap;
			record.mHeader._34         = header._34;
			record.mHeader.mFrames     = header.mFrames;
			card->mState               = 9;
			card->mCard.CloseEx();
			card->LogResult();
			if (card->mCard.mResult != CARD_RESULT_READY)
				continue;
			record.mStatus = 0;
		} else {
			record.mStatus = -1;
		}
		record.mState = 1;
		record.mStat  = card->mStat;
	}
	card->mState = 14;
	card->_16A04 = card->mCard.GetFreeBlocks((s32*)&card->_169FC, &card->_16A00);
	card->LogResult();
	if (card->mCard.mResult != CARD_RESULT_READY) {
		card->mCard.Unmount();
		return TRUE;
	}
	card->mState = 3;
	card->mCard.UnmountEx();
	card->LogResult();
	if (card->mCard.mResult != CARD_RESULT_READY)
		return TRUE;
	card->mState = 0;
	return FALSE;
}
