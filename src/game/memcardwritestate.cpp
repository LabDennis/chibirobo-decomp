#include "Dolphin/mtx.h"
#include "game/ccard.h"
#include "game/memcard.h"
#include <stdio.h>

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
	card->mState   = Memcard::Writing;
	CARDStat* stat = &card->mStat;
	for (s32 copy = 0, offset = 0; copy < 2; ++copy, offset += sizeof(SaveFile)) {
		card->mCard.Write(card->mBuffer, sizeof(SaveFile), offset);
		card->LogResult();
		if (card->mCard.mResult != CARD_RESULT_READY) {
			card->mCard.Close();
			card->mCard.Unmount();
			return TRUE;
		}
		if (copy == 1) {
			CARDStat st;
			card->mCard.GetStatus(&st);
			if (!HasSaveFileLayout(st)) {
				card->mState = Memcard::WritingStatus;
				card->mCard.SetStatus(stat);
				card->LogResult();
				if (card->mCard.mResult != CARD_RESULT_READY) {
					card->mCard.Close();
					card->mCard.Unmount();
					return TRUE;
				}
			}
		}
	}
	if (card->Close())
		return TRUE;
	if (card->Unmount())
		return TRUE;
	card->mState = Memcard::Idle;
	OSCalendarTime calendar;
	OSTicksToCalendarTime(OSSecondsToTicks((OSTime)card->mStat.time), &calendar);
	return FALSE;
}
