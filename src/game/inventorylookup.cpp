#include "game/xchr.h"

/**
 * @note Address: 0x800DCD30
 * @note Size: 0x140
 */
extern "C" s32 FindOwnedInventoryItemIndex(InventoryItem* entries, s16 id)
{
	for (u32 i = 0; i < 256; ++i) {
		if (id == entries[i].mId && entries[i].mCount > 0)
			return i;
	}
	return -1;
}

/**
 * @note Address: 0x800DCE70
 * @note Size: 0x15C
 */
extern "C" s32 GetOwnedInventoryItemCount(Params* stats, s16 id)
{
	s32 index = FindOwnedInventoryItemIndex(stats->mInventory.mItems, id);
	return index >= 0 ? stats->mInventory.mItems[index].mCount : 0;
}
