#include "Dolphin/mtx.h"
#include "game/ccard.h"
#include "game/memcard.h"

static inline BOOL ReadFailed(Memcard* card)
{
	card->mCard.Close();
	return TRUE;
}

/**
 * @note Address: 0x80087190
 * @note Size: 0x378
 */
extern "C" BOOL ReadValidatedSaveFileCopy(Memcard* card, SaveFile* file)
{
	card->mState = 12;
	for (s32 copy = 0; copy < 2; ++copy) {
		card->mCard.Read(file, sizeof(SaveFile), copy * (s32)sizeof(SaveFile));
		card->LogResult();
		if (card->mCard.mResult != CARD_RESULT_READY)
			return ReadFailed(card);
		SaveHeader* save = &file->mData.mHeader;
		if (save->mSignature != SAVE_SIGNATURE)
			card->mCard.mResult = -128;
		card->LogResult();
		if (card->mCard.mResult != CARD_RESULT_READY) {
			if (copy == 1)
				return ReadFailed(card);
			continue;
		}
		u32 checksum   = 0;
		const u8* data = reinterpret_cast<const u8*>(save);
		for (u32 i = 0; i < sizeof(SaveData); ++i)
			checksum += *data++;
		if (checksum != file->mChecksum)
			card->mCard.mResult = -128;
		card->LogResult();
		if (card->mCard.mResult != CARD_RESULT_READY) {
			if (copy == 1)
				return ReadFailed(card);
			continue;
		}
		if (!IsValidSave(save)) {
			if (copy == 1) {
				card->mCard.mResult = -128;
				return ReadFailed(card);
			}
			continue;
		}
		break;
	}
	return FALSE;
}
