#include "game/inventory.h"

static inline s32 FindItem(Inventory* inventory, s16 id)
{
	for (s32 i = 0; i < 256; ++i) {
		if (id == inventory->mItems[i].mId && inventory->mItems[i].mCount > 0)
			return i;
	}
	return -1;
}

static inline s32 FindAvailableSlot(Inventory* inventory, s16 id)
{
	s32 index = FindItem(inventory, id);
	if (index >= 0)
		return index;
	for (s32 i = 0; i < 256; ++i) {
		if (inventory->mItems[i].mCount <= 0)
			return i;
	}
	return -1;
}

static inline void CompactSlot(Inventory* inventory, s32 index)
{
	if (inventory->mCompact) {
		for (; index < 255; ++index) {
			inventory->mItems[index].mId    = inventory->mItems[index + 1].mId;
			inventory->mItems[index].mCount = inventory->mItems[index + 1].mCount;
		}
		inventory->mItems[index].mId    = -1;
		inventory->mItems[index].mCount = 0;
	}
}

// moves past a shifted slot even when the next one is empty too
/**
 * @note Address: 0x800DC62C
 * @note Size: 0x128
 */
extern "C" void CompactInventory(Inventory* inventory)
{
	for (s32 i = 0; i < 255; ++i) {
		if (inventory->mItems[i].mCount <= 0)
			CompactSlot(inventory, i);
	}
}

/**
 * @note Address: 0x800DC754
 * @note Size: 0x25C
 */
extern "C" s32 RemoveInventoryItemCount(Inventory* inventory, s16 id, s32 amount)
{
	s32 index = FindItem(inventory, id);
	if (index >= 0) {
		if (inventory->mItems[index].mCount > 0) {
			s16 count = inventory->mItems[index].mCount - amount;
			if (count < 0)
				count = 0;
			inventory->mItems[index].mCount = count;
			if (count == 0)
				CompactSlot(inventory, index);
			return 0;
		}
	}
	return 0;
}

/**
 * @note Address: 0x800DC9B0
 * @note Size: 0x380
 */
extern "C" s32 SetOrAddInventoryItemCount(Inventory* inventory, s16 id, s16 amount, BOOL add)
{
	s32 index = FindAvailableSlot(inventory, id);
	if (index >= 0) {
		inventory->mItems[index].mId = id;
		if (add) {
			s16 count                       = inventory->mItems[index].mCount;
			count                           = count > 0 ? count : 0;
			inventory->mItems[index].mCount = count + amount;
		} else {
			inventory->mItems[index].mCount = amount;
			if (amount == 0)
				CompactSlot(inventory, index);
		}
		if (amount > 0 && inventory->mItems[index].mCount > 10) {
			s16 excess                      = inventory->mItems[index].mCount - 10;
			inventory->mItems[index].mCount = 10;
			return excess;
		}
		return 0;
	}
	return -1;
}
