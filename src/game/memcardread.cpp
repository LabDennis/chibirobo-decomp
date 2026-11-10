#include "Dolphin/mtx.h"
#include "game/ccard.h"
#include "game/memcard.h"
#include "game/cthread.h"
#include <stdio.h>

/**
 * @note Address: 0x80089860
 * @note Size: 0x30
 */
extern "C" void* ReadMemoryCardPreviewsThread(void* arg)
{
	MemcardJob* job = (MemcardJob*)arg;
	ReadMemoryCardPreviews(job->mCard, job->mChannel);
	return NULL;
}

/**
 * @note Address: 0x80089890
 * @note Size: 0xEC
 */
extern "C" BOOL StartMemoryCardPreviewRead(Memcard* card, s32 channel, BOOL log)
{
	if (card->mBusy)
		return FALSE;
	card->mLog = log;
	for (u32 slot = 0; slot < 2; ++slot) {
		for (u32 file = 0; file < 5; ++file) {
			card->mSaves[slot][file].mState = 0;
		}
	}
	card->mJob.mCard    = card;
	card->mJob.mChannel = channel;
	gMemoryCardThread.Start(ReadMemoryCardPreviewsThread, &card->mJob, gMemoryCardThreadStack, sizeof(gMemoryCardThreadStack));
	card->mBusy = TRUE;
	return TRUE;
}

/**
 * @note Address: 0x8008997C
 * @note Size: 0x2E0
 */
extern "C" BOOL ReadMemoryCardSaveFile(Memcard* card, s32 channel, s32 file)
{
	if (card->Probe(channel))
		return TRUE;
	if (card->Mount(channel, TRUE))
		return TRUE;
	char name[128];
	sprintf(name, gSaveFileNameFormat, file);
	if (OpenOrCreateSaveFile(card, name, FALSE))
		return TRUE;
	if (ReadValidatedSaveIntoCardBuffer(card))
		return TRUE;
	if (card->Close())
		return TRUE;
	card->mState = 3;
	card->mCard.UnmountEx();
	card->LogResult();
	if (card->mCard.mResult != CARD_RESULT_READY)
		return TRUE;
	card->mState = 0;
	// retail converts the save time and drops it
	OSCalendarTime calendar;
	OSTicksToCalendarTime(OSSecondsToTicks((OSTime)card->mStat.time), &calendar);
	return FALSE;
}

/**
 * @note Address: 0x80089C5C
 * @note Size: 0x34
 */
extern "C" void* ReadMemoryCardSaveThread(void* arg)
{
	MemcardJob* job = (MemcardJob*)arg;
	ReadMemoryCardSaveFile(job->mCard, job->mChannel, job->mFile);
	return NULL;
}

/**
 * @note Address: 0x80089C90
 * @note Size: 0xAC
 */
extern "C" BOOL StartMemoryCardSaveRead(Memcard* card, s32 channel, s32 file, BOOL log)
{
	if (card->mBusy)
		return FALSE;
	card->mLog          = log;
	card->mJob.mCard    = card;
	card->mJob.mChannel = channel;
	card->mJob.mFile    = file;
	gMemoryCardThread.Start(ReadMemoryCardSaveThread, &card->mJob, gMemoryCardThreadStack, sizeof(gMemoryCardThreadStack));
	card->mBusy = TRUE;
	return TRUE;
}
