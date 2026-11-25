#include "Dolphin/mtx.h"
#include "game/ccard.h"
#include "game/memcard.h"
#include <stdio.h>
#include <string.h>

char gSaveReadFileName[0xC8];
// CARDRead's DMA target, so the buffer itself is 32-byte aligned
SaveFile gSavePreviewFileBuffer[2] ATTRIBUTE_ALIGN(32);

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
		SavePreviewRecord& preview = card->mAllSaves[slot];
		if (!card->ReadSavePreview(gSavePreviewFileBuffer)) {
			const SaveHeader& header    = gSavePreviewFileBuffer[0].mData.mHeader;
			preview.mHeader.mSignature  = header.mSignature;
			preview.mHeader.mVersion    = header.mVersion;
			preview.mHeader.mStage      = header.mStage;
			preview.mHeader.mTimeMode   = header.mTimeMode;
			preview.mHeader.mTime       = header.mTime;
			preview.mHeader.mDuration   = header.mDuration;
			preview.mHeader.mExp        = header.mExp;
			preview.mHeader.mLevel      = header.mLevel;
			preview.mHeader.mRank       = header.mRank;
			preview.mHeader.mBattery    = header.mBattery;
			preview.mHeader.mMaxBattery = header.mMaxBattery;
			preview.mHeader.mMoney      = header.mMoney;
			preview.mHeader.mScrap      = header.mScrap;
			preview.mHeader.mActive     = header.mActive;
			preview.mHeader.mFrames     = header.mFrames;
			card->mState                = Memcard::ClosingFile;
			card->mCard.CloseEx();
			card->LogResult();
			if (card->mCard.mResult != CARD_RESULT_READY)
				continue;
			preview.mStatus = 0;
		} else {
			preview.mStatus = -1;
		}
		preview.mState = 1;
		memcpy(&preview.mStat, stat, sizeof(CARDStat));
	}
	if (card->GetFreeBlocks())
		return TRUE;
	if (card->Unmount())
		return TRUE;
	card->mState = Memcard::Idle;
	return FALSE;
}
