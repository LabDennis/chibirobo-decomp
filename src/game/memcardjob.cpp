#include "Dolphin/mtx.h"
#include "game/ccard.h"
#include "game/memcard.h"
#include "game/cthread.h"
#include <string.h>

extern "C" BOOL CARDSetFastMode(BOOL enable);

/**
 * @note Address: 0x8008A1C0
 * @note Size: 0x34
 */
extern "C" void* WriteMemoryCardSaveThread(void* arg)
{
	MemcardJob* job = (MemcardJob*)arg;
	WriteMemoryCardSaveFile(job->mCard, job->mChannel, job->mFile);
	return NULL;
}

/**
 * @note Address: 0x8008A1F4
 * @note Size: 0xAC
 */
extern "C" BOOL StartMemoryCardSaveWrite(Memcard* card, s32 channel, s32 file, BOOL log)
{
	if (card->mBusy)
		return FALSE;
	card->mLog          = log;
	card->mJob.mCard    = card;
	card->mJob.mChannel = channel;
	card->mJob.mFile    = file;
	gMemoryCardThread.Start(WriteMemoryCardSaveThread, &card->mJob, gMemoryCardThreadStack, sizeof(gMemoryCardThreadStack));
	card->mBusy = TRUE;
	return TRUE;
}

/**
 * @note Address: 0x8008A2A0
 * @note Size: 0x8
 */
extern "C" BOOL fn_8008A2A0(Memcard* card)
{
	return TRUE;
}

/**
 * @note Address: 0x8008A2A8
 * @note Size: 0x6C
 */
extern "C" void InitMemoryCardManager(Memcard* card)
{
	card->mCard.Init();
	CARDSetFastMode(TRUE);
	memset(card->mBuffer, 0, sizeof(card->mBuffer));
	card->mState = 0;
	card->_169F8 = 0;
	card->mBusy  = FALSE;
	card->mFound = FALSE;
	card->mLog   = FALSE;
}
