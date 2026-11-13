#include "Dolphin/mtx.h"
#include "game/ccard.h"
#include "game/memcard.h"
#include <stdio.h>
#include <string.h>

extern "C" {
extern char gSaveReadFileName[];
extern SaveFile gSavePreviewFileBuffer[2];
}

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
			record.mHeader.mActive     = header.mActive;
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
		memcpy(&record.mStat, &card->mStat, sizeof(record.mStat));
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
