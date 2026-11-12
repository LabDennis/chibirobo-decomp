#include <dolphin/mtx/vec.h>
#include "game/savedata.h"

// NONMATCHING: retail loads slot->mStat.length before it saves r31, the build after
/**
 * @note Address: 0x80086F24
 * @note Size: 0x26C
 */
extern "C" BOOL IsValidSavePreview(SavePreviewRecord* slot)
{
	if (!HasSaveFileLayout(slot->mStat))
		return FALSE;
	return IsValidSave(&slot->mHeader) != FALSE;
}
