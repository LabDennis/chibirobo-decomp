#include "game/cgame.h"
#include "game/ccard.h"
#include <stdio.h>

extern "C" char gSaveScanFileName[];

// NONMATCHING: retail derives the stat pointer from the card base it keeps, the build
// materializes it standalone and makes the base a copy
/**
 * @note Address: 0x80088F20
 * @note Size: 0x338
 */
extern "C" BOOL ScanMemoryCardForSaveFiles(Memcard* card, s32 channel)
{
	BOOL found = FALSE;
	if (card->Probe(channel))
		return TRUE;
	if (card->Mount(channel, TRUE))
		return TRUE;
	CARDStat* stat = &card->mStat;
	for (s32 file = 0; file < 10; ++file) {
		sprintf(gSaveScanFileName, gSaveFileNameFormat, file);
		if (!card->OpenStatus(gSaveScanFileName, stat)) {
			card->mState = 9;
			card->mCard.CloseEx();
			card->LogResult();
			if (card->mCard.mResult == CARD_RESULT_READY) {
				found = TRUE;
				if (!(gGame.mStatus.mFlags & 0x4000ULL))
					break;
			}
		}
	}
	card->mState = 3;
	card->mCard.UnmountEx();
	card->LogResult();
	if (card->mCard.mResult != CARD_RESULT_READY)
		return TRUE;
	card->mFound = found;
	card->mState = 0;
	return FALSE;
}
