#include "Dolphin/mtx.h"
#include "game/ccard.h"
#include "game/memcard.h"

/**
 * @note Address: 0x80087854
 * @note Size: 0x1C8
 */
extern "C" BOOL OpenOrCreateSaveFile(Memcard* card, char* name, BOOL create)
{
	if (create) {
		card->mState = 7;
		card->mCard.Create(name, 0xC000);
		card->LogResult();
		if (card->mCard.mResult != CARD_RESULT_READY) {
			switch (card->mCard.mResult) {
			case CARD_RESULT_EXIST:
				card->mState = 8;
				card->mCard.Open(name);
				card->LogResult();
				if (card->mCard.mResult != CARD_RESULT_READY) {
					card->mCard.Unmount();
					return TRUE;
				}
				break;
			default:
				card->mCard.Unmount();
				return TRUE;
			}
		}
	} else {
		card->mState = 8;
		card->mCard.Open(name);
		card->LogResult();
		if (card->mCard.mResult != CARD_RESULT_READY) {
			card->mCard.Unmount();
			return TRUE;
		}
	}

	card->mState = 10;
	card->mCard.GetStatus(&card->mStat);
	card->LogResult();
	if (card->mCard.mResult != CARD_RESULT_READY) {
		card->mCard.Close();
		card->mCard.Unmount();
		return TRUE;
	}
	if (!create && card->mStat.length != 0xC000) {
		card->mCard.mResult = -128;
		card->mCard.Close();
		card->mCard.Unmount();
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x80087A1C
 * @note Size: 0x24
 */
extern "C" s32 ProbeMemoryCardCapacity(Memcard* card, s32 channel, s32* memorySize, s32* sectorSize)
{
	return card->mCard.Probe(channel, memorySize, sectorSize);
}

/**
 * @note Address: 0x80087A40
 * @note Size: 0xC
 */
extern "C" s32 GetSaveFileStage(Memcard* card)
{
	return card->mFile.mData.mHeader.mStage;
}
