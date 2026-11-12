#include "Dolphin/mtx.h"
#include "game/ccard.h"
#include "game/memcard.h"
#include <stdio.h>

extern "C" void CaptureGameSaveData(Memcard*);
extern "C" void BuildSaveBanner(Memcard*, s32);

/**
 * @note Address: 0x80089D3C
 * @note Size: 0x484
 */
extern "C" BOOL WriteMemoryCardSaveFile(Memcard* card, s32 channel, s32 slot)
{
	if (card->Probe(channel))
		return TRUE;
	if (card->Mount(channel, TRUE))
		return TRUE;
	char name[128];
	sprintf(name, gSaveFileNameFormat, slot);
	if (OpenOrCreateSaveFile(card, name, TRUE))
		return TRUE;
	BuildSaveBanner(card, slot);
	CaptureGameSaveData(card);
	card->mState = 13;
	BOOL failed  = FALSE;
	for (s32 copy = 0, offset = 0; copy < 2 && !failed; ++copy, offset += sizeof(SaveFile)) {
		card->mCard.Write(card->mBuffer, sizeof(SaveFile), offset);
		card->LogResult();
		if (card->mCard.mResult != CARD_RESULT_READY) {
			card->mCard.Close();
			card->mCard.Unmount();
			failed = TRUE;
		} else if (copy == 1) {
			CARDStat stat;
			card->mCard.GetStatus(&stat);
			if (!HasSaveFileLayout(stat)) {
				card->mState = 11;
				card->mCard.SetStatus(&card->mStat);
				card->LogResult();
				if (card->mCard.mResult != CARD_RESULT_READY) {
					card->mCard.Close();
					card->mCard.Unmount();
					failed = TRUE;
				}
			}
		}
	}
	if (failed)
		return TRUE;
	if (card->Close())
		return TRUE;
	card->mState = 3;
	card->mCard.UnmountEx();
	card->LogResult();
	if (card->mCard.mResult != CARD_RESULT_READY)
		return TRUE;
	card->mState = 0;
	OSCalendarTime calendar;
	OSTicksToCalendarTime(OSSecondsToTicks((OSTime)card->mStat.time), &calendar);
	return FALSE;
}
