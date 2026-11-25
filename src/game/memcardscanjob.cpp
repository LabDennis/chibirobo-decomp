#include "game/cgame.h"
#include "game/cthread.h"

/**
 * @note Address: 0x80089258
 * @note Size: 0x30
 */
extern "C" void* ScanMemoryCardThread(void* arg)
{
	MemcardJob* job = (MemcardJob*)arg;
	ScanMemoryCardForSaveFiles(job->mCard, job->mChannel);
	return NULL;
}

/**
 * @note Address: 0x80089288
 * @note Size: 0xB4
 */
extern "C" BOOL StartMemoryCardScan(Memcard* card, s32 channel, BOOL log)
{
	if (card->mBusy)
		return FALSE;
	card->mLog          = log;
	card->mFound        = FALSE;
	card->mJob.mCard    = card;
	card->mJob.mChannel = channel;
	gMemoryCardThread.Start(ScanMemoryCardThread, &card->mJob, gMemoryCardThreadStack, sizeof(gMemoryCardThreadStack));
	card->mBusy = TRUE;
	return TRUE;
}
