#include "Dolphin/mtx.h"
#include "game/ccard.h"
#include "game/memcard.h"

static inline BOOL ReadFailed(Memcard* card)
{
	card->mCard.Close();
	card->mCard.Unmount();
	return TRUE;
}

/**
 * @note Address: 0x80087508
 * @note Size: 0x34C
 */
extern "C" BOOL ReadValidatedSaveIntoCardBuffer(Memcard* card)
{
	card->mState     = 12;
	SaveHeader* save = &card->mFile.mData.mHeader;
	for (s32 copy = 0, offset = 0; copy < 2; ++copy, offset += sizeof(SaveFile)) {
		card->mCard.Read(card->mBuffer, sizeof(SaveFile), offset);
		card->LogResult();
		if (card->mCard.mResult != CARD_RESULT_READY)
			return ReadFailed(card);
		if (card->mFile.mData.mHeader.mSignature != SAVE_SIGNATURE) {
			if (copy == 1) {
				card->mCard.mResult = -128;
				return ReadFailed(card);
			}
			continue;
		}
		u32 checksum   = 0;
		const u8* data = reinterpret_cast<const u8*>(save);
		for (u32 i = 0; i < sizeof(SaveData); ++i)
			checksum += *data++;
		if (checksum != card->mFile.mChecksum) {
			if (copy == 1) {
				card->mCard.mResult = -128;
				return ReadFailed(card);
			}
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
