#include "Dolphin/vec.h"
#include "game/memcard.h"

/**
 * @note Address: 0x80087508
 * @note Size: 0x34C
 */
BOOL Memcard::ReadSaveFile()
{
	mState = Reading;
	for (s32 copy = 0, offset = 0; copy < 2; ++copy, offset += sizeof(SaveFile)) {
		if (ReadSaveCopy(&mFile, offset) != CARD_RESULT_READY)
			return FailSaveRead(TRUE);
		SaveData& save = mFile.mData;
		if (save.HasValidSignature() && save.CalculateChecksum() == mFile.mChecksum && ValidateSaveHeader(&save.mHeader))
			return FALSE;
	}
	mCard.mResult = CARD_RESULT_FATAL_ERROR;
	return FailSaveRead(TRUE);
}
