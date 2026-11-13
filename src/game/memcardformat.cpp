#include "Dolphin/mtx.h"
#include "game/ccard.h"
#include "game/memcard.h"
#include "game/cthread.h"
#include <stdio.h>

/**
 * @note Address: 0x80088848
 * @note Size: 0x58
 */
extern "C" void PollMemoryCardWorker(Memcard* card)
{
	if (card->mBusy && OSIsThreadTerminated(&gMemoryCardThread.mThread)) // the busy flag clears when the worker exits
		card->mBusy = FALSE;
}

/**
 * @note Address: 0x800888A0
 * @note Size: 0x278
 */
extern "C" BOOL DeleteMemoryCardSaveFile(Memcard* card, s32 channel, s32 file)
{
	if (card->Probe(channel))
		return TRUE;
	if (card->Mount(channel, TRUE))
		return TRUE;
	char name[128];
	sprintf(name, gSaveFileNameFormat, file);
	if (card->Delete(name))
		return TRUE;
	card->mState = 3;
	card->mCard.UnmountEx();
	card->LogResult();
	if (card->mCard.mResult != CARD_RESULT_READY)
		return TRUE;
	card->mState = 0;
	return FALSE;
}

/**
 * @note Address: 0x80088B18
 * @note Size: 0x34
 */
extern "C" void* DeleteMemoryCardSaveThread(void* arg)
{
	MemcardJob* job = (MemcardJob*)arg;
	DeleteMemoryCardSaveFile(job->mCard, job->mChannel, job->mFile);
	return NULL;
}

/**
 * @note Address: 0x80088B4C
 * @note Size: 0xAC
 */
extern "C" BOOL StartMemoryCardSaveDeletion(Memcard* card, s32 channel, s32 file, BOOL log)
{
	if (card->mBusy) // one job at a time
		return FALSE;
	card->mLog          = log;
	card->mJob.mCard    = card;
	card->mJob.mChannel = channel;
	card->mJob.mFile    = file;
	gMemoryCardThread.Start(DeleteMemoryCardSaveThread, &card->mJob, gMemoryCardThreadStack, sizeof(gMemoryCardThreadStack));
	card->mBusy = TRUE;
	return TRUE;
}

/**
 * @note Address: 0x80088BF8
 * @note Size: 0x250
 */
extern "C" BOOL CheckAndFormatMemoryCard(Memcard* card, s32 channel)
{
	if (card->Probe(channel))
		return TRUE;
	if (card->Mount(channel, FALSE)) {
		switch (card->mCard.mResult) {
		case CARD_RESULT_BROKEN:
		case CARD_RESULT_ENCODING: // a broken or wrong-encoding card gets reformatted
			if (card->Format())
				return TRUE;
			break;
		default:
			card->mCard.Unmount();
			return TRUE;
		}
	}
	card->mState = 3; // 3: unmounting
	card->mCard.UnmountEx();
	card->LogResult();
	if (card->mCard.mResult != CARD_RESULT_READY)
		return TRUE;
	card->mState = 0; // back to idle
	return FALSE;
}

/**
 * @note Address: 0x80088E48
 * @note Size: 0x30
 */
extern "C" void* CheckAndFormatMemoryCardThread(void* arg)
{
	MemcardJob* job = (MemcardJob*)arg;
	CheckAndFormatMemoryCard(job->mCard, job->mChannel);
	return NULL;
}

/**
 * @note Address: 0x80088E78
 * @note Size: 0xA8
 */
extern "C" BOOL StartMemoryCardFormatCheck(Memcard* card, s32 channel, BOOL log)
{
	if (card->mBusy)
		return FALSE;
	card->mLog          = log;
	card->mJob.mCard    = card;
	card->mJob.mChannel = channel;
	gMemoryCardThread.Start(CheckAndFormatMemoryCardThread, &card->mJob, gMemoryCardThreadStack, sizeof(gMemoryCardThreadStack));
	card->mBusy = TRUE;
	return TRUE;
}
